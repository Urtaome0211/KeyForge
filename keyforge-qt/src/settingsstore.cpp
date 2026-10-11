// ============================================================
// settingsstore.cpp —— 设置与成绩持久化实现
//
// 关键算法：历史最佳成绩的判定 —— 只要新 WPM 严格大于旧纪录即视为
// 新纪录（与网页版一致），accuracy/time 一并覆盖保存。
// 所有 QSettings 读写经 safeValue/safeSet 包裹，存储异常只丢弃不抛出。
// ============================================================
#include "settingsstore.h"

namespace Keyforge {

SettingsStore::SettingsStore()
    : m_settings(new QSettings(QStringLiteral("Keyforge"), QStringLiteral("Keyforge")))
    , m_isTest(false)
{
}

SettingsStore::SettingsStore(QSettings *storage)
    : m_settings(storage)
    , m_isTest(true)
{
}

SettingsStore::~SettingsStore()
{
    if (m_isTest)
        delete m_settings;
}

SettingsStore *SettingsStore::useTestStorage(const QString &iniPath)
{
    // 自测隔离：独立 INI 文件，测试结束直接删除文件即可彻底清理
    return new SettingsStore(new QSettings(iniPath, QSettings::IniFormat));
}

QVariant SettingsStore::safeValue(const QString &key, const QVariant &fallback) const
{
    try {
        return m_settings->value(key, fallback);
    } catch (...) {
        return fallback;
    }
}

void SettingsStore::safeSet(const QString &key, const QVariant &value)
{
    try {
        m_settings->setValue(key, value);
        m_settings->sync();
    } catch (...) {
        // 存储异常（磁盘满、权限等）静默忽略，不影响打字主流程
    }
}

bool SettingsStore::sound() const
{
    return safeValue(QStringLiteral("sound"), true).toBool();
}

void SettingsStore::setSound(bool on)
{
    safeSet(QStringLiteral("sound"), on);
}

QString SettingsStore::category() const
{
    return safeValue(QStringLiteral("category"), QString()).toString();
}

void SettingsStore::setCategory(const QString &cat)
{
    safeSet(QStringLiteral("category"), cat);
}

QString SettingsStore::snippetId() const
{
    return safeValue(QStringLiteral("snippetId"), QString()).toString();
}

void SettingsStore::setSnippetId(const QString &id)
{
    safeSet(QStringLiteral("snippetId"), id);
}

QString SettingsStore::customText() const
{
    return safeValue(QStringLiteral("customText"), QString()).toString();
}

void SettingsStore::setCustomText(const QString &text)
{
    safeSet(QStringLiteral("customText"), text);
}

// best/<id>/wpm、best/<id>/acc、best/<id>/timeMs 三个键组成一条纪录
bool SettingsStore::hasBest(const QString &snippetId) const
{
    return m_settings->contains(QStringLiteral("best/%1/wpm").arg(snippetId));
}

int SettingsStore::bestWpm(const QString &snippetId) const
{
    return safeValue(QStringLiteral("best/%1/wpm").arg(snippetId), 0).toInt();
}

double SettingsStore::bestAcc(const QString &snippetId) const
{
    return safeValue(QStringLiteral("best/%1/acc").arg(snippetId), 0.0).toDouble();
}

int SettingsStore::bestTimeMs(const QString &snippetId) const
{
    return safeValue(QStringLiteral("best/%1/timeMs").arg(snippetId), 0).toInt();
}

bool SettingsStore::saveBest(const QString &snippetId, int wpm, double acc, int timeMs)
{
    // 新纪录判定：没有旧纪录，或本次 WPM 更高
    const bool isRecord = !hasBest(snippetId) || wpm > bestWpm(snippetId);
    if (!isRecord)
        return false;
    safeSet(QStringLiteral("best/%1/wpm").arg(snippetId), wpm);
    safeSet(QStringLiteral("best/%1/acc").arg(snippetId), acc);
    safeSet(QStringLiteral("best/%1/timeMs").arg(snippetId), timeMs);
    return true;
}

} // namespace Keyforge
