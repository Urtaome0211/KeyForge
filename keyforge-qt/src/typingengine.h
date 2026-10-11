// ============================================================
// typingengine.h —— 打字练习核心状态机（QML 通过 Engine 单例对接）
//
// 模块职责：
//   1. 维护一次练习的完整状态机（typed 正确前缀 / wrong 错误缓冲 /
//      统计计数 / 计时 / 完成判定），逻辑与网页版 app.js 逐条一致；
//   2. 通过 qApp 事件过滤器接管全局键盘输入（见 eventFilter 注释）；
//   3. 生成 QML 富文本所需的高亮 HTML（codeHtml/hintHtml）；
//   4. 维护虚拟键盘 KeyItem 矩阵（keyRows）与「字符→键」映射，
//      驱动下一键提示（isNext）与按键点亮（isPressed）；
//   5. 成绩统计（accuracy/WPM/CPM）、评级（S~D）与纪录持久化。
//
// 关键公式：
//   accuracy = (keystrokes - errors) / keystrokes * 100（无击键为 100）
//   WPM = typedLength / 5 / 分钟；CPM = typedLength / 分钟
//   评级：S acc≥98 且 WPM≥45；A ≥95 且 ≥35；B ≥90 且 ≥25；C ≥85；其余 D
// ============================================================
#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QVariantList>

class QKeyEvent;
class QTimer;

namespace Keyforge {

class SettingsStore;
class SoundEngine;

// ============================================================
// KeyItem：单个虚拟键帽的运行时状态。
// 静态属性（code/cap/shift/label/fingerColor/hand/flex）构建时一次
// 设定；isNext（下一键高亮）与 isPressed（按下点亮）随打字实时变化。
// 注意：Qt moc 不支持嵌套 Q_OBJECT 类，故定义在命名空间层，
// 并在 TypingEngine 内以 using 别名保持 TypingEngine::KeyItem 写法。
// ============================================================
class KeyItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString code READ code NOTIFY codeChanged)
    Q_PROPERTY(QString cap READ cap NOTIFY capChanged)
    Q_PROPERTY(QString shift READ shift NOTIFY shiftChanged)
    Q_PROPERTY(QString label READ label NOTIFY labelChanged)
    Q_PROPERTY(QString fingerColor READ fingerColor NOTIFY fingerColorChanged)
    Q_PROPERTY(QString hand READ hand NOTIFY handChanged)
    Q_PROPERTY(double flex READ flex NOTIFY flexChanged)
    Q_PROPERTY(bool isNext READ isNext NOTIFY isNextChanged)
    Q_PROPERTY(bool isPressed READ isPressed NOTIFY isPressedChanged)
public:
    explicit KeyItem(QObject *parent = nullptr);

    QString code() const { return m_code; }
    void setCode(const QString &v) { if (m_code != v) { m_code = v; emit codeChanged(); } }
    QString cap() const { return m_cap; }
    void setCap(const QString &v) { if (m_cap != v) { m_cap = v; emit capChanged(); } }
    QString shift() const { return m_shift; }
    void setShift(const QString &v) { if (m_shift != v) { m_shift = v; emit shiftChanged(); } }
    QString label() const { return m_label; }
    void setLabel(const QString &v) { if (m_label != v) { m_label = v; emit labelChanged(); } }
    QString fingerColor() const { return m_fingerColor; }
    void setFingerColor(const QString &v) { if (m_fingerColor != v) { m_fingerColor = v; emit fingerColorChanged(); } }
    QString hand() const { return m_hand; }
    void setHand(const QString &v) { if (m_hand != v) { m_hand = v; emit handChanged(); } }
    double flex() const { return m_flex; }
    void setFlex(double v) { if (m_flex != v) { m_flex = v; emit flexChanged(); } }
    bool isNext() const { return m_isNext; }
    void setIsNext(bool v) { if (m_isNext != v) { m_isNext = v; emit isNextChanged(); } }
    bool isPressed() const { return m_isPressed; }
    void setIsPressed(bool v) { if (m_isPressed != v) { m_isPressed = v; emit isPressedChanged(); } }

signals:
    void codeChanged();
    void capChanged();
    void shiftChanged();
    void labelChanged();
    void fingerColorChanged();
    void handChanged();
    void flexChanged();
    void isNextChanged();
    void isPressedChanged();

private:
    QString m_code, m_cap, m_shift, m_label, m_fingerColor, m_hand;
    double m_flex = 1.0;
    bool m_isNext = false;
    bool m_isPressed = false;
};

class TypingEngine : public QObject
{
    Q_OBJECT

    // ---- 片段 / 输入状态 ----
    Q_PROPERTY(QString snippetCode READ snippetCode NOTIFY snippetLoaded)
    Q_PROPERTY(QString snippetTitle READ snippetTitle NOTIFY snippetLoaded)
    Q_PROPERTY(QString currentSnippetId READ currentSnippetId NOTIFY snippetLoaded)
    Q_PROPERTY(QString category READ category NOTIFY snippetLoaded)
    Q_PROPERTY(QString wrongText READ wrongText NOTIFY wrongTextChanged)
    Q_PROPERTY(QString currentChar READ currentChar NOTIFY stateChanged)
    Q_PROPERTY(QString codeHtml READ codeHtml NOTIFY codeHtmlChanged)
    Q_PROPERTY(QString hintHtml READ hintHtml NOTIFY stateChanged)
    Q_PROPERTY(QString customText READ customText WRITE setCustomText NOTIFY customTextChanged)
    Q_PROPERTY(int typedLength READ typedLength NOTIFY stateChanged)
    Q_PROPERTY(int errors READ errors NOTIFY stateChanged)
    Q_PROPERTY(int keystrokes READ keystrokes NOTIFY stateChanged)
    Q_PROPERTY(int progressPct READ progressPct NOTIFY stateChanged)
    Q_PROPERTY(bool placeholderMode READ placeholderMode NOTIFY snippetLoaded)
    Q_PROPERTY(bool finished READ finished NOTIFY finishedChanged)
    Q_PROPERTY(bool helpVisible READ helpVisible WRITE setHelpVisible NOTIFY stateChanged)

    // ---- 实时统计 ----
    Q_PROPERTY(double accuracy READ accuracy NOTIFY stateChanged)
    Q_PROPERTY(int wpm READ wpm NOTIFY stateChanged)
    Q_PROPERTY(int cpm READ cpm NOTIFY stateChanged)
    Q_PROPERTY(int elapsedMs READ elapsedMs NOTIFY stateChanged)

    // ---- 完成报告 ----
    Q_PROPERTY(bool reportVisible READ reportVisible NOTIFY stateChanged)
    Q_PROPERTY(QString reportGrade READ reportGrade NOTIFY finishedChanged)
    Q_PROPERTY(QString reportNote READ reportNote NOTIFY finishedChanged)
    Q_PROPERTY(QString reportTime READ reportTime NOTIFY finishedChanged)
    Q_PROPERTY(QString reportWpm READ reportWpm NOTIFY finishedChanged)
    Q_PROPERTY(QString reportAcc READ reportAcc NOTIFY finishedChanged)
    Q_PROPERTY(QString reportKeys READ reportKeys NOTIFY finishedChanged)
    Q_PROPERTY(QString reportErr READ reportErr NOTIFY finishedChanged)
    Q_PROPERTY(QString reportBest READ reportBest NOTIFY finishedChanged)
    Q_PROPERTY(bool reportRecord READ reportRecord NOTIFY finishedChanged)

    // ---- 设置 ----
    Q_PROPERTY(bool soundOn READ soundOn NOTIFY soundChanged)

    // ---- 数据列表（QML 下拉框 / Repeater 消费） ----
    Q_PROPERTY(QVariantList categories READ categories NOTIFY stateChanged)
    Q_PROPERTY(QVariantList snippets READ snippets NOTIFY snippetLoaded)
    Q_PROPERTY(QVariantList keyRows READ keyRows NOTIFY stateChanged)

public:
    // KeyItem 实际定义在命名空间层（moc 不支持嵌套 Q_OBJECT），
    // 此处用别名保留 TypingEngine::KeyItem 的嵌套写法。
    using KeyItem = ::Keyforge::KeyItem;

    // 引擎接管传入的 SettingsStore 的所有权
    explicit TypingEngine(SettingsStore *store, QObject *parent = nullptr);
    ~TypingEngine() override;

    // C++ 入口：恢复设置 → 构建键盘数据 → 恢复上次分类/片段
    void initialize();

    // ---- QML 可调用接口 ----
    Q_INVOKABLE void setCategory(const QString &cat);
    Q_INVOKABLE void loadSnippet(const QString &id);
    Q_INVOKABLE void loadCustom(const QString &text);
    Q_INVOKABLE void restart();
    Q_INVOKABLE void nextSnippet();
    Q_INVOKABLE void randomSnippet();
    Q_INVOKABLE void startCustom();
    Q_INVOKABLE void toggleSound();
    Q_INVOKABLE void closeReport();
    Q_INVOKABLE void setCustomTextFromUi(const QString &text);

    // ---- 属性访问器 ----
    QString snippetCode() const { return m_snippetCode; }
    QString snippetTitle() const { return m_snippetTitle; }
    QString currentSnippetId() const { return m_currentId; }
    QString category() const { return m_category; }
    QString wrongText() const { return m_wrong; }
    QString currentChar() const { return m_currentChar; }
    QString codeHtml() const { return m_codeHtml; }
    QString hintHtml() const { return m_hintHtml; }
    QString customText() const { return m_customText; }
    void setCustomText(const QString &v) { if (m_customText != v) { m_customText = v; emit customTextChanged(); } }
    int typedLength() const { return m_typed.length(); }
    int errors() const { return m_errors; }
    int keystrokes() const { return m_keystrokes; }
    int progressPct() const { return m_progressPct; }
    bool placeholderMode() const { return m_placeholderMode; }
    bool finished() const { return m_finished; }
    bool helpVisible() const { return m_helpVisible; }
    void setHelpVisible(bool v) { if (m_helpVisible != v) { m_helpVisible = v; emit stateChanged(); } }
    double accuracy() const { return m_accuracy; }
    int wpm() const { return m_wpm; }
    int cpm() const { return m_cpm; }
    int elapsedMs() const { return int(elapsed()); }
    bool reportVisible() const { return m_reportVisible; }
    QString reportGrade() const { return m_reportGrade; }
    QString reportNote() const { return m_reportNote; }
    QString reportTime() const { return m_reportTime; }
    QString reportWpm() const { return m_reportWpm; }
    QString reportAcc() const { return m_reportAcc; }
    QString reportKeys() const { return m_reportKeys; }
    QString reportErr() const { return m_reportErr; }
    QString reportBest() const { return m_reportBest; }
    bool reportRecord() const { return m_reportRecord; }
    bool soundOn() const { return m_soundOn; }
    QVariantList categories() const { return m_categories; }
    QVariantList snippets() const { return m_snippets; }
    QVariantList keyRows() const { return m_keyRows; }

    // 全局键盘接管：见 .cpp 中 handleKeyEvent 的完整规则说明
    bool eventFilter(QObject *watched, QEvent *event) override;

    // ---- 自测辅助（公开静态，selftest 用它验证公式与引擎行为） ----
    static double accFor(int keystrokes, int errors);          // accuracy 公式
    static int wpmFor(int typedLength, qint64 elapsedMs);      // WPM 公式（分钟<0.01 记 0）
    static int cpmFor(int typedLength, qint64 elapsedMs);      // CPM 公式（分钟<0.01 记 0）
    static QString calcGrade(double acc, int wpm);             // S/A/B/C/D 评级
    static void setSelftestMode(bool on);                      // 开启后可用时间覆盖
    static void setElapsedOverrideForTest(qint64 ms);          // <0 表示关闭覆盖

signals:
    void stateChanged();        // 输入状态/统计/面板可见性等整体变化
    void snippetLoaded();       // 加载/切换片段（含分类、列表变化）
    void finishedChanged();     // 练习完成（报告字段齐备）
    void soundChanged();        // 音效开关变化
    void codeHtmlChanged();     // 代码区 HTML 重绘
    void wrongTextChanged();    // 错误串变化（QML 用于抖动提示）
    void customTextChanged();   // 自定义文本变化

private:
    // ---- 键盘事件处理 ----
    bool handleKeyEvent(QKeyEvent *ke, bool press);
    void handleBackspace();
    void clearAllPressed();
    bool focusOnControl() const;   // 焦点在按钮/下拉框等 QML 控件上
    bool focusOnTextInput() const; // 焦点在文本输入框上

    // ---- 状态机 ----
    void resetState();          // 清空输入状态并渲染（不触发报告）
    void render();              // 重建 codeHtml/hintHtml/统计/下一键高亮
    void finish();              // 完成：结算统计、评级、纪录并弹报告
    void updateStats();         // 重算 accuracy/WPM/CPM（含 tick 定时刷新）
    void highlightNext();       // 点亮下一键（需 Shift 时双 Shift 高亮）
    qint64 elapsed() const;      // 练习耗时（自测模式可被覆盖）

    // ---- 数据构建 ----
    void buildKeyboard();       // 从 keyboarddata 构建 KeyItem 与映射
    void rebuildSnippetList();  // 按当前分类重建 snippets 列表

    // ---- 工具 ----
    static QString keyCodeFor(const QKeyEvent *ke);           // Qt 键 → KeyboardEvent.code
    static QString displayCharFor(const QKeyEvent *ke);       // 错误缓冲显示字符（Enter→⏎ 等）
    static QString fingerColorFor(int finger);                // 手指编号 → 颜色
    static QString fingerName(int finger);                    // 手指编号 → 中文名
    static QString escHtml(const QString &s);                 // & < > 转义
    static QString codeFragmentHtml(const QString &raw, const QString &color, const QString &bg); // 高亮片段
    static QString fmtTime(qint64 ms);                        // m:ss.s（秒补零 4 位）
    void playTickFx();                                        // 击键音（受 soundOn 控制）
    void playErrorFx();
    void playDoneFx();

    // ---- 成员 ----
    SettingsStore *m_store;      // 设置/纪录存储（引擎拥有）
    SoundEngine *m_sound;        // 音效合成器（引擎拥有）
    QTimer *m_tickTimer;         // 120ms 定时刷新实时统计

    // 片段状态
    QString m_snippetCode;       // 当前练习全文
    QString m_snippetTitle;      // 当前片段标题
    QString m_category;          // 当前分类 id（all/custom/...）
    QString m_currentId;         // 当前内置片段 id（自定义为空）
    QString m_customText;        // 自定义分类的文本（设置持久化）
    bool m_placeholderMode = false;

    // 输入状态机
    QString m_typed;             // 正确前缀
    QString m_wrong;             // 错误缓冲（已替换为显示字符 ⏎/⇥）
    int m_errors = 0;
    int m_keystrokes = 0;
    bool m_finished = false;
    QElapsedTimer m_startTime;   // 首次击键启动
    bool m_started = false;
    qint64 m_finalElapsed = -1;  // 完成时刻冻结的耗时

    // 缓存的计算值（render/tick 时更新，属性直接返回）
    QString m_currentChar, m_codeHtml, m_hintHtml;
    int m_progressPct = 0;
    double m_accuracy = 100.0;
    int m_wpm = 0;
    int m_cpm = 0;

    // 完成报告（finish 时一次性填好）
    bool m_reportVisible = false;
    QString m_reportGrade, m_reportNote, m_reportTime, m_reportWpm,
            m_reportAcc, m_reportKeys, m_reportErr, m_reportBest;
    bool m_reportRecord = false;

    // 设置/界面
    bool m_soundOn = true;
    bool m_helpVisible = false;

    // 键盘数据
    QVector<QVector<KeyItem *>> m_keyItems;               // 与 kRows 同形
    QVariantList m_keyRows;                               // QML 消费的 5 行列表
    QHash<QString, KeyItem *> m_codeMap;                  // code → 键
    QMap<QChar, QPair<KeyItem *, bool>> m_charMap;        // 字符 → (键, 是否上档)
    QHash<KeyItem *, int> m_fingerOf;                     // 键 → 手指编号（hint 文案用）
    KeyItem *m_nextKey = nullptr;                         // 当前下一键
    bool m_nextShift = false;

    // 分类/列表
    QVariantList m_categories;                            // 固定 10 类 {id,name}
    QVariantList m_snippets;                              // 当前分类片段 {id,title,lang}

    // 自测模式
    static bool s_selftest;
    static qint64 s_elapsedOverride;
};

} // namespace Keyforge
