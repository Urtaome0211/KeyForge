// ============================================================
// typingengine.cpp —— 打字练习核心状态机实现
//
// 键盘事件接管规则（eventFilter → handleKeyEvent，顺序与网页版
// app.js 的 onKeyDown 完全一致）：
//   1. 自动重复（isAutoRepeat）直接忽略；
//   2. 键帽视觉点亮/熄灭（对映射到键盘的每个键，先于一切逻辑）；
//   3. Escape：先关帮助面板，再关成绩报告（都未开时同样消费）；
//   4. 已完成状态：Enter/Space 触发 restart（焦点在按钮/控件上放行）；
//   5. 未加载片段：不接管；
//   6. 焦点在文本输入框（QQuickTextInput/TextEdit）放行——自定义文本
//      编辑等场景由 QML 自己处理；
//   7. Ctrl/Alt/Meta 组合键放行不计数；
//   8. Backspace：先删错误缓冲，再删正确前缀；
//   9. 非单字符键（方向键/F 键/纯修饰键，text 为空或超长）放行；
//   10. 错误缓冲非空时，任何输入一律记错（退格是唯一出路）；
//   11. 匹配判定：期望 '\n'↔Return/Enter、'\t'↔Tab、空格↔Space，
//       其余用 event->text() 与期望字符比较；命中推进正确前缀，
//       未命中追加错误缓冲。命中/未命中都消费事件。
//
// 统计公式：accuracy = (keystrokes-errors)/keystrokes*100（无击键 100）；
// WPM = typedLength/5/分钟；CPM = typedLength/分钟（分钟<0.01 记 0）。
// 评级：S acc≥98 且 WPM≥45；A ≥95 且 ≥35；B ≥90 且 ≥25；C ≥85；其余 D。
// ============================================================
#include "typingengine.h"

#include "keyboarddata.h"
#include "settingsstore.h"
#include "snippetdata.h"
#include "soundengine.h"

#include <QEvent>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QRandomGenerator>
#include <QTimer>
#include <QtMath>

namespace Keyforge {

// 自测模式开关（时间可被覆盖，保证公式验证确定可重复）
bool TypingEngine::s_selftest = false;
qint64 TypingEngine::s_elapsedOverride = -1;

namespace {

// 固定分类（id + 显示名），与网页版分类标签一致
const char *const kCategoryIds[] = {
    "all", "js", "py", "java", "c", "go", "html", "css", "sql", "other", "custom"
};
const char *const kCategoryNames[] = {
    "全部", "JavaScript", "Python", "Java", "C", "Go", "HTML", "CSS", "SQL", "其他", "自定义"
};
constexpr int kCategoryCount = int(sizeof(kCategoryIds) / sizeof(kCategoryIds[0]));

// 占位提示（自定义分类 / 未加载片段时显示在代码区）
const QString kPlaceholderHtml = QStringLiteral(
    "<span style=\"color:#594afc7f;\">◇ 请选择上方代码片段<br>"
    "◇ 或在「自定义」分类里粘贴自己的代码练习</span>");

// 语言分类 id → 下拉框显示名
QString langDisplayName(const QString &lang)
{
    if (lang == QLatin1String("js")) return QStringLiteral("JavaScript");
    if (lang == QLatin1String("py")) return QStringLiteral("Python");
    if (lang == QLatin1String("java")) return QStringLiteral("Java");
    if (lang == QLatin1String("c")) return QStringLiteral("C");
    if (lang == QLatin1String("go")) return QStringLiteral("Go");
    if (lang == QLatin1String("html")) return QStringLiteral("HTML");
    if (lang == QLatin1String("css")) return QStringLiteral("CSS");
    if (lang == QLatin1String("sql")) return QStringLiteral("SQL");
    return QStringLiteral("其他");
}

// 评级 → 报告评语（与网页版 GRADE_NOTES 逐字一致）
QString gradeNote(const QString &grade)
{
    if (grade == QLatin1String("S")) return QStringLiteral("出神入化，键盘已与你融为一体");
    if (grade == QLatin1String("A")) return QStringLiteral("非常出色，继续保持节奏");
    if (grade == QLatin1String("B")) return QStringLiteral("不错的成绩，还有提升空间");
    if (grade == QLatin1String("C")) return QStringLiteral("先放慢速度，稳住准确率");
    return QStringLiteral("从指法开始，慢即是快");
}

} // namespace

// ============================================================
// KeyItem
// ============================================================
KeyItem::KeyItem(QObject *parent)
    : QObject(parent)
{
}

// ============================================================
// 构造 / 初始化
// ============================================================
TypingEngine::TypingEngine(SettingsStore *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
    , m_sound(new SoundEngine(this))
    , m_tickTimer(new QTimer(this))
{
    // 固定分类列表（QML 下拉框数据源）
    m_categories.reserve(kCategoryCount);
    for (int i = 0; i < kCategoryCount; ++i) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), QString::fromLatin1(kCategoryIds[i]));
        m.insert(QStringLiteral("name"), QString::fromUtf8(kCategoryNames[i]));
        m_categories.append(m);
    }

    // 120ms 定时刷新实时统计（WPM/CPM/耗时），完成后冻结不再刷新
    m_tickTimer->setInterval(120);
    connect(m_tickTimer, &QTimer::timeout, this, [this] {
        if (m_started && !m_finished) {
            updateStats();
            emit stateChanged();
        }
    });
    m_tickTimer->start();
}

TypingEngine::~TypingEngine()
{
    delete m_store;
}

void TypingEngine::initialize()
{
    // 恢复设置 → 构建键盘数据 → 恢复上次分类（连带恢复片段）
    m_soundOn = m_store->sound();
    emit soundChanged();
    buildKeyboard();
    m_customText = m_store->customText();
    emit customTextChanged();

    QString cat = m_store->category();
    if (cat.isEmpty())
        cat = QStringLiteral("all");
    bool valid = false;
    for (const QVariant &v : std::as_const(m_categories)) {
        if (v.toMap().value(QStringLiteral("id")).toString() == cat) {
            valid = true;
            break;
        }
    }
    if (!valid)
        cat = QStringLiteral("all");
    setCategory(cat);
}

// ============================================================
// 键盘数据构建
// ============================================================
void TypingEngine::buildKeyboard()
{
    // 遍历生成的五行键位：构建 KeyItem 树 + code/字符双重映射
    for (const QVector<KeyDef> &row : kRows) {
        QVector<KeyItem *> items;
        QVariantList rowList;
        for (const KeyDef &def : row) {
            auto *item = new KeyItem(this);
            item->setCode(def.code);
            item->setCap(def.cap);
            item->setShift(def.shift);
            item->setLabel(def.label);
            item->setFingerColor(fingerColorFor(def.finger));
            item->setHand(def.hand == QLatin1Char('R') ? QStringLiteral("右手") : QStringLiteral("左手"));
            item->setFlex(def.w);
            items.append(item);
            // 注意：rowList 必须作为「一个元素」包装进 QVariant，
            // 否则 QVariantList::append(另一个 QVariantList) 会命中
            // QList 的拼接重载，把整行摊平成 58 个元素。
            rowList.append(QVariant::fromValue(static_cast<QObject *>(item)));
            m_codeMap.insert(def.code, item);
            m_fingerOf.insert(item, def.finger);

            // 字符 → 键映射：主字符/上档字符/特殊键（\n→Enter、\t→Tab、空格→Space）
            if (!def.cap.isEmpty())
                m_charMap.insert(def.cap.at(0), qMakePair(item, false));
            if (!def.shift.isEmpty())
                m_charMap.insert(def.shift.at(0), qMakePair(item, true));
            if (def.code == QLatin1String("Enter"))
                m_charMap.insert(QLatin1Char('\n'), qMakePair(item, false));
            else if (def.code == QLatin1String("Tab"))
                m_charMap.insert(QLatin1Char('\t'), qMakePair(item, false));
            else if (def.code == QLatin1String("Space"))
                m_charMap.insert(QLatin1Char(' '), qMakePair(item, false));
        }
        // 注意：不能用 append(rowList)/append(items)——QVariantList/QVector 与
        // QList 的 append(同类列表) 重载是「拼接」，会把 5 行摊平成 58 个元素；
        // push_back 才会把整行作为单个元素放入外层列表。
        m_keyItems.push_back(items);
        m_keyRows.push_back(QVariant::fromValue(rowList));
    }
}

QString TypingEngine::fingerColorFor(int finger)
{
    switch (finger) {
    case 0: return QStringLiteral("#ff6ad5"); // 小指
    case 1: return QStringLiteral("#52e5ff"); // 无名指
    case 2: return QStringLiteral("#ffb454"); // 中指
    case 3: return QStringLiteral("#3afb6f"); // 食指
    default: return QStringLiteral("#c9d6c9"); // 拇指
    }
}

QString TypingEngine::fingerName(int finger)
{
    switch (finger) {
    case 0: return QStringLiteral("小指");
    case 1: return QStringLiteral("无名指");
    case 2: return QStringLiteral("中指");
    case 3: return QStringLiteral("食指");
    default: return QStringLiteral("拇指");
    }
}

// ============================================================
// 片段管理
// ============================================================
void TypingEngine::rebuildSnippetList()
{
    m_snippets.clear();
    if (m_category == QLatin1String("custom"))
        return; // 自定义分类无片段列表
    for (const SnippetDef &s : kSnippets) {
        if (m_category != QLatin1String("all") && s.lang != m_category)
            continue;
        QVariantMap m;
        m.insert(QStringLiteral("id"), s.id);
        m.insert(QStringLiteral("title"), s.title);
        m.insert(QStringLiteral("lang"), langDisplayName(s.lang)); // QML 下拉框直接显示
        m_snippets.append(m);
    }
}

void TypingEngine::setCategory(const QString &cat)
{
    m_category = cat;
    m_store->setCategory(cat);
    rebuildSnippetList();

    if (cat == QLatin1String("custom")) {
        // 自定义分类：进入占位状态，等用户在文本框粘贴内容后点开始
        m_currentId.clear();
        loadSnippet(QString());
        return;
    }

    // 恢复上次片段：不在当前列表中则取第一段
    const QString saved = m_store->snippetId();
    QString target;
    for (const QVariant &v : std::as_const(m_snippets)) {
        const QString id = v.toMap().value(QStringLiteral("id")).toString();
        if (id == saved) {
            target = id;
            break;
        }
    }
    if (target.isEmpty() && !m_snippets.isEmpty())
        target = m_snippets.first().toMap().value(QStringLiteral("id")).toString();
    loadSnippet(target);
}

void TypingEngine::loadSnippet(const QString &id)
{
    const SnippetDef *def = nullptr;
    for (const SnippetDef &s : kSnippets) {
        if (s.id == id) {
            def = &s;
            break;
        }
    }
    if (!def) {
        // 空加载（自定义分类 / 分类内无片段）：清空并显示占位提示
        m_currentId.clear();
        m_snippetCode.clear();
        m_snippetTitle.clear();
        m_placeholderMode = (m_category == QLatin1String("custom"));
        resetState();
        emit snippetLoaded();
        return;
    }
    m_currentId = def->id;
    m_snippetCode = def->code;
    m_snippetTitle = def->title;
    m_placeholderMode = false;
    m_store->setSnippetId(def->id);
    resetState();
    emit snippetLoaded();
}

void TypingEngine::loadCustom(const QString &text)
{
    if (text.isEmpty())
        return; // 空内容静默忽略
    m_currentId.clear();
    m_snippetCode = text;
    m_snippetTitle = QStringLiteral("自定义练习");
    m_placeholderMode = false;
    resetState();
    emit snippetLoaded();
}

void TypingEngine::startCustom()
{
    // 统一换行为 \n 并去掉尾部空行（与网页版 startCustom 一致）
    QString t = m_customText;
    t.replace(QStringLiteral("\r\n"), QLatin1String("\n"));
    t.replace(QLatin1Char('\r'), QLatin1String("\n"));
    while (t.endsWith(QLatin1Char('\n')))
        t.chop(1);
    loadCustom(t);
}

void TypingEngine::restart()
{
    // 重新开始当前内容：m_currentId 为空=当前练的是自定义文本（重载原文），
    // 否则按片段 id 重载。不依赖分类判断，避免 loadCustom 后未切分类时失效。
    if (m_snippetCode.isEmpty())
        return;
    if (m_currentId.isEmpty())
        loadCustom(m_snippetCode);
    else
        loadSnippet(m_currentId);
}

void TypingEngine::nextSnippet()
{
    if (m_snippets.isEmpty())
        return;
    int idx = -1;
    for (int i = 0; i < m_snippets.size(); ++i) {
        if (m_snippets.at(i).toMap().value(QStringLiteral("id")).toString() == m_currentId) {
            idx = i;
            break;
        }
    }
    // 循环取下一段（未找到时 idx=-1 → 从第一段开始，与网页一致）
    idx = (idx + 1) % m_snippets.size();
    loadSnippet(m_snippets.at(idx).toMap().value(QStringLiteral("id")).toString());
}

void TypingEngine::randomSnippet()
{
    if (m_snippets.isEmpty())
        return;
    const int idx = int(QRandomGenerator::global()->bounded(quint32(m_snippets.size())));
    loadSnippet(m_snippets.at(idx).toMap().value(QStringLiteral("id")).toString());
}

// ============================================================
// 设置与面板
// ============================================================
void TypingEngine::toggleSound()
{
    m_soundOn = !m_soundOn;
    m_store->setSound(m_soundOn);
    emit soundChanged();
}

void TypingEngine::closeReport()
{
    if (m_reportVisible) {
        m_reportVisible = false;
        emit stateChanged();
    }
}

void TypingEngine::setCustomTextFromUi(const QString &text)
{
    m_customText = text;
    m_store->setCustomText(text);
    emit customTextChanged();
}

// ============================================================
// 状态机核心
// ============================================================
void TypingEngine::resetState()
{
    m_typed.clear();
    m_wrong.clear();
    m_errors = 0;
    m_keystrokes = 0;
    m_finished = false;
    m_started = false;
    m_finalElapsed = -1;
    m_reportVisible = false;
    render();
    emit wrongTextChanged();
    emit finishedChanged();
}

qint64 TypingEngine::elapsed() const
{
    if (s_selftest && s_elapsedOverride >= 0)
        return s_elapsedOverride;
    if (m_finished && m_finalElapsed >= 0)
        return m_finalElapsed;
    if (!m_started)
        return 0;
    return m_startTime.elapsed();
}

void TypingEngine::updateStats()
{
    const qint64 ms = elapsed();
    m_accuracy = accFor(m_keystrokes, m_errors);
    m_wpm = wpmFor(m_typed.length(), ms);
    m_cpm = cpmFor(m_typed.length(), ms);
}

void TypingEngine::render()
{
    // ---- 代码区 HTML（富文本，仅用基础 CSS 属性） ----
    if (m_snippetCode.isEmpty()) {
        m_codeHtml = kPlaceholderHtml;
        m_currentChar.clear();
        m_progressPct = 0;
    } else {
        const int pos = m_typed.length();
        const int total = m_snippetCode.length();
        QString html;
        // 正确前缀（绿底）
        html += codeFragmentHtml(m_snippetCode.left(pos), QStringLiteral("#664afc7f"), QString());
        // 当前字符（亮字 + 浅绿底）
        if (!m_finished)
            html += codeFragmentHtml(m_snippetCode.mid(pos, 1), QStringLiteral("#f0fff4"),
                                     QStringLiteral("#2e4afc7f"));
        // 错误缓冲（红字红底，追加在当前位置之后）
        if (!m_wrong.isEmpty())
            html += codeFragmentHtml(m_wrong, QStringLiteral("#ff5e5b"), QStringLiteral("#24ff5e5b"));
        // 剩余文本（默认字色，由 QML Text 基色呈现）
        if (pos + 1 < total)
            html += escHtml(m_snippetCode.mid(pos + 1)).replace(QLatin1Char('\n'), QStringLiteral("⏎<br>"))
                        .replace(QLatin1Char('\t'), QStringLiteral("⇥"))
                        .replace(QLatin1Char(' '), QStringLiteral("·"));
        m_codeHtml = html;
        m_currentChar = m_finished ? QString() : QString(m_snippetCode.at(pos));
        // 进度 = 已输入 / 全长 * 100（向下取整，与网页一致）
        m_progressPct = qFloor(100.0 * pos / total);
    }

    updateStats();
    highlightNext();

    emit stateChanged();
    emit codeHtmlChanged();
}

void TypingEngine::highlightNext()
{
    // 清除旧高亮
    for (const QVector<KeyItem *> &row : std::as_const(m_keyItems)) {
        for (KeyItem *k : row)
            k->setIsNext(false);
    }
    m_nextKey = nullptr;
    m_nextShift = false;

    // 无片段 / 已完成：提示文案
    if (m_snippetCode.isEmpty()) {
        m_hintHtml = QStringLiteral("▶ 等待开始……");
        return;
    }
    if (m_finished || m_typed.length() >= m_snippetCode.length()) {
        m_hintHtml = QStringLiteral("✔ 任务完成 · Enter 再来一次 · Esc 关闭报告");
        return;
    }

    // 查找下一字符对应的键
    const QChar c = m_snippetCode.at(m_typed.length());
    const auto it = m_charMap.constFind(c);
    if (it == m_charMap.constEnd()) {
        m_hintHtml = QStringLiteral("▶ 下一个键：任意键（无指法要求）");
        return;
    }
    KeyItem *key = it->first;
    m_nextKey = key;
    m_nextShift = it->second;
    key->setIsNext(true);
    if (m_nextShift) {
        // 需要 Shift：两个 Shift 键同时高亮提示
        if (KeyItem *l = m_codeMap.value(kShiftCodeLeft, nullptr))
            l->setIsNext(true);
        if (KeyItem *r = m_codeMap.value(kShiftCodeRight, nullptr))
            r->setIsNext(true);
    }

    // 键帽标签：特殊键用 label，普通键按是否上档取字符
    QString label = key->label();
    if (label.isEmpty())
        label = m_nextShift ? key->shift() : key->cap();
    m_hintHtml = QStringLiteral("▶ 下一个键 <b>[ %1 ]</b> → <font color='%2'>●</font> %3%4%5")
                     .arg(label, key->fingerColor(), key->hand(), fingerName(m_fingerOf.value(key, 3)),
                          m_nextShift ? QStringLiteral(" · 按住 Shift") : QString());
}

void TypingEngine::finish()
{
    m_finished = true;
    m_finalElapsed = elapsed();
    if (!m_started) {
        m_started = true;
        m_startTime.start();
    }

    // 结算统计（用完成时刻冻结的耗时）
    updateStats();
    const double acc = m_accuracy;
    const int wpm = m_wpm;

    // 评级与评语
    m_reportGrade = calcGrade(acc, wpm);
    m_reportNote = gradeNote(m_reportGrade);

    // 报告字段
    m_reportTime = fmtTime(m_finalElapsed);
    m_reportWpm = QStringLiteral("%1 WPM").arg(wpm);
    m_reportAcc = QString::number(acc, 'f', 1) + QStringLiteral("%");
    m_reportKeys = QString::number(m_keystrokes);
    m_reportErr = QString::number(m_errors);

    // 纪录：内置片段按 id 存；自定义练习共用一个键（与网页版行为一致）。
    // 用 m_currentId 判空识别「当前练的是自定义文本」，不依赖分类名
    // （loadCustom 后可能还没切换到 custom 分类）。
    QString recordId;
    if (!m_currentId.isEmpty())
        recordId = m_currentId;
    else if (!m_snippetCode.isEmpty())
        recordId = QStringLiteral("custom");
    if (!recordId.isEmpty()) {
        const bool hadPrev = m_store->hasBest(recordId);
        const int prevWpm = hadPrev ? m_store->bestWpm(recordId) : 0;
        // WPM 更优即新纪录（无旧纪录时首次完成也保存）
        const bool isRecord = !hadPrev || wpm > prevWpm;
        if (isRecord)
            m_store->saveBest(recordId, wpm, acc, int(m_finalElapsed));
        // reportRecord 只表示「打破既有纪录」（与网页版星标展示一致）
        m_reportRecord = hadPrev && wpm > prevWpm;
        if (hadPrev) {
            const double prevAcc = m_store->bestAcc(recordId);
            m_reportBest = m_reportRecord
                ? QStringLiteral("★ 新纪录！（原最快 %1 WPM）").arg(prevWpm)
                : QStringLiteral("最快 %1 WPM · %2%").arg(prevWpm).arg(QString::number(prevAcc, 'f', 1));
        } else {
            m_reportBest = QStringLiteral("—");
        }
    } else {
        m_reportRecord = false;
        m_reportBest = QStringLiteral("—");
    }

    m_reportVisible = true;
    playDoneFx();
    render(); // 完成渲染：全文变绿、无当前字符、提示完成文案
    emit finishedChanged();
}

// ============================================================
// 键盘事件接管
// ============================================================
bool TypingEngine::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (handleKeyEvent(static_cast<QKeyEvent *>(event), true))
            return true; // 消费事件，QML 不再收到
        break;
    case QEvent::KeyRelease:
        handleKeyEvent(static_cast<QKeyEvent *>(event), false);
        break;
    case QEvent::WindowDeactivate:
        clearAllPressed(); // 窗口失焦：清除所有点亮状态
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

bool TypingEngine::handleKeyEvent(QKeyEvent *ke, bool press)
{
    // 1) 自动重复：不更新视觉、不计数（与网页 e.repeat 一致）
    if (ke->isAutoRepeat())
        return false;

    // 2) 键帽视觉反馈（对所有映射到的键，先于其他逻辑）
    const QString code = keyCodeFor(ke);
    if (!code.isEmpty()) {
        if (KeyItem *item = m_codeMap.value(code, nullptr))
            item->setIsPressed(press);
    }
    if (!press)
        return false; // 松开只做视觉复位

    // 3) Escape：先关帮助，再关报告（都未开时同样消费）
    if (ke->key() == Qt::Key_Escape) {
        if (m_helpVisible) {
            setHelpVisible(false);
            return true;
        }
        if (m_reportVisible) {
            closeReport();
            return true;
        }
        return true;
    }

    // 4) 已完成：Enter/Space 重新开始；焦点在控件（按钮）上放行
    if (m_finished) {
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Space) {
            if (focusOnControl())
                return false;
            restart();
        }
        return false;
    }

    // 5) 未加载片段：不接管
    if (m_snippetCode.isEmpty())
        return false;

    // 6) 焦点在文本输入上放行（自定义文本编辑等）
    if (focusOnTextInput())
        return false;

    // 7) Ctrl/Alt/Meta 组合：放行不计数
    if (ke->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        return false;

    // 8) 退格：先删错误缓冲，再删正确前缀
    if (ke->key() == Qt::Key_Backspace) {
        handleBackspace();
        return true;
    }

    // 9) 非单字符键（方向键/F 键/纯修饰键等）：放行不计数
    const QString text = ke->text();
    const bool special = (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter
                          || ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Space);
    if (text.length() != 1 && !special)
        return false;

    // 首次有效击键：启动计时
    if (!m_started) {
        m_started = true;
        m_startTime.start();
    }

    // 10) 错误缓冲非空：任何输入一律记错
    if (!m_wrong.isEmpty()) {
        m_wrong += displayCharFor(ke);
        ++m_errors;
        playErrorFx();
        render();
        emit wrongTextChanged();
        return true;
    }

    // 11) 匹配判定
    const QChar expected = m_snippetCode.at(m_typed.length());
    bool hit = false;
    if (expected == QLatin1Char('\n'))
        hit = (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter);
    else if (expected == QLatin1Char('\t'))
        hit = (ke->key() == Qt::Key_Tab);
    else if (expected == QLatin1Char(' '))
        hit = (ke->key() == Qt::Key_Space);
    else
        hit = (text.length() == 1 && text.at(0) == expected);

    if (hit) {
        m_typed += expected;
        ++m_keystrokes;
        playTickFx();
        if (m_typed.length() >= m_snippetCode.length())
            finish();
        else
            render();
    } else {
        m_wrong += displayCharFor(ke);
        ++m_errors;
        playErrorFx();
        render();
        emit wrongTextChanged();
    }
    return true;
}

void TypingEngine::handleBackspace()
{
    // 错误缓冲优先删除（与网页一致：先纠错再回退正确输入）
    if (!m_wrong.isEmpty()) {
        m_wrong.chop(1);
        emit wrongTextChanged();
    } else if (!m_typed.isEmpty()) {
        m_typed.chop(1);
    } else {
        return;
    }
    render();
}

void TypingEngine::clearAllPressed()
{
    for (const QVector<KeyItem *> &row : std::as_const(m_keyItems)) {
        for (KeyItem *k : row)
            k->setIsPressed(false);
    }
}

bool TypingEngine::focusOnControl() const
{
    // 焦点在 QML 控件（按钮/下拉框等）上：完成态的 Enter/Space 放行给控件。
    // 自测（QCoreApplication，无 GUI 实例）时恒为 false。
    if (auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        QObject *fo = app->focusObject();
        if (fo && (fo->inherits("QQuickControl") || fo->inherits("QQuickTextInput")
                   || fo->inherits("QQuickTextEdit")))
            return true;
    }
    return false;
}

bool TypingEngine::focusOnTextInput() const
{
    if (auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        QObject *fo = app->focusObject();
        if (fo && (fo->inherits("QQuickTextInput") || fo->inherits("QQuickTextEdit")))
            return true;
    }
    return false;
}

// ============================================================
// 工具函数
// ============================================================
QString TypingEngine::keyCodeFor(const QKeyEvent *ke)
{
    // Qt 键值 → KeyboardEvent.code 命名（键盘数据与网页共用同一套 code）
    const int k = ke->key();
    if (k >= Qt::Key_A && k <= Qt::Key_Z)
        return QStringLiteral("Key") + QChar(char(k));
    if (k >= Qt::Key_0 && k <= Qt::Key_9)
        return QStringLiteral("Digit") + QChar(char(k));

    // 修饰键：用 Windows 虚拟键码区分左右
    if (k == Qt::Key_Shift)
        return ke->nativeVirtualKey() == 0xA1 ? kShiftCodeRight : kShiftCodeLeft;
    if (k == Qt::Key_Control)
        return ke->nativeVirtualKey() == 0xA3 ? QStringLiteral("ControlRight") : QStringLiteral("ControlLeft");
    if (k == Qt::Key_Alt)
        return ke->nativeVirtualKey() == 0xA5 ? QStringLiteral("AltRight") : QStringLiteral("AltLeft");
    if (k == Qt::Key_Meta)
        return ke->nativeVirtualKey() == 0x5C ? QStringLiteral("MetaRight") : QStringLiteral("MetaLeft");

    switch (k) {
    case Qt::Key_Space: return QStringLiteral("Space");
    case Qt::Key_Tab: return QStringLiteral("Tab");
    case Qt::Key_Return:
    case Qt::Key_Enter: return QStringLiteral("Enter");
    case Qt::Key_Backspace: return QStringLiteral("Backspace");
    case Qt::Key_CapsLock: return QStringLiteral("CapsLock");
    case Qt::Key_Semicolon: return QStringLiteral("Semicolon");
    case Qt::Key_QuoteLeft: return QStringLiteral("Backquote");
    case Qt::Key_Apostrophe: return QStringLiteral("Quote");
    case Qt::Key_BracketLeft: return QStringLiteral("BracketLeft");
    case Qt::Key_BracketRight: return QStringLiteral("BracketRight");
    case Qt::Key_Comma: return QStringLiteral("Comma");
    case Qt::Key_Period: return QStringLiteral("Period");
    case Qt::Key_Slash: return QStringLiteral("Slash");
    case Qt::Key_Backslash: return QStringLiteral("Backslash");
    case Qt::Key_Minus: return QStringLiteral("Minus");
    case Qt::Key_Equal: return QStringLiteral("Equal");
    // 上档符号：物理键与未按 Shift 时相同（e.code 是物理键位）
    case Qt::Key_Exclam: return QStringLiteral("Digit1");
    case Qt::Key_At: return QStringLiteral("Digit2");
    case Qt::Key_NumberSign: return QStringLiteral("Digit3");
    case Qt::Key_Dollar: return QStringLiteral("Digit4");
    case Qt::Key_Percent: return QStringLiteral("Digit5");
    case Qt::Key_AsciiCircum: return QStringLiteral("Digit6");
    case Qt::Key_Ampersand: return QStringLiteral("Digit7");
    case Qt::Key_Asterisk: return QStringLiteral("Digit8");
    case Qt::Key_ParenLeft: return QStringLiteral("Digit9");
    case Qt::Key_ParenRight: return QStringLiteral("Digit0");
    case Qt::Key_Colon: return QStringLiteral("Semicolon");
    case Qt::Key_QuoteDbl: return QStringLiteral("Quote");
    case Qt::Key_BraceLeft: return QStringLiteral("BracketLeft");
    case Qt::Key_BraceRight: return QStringLiteral("BracketRight");
    case Qt::Key_Less: return QStringLiteral("Comma");
    case Qt::Key_Greater: return QStringLiteral("Period");
    case Qt::Key_Question: return QStringLiteral("Slash");
    case Qt::Key_Bar: return QStringLiteral("Backslash");
    case Qt::Key_AsciiTilde: return QStringLiteral("Backquote");
    case Qt::Key_Underscore: return QStringLiteral("Minus");
    case Qt::Key_Plus: return QStringLiteral("Equal");
    default: return QString();
    }
}

QString TypingEngine::displayCharFor(const QKeyEvent *ke)
{
    // 错误缓冲显示字符：Enter/Tab 用可视化符号（与网页 wrong 缓冲一致）
    const int k = ke->key();
    if (k == Qt::Key_Return || k == Qt::Key_Enter)
        return QStringLiteral("⏎");
    if (k == Qt::Key_Tab)
        return QStringLiteral("⇥");
    return ke->text();
}

QString TypingEngine::escHtml(const QString &s)
{
    QString out;
    out.reserve(s.size());
    for (const QChar c : s) {
        switch (c.unicode()) {
        case '&': out += QStringLiteral("&amp;"); break;
        case '<': out += QStringLiteral("&lt;"); break;
        case '>': out += QStringLiteral("&gt;"); break;
        default: out += c; break;
        }
    }
    return out;
}

QString TypingEngine::codeFragmentHtml(const QString &raw, const QString &color, const QString &bg)
{
    // 单段代码 HTML：转义 + 显示字符替换（⏎/⇥/·），可选背景色
    QString t = escHtml(raw);
    t.replace(QLatin1Char('\n'), QStringLiteral("⏎<br>"));
    t.replace(QLatin1Char('\t'), QStringLiteral("⇥"));
    t.replace(QLatin1Char(' '), QStringLiteral("·"));
    if (bg.isEmpty())
        return QStringLiteral("<span style=\"color:%1;\">%2</span>").arg(color, t);
    return QStringLiteral("<span style=\"color:%1;background-color:%2;\">%3</span>").arg(color, bg, t);
}

QString TypingEngine::fmtTime(qint64 ms)
{
    // m:ss.s（秒部分补零到 4 位，与网页 fmtTime 一致）
    const qint64 m = ms / 60000;
    const double s = (ms % 60000) / 1000.0;
    return QStringLiteral("%1:%2").arg(m).arg(QString::number(s, 'f', 1).rightJustified(4, QLatin1Char('0')));
}

// ============================================================
// 统计公式（静态，自测与引擎共用同一实现）
// ============================================================
double TypingEngine::accFor(int keystrokes, int errors)
{
    if (keystrokes <= 0)
        return 100.0;
    return (keystrokes - errors) * 100.0 / keystrokes;
}

int TypingEngine::wpmFor(int typedLength, qint64 elapsedMs)
{
    const double mins = elapsedMs / 60000.0;
    if (mins <= 0.01)
        return 0;
    return qRound(typedLength / 5.0 / mins);
}

int TypingEngine::cpmFor(int typedLength, qint64 elapsedMs)
{
    const double mins = elapsedMs / 60000.0;
    if (mins <= 0.01)
        return 0;
    return qRound(typedLength / mins);
}

QString TypingEngine::calcGrade(double acc, int wpm)
{
    if (acc >= 98.0 && wpm >= 45)
        return QStringLiteral("S");
    if (acc >= 95.0 && wpm >= 35)
        return QStringLiteral("A");
    if (acc >= 90.0 && wpm >= 25)
        return QStringLiteral("B");
    if (acc >= 85.0)
        return QStringLiteral("C");
    return QStringLiteral("D");
}

void TypingEngine::setSelftestMode(bool on)
{
    s_selftest = on;
}

void TypingEngine::setElapsedOverrideForTest(qint64 ms)
{
    s_elapsedOverride = ms;
}

// ============================================================
// 音效（受 soundOn 开关控制）
// ============================================================
void TypingEngine::playTickFx()
{
    if (m_soundOn)
        m_sound->playTick(m_typed.length());
}

void TypingEngine::playErrorFx()
{
    if (m_soundOn)
        m_sound->playError();
}

void TypingEngine::playDoneFx()
{
    if (m_soundOn)
        m_sound->playDone();
}

} // namespace Keyforge
