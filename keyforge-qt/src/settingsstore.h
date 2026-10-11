// ============================================================
// settingsstore.h —— 设置与成绩持久化
//
// 模块职责：把用户设置（音效开关、上次分类、上次片段、自定义文本）
// 与各片段的历史最佳成绩写入 QSettings（org "Keyforge"/app "Keyforge"），
// 读写全部包裹 try/catch，损坏的存储不会导致程序崩溃。
// ============================================================
#pragma once

#include <QSettings>
#include <QString>

namespace Keyforge {

class SettingsStore
{
public:
    // 生产存储：注册表（Windows 上为 HKCU\Software\Keyforge\Keyforge）
    SettingsStore();
    ~SettingsStore();

    // 自测用：切换到独立的 INI 文件，避免污染用户真实设置
    static SettingsStore *useTestStorage(const QString &iniPath);

    // 无需拷贝
    SettingsStore(const SettingsStore &) = delete;
    SettingsStore &operator=(const SettingsStore &) = delete;

    // ---- 普通设置 ----
    bool sound() const;                    // 默认 true
    void setSound(bool on);

    QString category() const;              // 上次选择的分类 id，默认空
    void setCategory(const QString &cat);

    QString snippetId() const;             // 上次打开的片段 id，默认空
    void setSnippetId(const QString &id);

    QString customText() const;            // 自定义分类的文本，默认空
    void setCustomText(const QString &text);

    // ---- 历史最佳成绩（键 "best/<id>/wpm|acc|timeMs"） ----
    bool hasBest(const QString &snippetId) const;
    int bestWpm(const QString &snippetId) const;
    double bestAcc(const QString &snippetId) const;
    int bestTimeMs(const QString &snippetId) const;
    // 写入并返回是否为新纪录（WPM 更优即新纪录：无旧纪录或新 WPM 更大）
    bool saveBest(const QString &snippetId, int wpm, double acc, int timeMs);

private:
    explicit SettingsStore(QSettings *storage); // 接管传入的存储对象

    QSettings *m_settings;                 // 实际存储后端（生产或测试）
    bool m_isTest;                         // 测试存储由 useTestStorage 创建，需自行释放

    // 带异常保护的读/写包装，失败时返回 fallback / 静默忽略
    QVariant safeValue(const QString &key, const QVariant &fallback) const;
    void safeSet(const QString &key, const QVariant &value);
};

} // namespace Keyforge
