// ============================================================
// soundengine.h —— 音效合成与播放
//
// 模块职责：用代码合成三种按键音效（击键 tick / 错误 error / 完成
// done 琶音），以内存 WAV 经 QAudioSink 播放，不依赖任何外部音频文件。
//
// 关键算法：
//   - 全部音效 = 44100Hz / 16bit / 单声道的 PCM 采样序列，直接拼进
//     标准 44 字节 WAV 头后一次性写入 QAudioSink；
//   - tick 是频率随进度变化的方波（f = 430 + (typedLength % 11) * 45，
//     越往后音越高，形成可感知的推进感）；
//   - error 是 120Hz 锯齿波（波形线性上升后骤降，听感尖锐）；
//   - done 是 C 大调和弦琶音 523/659/784/1047Hz 三角波，四音各 160ms、
//     间隔 90ms，三角波谐波少、听感柔和；
//   - 音量通过采样幅值缩放实现（tick .045 / error .06 / done .07）。
//
// 容错原则：QAudioSink 惰性初始化；任何一步失败（无声卡、格式不支持）
// 只做静默禁用，绝不崩溃。
// ============================================================
#pragma once

#include <QObject>

class QAudioSink;
class QBuffer;
class QByteArray;

namespace Keyforge {

class SoundEngine : public QObject
{
    Q_OBJECT
public:
    explicit SoundEngine(QObject *parent = nullptr);
    ~SoundEngine() override;

    void playTick(int typedLength);   // 正确击键音（音高随进度升高）
    void playError();                 // 错误击键音（锯齿波）
    void playDone();                  // 完成琶音（四音上行）

private:
    // 合成一段 16bit 单声道采样（value = 振幅 -1..1，返回小端字节序）
    static QByteArray synth16Mono(int samples, double (*wave)(int, int), double freq, double amp);
    // 把一段采样包装成内存 WAV 并播放；失败静默禁用
    void playPcm(const QByteArray &pcm, int sampleRate);

    // 波形函数：phase 为当前采样序号（第 n 个采样）
    static double squareWave(int n, int period);   // 方波：前半周期 +1、后半 -1
    static double sawWave(int n, int period);      // 锯齿：周期内线性 +1 → -1
    static double triangleWave(int n, int period); // 三角：线性上升到 +1 再降到 -1

    QAudioSink *m_sink = nullptr;  // 播放设备（惰性创建，失败为 nullptr）
    bool m_muted = false;          // 初始化失败后永久静默，避免反复报错
};

} // namespace Keyforge
