// ============================================================
// main.cpp —— 程序入口
//
// 三种运行模式：
//   1. --selftest：QCoreApplication（无 GUI），对打字核心状态机做完整
//      自测（输入推进/错误缓冲/退格/统计公式/Enter↔换行/完成报告/
//      纪录写入/设置恢复），全部通过打印 "SELFTEST OK" 并返回 0；
//   2. --smoke：QGuiApplication + offscreen 平台，加载 QML 后 2.5 秒
//      自动退出（打印 "SMOKE OK"），用于无显示器环境冒烟验证；
//   3. 正常启动：注册 Engine 单例 → 加载 "Keyforge" 模块的 Main.qml
//      → 安装全局键盘事件过滤器 → 进入事件循环。
// ============================================================
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QTimer>
#include <QtMath>
#include <QtQuickControls2/QQuickStyle>
#include <cstdio>

#include "settingsstore.h"
#include "typingengine.h"

using namespace Keyforge;

namespace {

// ---- 自测工具 ----------------------------------------------------

int s_failures = 0;

void check(bool ok, const char *what)
{
    std::printf("%s %s\n", ok ? "[PASS]" : "[FAIL]", what);
    if (!ok)
        ++s_failures;
}

// 合成一次按键并送入引擎事件过滤器（返回是否被消费）
bool sendKey(TypingEngine &engine, QObject &dummy, int qtKey, const QString &text,
             quint32 vk = 0, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    // Qt 6.12 的扩展构造：Type,key,modifiers,scanCode,virtualKey,nativeMods,text,autorep,count
    QKeyEvent ev(QEvent::KeyPress, qtKey, mods, 0, vk, 0, text, false, 1);
    return engine.eventFilter(&dummy, &ev);
}

// 常用字符 → Qt 键值（自测只需字母/数字/空格/换行等）
int qtKeyFor(QChar c)
{
    const ushort u = c.unicode();
    if (u >= 'a' && u <= 'z') return Qt::Key_A + (u - 'a');
    if (u >= 'A' && u <= 'Z') return Qt::Key_A + (u - 'A');
    if (u >= '0' && u <= '9') return Qt::Key_0 + (u - '0');
    switch (u) {
    case ' ': return Qt::Key_Space;
    case '\n': return Qt::Key_Return;
    case '\t': return Qt::Key_Tab;
    default: return 0;
    }
}

// 逐字符输入一段文本（全部按命中处理，用于推进进度）
void typeText(TypingEngine &engine, QObject &dummy, const QString &text)
{
    for (const QChar c : text)
        sendKey(engine, dummy, qtKeyFor(c), QString(c));
}

// 在 keyRows 中查找 code 对应的 KeyItem
TypingEngine::KeyItem *findKey(TypingEngine &engine, const QString &code)
{
    const QVariantList rows = engine.keyRows();
    for (const QVariant &rv : rows) {
        const QVariantList row = rv.toList();
        for (const QVariant &kv : row) {
            auto *item = static_cast<TypingEngine::KeyItem *>(qvariant_cast<QObject *>(kv));
            if (item && item->code() == code)
                return item;
        }
    }
    return nullptr;
}

int runSelftest()
{
    std::printf("== 键锋 KEYFORGE 核心自测 ==\n");
    TypingEngine::setSelftestMode(true);

    // 独立测试存储（临时 INI），避免污染用户真实设置
    const QString iniPath = QDir::temp().filePath(QStringLiteral("keyforge-selftest.ini"));
    QFile::remove(iniPath);
    SettingsStore *store = SettingsStore::useTestStorage(iniPath);
    TypingEngine engine(store);
    QObject dummy;

    // ---- 1. 构造引擎 + 初始化：恢复默认分类 all，加载第一段 js-fib ----
    engine.initialize();
    check(engine.category() == QLatin1String("all"), "初始化后分类为 all");
    check(engine.snippetCode().startsWith(QLatin1String("function fib(n) {")),
          "加载内置首段 js-fib（代码以 function fib(n) { 开头）");
    check(!engine.snippetTitle().isEmpty(), "片段标题非空");
    check(engine.typedLength() == 0 && engine.errors() == 0 && !engine.finished(),
          "初始状态：无输入/无错误/未完成");
    check(engine.snippets().size() == 32, "all 分类包含全部 32 段");

    // ---- 2. 键盘数据：5 行键位、code 映射存在、下一键高亮 ----
    check(engine.keyRows().size() == 5, "键盘 5 行键位已构建");
    TypingEngine::KeyItem *keyA = findKey(engine, QStringLiteral("KeyA"));
    check(keyA != nullptr, "code 映射：KeyA 存在");
    TypingEngine::KeyItem *keyF = findKey(engine, QStringLiteral("KeyF"));
    check(keyF && keyF->isNext(), "下一键高亮：KeyF isNext=true");
    check(keyA && !keyA->isNext(), "非下一键（KeyA）未高亮");
    check(engine.hintHtml().contains(QStringLiteral("下一个键")), "提示文案已生成");

    // ---- 3. 正确输入推进（"function fib" = 8+1+3 = 12 字符）----
    typeText(engine, dummy, QStringLiteral("function fib"));
    check(engine.typedLength() == 12, "正确输入 12 字符后 typedLength=12");
    check(engine.errors() == 0 && engine.keystrokes() == 12, "全部命中：errors=0 keystrokes=12");
    check(engine.codeHtml().contains(QStringLiteral("#664afc7f")), "codeHtml 含正确前缀高亮色");
    check(engine.codeHtml().contains(QStringLiteral("background-color:#2e4afc7f")),
          "codeHtml 含当前字符高亮色");

    // ---- 4. 错误缓冲：错输入堆积、退格逐字符清理 ----
    sendKey(engine, dummy, Qt::Key_X, QStringLiteral("X"));
    sendKey(engine, dummy, Qt::Key_Y, QStringLiteral("Y"));
    check(engine.wrongText() == QLatin1String("XY"), "两次错输入进入错误缓冲 wrongText=XY");
    check(engine.errors() == 2 && engine.typedLength() == 12, "错输入不推进正确前缀，errors=2");
    sendKey(engine, dummy, Qt::Key_Backspace, QStringLiteral("\b"));
    check(engine.wrongText() == QLatin1String("X"), "退格先删错误缓冲：XY→X");
    sendKey(engine, dummy, Qt::Key_Backspace, QStringLiteral("\b"));
    check(engine.wrongText().isEmpty(), "退格再删：X→空");
    check(engine.typedLength() == 12, "错误缓冲清空前不动正确前缀");
    sendKey(engine, dummy, Qt::Key_Backspace, QStringLiteral("\b"));
    check(engine.typedLength() == 11, "缓冲清空后退格开始删正确前缀：12→11");
    typeText(engine, dummy, QStringLiteral("b")); // 第 12 位期望字符是 b
    check(engine.typedLength() == 12, "补回字符 b 后 typedLength=12");
    check(engine.errors() == 2, "纠错过程错误数保持 2");
    check(engine.progressPct() == qFloor(100.0 * 12 / engine.snippetCode().length()),
          "progressPct=floor(12/全长*100)");

    // ---- 5. 统计/评级公式（静态函数直测） ----
    check(qFuzzyCompare(TypingEngine::accFor(10, 2), 80.0), "accFor(10,2)=80");
    check(qFuzzyCompare(TypingEngine::accFor(0, 0), 100.0), "accFor(0,0)=100");
    check(TypingEngine::wpmFor(50, 60000) == 10, "wpmFor(50,60s)=10");
    check(TypingEngine::wpmFor(50, 100) == 0, "wpmFor(50,0.1s)=0（分钟<0.01）");
    check(TypingEngine::cpmFor(120, 60000) == 120, "cpmFor(120,60s)=120");
    check(TypingEngine::calcGrade(99, 50) == QLatin1String("S"), "评级 S：acc99 wpm50");
    check(TypingEngine::calcGrade(96, 40) == QLatin1String("A"), "评级 A：acc96 wpm40");
    check(TypingEngine::calcGrade(91, 30) == QLatin1String("B"), "评级 B：acc91 wpm30");
    check(TypingEngine::calcGrade(86, 0) == QLatin1String("C"), "评级 C：acc86（不卡速度）");
    check(TypingEngine::calcGrade(80, 50) == QLatin1String("D"), "评级 D：acc80");

    // ---- 6. Enter 对应换行：自定短文本 a\nb 逐字符验证 ----
    engine.loadCustom(QStringLiteral("a\nb"));
    check(engine.snippetCode() == QLatin1String("a\nb"), "自定义文本加载（a\\nb）");
    sendKey(engine, dummy, Qt::Key_A, QStringLiteral("a"));
    check(engine.typedLength() == 1 && engine.wrongText().isEmpty(), "字符 a 命中");
    sendKey(engine, dummy, Qt::Key_Return, QStringLiteral("\r"));
    check(engine.typedLength() == 2 && engine.snippetCode() == QLatin1String("a\nb"),
          "Enter 命中期望换行符 \\n");
    check(engine.errors() == 0, "Enter 命中不记错");

    // ---- 7. 完成：报告字段 + 评级 + 首次纪录 ----
    TypingEngine::setElapsedOverrideForTest(60000); // 固定 60 秒：WPM 可确定
    sendKey(engine, dummy, Qt::Key_B, QStringLiteral("b"));
    check(engine.finished(), "输入末字符后 finished=true");
    check(engine.reportVisible(), "完成后报告面板可见");
    check(engine.wpm() == 1, "WPM=3/5/1分钟=1（60 秒）");
    check(qFuzzyCompare(engine.accuracy(), 100.0), "全对 accuracy=100");
    check(engine.reportGrade() == QLatin1String("C"), "评级 C（全对但速度不足）");
    check(engine.reportNote() == QStringLiteral("先放慢速度，稳住准确率"), "评语文案（C）");
    check(engine.reportTime() == QLatin1String("1:00.0"), "reportTime=1:00.0（fmtTime 补零）");
    check(engine.reportWpm() == QLatin1String("1 WPM"), "reportWpm=1 WPM");
    check(engine.reportAcc() == QLatin1String("100.0%"), "reportAcc=100.0%");
    check(engine.reportKeys() == QLatin1String("3"), "reportKeys=3");
    check(engine.reportErr() == QLatin1String("0"), "reportErr=0");
    check(engine.reportBest() == QStringLiteral("—"), "首次完成 reportBest=—");
    check(!engine.reportRecord(), "首次完成不算打破纪录（reportRecord=false）");
    check(store->hasBest(QStringLiteral("custom")) && store->bestWpm(QStringLiteral("custom")) == 1,
          "首次成绩已写入存储（custom 键 wpm=1）");
    check(engine.hintHtml().contains(QStringLiteral("任务完成")), "完成提示文案");

    // ---- 8. 完成态：Enter 重新开始、Escape 关闭报告 ----
    sendKey(engine, dummy, Qt::Key_Return, QStringLiteral("\r"));
    check(!engine.finished() && engine.typedLength() == 0, "完成态按 Enter 重新开始");
    typeText(engine, dummy, QStringLiteral("a\nb"));
    check(engine.finished(), "第二次完成（重打 a\\nb）");
    sendKey(engine, dummy, Qt::Key_Escape, QString());
    check(!engine.reportVisible(), "Escape 关闭报告面板");
    engine.setHelpVisible(true);
    sendKey(engine, dummy, Qt::Key_Escape, QString());
    check(!engine.helpVisible(), "Escape 关闭帮助面板（先帮助后报告）");

    // ---- 9. 新纪录：更快重打 → 星标 + 存储更新 ----
    sendKey(engine, dummy, Qt::Key_Return, QStringLiteral("\r")); // 再来一次
    TypingEngine::setElapsedOverrideForTest(15000); // 15 秒：WPM=3/5/0.25=2.4→2
    typeText(engine, dummy, QStringLiteral("a\nb"));
    check(engine.finished() && engine.wpm() == 2, "15 秒完成 WPM=2");
    check(engine.reportRecord(), "WPM 2>1 判定为新纪录（reportRecord=true）");
    check(engine.reportBest() == QStringLiteral("★ 新纪录！（原最快 1 WPM）"), "新纪录文案");
    check(store->bestWpm(QStringLiteral("custom")) == 2, "新纪录已覆盖存储（wpm=2）");

    // ---- 10. 未破纪录：慢重打 → 普通文案、不覆盖 ----
    sendKey(engine, dummy, Qt::Key_Return, QStringLiteral("\r"));
    TypingEngine::setElapsedOverrideForTest(90000); // 90 秒：WPM=3/5/1.5=0.4→0
    typeText(engine, dummy, QStringLiteral("a\nb"));
    check(engine.finished() && engine.wpm() == 0, "90 秒完成 WPM=0");
    check(!engine.reportRecord(), "WPM 0<2 不打破纪录");
    check(engine.reportBest() == QStringLiteral("最快 2 WPM · 100.0%"), "未破纪录文案（含旧 acc）");
    check(store->bestWpm(QStringLiteral("custom")) == 2, "未破纪录时存储不被覆盖");

    // ---- 11. 自定义分类：占位态 → startCustom 规范化换行 ----
    engine.setCustomTextFromUi(QStringLiteral("hello\r\n\r\n"));
    engine.setCategory(QStringLiteral("custom"));
    check(engine.placeholderMode(), "custom 分类进入占位模式");
    check(engine.snippetCode().isEmpty(), "custom 分类代码区为空");
    check(engine.codeHtml().contains(QStringLiteral("请选择上方代码片段")), "占位提示已放入 codeHtml");
    check(engine.snippets().isEmpty(), "custom 分类无片段列表");
    engine.startCustom();
    check(engine.snippetCode() == QLatin1String("hello"), "startCustom 统一 \\r\\n→\\n 并去尾空行");
    check(!engine.placeholderMode(), "开始自定义后退出占位模式");

    // ---- 12. 分类切换 + 列表 + 设置恢复 ----
    engine.setCategory(QStringLiteral("py"));
    check(engine.category() == QLatin1String("py"), "切换到 py 分类");
    check(!engine.snippets().isEmpty()
              && engine.snippets().first().toMap().value(QStringLiteral("id")).toString().startsWith(
                  QLatin1String("py-")),
          "py 列表全部为 py 片段");
    check(engine.snippets().first().toMap().value(QStringLiteral("lang")).toString()
              == QLatin1String("Python"),
          "snippets 的 lang 字段为显示名 Python");
    check(!engine.snippetCode().isEmpty() && !engine.placeholderMode(), "py 分类自动加载首段");
    {
        // 新引擎读同一存储：分类/片段应恢复为 py + 首段
        SettingsStore *store2 = SettingsStore::useTestStorage(iniPath);
        TypingEngine engine2(store2);
        engine2.initialize();
        check(engine2.category() == QLatin1String("py"), "新引擎恢复上次分类 py");
        check(!engine2.snippetCode().isEmpty() && engine2.snippetCode() == engine.snippetCode(),
              "新引擎恢复上次片段（py 首段）");
    }

    // ---- 13. 音效开关持久化 ----
    check(engine.soundOn(), "音效默认开启");
    engine.toggleSound();
    check(!engine.soundOn(), "toggleSound 关闭音效");
    check(!store->sound(), "音效开关已写入存储");

    TypingEngine::setElapsedOverrideForTest(-1);
    QFile::remove(iniPath);

    if (s_failures == 0)
        std::printf("SELFTEST OK\n");
    else
        std::printf("SELFTEST FAILED：%d 项未通过\n", s_failures);
    return s_failures == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char *argv[])
{
    // 参数预扫描（在创建 QApplication 前）
    bool selftest = false;
    bool smoke = false;
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if (a == QLatin1String("--selftest"))
            selftest = true;
        else if (a == QLatin1String("--smoke"))
            smoke = true;
    }

    // 自测：无 GUI，直接跑核心逻辑验证
    if (selftest) {
        QCoreApplication app(argc, argv);
        return runSelftest();
    }

    // 冒烟测试：强制 offscreen 平台，无显示器也能加载 QML
    if (smoke)
        qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication app(argc, argv);

    // 强制 Basic 控件样式：默认平台样式（Windows）不支持 background/contentItem/
    // indicator 等控件定制，会忽略 Main.qml 里全手写的 ComboBox/ScrollBar 外观。
    // Basic 样式完全按 QML 声明渲染，才能还原 CRT 主题。
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // 引擎：持有设置存储，初始化（恢复设置 → 键盘数据 → 上次分类片段）
    SettingsStore *store = new SettingsStore();
    TypingEngine engine(store);
    engine.initialize();

    // 注册 QML 单例 "Engine" + 安装全局键盘事件过滤器
    qmlRegisterSingletonInstance("Keyforge", 1, 0, "Engine", &engine);
    qApp->installEventFilter(&engine);

    QQmlApplicationEngine qml;
    qml.loadFromModule("Keyforge", "Main");
    if (qml.rootObjects().isEmpty()) {
        std::fprintf(stderr, "QML 加载失败：Keyforge/Main\n");
        return 1;
    }

    if (smoke) {
        std::printf("SMOKE OK\n");
        QTimer::singleShot(2500, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
