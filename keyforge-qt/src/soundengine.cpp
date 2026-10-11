// ============================================================
// soundengine.cpp —— 音效合成与播放实现
//
// 播放路径：合成 PCM → 加 44 字节 WAV 头 → QBuffer 包装 → QAudioSink。
// QAudioSink 创建一次后复用；创建或写入失败即 m_muted，之后全部短路。
// 无 QGuiApplication 时（--selftest）QAudioSink 不可用，同样静默禁用。
// ============================================================
#include "soundengine.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QBuffer>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMediaDevices>
#include <QtMath>

namespace Keyforge {

namespace {

constexpr int kSampleRate = 44100;   // 采样率 44100Hz
constexpr int kTickMs = 50;          // 击键音时长
constexpr int kErrorMs = 140;        // 错误音时长
constexpr int kDoneNoteMs = 160;     // 完成琶音单音时长
constexpr int kDoneGapMs = 90;       // 琶音间静音间隔
constexpr int kBytesPerSample = 2;   // 16bit

// 把 -1..1 的浮点振幅转为 16bit 小端字节
inline void appendSample(QByteArray &out, double v)
{
    int s = qBound(-32768, int(v * 32767.0), 32767);
    out.append(char(s & 0xff));
    out.append(char((s >> 8) & 0xff));
}

// 时长（毫秒）对应的采样数
inline int msToSamples(int ms)
{
    return qMax(1, kSampleRate * ms / 1000);
}

} // namespace

SoundEngine::SoundEngine(QObject *parent)
    : QObject(parent)
{
}

SoundEngine::~SoundEngine()
{
    if (m_sink) {
        m_sink->stop();
        delete m_sink;
    }
}

double SoundEngine::squareWave(int n, int period)
{
    return (n % period) < period / 2 ? 1.0 : -1.0;
}

double SoundEngine::sawWave(int n, int period)
{
    return 2.0 * (n % period) / double(period) - 1.0;
}

double SoundEngine::triangleWave(int n, int period)
{
    const int half = period / 2;
    const int r = n % period;
    return r < half ? 2.0 * r / half - 1.0 : 2.0 * (period - r) / half - 1.0;
}

QByteArray SoundEngine::synth16Mono(int samples, double (*wave)(int, int), double freq, double amp)
{
    QByteArray out;
    out.reserve(samples * kBytesPerSample);
    // 每周期采样数 = 采样率 / 频率；低频时周期可能很长，用 double 精确计算
    const double period = double(kSampleRate) / freq;
    for (int n = 0; n < samples; ++n)
        appendSample(out, amp * wave(n, qMax(1, int(period))));
    return out;
}

void SoundEngine::playPcm(const QByteArray &pcm, int sampleRate)
{
    if (m_muted)
        return;
    // QAudioSink 依赖 GUI 应用环境；--selftest 下直接禁用
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        m_muted = true;
        return;
    }

    if (!m_sink) {
        QAudioFormat fmt;
        fmt.setSampleRate(sampleRate);
        fmt.setChannelCount(1);
        fmt.setSampleFormat(QAudioFormat::Int16);
        const QAudioDevice dev = QMediaDevices::defaultAudioOutput();
        if (dev.isNull() || !dev.isFormatSupported(fmt)) {
            m_muted = true; // 无声卡或不支持格式：永久静默
            return;
        }
        m_sink = new QAudioSink(dev, fmt, this);
    }

    // 组装内存 WAV：44 字节 RIFF 头 + PCM 数据，QBuffer 提供可读数据源
    QByteArray wav;
    wav.reserve(44 + pcm.size());
    const int dataSize = pcm.size();
    const int byteRate = sampleRate * 1 * kBytesPerSample;
    wav.append("RIFF");
    wav.append(char(36 + dataSize & 0xff)).append(char((36 + dataSize >> 8) & 0xff))
        .append(char((36 + dataSize >> 16) & 0xff)).append(char((36 + dataSize >> 24) & 0xff));
    wav.append("WAVE");
    wav.append("fmt ");
    wav.append(char(16)).append(char(0)).append(char(0)).append(char(0));       // fmt 块大小 16
    wav.append(char(1)).append(char(0));                                        // PCM 格式
    wav.append(char(1)).append(char(0));                                        // 单声道
    wav.append(char(sampleRate & 0xff)).append(char((sampleRate >> 8) & 0xff))
        .append(char((sampleRate >> 16) & 0xff)).append(char((sampleRate >> 24) & 0xff));
    wav.append(char(byteRate & 0xff)).append(char((byteRate >> 8) & 0xff))
        .append(char((byteRate >> 16) & 0xff)).append(char((byteRate >> 24) & 0xff));
    wav.append(char(kBytesPerSample)).append(char(0));                          // 块对齐
    wav.append(char(16)).append(char(0));                                       // 位深 16
    wav.append("data");
    wav.append(char(dataSize & 0xff)).append(char((dataSize >> 8) & 0xff))
        .append(char((dataSize >> 16) & 0xff)).append(char((dataSize >> 24) & 0xff));
    wav.append(pcm);

    QBuffer *buf = new QBuffer(this);
    buf->setData(wav);
    if (!buf->open(QIODevice::ReadOnly)) {
        delete buf;
        m_muted = true;
        return;
    }
    m_sink->stop();
    m_sink->start(buf);
    // QAudioSink 会自行接管 QBuffer 的读取；播放结束由 Qt 处理，无需手动 wait
}

void SoundEngine::playTick(int typedLength)
{
    // 音高随已输入长度阶梯升高：430Hz 起，每多 11 个字符升一档（45Hz）
    const double freq = 430.0 + (typedLength % 11) * 45.0;
    const QByteArray pcm = synth16Mono(msToSamples(kTickMs), squareWave, freq, 0.045);
    playPcm(pcm, kSampleRate);
}

void SoundEngine::playError()
{
    const QByteArray pcm = synth16Mono(msToSamples(kErrorMs), sawWave, 120.0, 0.06);
    playPcm(pcm, kSampleRate);
}

void SoundEngine::playDone()
{
    // 琶音：523/659/784/1047Hz 各 160ms，中间隔 90ms 静音；三角波听感柔和
    static const double notes[] = { 523.0, 659.0, 784.0, 1047.0 };
    QByteArray pcm;
    const QByteArray silence(msToSamples(kDoneGapMs) * kBytesPerSample, char(0));
    for (double note : notes) {
        pcm += synth16Mono(msToSamples(kDoneNoteMs), triangleWave, note, 0.07);
        pcm += silence;
    }
    playPcm(pcm, kSampleRate);
}

} // namespace Keyforge
