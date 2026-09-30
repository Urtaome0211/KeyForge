---
title: 键锋 KEYFORGE：把练打字做成一台 CRT 终端机
date: 2026-09-29
tags: [前端, JavaScript, CSS, 打字练习, 复古风格, 纯前端]
summary: 用零依赖的纯前端技术栈，做一个 CRT 荧光终端风格的编程打字练习网站。分享打字引擎状态机、指法键盘渲染、WebAudio 音效，以及一个关于 CSS filter 与 fixed 定位的真实踩坑。
---

# 键锋 KEYFORGE：把练打字做成一台 CRT 终端机

> 键盘千锤百炼，指法键键生锋。

程序员每天要敲几千行代码，但大多数人的指法并不标准。我最近做了一个编程打字练习网站 —— **键锋 KEYFORGE**：输入真实代码片段，实时统计准确率和速度，屏幕键盘用颜色提示「下一个键该用哪根手指」。

它最大的特点是整体视觉做成了复古 **CRT 荧光终端**：黑底磷光绿字、扫描线、暗角、辉光脉冲、开机爆闪。这篇文章记录它的功能、实现细节，以及开发中一个真实的坑。

## 功能一览

- **32 段内置代码**：JavaScript / Python / Java / C / Go / HTML / CSS / SQL，包含冒泡、插入、选择、归并、快速等排序算法，二分查找、BFS/DFS 等查找搜索算法，以及链表、栈等数据结构
- **实时统计**：准确率、速度 WPM、击键 CPM、用时、进度条与错误计数
- **指法提示**：键盘每个键上有手指颜色圆点，下一个要按的键会辉光脉冲，需要 Shift 时两个 Shift 键同步发光，底部实时提示「下一个键 → 左手中指」
- **错误处理**：按错标红并抖动 + 低音提示，必须退格修正
- **任务报告**：完成后弹出终端报表，评级 S/A/B/C/D，每篇历史最佳记录、新纪录提示
- **自定义练习**：粘贴自己的代码练习；**按键音效**：WebAudio 合成，可开关
- **纯前端**：无构建、无依赖、无网络请求，数据仅存 localStorage

## 打字引擎：一个简单的状态机

核心逻辑只有五个状态字段：

```js
const state = {
  snippet: null,   // 当前练习片段
  typed: '',       // 已正确输入的前缀
  wrong: '',       // 当前待修正的错误串
  errors: 0,       // 累计错误击键
  keystrokes: 0,   // 累计有效击键
  startTime: null, // 首次击键时间
  finished: false
};
```

每次 `keydown` 时做一次字符匹配。这里有个细节：代码里有换行和制表符，而键盘事件里对应的是 `Enter` 和 `Tab`，所以要单独映射：

```js
const isMatch =
  (expected === '\n' && e.key === 'Enter') ||
  (expected === '\t' && e.key === 'Tab') ||
  (expected !== '\n' && expected !== '\t' && e.key === expected);
```

按错时不前进，把错误字符放进 `wrong` 缓冲并显示为红色；此时任何输入都继续记错，**必须先退格修正**（`wrong` 优先弹出，之后才回退 `typed`）。这和主流打字训练器的行为一致，能强迫用户形成「错了就改」的肌肉记忆。

准确率与速度的计算也很直白：

```js
const acc = (keystrokes - errors) / keystrokes * 100; // 准确率
const wpm = typed.length / 5 / (elapsed / 60000);      // 速度：正确字符 ÷ 5 ÷ 分钟
```

## 指法键盘：数据驱动渲染

键盘不是写死的 HTML，而是用一份键位数据表驱动：

```js
function K(cap, shift, finger, hand, code, w) {
  return { cap, shift, finger, hand, code, w: w || 1 };
}
// 主字符 / 上档字符 / 手指(0小指-3食指,4拇指) / 左右手 / 键码 / 宽度单位
```

五行键位全部由 `ROWS` 数组定义，每个键用 `flex-grow: w` 模拟真实键宽（Tab 1.5、Caps 1.8、回车 2.2、空格 8.6），整行加起来都是 15 个单位，天然对齐。构建时顺便生成两张映射表：

- `charMap`：字符 → 键（含上档关系），用于高亮「下一个键」，如 `'{'` 会同时点亮 `[` 键和两个 Shift；
- `codeMap`：`event.code` → 键，用于按键按下时的视觉反馈。

手指配色只有五种、左右手镜像：小指品红、无名指青、中指琥珀、食指绿、拇指灰绿，键盘上的小圆点和图例一一对应。下一个要按的键用 `box-shadow` 辉光 + 脉冲动画提示，按 Shift 时联动高亮，视觉效果非常直观。

## CRT 视觉：几行 CSS 的事

「终端感」主要靠四层叠加：

```css
/* 扫描线：1px 黑线 + 2px 间隔 */
body::after {
  content: '';
  position: fixed;
  inset: 0;
  background: repeating-linear-gradient(0deg,
    rgba(0, 0, 0, 0.16) 0 1px, transparent 1px 3px);
  animation: flicker 5s infinite; /* 偶尔闪一下，模拟显像管 */
}
```

- **磷光绿字**：主色 `#4afc7f`，标题和按键辉光靠多层 `text-shadow` / `box-shadow`；
- **暗角**：`body::before` 叠一个径向渐变，四周压暗；
- **开机爆闪**：页面加载时一个覆盖层先保持暗屏、再爆闪一下、最后淡出，非常「开机」；
- **细节**：Logo 光标步进闪烁、等级字母呼吸辉光、滚动条也染成绿色。

## 音效：零素材的 WebAudio

按键声不需要任何音频文件，用振荡器现场合成：

```js
function tone(freq, dur, type, vol, delay) {
  const t0 = audioCtx.currentTime + (delay || 0);
  const osc = audioCtx.createOscillator();
  const g = audioCtx.createGain();
  // 增益包络：快速起音、指数衰减，避免爆音
  g.gain.setValueAtTime(0.0001, t0);
  g.gain.exponentialRampToValueAtTime(vol, t0 + 0.008);
  g.gain.exponentialRampToValueAtTime(0.0001, t0 + dur);
  osc.connect(g).connect(audioCtx.destination);
  osc.start(t0);
  osc.stop(t0 + dur + 0.02);
}
```

正确按键是一个随进度微升音高的短方波，按错是 120Hz 的锯齿低音，完成时来一段四音琶音。`AudioContext` 在首次用户手势时惰性创建，避免浏览器自动播放策略拦截。

## 一个真实的坑：CSS filter 会劫持 fixed 定位

最初的开机动画是加在 `body` 上的：

```css
body { animation: poweron 0.5s ease-out both; }
@keyframes poweron { 0% { filter: brightness(0); } 100% { filter: brightness(1); } }
```

效果不错，但任务报告弹层（`position: fixed`）突然不在视口居中了 —— 而是跑到了整页文档的中间，完成练习后可能根本看不见报告。

原因：**祖先元素只要应用了任何非 `none` 的 `filter`，就会成为 fixed/absolute 后代的包含块**。而 `animation-fill-mode: both` 让 `filter: brightness(1)` 在动画结束后依然生效，于是 `inset: 0` 的相对对象从视口变成了整个 `body`（文档高度远大于视口），弹层就被「拉」到了页面中部。

修复方式是把开机效果从 `body` 上移走，改用独立的覆盖层：

```css
#boot {
  position: fixed;
  inset: 0;
  background: #020503;
  pointer-events: none;
  animation: bootflash 0.7s ease-out forwards;
}
```

覆盖层自己用 `fixed`，动画只动 `opacity` 和 `background`，不影响任何后代定位。这个坑很小众，但一旦遇到会非常难排查 —— 记住「filter / transform 会创建包含块」这条规则，能少走很多弯路。

## 无头浏览器验证：没有截图也要保证能跑

这个项目没有构建流程，验证分两层：

- **数据层**：用 Node 脚本校验 32 段代码 —— ID 无重复、不含模板字符串转义风险、所有字符都能映射到键盘（例如 C 代码里的 `\n` 必须以反斜杠 + n 存储，否则会变成真实换行破坏显示）；
- **渲染层**：用 Edge 无头模式跑真实页面：

```bash
msedge --headless=new --virtual-time-budget=4000 --dump-dom file:///D:/path/index.html
```

`--dump-dom` 会执行完 JS 再输出 DOM，检查输出的键盘节点数（58 个键）、分类标签数、当前高亮字符是否存在，就能确认引擎正常初始化。再配合 `--screenshot` 出一张预览图。

## 写在最后

整个项目只有四个文件：`index.html`、`style.css`、`app.js`、`snippets.js`，双击就能玩。数据全部存在浏览器本地，没有任何后端。

如果你也想练，直接打开页面选一篇代码开始敲就行；如果你更感兴趣的是实现，建议从 `app.js` 的 `onKeyDown` 状态机读起，那是最核心的二十几行。

后续想加的东西：练习时长统计曲线、键位热力图（哪些键最容易按错）、以及每根手指的独立正确率统计 —— 让「键键生锋」这件事变得可度量。


---

---

---

## 附录：完整源代码

> 以下为项目全部源码（纯前端，零依赖，另有 README.md 为项目文档）。
> 保存以上四个文件到同一目录，用浏览器打开 index.html 即可运行。

- `index.html` — 页面结构
- `style.css` — CRT 荧光终端主题样式
- `app.js` — 打字引擎、指法键盘、统计、音效、报告
- `snippets.js` — 32 段内置练习代码

### index.html

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta name="theme-color" content="#050a06">
  <title>键锋 KEYFORGE — 编程打字练习</title>
  <link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'%3E%3Crect width='16' height='16' fill='%23050a06'/%3E%3Crect x='3' y='4' width='2.5' height='8' fill='%234afc7f'/%3E%3C/svg%3E">
  <link rel="stylesheet" href="style.css">
</head>
<body>
  <div class="wrap">

    <!-- 顶部：标题 Logo 与音效/帮助按钮 -->
    <header>
      <div>
        <div class="logo">▚ KEYFORGE <span style="font-size:.7em">键锋</span><span class="cursor">▮</span></div>
        <div class="subtitle">键盘千锤百炼 · 指法键键生锋 · CRT TERMINAL v1.0</div>
      </div>
      <div class="topbar">
        <button id="btnSound" class="iconbtn" title="开关按键音效">♪ 音效 ON</button>
        <button id="btnHelp" class="iconbtn" title="玩法说明">? 帮助</button>
      </div>
    </header>

    <!-- 语言分类标签（由 app.js 动态生成） -->
    <nav class="tabs" id="tabs"></nav>

    <!-- 片段选择：下拉框 + 随机 / 重新开始 -->
    <div class="select-row" id="selectRow">
      <div class="select-wrap">
        <select id="selSnippet" title="选择练习片段"></select>
      </div>
      <button id="btnShuffle" class="btn" title="随机选一篇">⟳ 随机一篇</button>
      <button id="btnRestart" class="btn" title="从头开始当前片段">↺ 重新开始</button>
    </div>

    <!-- 代码练习面板：进度条 + 代码区 + 元信息（进度/错误数/片段标题） -->
    <section class="panel">
      <div class="progress"><div class="bar" id="progressBar"></div></div>
      <div class="code" id="code"></div>
      <div class="code-meta">
        <span id="progressPct">0%</span>
        <span id="errCount">错误 0</span>
        <span id="snippetTitle"></span>
      </div>
    </section>

    <!-- 实时统计：准确率 / 速度 WPM / 击键 CPM / 用时 -->
    <section class="stats">
      <div class="stat"><div class="v" id="accVal">100.0%</div><div class="l">准确率 ACCURACY</div></div>
      <div class="stat"><div class="v" id="wpmVal">0</div><div class="l">速度 WPM</div></div>
      <div class="stat"><div class="v" id="cpmVal">0</div><div class="l">击键 CPM</div></div>
      <div class="stat"><div class="v" id="timeVal">0:00.0</div><div class="l">用时 TIME</div></div>
    </section>

    <!-- 指法键盘：键位（JS 生成）+ 手指颜色图例 + 实时提示 -->
    <section class="panel">
      <div class="sec-title">指法 FINGERING<span class="sec-sub">键上圆点颜色 = 该用哪根手指 · 高亮键 = 下一个要按的键</span></div>
      <div class="kb-scroll"><div class="kb" id="kb"></div></div>
      <div class="legend" id="legend"></div>
      <div class="hint" id="hint">▶ 等待开始……</div>
    </section>

    <!-- 自定义练习：粘贴自己的代码（默认隐藏，切到「自定义」分类时显示） -->
    <section class="panel custom-panel" id="customPanel" hidden>
      <div class="sec-title">自定义练习 CUSTOM<span class="sec-sub">粘贴任意代码 / 文本，练习自己的内容</span></div>
      <textarea id="customText" rows="6" spellcheck="false"
        placeholder="把想练的代码粘贴到这里，例如：&#10;function hello() {&#10;  console.log('hello');&#10;}"></textarea>
      <div><button id="btnCustom" class="btn primary">▶ 开始自定义练习</button></div>
    </section>

    <!-- 页脚：玩法提示与数据说明 -->
    <footer>
      提示：错误字符显示为红色，需退格修正；速度 = 正确字符数 ÷ 5 ÷ 分钟；成绩会保存在本地浏览器。
      <br>
      美术风格为原创 CRT 荧光终端 · 数据仅存于本地 localStorage
    </footer>

  </div>

  <!-- 任务报告弹层（完成练习后由 JS 显示） -->
  <div class="overlay" id="overlay">
    <div class="report">
      <h2>SYS·REPORT 任务报告</h2>
      <div class="grade" id="gradeVal">A</div>
      <div class="grade-note" id="gradeNote"></div>
      <div class="rows">
        <div class="r"><span>用时 TIME</span><span id="rTime">0:00.0</span></div>
        <div class="r"><span>速度 SPEED</span><span id="rWpm">0 WPM</span></div>
        <div class="r"><span>准确率 ACCURACY</span><span id="rAcc">100.0%</span></div>
        <div class="r"><span>击键 KEYSTROKES</span><span id="rKeys">0</span></div>
        <div class="r"><span>错误 ERRORS</span><span id="rErr">0</span></div>
        <div class="r"><span>历史最佳 BEST</span><span id="rBest">—</span></div>
      </div>
      <div class="report-btns">
        <button class="btn primary" id="rAgain">↺ 再来一次</button>
        <button class="btn" id="rNext">→ 下一篇</button>
        <button class="btn" id="rShuffle">⟳ 随机</button>
        <button class="btn" id="rClose">✕ 关闭</button>
      </div>
      <div class="report-tip">Enter 再来一次 · Esc 关闭</div>
    </div>
  </div>

  <!-- 帮助面板（点击「? 帮助」或 Esc 开关） -->
  <div class="help" id="help" hidden>
    <button class="help-close" id="helpClose" title="关闭">✕</button>
    <h3>▸ 玩法说明</h3>
    <ul>
      <li>输入上方代码：绿色为已输入，<b>高亮字符</b>是当前要按的键。</li>
      <li>按错显示为红色，需先按退格键修正。</li>
      <li>键盘上发光的键 = 下一个要按的键，颜色代表该用的手指。</li>
      <li>大写与符号需同时按 <b>Shift</b>，Shift 也会同步发光提示。</li>
      <li>完成整段后查看任务报告，可挑战更高评级（S/A/B/C/D）。</li>
      <li>在「自定义」分类可粘贴自己的代码练习。</li>
    </ul>
  </div>

  <!-- CRT 开机爆闪覆盖层（纯视觉，加载后自动淡出） -->
  <div id="boot" aria-hidden="true"></div>

  <!-- 先加载代码库，再加载主逻辑 -->
  <script src="snippets.js"></script>
  <script src="app.js"></script>
</body>
</html>
```

### style.css

```css
/* 键锋 KEYFORGE —— CRT 荧光终端主题
   换肤入口：改 :root 里的 CSS 变量即可整体换色。
   图层结构（z-index）：暗角 60 < 报告弹层 90 < 帮助面板 100 < 扫描线 120 < 开机覆盖层 200 */
:root {
  --green: #4afc7f;
  --green-soft: rgba(74, 252, 127, 0.5);
  --green-line: rgba(74, 252, 127, 0.22);
  --amber: #ffb454;
  --red: #ff5e5b;
  --panel: rgba(9, 22, 13, 0.78);
  --mono: "Cascadia Code", "JetBrains Mono", "Fira Code", "Sarasa Mono SC", Consolas, "Courier New", monospace;
}

* { margin: 0; padding: 0; box-sizing: border-box; }

html { scrollbar-color: rgba(74, 252, 127, 0.4) #07100a; }

/* 页面底色：顶部微亮 + 逐级压暗，营造屏幕自发光的感觉 */
body {
  min-height: 100vh;
  background:
    radial-gradient(120% 90% at 50% 0%, rgba(20, 50, 26, 0.55) 0%, rgba(6, 13, 7, 0) 55%),
    linear-gradient(180deg, #07130a 0%, #050a06 60%, #030603 100%);
  background-attachment: fixed;
  color: var(--green);
  font-family: var(--mono);
  -webkit-font-smoothing: antialiased;
}

/* 开机爆闪：暗屏保持 → 荧光爆闪 → 淡入页面。
   特意用独立覆盖层实现，而不是给 body 加 filter 动画：
   祖先元素的 filter 会创建包含块，导致 position:fixed 的弹层不再相对视口定位。 */
#boot {
  position: fixed;
  inset: 0;
  z-index: 200;
  background: #020503;
  pointer-events: none;
  animation: bootflash 0.7s ease-out forwards;
}
@keyframes bootflash {
  0% { opacity: 1; background: #020503; }
  40% { opacity: 1; background: #020503; }
  50% { opacity: 1; background: #a8ffc8; }
  58% { opacity: 1; background: #020503; }
  100% { opacity: 0; background: #020503; }
}

/* 暗角：四周压暗、突出屏幕中心（pointer-events:none 不挡点击） */
body::before {
  content: '';
  position: fixed; inset: 0;
  background: radial-gradient(ellipse at center, transparent 58%, rgba(0, 0, 0, 0.5) 100%);
  pointer-events: none;
  z-index: 60;
}
/* 扫描线：1px 黑线 + 2px 间隔，模拟显像管栅格；flicker 偶尔闪一下 */
body::after {
  content: '';
  position: fixed; inset: 0;
  background: repeating-linear-gradient(0deg, rgba(0, 0, 0, 0.16) 0 1px, transparent 1px 3px);
  pointer-events: none;
  z-index: 120;
  animation: flicker 5s infinite;
}
@keyframes flicker {
  0%, 91%, 94%, 97%, 100% { opacity: 1; }
  92% { opacity: 0.82; }
  95% { opacity: 0.9; }
}

/* 选中文字也保持荧光主题 */
::selection { background: rgba(74, 252, 127, 0.35); color: #eafff0; }

/* 页面主体容器 */
.wrap { max-width: 1040px; margin: 0 auto; padding: 22px 20px 48px; }

header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
  flex-wrap: wrap;
}
/* 标题辉光用红蓝错位 text-shadow 模拟 CRT 色散 */
.logo {
  font-size: clamp(20px, 3.4vw, 28px);
  font-weight: 700;
  letter-spacing: 3px;
  text-shadow:
    1.5px 0 rgba(255, 74, 122, 0.35),
    -1.5px 0 rgba(84, 224, 255, 0.35),
    0 0 18px rgba(74, 252, 127, 0.55);
}
/* Logo 末尾光标：steps(1) 硬切闪烁 */
.logo .cursor { animation: blink 1.1s steps(1) infinite; }
@keyframes blink { 50% { opacity: 0; } }
.subtitle { margin-top: 6px; font-size: 12.5px; color: var(--green-soft); letter-spacing: 1px; }
.topbar { display: flex; gap: 8px; }

/* 通用按钮：暗底绿边，悬停微亮；primary 反色填充 */
.iconbtn, .btn {
  font-family: inherit;
  font-size: 13px;
  padding: 7px 14px;
  color: var(--green);
  background: rgba(74, 252, 127, 0.06);
  border: 1px solid var(--green-line);
  border-radius: 6px;
  cursor: pointer;
  letter-spacing: 1px;
  transition: all 0.15s ease;
}
.iconbtn:hover, .btn:hover {
  border-color: var(--green-soft);
  box-shadow: 0 0 10px rgba(74, 252, 127, 0.25);
  background: rgba(74, 252, 127, 0.12);
}
.iconbtn.off { color: rgba(160, 200, 170, 0.5); border-color: rgba(160, 200, 170, 0.2); }
.btn.primary {
  background: var(--green);
  color: #03130a;
  font-weight: 700;
  border-color: var(--green);
}
.btn.primary:hover { box-shadow: 0 0 16px rgba(74, 252, 127, 0.6); background: #6dff97; }

/* 分类标签：胶囊形，激活态反色填充 */
.tabs { display: flex; flex-wrap: wrap; gap: 8px; margin: 18px 0 12px; }
.tabs button {
  font-family: inherit;
  font-size: 13px;
  padding: 6px 15px;
  cursor: pointer;
  color: var(--green-soft);
  background: transparent;
  border: 1px solid var(--green-line);
  border-radius: 999px;
  transition: all 0.15s ease;
}
.tabs button:hover { border-color: var(--green-soft); }
.tabs button.active {
  background: var(--green);
  border-color: var(--green);
  color: #03130a;
  font-weight: 700;
  box-shadow: 0 0 14px rgba(74, 252, 127, 0.45);
}

.select-row {
  display: flex;
  gap: 10px;
  align-items: center;
  margin-bottom: 12px;
  flex-wrap: wrap;
}
/* 下拉框：去掉系统默认外观，用伪元素补一个箭头 */
.select-wrap { position: relative; }
.select-wrap::after {
  content: '▾';
  position: absolute;
  right: 10px;
  top: 50%;
  transform: translateY(-50%);
  pointer-events: none;
  color: var(--green-soft);
  font-size: 11px;
}
select {
  font-family: inherit;
  font-size: 13px;
  appearance: none;
  -webkit-appearance: none;
  background: #08140c;
  color: var(--green);
  border: 1px solid var(--green-line);
  border-radius: 6px;
  padding: 7px 30px 7px 12px;
  cursor: pointer;
  min-width: 280px;
  letter-spacing: 0.5px;
}
select:focus { outline: none; border-color: var(--green-soft); box-shadow: 0 0 8px rgba(74, 252, 127, 0.25); }
select option { background: #08140c; color: var(--green); }

/* 内容面板：半透明暗绿底 + 外内双层辉光 */
.panel {
  background: var(--panel);
  border: 1px solid var(--green-line);
  border-radius: 10px;
  box-shadow: 0 0 30px rgba(74, 252, 127, 0.05), inset 0 0 40px rgba(74, 252, 127, 0.03);
  margin-bottom: 14px;
  overflow: hidden;
}
.progress { height: 5px; background: rgba(74, 252, 127, 0.08); }
.progress .bar {
  height: 100%;
  width: 0;
  background: linear-gradient(90deg, #1d7a3d, var(--green));
  box-shadow: 0 0 12px rgba(74, 252, 127, 0.7);
  transition: width 0.15s ease;
}
/* 代码区：pre-wrap 保留缩进与空白，长行允许折行；
   字符颜色分层：默认暗绿(.code) / 已输入更暗(.done) / 当前高亮(.current) / 错误红(.wrong) */
.code {
  position: relative;
  height: 300px;
  overflow-y: auto;
  padding: 18px 20px;
  font-size: 20px;
  line-height: 1.75;
  white-space: pre-wrap;
  word-break: break-word;
  color: rgba(158, 226, 178, 0.55);
  scroll-behavior: smooth;
}
.code .done { color: rgba(74, 252, 127, 0.4); }
/* 当前字符：底部光标下划线用 steps(2) 硬切闪烁（二分切，避免渐变淡化） */
.code .current {
  color: #f0fff4;
  background: rgba(74, 252, 127, 0.18);
  border-bottom: 2px solid var(--green);
  border-radius: 3px;
  animation: caret 0.9s steps(2) infinite;
}
@keyframes caret { 50% { border-bottom-color: transparent; } }
.code .wrong {
  color: var(--red);
  background: rgba(255, 94, 91, 0.14);
  border-bottom: 2px solid var(--red);
  border-radius: 3px;
}
.code .placeholder { color: rgba(74, 252, 127, 0.35); letter-spacing: 1px; }
/* 按错时整体抖动（JS 触发两次后自动移除类） */
.code.shake { animation: shake 0.15s linear 2; }
@keyframes shake {
  25% { transform: translateX(-3px); }
  75% { transform: translateX(3px); }
}

.code-meta {
  display: flex;
  gap: 18px;
  align-items: center;
  padding: 9px 20px;
  font-size: 12px;
  color: var(--green-soft);
  border-top: 1px dashed var(--green-line);
  letter-spacing: 1px;
}
.code-meta #snippetTitle { margin-left: auto; }

/* 四项统计卡片：数值颜色由 JS 按准确率分级（good 绿 / warn 琥珀 / bad 红） */
.stats {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 12px;
  margin-bottom: 14px;
}
.stat {
  background: var(--panel);
  border: 1px solid var(--green-line);
  border-radius: 10px;
  padding: 14px 16px;
  text-align: center;
}
.stat .v {
  font-size: 26px;
  font-weight: 700;
  text-shadow: 0 0 14px rgba(74, 252, 127, 0.5);
}
.stat .v.warn { color: var(--amber); text-shadow: 0 0 14px rgba(255, 180, 84, 0.5); }
.stat .v.bad { color: var(--red); text-shadow: 0 0 14px rgba(255, 94, 91, 0.5); }
.stat .l { margin-top: 4px; font-size: 11px; color: var(--green-soft); letter-spacing: 1.5px; }

.sec-title {
  display: flex;
  align-items: baseline;
  gap: 12px;
  padding: 14px 20px 6px;
  font-size: 14px;
  letter-spacing: 3px;
}
.sec-sub { font-size: 11.5px; color: var(--green-soft); letter-spacing: 1px; }

/* 键盘：min-width 保证小屏时横向滚动而不是挤成一团；
   每行 flex-grow 之和固定（=15 个单位，见 app.js 键位表），各行宽度天然对齐 */
.kb-scroll { overflow-x: auto; padding: 14px 16px 8px; }
.kb { min-width: 680px; user-select: none; }
.kb-row { display: flex; gap: 6px; margin-bottom: 6px; }
/* 键帽：下边框加厚模拟键程；--finger 变量由 JS 注入（该键应用的手指颜色） */
.kb-key {
  flex: 1 1 0;
  min-width: 28px;
  height: 46px;
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(180deg, #0c1a11 0%, #07100a 100%);
  border: 1px solid rgba(74, 252, 127, 0.25);
  border-bottom: 3px solid rgba(74, 252, 127, 0.35);
  border-radius: 7px;
  color: #a7e6bc;
  font-size: 15px;
  transition: transform 0.06s ease, border-color 0.12s ease, box-shadow 0.12s ease, background 0.12s ease;
}
/* 键帽左上角圆点 = 该键应用的手指颜色（--finger 变量） */
.kb-key::before {
  content: '';
  position: absolute;
  left: 5px;
  top: 5px;
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: var(--finger);
  opacity: 0.7;
  box-shadow: 0 0 6px var(--finger);
}
.kb-key:hover { border-color: var(--finger); }
.kb-shift {
  position: absolute;
  top: 4px;
  right: 7px;
  font-size: 9.5px;
  opacity: 0.5;
  color: inherit;
}
.kb-label { font-size: 12.5px; letter-spacing: 0.5px; opacity: 0.9; }

/* 下一个要按的键：手指色辉光 + 脉冲动画（上档字符会同时点亮两个 Shift，由 JS 控制） */
.kb-key.next {
  border-color: var(--finger);
  color: var(--finger);
  background: rgba(255, 255, 255, 0.06);
  box-shadow: 0 0 16px var(--finger), inset 0 0 10px rgba(255, 255, 255, 0.06);
  animation: keyPulse 0.9s ease-in-out infinite;
  z-index: 2;
}
@keyframes keyPulse {
  50% { box-shadow: 0 0 30px var(--finger), inset 0 0 16px rgba(255, 255, 255, 0.1); }
}
/* 按下状态：下沉 2px、下边框变薄，模拟真实键程 */
.kb-key.pressed {
  transform: translateY(2px);
  border-bottom-width: 1px;
  background: linear-gradient(180deg, #12301b 0%, #0c1a11 100%);
}
.kb-key.pressed::before { opacity: 1; }

/* 指法图例：五种手指颜色 + 说明 */
.legend {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
  align-items: center;
  padding: 4px 20px 14px;
  font-size: 12px;
  color: var(--green-soft);
}
.legend .lg { display: inline-flex; align-items: center; gap: 7px; letter-spacing: 1px; }
.legend .lg i { display: inline-block; width: 12px; height: 12px; border-radius: 3px; }
.legend .note { opacity: 0.65; }

/* 实时提示行：下一个键 → 哪只手哪根手指（内容由 JS 生成） */
.hint {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 11px 20px;
  font-size: 13.5px;
  color: rgba(74, 252, 127, 0.8);
  border-top: 1px dashed var(--green-line);
  letter-spacing: 0.5px;
  min-height: 42px;
}
.hint .dot { width: 10px; height: 10px; border-radius: 50%; display: inline-block; margin: 0 2px; }
.hint b { color: var(--green); text-shadow: 0 0 10px rgba(74, 252, 127, 0.6); }

/* 自定义练习面板：内容为空点开始时红框警告（.bad 由 JS 短暂添加） */
.custom-panel { padding-bottom: 18px; }
.custom-panel textarea {
  display: block;
  width: calc(100% - 40px);
  margin: 14px 20px;
  padding: 12px 14px;
  font-family: var(--mono);
  font-size: 14px;
  line-height: 1.6;
  background: #061009;
  color: var(--green);
  border: 1px solid var(--green-line);
  border-radius: 8px;
  resize: vertical;
}
.custom-panel textarea:focus {
  outline: none;
  border-color: var(--green-soft);
  box-shadow: 0 0 10px rgba(74, 252, 127, 0.2);
}
.custom-panel textarea.bad {
  border-color: var(--red);
  box-shadow: 0 0 10px rgba(255, 94, 91, 0.4);
}
.custom-panel .btn { margin-left: 20px; }

footer {
  margin-top: 26px;
  font-size: 12px;
  line-height: 1.9;
  color: rgba(74, 252, 127, 0.45);
  letter-spacing: 0.5px;
}
footer a {
  color: var(--green-soft);
  text-decoration: none;
  border-bottom: 1px dotted var(--green-soft);
}
footer a:hover { color: var(--green); }

/* 任务报告弹层：半透明遮罩 + 居中卡片（.show 由 JS 切换） */
.overlay {
  position: fixed;
  inset: 0;
  z-index: 90;
  display: none;
  align-items: center;
  justify-content: center;
  background: rgba(1, 4, 2, 0.85);
  backdrop-filter: blur(3px);
}
.overlay.show { display: flex; }
.report {
  width: min(480px, 92vw);
  padding: 26px 28px;
  background: #07130b;
  border: 1px solid var(--green-soft);
  border-radius: 12px;
  text-align: center;
  box-shadow: 0 0 60px rgba(74, 252, 127, 0.25), inset 0 0 40px rgba(74, 252, 127, 0.05);
  animation: reportIn 0.25s ease-out both;
}
@keyframes reportIn {
  from { transform: scale(0.92); opacity: 0; }
}
.report h2 {
  font-size: 16px;
  letter-spacing: 5px;
  margin-bottom: 6px;
  text-shadow: 0 0 12px rgba(74, 252, 127, 0.5);
}
/* 评级大字：呼吸辉光 */
.report .grade {
  font-size: 76px;
  font-weight: 700;
  line-height: 1.1;
  animation: gradeGlow 1.6s ease-in-out infinite alternate;
}
@keyframes gradeGlow {
  from { text-shadow: 0 0 18px rgba(74, 252, 127, 0.5); }
  to { text-shadow: 0 0 42px rgba(74, 252, 127, 0.95); }
}
.report .grade-note {
  font-size: 12px;
  color: var(--green-soft);
  letter-spacing: 3px;
  margin-bottom: 12px;
}
.report .rows { text-align: left; margin: 10px 0 16px; font-size: 13.5px; }
.report .r {
  display: flex;
  justify-content: space-between;
  padding: 7px 2px;
  border-bottom: 1px dashed var(--green-line);
}
.report .r span:last-child { color: #d8ffe2; }
.report-btns { display: flex; gap: 8px; justify-content: center; flex-wrap: wrap; }
.report-tip {
  margin-top: 14px;
  font-size: 11px;
  color: rgba(74, 252, 127, 0.4);
  letter-spacing: 1px;
}

/* 帮助面板：右上角浮层 */
.help {
  position: fixed;
  top: 64px;
  right: 16px;
  z-index: 100;
  width: min(320px, calc(100vw - 32px));
  background: #07130b;
  border: 1px solid var(--green-soft);
  border-radius: 10px;
  box-shadow: 0 0 30px rgba(74, 252, 127, 0.25);
  padding: 16px 18px;
  font-size: 12.5px;
  line-height: 1.9;
  color: rgba(74, 252, 127, 0.85);
}
.help h3 { font-size: 13px; letter-spacing: 3px; margin-bottom: 8px; }
.help ul { list-style: none; }
.help li { margin-bottom: 6px; }
.help li::before { content: '▸ '; color: var(--green-soft); }
.help b { color: var(--green); }
.help .help-close {
  position: absolute;
  top: 8px;
  right: 10px;
  background: none;
  border: none;
  color: var(--green-soft);
  font-size: 14px;
  cursor: pointer;
  font-family: inherit;
}
.help .help-close:hover { color: var(--green); }

/* 通用隐藏工具类（!important 覆盖 flex 等 display 声明） */
.hidden { display: none !important; }

/* 滚动条同主题配色 */
::-webkit-scrollbar { width: 10px; height: 10px; }
::-webkit-scrollbar-track { background: #07100a; }
::-webkit-scrollbar-thumb { background: rgba(74, 252, 127, 0.3); border-radius: 5px; }
::-webkit-scrollbar-thumb:hover { background: rgba(74, 252, 127, 0.5); }

/* 小屏适配：统计两列、代码字号缩小、下拉框变窄 */
@media (max-width: 700px) {
  .stats { grid-template-columns: repeat(2, 1fr); }
  .code { font-size: 17px; height: 260px; }
  .stat .v { font-size: 22px; }
  select { min-width: 200px; }
}
```

### app.js

```js
// 键锋 KEYFORGE —— 编程打字练习（CRT 荧光终端风格）
// 单文件主逻辑：由 index.html 在 body 末尾引入，依赖 window.SNIPPETS（见 snippets.js）。
// 建议阅读顺序：
//   1. 工具函数与指法数据 —— 键盘与手指如何用数据描述
//   2. 全局状态 —— 一次练习的所有中间量
//   3. 键盘渲染 —— 屏幕键盘如何生成、如何高亮下一个键
//   4. 输入处理 onKeyDown —— 核心状态机（最值得读的部分）
//   5. 评级报告与片段管理
(function () {
  'use strict';

  // ---- 工具函数 ----
  // $：快捷查找 DOM；esc：代码片段会原样拼进 HTML，必须转义 < > &，否则含 HTML 的片段会被当标签解析
  const $ = (id) => document.getElementById(id);
  const esc = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  const fmtTime = (ms) => {
    const m = Math.floor(ms / 60000);
    const s = (ms % 60000) / 1000;
    return m + ':' + s.toFixed(1).padStart(4, '0');
  };

  // localStorage 键名：settings = 用户设置（音效/分类/最近片段），best = 每篇历史最佳成绩
  const LS_SETTINGS = 'cf-settings';
  const LS_BEST = 'cf-best';

  // ---- 指法数据 ----
  // 手指编号 0-4 = 小指/无名指/中指/食指/拇指；左右手共用同一套颜色（镜像配色）
  const FINGER_COLORS = ['#ff6ad5', '#52e5ff', '#ffb454', '#3afb6f', '#c9d6c9'];
  const FINGER_NAMES = ['小指', '无名指', '中指', '食指', '拇指'];
  const LANG_NAMES = { js: 'JavaScript', py: 'Python', java: 'Java', c: 'C', go: 'Go', html: 'HTML', css: 'CSS', sql: 'SQL', other: '其他', custom: '自定义' };

  // 分类标签栏（数组顺序即标签显示顺序）；snippets.js 里每段的 lang 必须能在这里找到对应分类
  const CATEGORIES = [
    { id: 'all', name: '全部' },
    { id: 'js', name: 'JavaScript' },
    { id: 'py', name: 'Python' },
    { id: 'java', name: 'Java' },
    { id: 'c', name: 'C' },
    { id: 'go', name: 'Go' },
    { id: 'html', name: 'HTML' },
    { id: 'css', name: 'CSS' },
    { id: 'sql', name: 'SQL' },
    { id: 'other', name: '其他' },
    { id: 'custom', name: '自定义' }
  ];

  // ---- 键位表 ----
  // 用数据描述整块键盘，五行数组即五行键。K() 的参数：
  //   cap    主字符（按一下打出的字符）
  //   shift  上档字符（Shift+键 打出的字符）
  //   finger 手指编号（0-3 = 小指~食指，4 = 拇指）
  //   hand   'L' 左手 / 'R' 右手
  //   code   KeyboardEvent.code，用于按下时定位键帽
  //   w      键宽单位，配合 CSS flex-grow 模拟真实键宽；每行总和都是 15，各行宽度天然对齐
  /* 键位定义：cap 主字符 / shift 上档字符 / finger 手指(0小指-3食指,4拇指) / hand 手 / code 键码 / w 宽度单位 */
  function K(cap, shift, finger, hand, code, w) {
    return { cap: cap, shift: shift, finger: finger, hand: hand, code: code, w: w || 1 };
  }
  const ROWS = [
    [
      K('`', '~', 0, 'L', 'Backquote'),
      K('1', '!', 0, 'L', 'Digit1'),
      K('2', '@', 1, 'L', 'Digit2'),
      K('3', '#', 2, 'L', 'Digit3'),
      K('4', '$', 3, 'L', 'Digit4'),
      K('5', '%', 3, 'L', 'Digit5'),
      K('6', '^', 3, 'R', 'Digit6'),
      K('7', '&', 3, 'R', 'Digit7'),
      K('8', '*', 2, 'R', 'Digit8'),
      K('9', '(', 1, 'R', 'Digit9'),
      K('0', ')', 0, 'R', 'Digit0'),
      K('-', '_', 0, 'R', 'Minus'),
      K('=', '+', 0, 'R', 'Equal'),
      { label: '⌫', title: 'Backspace', finger: 0, hand: 'R', code: 'Backspace', w: 2 }
    ],
    [
      { label: 'Tab', title: 'Tab', finger: 0, hand: 'L', code: 'Tab', w: 1.5 },
      K('q', 'Q', 0, 'L', 'KeyQ'),
      K('w', 'W', 1, 'L', 'KeyW'),
      K('e', 'E', 2, 'L', 'KeyE'),
      K('r', 'R', 3, 'L', 'KeyR'),
      K('t', 'T', 3, 'L', 'KeyT'),
      K('y', 'Y', 3, 'R', 'KeyY'),
      K('u', 'U', 3, 'R', 'KeyU'),
      K('i', 'I', 2, 'R', 'KeyI'),
      K('o', 'O', 1, 'R', 'KeyO'),
      K('p', 'P', 0, 'R', 'KeyP'),
      K('[', '{', 0, 'R', 'BracketLeft'),
      K(']', '}', 0, 'R', 'BracketRight'),
      K('\\', '|', 0, 'R', 'Backslash', 1.5)
    ],
    [
      { label: 'Caps', title: 'Caps Lock', finger: 0, hand: 'L', code: 'CapsLock', w: 1.8 },
      K('a', 'A', 0, 'L', 'KeyA'),
      K('s', 'S', 1, 'L', 'KeyS'),
      K('d', 'D', 2, 'L', 'KeyD'),
      K('f', 'F', 3, 'L', 'KeyF'),
      K('g', 'G', 3, 'L', 'KeyG'),
      K('h', 'H', 3, 'R', 'KeyH'),
      K('j', 'J', 3, 'R', 'KeyJ'),
      K('k', 'K', 2, 'R', 'KeyK'),
      K('l', 'L', 1, 'R', 'KeyL'),
      K(';', ':', 0, 'R', 'Semicolon'),
      K("'", '"', 0, 'R', 'Quote'),
      { label: '⏎', title: 'Enter 回车', finger: 0, hand: 'R', code: 'Enter', w: 2.2 }
    ],
    [
      { label: 'Shift', title: 'Shift', finger: 0, hand: 'L', code: 'ShiftLeft', w: 2.3 },
      K('z', 'Z', 0, 'L', 'KeyZ'),
      K('x', 'X', 1, 'L', 'KeyX'),
      K('c', 'C', 2, 'L', 'KeyC'),
      K('v', 'V', 3, 'L', 'KeyV'),
      K('b', 'B', 3, 'L', 'KeyB'),
      K('n', 'N', 3, 'R', 'KeyN'),
      K('m', 'M', 3, 'R', 'KeyM'),
      K(',', '<', 2, 'R', 'Comma'),
      K('.', '>', 1, 'R', 'Period'),
      K('/', '?', 0, 'R', 'Slash'),
      { label: 'Shift', title: 'Shift', finger: 0, hand: 'R', code: 'ShiftRight', w: 2.7 }
    ],
    [
      { label: 'Ctrl', title: 'Control', finger: 0, hand: 'L', code: 'ControlLeft', w: 1.6 },
      { label: 'Alt', title: 'Alt', finger: 4, hand: 'L', code: 'AltLeft', w: 1.6 },
      { label: 'Space', title: '空格', finger: 4, hand: 'L', code: 'Space', w: 8.6 },
      { label: 'Alt', title: 'Alt', finger: 4, hand: 'R', code: 'AltRight', w: 1.6 },
      { label: 'Ctrl', title: 'Control', finger: 0, hand: 'R', code: 'ControlRight', w: 1.6 }
    ]
  ];
  const SHIFT_CODES = ['ShiftLeft', 'ShiftRight'];

  // ---- 全局状态：一次练习的全部中间量 ----
  // typed      已正确输入的前缀（其长度即进度）
  // wrong      按错后待修正的字符串；规则：wrong 非空时任何输入都记错，必须先退格清空
  // errors     累计按错次数（含 wrong 期间的每次击键）
  // keystrokes 累计被计数的击键次数，准确率 = (keystrokes - errors) / keystrokes
  // startTime  第一次击键的时间戳（计时从第一键开始），endTime 完成时间
  const state = {
    snippet: null,
    typed: '',
    wrong: '',
    errors: 0,
    keystrokes: 0,
    startTime: null,
    endTime: null,
    finished: false,
    category: 'all'
  };
  let currentId = null; // 当前片段的 id（'custom' 表示自定义练习）
  let ticker = null;    // 计时器句柄，每 120ms 刷新一次速度/用时

  // 用户设置，存 localStorage（cf-settings）
  const settings = {
    sound: true,
    category: 'all',
    snippetId: null,
    customText: ''
  };

  // ---- 按键音效：WebAudio 实时合成，零音频素材 ----
  // AudioContext 必须在用户手势之后创建，否则被浏览器自动播放策略拦截，
  // 所以这里惰性初始化：第一次 keydown 时调用 ensureAudio()
  let audioCtx = null;
  let soundOn = true;

  function ensureAudio() {
    if (!audioCtx) {
      try {
        audioCtx = new (window.AudioContext || window.webkitAudioContext)();
      } catch (e) {
        audioCtx = null;
      }
    }
    if (audioCtx && audioCtx.state === 'suspended') audioCtx.resume();
  }
  // 播放一个音符：振荡器 + 增益包络（快起音、指数衰减），避免爆音和拖尾
  function tone(freq, dur, type, vol, delay) {
    if (!soundOn || !audioCtx) return;
    const t0 = audioCtx.currentTime + (delay || 0);
    const osc = audioCtx.createOscillator();
    const g = audioCtx.createGain();
    osc.type = type || 'square';
    osc.frequency.value = freq;
    g.gain.setValueAtTime(0.0001, t0);
    g.gain.exponentialRampToValueAtTime(vol || 0.05, t0 + 0.008);
    g.gain.exponentialRampToValueAtTime(0.0001, t0 + dur);
    osc.connect(g);
    g.connect(audioCtx.destination);
    osc.start(t0);
    osc.stop(t0 + dur + 0.02);
  }
  // 正确击键：短方波，音高随进度循环微升，给"一直在前进"的听感
  function tickFx() { tone(430 + (state.typed.length % 11) * 45, 0.05, 'square', 0.045); }
  // 按错：120Hz 锯齿低音，明显不悦耳，强化错误记忆
  function errorFx() { tone(120, 0.14, 'sawtooth', 0.06); }
  // 完成：C 大调琶音，四个音符依次响起
  function doneFx() {
    [523, 659, 784, 1047].forEach((f, i) => tone(f, 0.16, 'triangle', 0.07, i * 0.09));
  }

  // ---- 屏幕键盘 ----
  // charMap：练习字符 -> { 键, 是否需要 Shift }，用于高亮"下一个要按的键"
  //          例如 charMap['{'] = { key: BracketLeft, shift: true }
  // codeMap：event.code -> 键对象，用于按键按下/松开时定位键帽做视觉反馈
  // pressedSet：当前处于按下状态的键帽集合（窗口失焦时统一清空，防止"卡键"）
  const charMap = {};
  const codeMap = {};
  const pressedSet = new Set();

  function buildKeyboard() {
    const kb = $('kb');
    kb.innerHTML = '';
    ROWS.forEach((row) => {
      const rowEl = document.createElement('div');
      rowEl.className = 'kb-row';
      row.forEach((key) => {
        const el = document.createElement('div');
        el.className = 'kb-key';
        el.dataset.code = key.code;
        // --finger 是 CSS 变量，驱动键帽左上角的手指颜色圆点（见 style.css .kb-key::before）
        el.style.setProperty('--finger', FINGER_COLORS[key.finger]);
        el.style.flexGrow = key.w;
        el.title = key.title || (key.cap + (key.shift ? ' / Shift+' + key.shift : ''));
        if (key.label !== undefined) {
          el.classList.add('kb-special');
          const s = document.createElement('span');
          s.className = 'kb-label';
          s.textContent = key.label;
          el.appendChild(s);
        } else {
          if (key.shift && key.shift !== key.cap) {
            const s = document.createElement('span');
            s.className = 'kb-shift';
            s.textContent = key.shift;
            el.appendChild(s);
          }
          const c = document.createElement('span');
          c.className = 'kb-cap';
          c.textContent = key.cap;
          el.appendChild(c);
        }
        key.el = el;
        codeMap[key.code] = key;
        rowEl.appendChild(el);
        // 建立字符 -> 键的映射：主字符与上档字符各一条（如 '1' 与 '!' 都指向 Digit1）
        if (key.cap) charMap[key.cap] = { key: key, shift: false };
        if (key.shift) charMap[key.shift] = { key: key, shift: true };
      });
      kb.appendChild(rowEl);
    });
    // 代码中的空白字符映射到对应键：\n=Enter、\t=Tab、空格=Space
    charMap['\n'] = { key: codeMap['Enter'], shift: false };
    charMap['\t'] = { key: codeMap['Tab'], shift: false };
    charMap[' '] = { key: codeMap['Space'], shift: false };
  }

  function renderLegend() {
    const legend = $('legend');
    let html = '';
    for (let f = 0; f < 5; f++) {
      html += '<span class="lg"><i style="background:' + FINGER_COLORS[f] +
        ';box-shadow:0 0 8px ' + FINGER_COLORS[f] + '"></i>' + FINGER_NAMES[f] + '</span>';
    }
    html += '<span class="note">（左右手同色 · 空格用拇指）</span>';
    legend.innerHTML = html;
  }

  function clearNext() {
    document.querySelectorAll('.kb-key.next').forEach((el) => el.classList.remove('next'));
  }

  // 高亮"下一个要按的键"；若需要 Shift（大写/上档符号），两个 Shift 键同步点亮
  function highlightNext(ch) {
    clearNext();
    if (ch === undefined || ch === null) return null;
    const m = charMap[ch];
    if (!m) return null;
    m.key.el.classList.add('next');
    if (m.shift) {
      SHIFT_CODES.forEach((c) => {
        if (codeMap[c]) codeMap[c].el.classList.add('next');
      });
    }
    return m;
  }

  function displayName(ch) {
    if (ch === '\n') return 'Enter';
    if (ch === '\t') return 'Tab';
    if (ch === ' ') return '空格';
    return ch;
  }

  // 底部文字提示：下一个键是什么、该用哪只手哪根手指、要不要按 Shift
  function updateHint(ch) {
    const hintEl = $('hint');
    if (ch === undefined || ch === null) return;
    const m = charMap[ch];
    if (!m) {
      hintEl.innerHTML = '▶ 下一个键：任意键（无指法要求）';
      return;
    }
    const color = FINGER_COLORS[m.key.finger];
    const hand = m.key.hand === 'L' ? '左手' : '右手';
    const extra = m.shift ? ' · 按住 Shift' : '';
    hintEl.innerHTML = '▶ 下一个键 <b>[ ' + esc(displayName(ch)) + ' ]</b> → ' +
      '<i class="dot" style="background:' + color + ';box-shadow:0 0 6px ' + color + '"></i>' +
      hand + FINGER_NAMES[m.key.finger] + extra;
  }

  // ---- 本地存储 ----
  // 全部 try/catch：隐私模式 / 禁用 cookie 时 localStorage 会抛异常，不能因此崩掉页面
  function loadSettings() {
    try {
      const raw = localStorage.getItem(LS_SETTINGS);
      if (raw) Object.assign(settings, JSON.parse(raw));
    } catch (e) { /* 忽略 */ }
    soundOn = settings.sound !== false;
  }
  function saveSettings() {
    try {
      localStorage.setItem(LS_SETTINGS, JSON.stringify(settings));
    } catch (e) { /* 忽略 */ }
  }
  function getBest(id) {
    try {
      const raw = localStorage.getItem(LS_BEST);
      if (!raw) return null;
      const all = JSON.parse(raw);
      return all[id] || null;
    } catch (e) { return null; }
  }
  function setBest(id, rec) {
    try {
      const raw = localStorage.getItem(LS_BEST);
      const all = raw ? JSON.parse(raw) : {};
      all[id] = rec;
      localStorage.setItem(LS_BEST, JSON.stringify(all));
    } catch (e) { /* 忽略 */ }
  }

  // ---- 元素引用（一次性缓存，避免每次渲染都查 DOM）----
  const codeEl = $('code');
  const progressBar = $('progressBar');
  const progressPct = $('progressPct');
  const errCount = $('errCount');
  const snippetTitle = $('snippetTitle');
  const accEl = $('accVal');
  const wpmEl = $('wpmVal');
  const cpmEl = $('cpmVal');
  const timeEl = $('timeVal');
  const hintEl = $('hint');
  const tabsEl = $('tabs');
  const selectEl = $('selSnippet');
  const selectRow = $('selectRow');
  const customPanel = $('customPanel');
  const customTextEl = $('customText');
  const overlay = $('overlay');
  const helpEl = $('help');
  const btnSound = $('btnSound');

  // ---- 计时与统计 ----
  // 计时基准用 performance.now()（单调时钟，不受系统改时间影响）
  function elapsed() {
    if (!state.startTime) return 0;
    return (state.endTime || performance.now()) - state.startTime;
  }
  // 第一次击键才启动计时器；ticker 每 120ms 刷新速度/用时（打字时 render 也会顺带刷新）
  function startTimer() {
    if (state.startTime) return;
    state.startTime = performance.now();
    ticker = setInterval(tick, 120);
  }
  function tick() {
    if (state.finished || !state.startTime) return;
    updateStats();
  }
  // 准确率 = 正确击键 / 总击键；WPM = 正确字符数 ÷ 5（折算成"词"）÷ 分钟；CPM = 正确字符数 ÷ 分钟
  // 准确率颜色分级：>=95 绿（good）、>=85 琥珀（warn）、其余红（bad）
  function updateStats() {
    const keystrokes = state.keystrokes;
    const acc = keystrokes ? (keystrokes - state.errors) / keystrokes * 100 : 100;
    accEl.textContent = acc.toFixed(1) + '%';
    accEl.className = 'v ' + (acc >= 95 ? 'good' : acc >= 85 ? 'warn' : 'bad');
    const mins = elapsed() / 60000;
    const pos = state.typed.length;
    wpmEl.textContent = mins > 0.01 ? Math.round(pos / 5 / mins) : 0;
    cpmEl.textContent = mins > 0.01 ? Math.round(pos / mins) : 0;
    timeEl.textContent = fmtTime(elapsed());
  }

  // 当前字符的显示形式：换行/制表/空格不可见，替换成可见符号
  function curCharHtml(ch) {
    if (ch === '\n') return '⏎\n'; // 换行符号后补真实换行，让后续代码折到下一行
    if (ch === '\t') return '⇥';
    if (ch === ' ') return '·';
    return esc(ch);
  }

  // ---- 渲染管线（每次击键后整体重绘，片段只有几百字符，性能足够）----
  // 拼出的代码区结构：已输入(done) + 当前字符(current) + 错误串(wrong) + 未输入部分
  function render() {
    const sn = state.snippet;
    if (!sn) return;
    const total = sn.code.length;
    const pos = state.typed.length;

    let html = esc(sn.code.slice(0, pos));
    if (!state.finished && pos < total) {
      html += '<span class="current">' + curCharHtml(sn.code[pos]) + '</span>';
      if (state.wrong) html += '<span class="wrong">' + esc(state.wrong) + '</span>';
      html += esc(sn.code.slice(pos + 1));
    } else if (state.wrong) {
      html += '<span class="wrong">' + esc(state.wrong) + '</span>';
    }
    codeEl.innerHTML = html;

    updateStats();

    const pct = Math.floor(pos / total * 100);
    progressBar.style.width = pct + '%';
    progressPct.textContent = pct + '%';
    errCount.textContent = '错误 ' + state.errors;

    const nextCh = state.finished ? undefined : sn.code[pos];
    highlightNext(nextCh);
    updateHint(nextCh);
    scrollToCurrent();
  }

  // 让当前字符始终滚动到可视区域中部附近
  function scrollToCurrent() {
    const cur = codeEl.querySelector('.current');
    if (!cur) return;
    const target = cur.offsetTop - (codeEl.clientHeight - 60) / 2;
    codeEl.scrollTop = Math.max(0, target);
  }

  // 退格：先删错误串，错误串空了才回退正确前缀（与"必须先修正错误"的规则一致）
  function handleBackspace() {
    if (state.wrong.length) {
      state.wrong = state.wrong.slice(0, -1);
    } else if (state.typed.length) {
      state.typed = state.typed.slice(0, -1);
    }
    render();
  }

  function shakeCode() {
    codeEl.classList.remove('shake');
    void codeEl.offsetWidth; // 强制回流以重启动画
    codeEl.classList.add('shake');
    setTimeout(() => codeEl.classList.remove('shake'), 320);
  }

  // ---- 输入处理：核心状态机 ----
  // 处理顺序与取舍：
  //   1. e.repeat 直接忽略 —— 长按一键连发会造成"按住即作弊"
  //   2. 视觉按键反馈：键帽加 pressed 类，keyup 移除
  //   3. Esc：关帮助/报告，永不参与打字
  //   4. 完成态：Enter / 空格 = 再来一次（焦点在按钮上时交还浏览器）
  //   5. 焦点在文本框/下拉框时交还浏览器，否则自定义文本框无法输入
  //   6. Ctrl/Alt/Meta 组合键、方向键、F 键等全部忽略，不记错误
  //   7. Backspace 单独处理：不启动计时、不计错误
  //   8. 其余按键 preventDefault：防止空格滚动页面、引号/斜杠触发浏览器快捷搜索
  //   9. wrong 非空 → 一律记错（必须退格修正后才继续）
  //  10. 字符匹配：Enter/Tab 分别对应代码里的 \n 与 \t
  function onKeyDown(e) {
    if (e.repeat) return; // 长按产生的重复 keydown

    // 视觉按键反馈
    const key = codeMap[e.code];
    if (key && !e.repeat) {
      key.el.classList.add('pressed');
      pressedSet.add(key.el);
    }

    // Esc：关闭帮助 / 报告
    if (e.key === 'Escape') {
      if (!helpEl.hidden) helpEl.hidden = true;
      else if (overlay.classList.contains('show')) hideOverlay();
      return;
    }

    ensureAudio();

    // 完成态：Enter/空格 重来
    if (state.finished) {
      if ((e.key === 'Enter' || e.key === ' ') &&
          !(e.target && e.target.tagName === 'BUTTON')) {
        e.preventDefault();
        restartSame();
      }
      return;
    }

    if (!state.snippet) return;

    // 焦点在表单控件上时交给浏览器处理
    if (e.target === customTextEl || e.target === selectEl) return;

    // 忽略组合键与修饰键
    if (e.ctrlKey || e.metaKey || e.altKey) return;
    if (e.key === 'Backspace') {
      e.preventDefault();
      handleBackspace();
      return;
    }
    if (e.key.length > 1) {
      if (e.key !== 'Enter' && e.key !== 'Tab') return; // 方向键/F键等忽略
    }
    e.preventDefault(); // Enter / Tab / 空格 / 可见字符

    startTimer();
    const sn = state.snippet;
    const expected = sn.code[state.typed.length];

    // 有待修正的错误字符：必须先退格
    if (state.wrong.length > 0) {
      state.keystrokes++;
      state.errors++;
      errorFx();
      shakeCode();
      render();
      return;
    }

    // 代码里的 \n、\t 分别由 Enter、Tab 键打出，其余字符直接比对 e.key
    const isMatch =
      (expected === '\n' && e.key === 'Enter') ||
      (expected === '\t' && e.key === 'Tab') ||
      (expected !== '\n' && expected !== '\t' && e.key === expected);

    if (isMatch) {
      state.typed += expected;
      state.keystrokes++;
      tickFx();
      if (state.typed.length >= sn.code.length) {
        finish();
      } else {
        render();
      }
    } else {
      state.keystrokes++;
      state.errors++;
      state.wrong += (e.key === 'Enter' ? '⏎' : e.key === 'Tab' ? '⇥' : e.key);
      errorFx();
      shakeCode();
      render();
    }
  }

  function onKeyUp(e) {
    const key = codeMap[e.code];
    if (key) {
      key.el.classList.remove('pressed');
      pressedSet.delete(key.el);
    }
  }

  // 窗口失焦时清空所有"按下"状态，避免键帽视觉上"卡住"
  function clearPressed() {
    pressedSet.forEach((el) => el.classList.remove('pressed'));
    pressedSet.clear();
  }

  // ---- 评级：准确率优先，速度加权 ----
  // S: acc>=98 且 wpm>=45；A: >=95 且 >=35；B: >=90 且 >=25；C: >=85；其余 D
  const GRADE_NOTES = {
    S: '出神入化，键盘已与你融为一体',
    A: '非常出色，继续保持节奏',
    B: '不错的成绩，还有提升空间',
    C: '先放慢速度，稳住准确率',
    D: '从指法开始，慢即是快'
  };
  function calcGrade(acc, wpm) {
    if (acc >= 98 && wpm >= 45) return 'S';
    if (acc >= 95 && wpm >= 35) return 'A';
    if (acc >= 90 && wpm >= 25) return 'B';
    if (acc >= 85) return 'C';
    return 'D';
  }

  // 完成：定格时间、停止计时、计算成绩、写入历史最佳、弹出任务报告
  function finish() {
    state.finished = true;
    state.endTime = performance.now();
    if (ticker) {
      clearInterval(ticker);
      ticker = null;
    }
    render();
    doneFx();
    hintEl.innerHTML = '✔ 任务完成 · Enter 再来一次 · Esc 关闭报告';

    const keystrokes = state.keystrokes;
    const acc = keystrokes ? (keystrokes - state.errors) / keystrokes * 100 : 100;
    const mins = elapsed() / 60000;
    const wpm = mins > 0.01 ? Math.round(state.typed.length / 5 / mins) : 0;
    const grade = calcGrade(acc, wpm);

    const prev = getBest(currentId);
    const isRecord = !prev || wpm > prev.wpm;
    if (isRecord) {
      setBest(currentId, { wpm: wpm, acc: acc.toFixed(1), time: Math.round(elapsed()) });
    }

    $('gradeVal').textContent = grade;
    $('gradeNote').textContent = GRADE_NOTES[grade];
    $('rTime').textContent = fmtTime(elapsed());
    $('rWpm').textContent = wpm + ' WPM';
    $('rAcc').textContent = acc.toFixed(1) + '%';
    $('rKeys').textContent = String(state.keystrokes);
    $('rErr').textContent = String(state.errors);
    $('rBest').innerHTML = prev
      ? (isRecord ? '★ 新纪录！（原最快 ' + prev.wpm + ' WPM）' : '最快 ' + prev.wpm + ' WPM · ' + prev.acc + '%')
      : '—';
    overlay.classList.add('show');
  }

  function hideOverlay() {
    overlay.classList.remove('show');
  }

  // ---- 片段管理：加载 / 重置 / 随机 / 自定义 ----
  function resetState() {
    state.typed = '';
    state.wrong = '';
    state.errors = 0;
    state.keystrokes = 0;
    state.startTime = null;
    state.endTime = null;
    state.finished = false;
    if (ticker) {
      clearInterval(ticker);
      ticker = null;
    }
    clearNext();
  }

  // 加载内置片段；id 不存在时进入"空状态"（自定义分类里显示提示文案）
  function loadSnippet(id) {
    const sn = (window.SNIPPETS || []).find((s) => s.id === id) || null;
    state.snippet = sn;
    currentId = sn ? sn.id : null;
    resetState();
    hideOverlay();
    codeEl.classList.remove('shake');
    if (sn) {
      selectEl.value = id;
      settings.snippetId = id;
      saveSettings();
      snippetTitle.textContent = LANG_NAMES[sn.lang] + ' · ' + sn.title;
      render();
    } else {
      codeEl.innerHTML = '<span class="placeholder">◇ 请选择上方代码片段<br>◇ 或在「自定义」分类里粘贴自己的代码练习</span>';
      progressBar.style.width = '0%';
      progressPct.textContent = '0%';
      errCount.textContent = '错误 0';
      snippetTitle.textContent = '';
      hintEl.innerHTML = '▶ 等待开始……';
      updateStats();
    }
    if (document.activeElement && document.activeElement.blur) document.activeElement.blur();
  }

  // 加载用户粘贴的自定义内容（不走 SNIPPETS 库，直接用临时片段对象）
  function loadCustom(code) {
    state.snippet = { id: 'custom', lang: 'custom', title: '自定义练习', code: code };
    currentId = 'custom';
    resetState();
    hideOverlay();
    codeEl.classList.remove('shake');
    snippetTitle.textContent = '自定义 · 练习内容';
    render();
    if (document.activeElement && document.activeElement.blur) document.activeElement.blur();
  }

  function filteredList() {
    return (window.SNIPPETS || []).filter((s) => state.category === 'all' || s.lang === state.category);
  }

  function populateSelect(list) {
    selectEl.innerHTML = '';
    list.forEach((s) => {
      const opt = document.createElement('option');
      opt.value = s.id;
      opt.textContent = '[' + (LANG_NAMES[s.lang] || s.lang) + '] ' + s.title;
      selectEl.appendChild(opt);
    });
  }

  function renderTabs() {
    tabsEl.innerHTML = '';
    CATEGORIES.forEach((cat) => {
      const b = document.createElement('button');
      b.textContent = cat.name;
      b.dataset.cat = cat.id;
      if (cat.id === state.category) b.classList.add('active');
      b.addEventListener('click', () => setCategory(cat.id));
      tabsEl.appendChild(b);
    });
  }

  // 切换分类：重建标签栏与下拉框；恢复上次在该分类下的片段（settings.snippetId）
  // 'custom' 分类特殊处理：显示粘贴面板，代码区进入空状态
  function setCategory(id) {
    state.category = id;
    settings.category = id;
    saveSettings();
    renderTabs();
    const isCustom = id === 'custom';
    customPanel.hidden = !isCustom;
    selectRow.classList.toggle('hidden', isCustom);
    if (isCustom) {
      loadSnippet(null);
      return;
    }
    const list = filteredList();
    populateSelect(list);
    const saved = settings.snippetId && list.find((s) => s.id === settings.snippetId);
    loadSnippet(saved ? saved.id : (list.length ? list[0].id : null));
  }

  function restartSame() {
    if (currentId === 'custom') {
      if (state.snippet) loadCustom(state.snippet.code);
      return;
    }
    if (currentId) loadSnippet(currentId);
  }

  function nextSnippet() {
    if (currentId === 'custom') {
      restartSame();
      return;
    }
    const list = filteredList();
    if (!list.length) return;
    const i = list.findIndex((s) => s.id === currentId);
    loadSnippet(list[(i + 1) % list.length].id);
  }

  function randomSnippet() {
    const list = filteredList();
    if (!list.length) return;
    loadSnippet(list[Math.floor(Math.random() * list.length)].id);
  }

  // 开始自定义练习：统一换行符（\r\n 与 \r 都转成 \n），去掉末尾多余空行
  function startCustom() {
    const code = customTextEl.value.replace(/\r\n?/g, '\n').replace(/\n+$/, '');
    if (!code.trim()) {
      customTextEl.classList.add('bad');
      customTextEl.focus();
      setTimeout(() => customTextEl.classList.remove('bad'), 600);
      return;
    }
    settings.customText = customTextEl.value;
    saveSettings();
    loadCustom(code);
  }

  function toggleSound() {
    soundOn = !soundOn;
    settings.sound = soundOn;
    saveSettings();
    btnSound.textContent = '♪ 音效 ' + (soundOn ? 'ON' : 'OFF');
    btnSound.classList.toggle('off', !soundOn);
    if (soundOn) ensureAudio();
  }

  function toggleHelp() {
    helpEl.hidden = !helpEl.hidden;
  }

  // ---- 事件绑定 ----
  // 键盘事件绑在 document 上（全局生效，无需焦点在特定元素）；
  // 窗口失焦清空按键状态，防止 Alt+Tab 切走后键帽"卡住"
  function bindEvents() {
    document.addEventListener('keydown', onKeyDown);
    document.addEventListener('keyup', onKeyUp);
    window.addEventListener('blur', clearPressed);

    selectEl.addEventListener('change', () => {
      loadSnippet(selectEl.value);
    });
    $('btnShuffle').addEventListener('click', randomSnippet);
    $('btnRestart').addEventListener('click', restartSame);
    $('btnCustom').addEventListener('click', startCustom);
    btnSound.addEventListener('click', toggleSound);
    $('btnHelp').addEventListener('click', toggleHelp);
    $('helpClose').addEventListener('click', () => { helpEl.hidden = true; });

    customTextEl.addEventListener('input', () => {
      settings.customText = customTextEl.value;
      saveSettings();
    });

    $('rAgain').addEventListener('click', restartSame);
    $('rNext').addEventListener('click', nextSnippet);
    $('rShuffle').addEventListener('click', randomSnippet);
    $('rClose').addEventListener('click', hideOverlay);
    overlay.addEventListener('click', (e) => {
      if (e.target === overlay) hideOverlay();
    });
  }

  // 启动顺序：读设置 → 建键盘 → 画图例 → 绑事件 → 恢复上次的分类与片段
  function init() {
    loadSettings();
    buildKeyboard();
    renderLegend();
    bindEvents();
    customTextEl.value = settings.customText || '';
    btnSound.textContent = '♪ 音效 ' + (soundOn ? 'ON' : 'OFF');
    btnSound.classList.toggle('off', !soundOn);
    setCategory(settings.category || 'all');
  }

  init();
})();
```

### snippets.js

```js
// 键锋 KEYFORGE —— 内置练习代码库（window.SNIPPETS，由 app.js 读取）
// 每段的结构：
//   id    全局唯一标识（同时用作历史最佳成绩的 localStorage 存储键）
//   lang  语言分类，必须与 app.js 的 CATEGORIES 对应（js/py/java/c/go/html/css/sql/other）
//   title 下拉框里显示的标题
//   code  要练习的代码原文
// 新增片段注意事项：
//   1. code 只使用可打印 ASCII + 换行；中文等字符无法用键盘直接打出，不适合练习
//   2. 避免反引号与 ${，模板字符串会提前解析导致代码损坏
//   3. 代码里的反斜杠要写成 \\（例如 C 的 "\n" 要写成 "\\n"）
//   4. 缩进统一用空格（不要用 Tab，Tab 会映射成练习里的制表符）

window.SNIPPETS = [
  // ---- JavaScript：语言特性 ----
  {
    id: 'js-fib',
    lang: 'js',
    title: '递归求斐波那契',
    code:
`function fib(n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}

const seq = [];
for (let i = 0; i < 10; i++) {
  seq.push(fib(i));
}
console.log(seq.join(', '));`
  },
  {
    id: 'js-debounce',
    lang: 'js',
    title: '防抖函数',
    code:
`function debounce(fn, wait) {
  let timer = null;
  return function (...args) {
    clearTimeout(timer);
    timer = setTimeout(() => fn(...args), wait);
  };
}

const onResize = debounce(() => {
  console.log('resized');
}, 300);`
  },
  {
    id: 'js-array',
    lang: 'js',
    title: '数组映射与过滤',
    code:
`const nums = [1, 2, 3, 4, 5, 6];
const squares = nums.map(n => n * n);
const evens = nums.filter(n => n % 2 === 0);
const total = nums.reduce((a, b) => a + b, 0);
console.log(squares, evens, total);`
  },
  {
    id: 'js-fetch',
    lang: 'js',
    title: 'async / await 请求',
    code:
`async function getUser(id) {
  const res = await fetch('/api/user/' + id);
  if (!res.ok) throw new Error('not found');
  const data = await res.json();
  return data;
}

getUser(42).then(console.log);`
  },
  {
    id: 'js-class',
    lang: 'js',
    title: '计数器类',
    code:
`class Counter {
  constructor() {
    this.count = 0;
  }
  up() {
    this.count += 1;
    return this;
  }
}

const c = new Counter();
c.up().up().up();
console.log(c.count); // 3`
  },
  {
    id: 'js-regex',
    lang: 'js',
    title: '正则校验邮箱',
    code:
`const email = 'dev@example.com';
const rule = /^[a-z0-9._-]+@[a-z0-9.-]+$/;
const ok = rule.test(email);

if (!ok) {
  console.warn('invalid email: ' + email);
}`
  },

  // ---- JavaScript：排序 / 查找 / 图遍历 / 链表 ----
  {
    id: 'js-bubble',
    lang: 'js',
    title: '冒泡排序',
    code:
`function bubbleSort(arr) {
  const n = arr.length;
  for (let i = 0; i < n - 1; i++) {
    for (let j = 0; j < n - 1 - i; j++) {
      if (arr[j] > arr[j + 1]) {
        const tmp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = tmp;
      }
    }
  }
  return arr;
}

console.log(bubbleSort([5, 3, 8, 1, 2]));`
  },
  {
    id: 'js-binary',
    lang: 'js',
    title: '二分查找',
    code:
`function binarySearch(arr, target) {
  let lo = 0;
  let hi = arr.length - 1;
  while (lo <= hi) {
    const mid = lo + ((hi - lo) >> 1);
    if (arr[mid] === target) return mid;
    if (arr[mid] < target) lo = mid + 1;
    else hi = mid - 1;
  }
  return -1;
}

console.log(binarySearch([1, 3, 5, 7, 9], 7));`
  },
  {
    id: 'js-linkedlist',
    lang: 'js',
    title: '单链表',
    code:
`class Node {
  constructor(value) {
    this.value = value;
    this.next = null;
  }
}

class LinkedList {
  constructor() {
    this.head = null;
  }
  add(value) {
    const node = new Node(value);
    if (!this.head) {
      this.head = node;
      return;
    }
    let cur = this.head;
    while (cur.next) cur = cur.next;
    cur.next = node;
  }
  print() {
    let cur = this.head;
    while (cur) {
      console.log(cur.value);
      cur = cur.next;
    }
  }
}`
  },
  {
    id: 'js-dfs',
    lang: 'js',
    title: '深度优先搜索 DFS',
    code:
`const graph = {
  A: ['B', 'C'],
  B: ['D', 'E'],
  C: ['F'],
  D: [],
  E: [],
  F: []
};

function dfs(node, visited) {
  if (!visited.has(node)) {
    console.log(node);
    visited.add(node);
    graph[node].forEach(n => dfs(n, visited));
  }
}

dfs('A', new Set());`
  },

  // ---- Python：经典算法与数据结构 ----
  {
    id: 'py-fizzbuzz',
    lang: 'py',
    title: 'FizzBuzz',
    code:
`def fizzbuzz(n):
    if n % 15 == 0:
        return 'FizzBuzz'
    if n % 3 == 0:
        return 'Fizz'
    if n % 5 == 0:
        return 'Buzz'
    return str(n)

for i in range(1, 16):
    print(fizzbuzz(i))`
  },
  {
    id: 'py-comprehension',
    lang: 'py',
    title: '列表推导式与字典',
    code:
`prices = [12.5, 9.0, 33.2, 4.5]
hot = [p for p in prices if p > 10]
discount = {p: round(p * 0.8, 2) for p in prices}
print(hot)
print(discount)`
  },
  {
    id: 'py-decorator',
    lang: 'py',
    title: '计时装饰器',
    code:
`import time

def timer(func):
    def wrapper(*args):
        start = time.time()
        result = func(*args)
        cost = time.time() - start
        print('cost:', cost)
        return result
    return wrapper

@timer
def slow(n):
    return sum(range(n))`
  },
  {
    id: 'py-qsort',
    lang: 'py',
    title: '快速排序',
    code:
`def qsort(a):
    if len(a) <= 1:
        return a
    pivot = a[len(a) // 2]
    left = [x for x in a if x < pivot]
    mid = [x for x in a if x == pivot]
    right = [x for x in a if x > pivot]
    return qsort(left) + mid + qsort(right)`
  },

  {
    id: 'py-insertion',
    lang: 'py',
    title: '插入排序',
    code:
`def insertion_sort(a):
    for i in range(1, len(a)):
        key = a[i]
        j = i - 1
        while j >= 0 and a[j] > key:
            a[j + 1] = a[j]
            j -= 1
        a[j + 1] = key
    return a

print(insertion_sort([9, 4, 7, 2, 6]))`
  },
  {
    id: 'py-bfs',
    lang: 'py',
    title: '广度优先搜索 BFS',
    code:
`from collections import deque

graph = {
    'A': ['B', 'C'],
    'B': ['D'],
    'C': ['E', 'F'],
    'D': [],
    'E': [],
    'F': [],
}

def bfs(start):
    visited = set()
    queue = deque([start])
    while queue:
        node = queue.popleft()
        if node not in visited:
            visited.add(node)
            print(node)
            queue.extend(graph[node])

bfs('A')`
  },
  {
    id: 'py-gcd',
    lang: 'py',
    title: '欧几里得求最大公约数',
    code:
`def gcd(a, b):
    while b != 0:
        a, b = b, a % b
    return a

def lcm(a, b):
    return a * b // gcd(a, b)

print(gcd(48, 18))
print(lcm(4, 6))`
  },

  // ---- Java：排序与查找 ----
  {
    id: 'java-merge',
    lang: 'java',
    title: '归并排序',
    code:
`public class MergeSort {
    static void merge(int[] a, int lo, int mid, int hi) {
        int[] tmp = new int[hi - lo + 1];
        int i = lo, j = mid + 1, k = 0;
        while (i <= mid && j <= hi) {
            tmp[k++] = a[i] <= a[j] ? a[i++] : a[j++];
        }
        while (i <= mid) tmp[k++] = a[i++];
        while (j <= hi) tmp[k++] = a[j++];
        for (int t = 0; t < tmp.length; t++) {
            a[lo + t] = tmp[t];
        }
    }

    static void sort(int[] a, int lo, int hi) {
        if (lo >= hi) return;
        int mid = lo + (hi - lo) / 2;
        sort(a, lo, mid);
        sort(a, mid + 1, hi);
        merge(a, lo, mid, hi);
    }

    public static void main(String[] args) {
        int[] a = {5, 2, 9, 1, 7};
        sort(a, 0, a.length - 1);
        System.out.println(java.util.Arrays.toString(a));
    }
}`
  },
  {
    id: 'java-binary',
    lang: 'java',
    title: '二分查找',
    code:
`public class BinarySearch {
    static int search(int[] a, int target) {
        int lo = 0, hi = a.length - 1;
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            if (a[mid] == target) return mid;
            if (a[mid] < target) lo = mid + 1;
            else hi = mid - 1;
        }
        return -1;
    }

    public static void main(String[] args) {
        int[] a = {2, 4, 6, 8, 10};
        System.out.println(search(a, 8));
    }
}`
  },

  // ---- C：指针操作 + 排序 / 查找 / 栈 ----
  {
    id: 'c-quick',
    lang: 'c',
    title: '快速排序',
    code:
`#include <stdio.h>

void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int partition(int a[], int lo, int hi) {
    int pivot = a[hi];
    int i = lo - 1;
    for (int j = lo; j < hi; j++) {
        if (a[j] < pivot) {
            i++;
            swap(&a[i], &a[j]);
        }
    }
    swap(&a[i + 1], &a[hi]);
    return i + 1;
}

void quicksort(int a[], int lo, int hi) {
    if (lo < hi) {
        int p = partition(a, lo, hi);
        quicksort(a, lo, p - 1);
        quicksort(a, p + 1, hi);
    }
}

int main(void) {
    int a[] = {4, 8, 2, 7, 1};
    quicksort(a, 0, 4);
    for (int i = 0; i < 5; i++) {
        printf("%d ", a[i]);
    }
    printf("\\n");
    return 0;
}`
  },
  {
    id: 'c-binary',
    lang: 'c',
    title: '二分查找（指针版）',
    code:
`#include <stdio.h>

int binary_search(int a[], int n, int target) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] == target) return mid;
        if (a[mid] < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

int main(void) {
    int a[] = {1, 3, 5, 7, 9};
    int pos = binary_search(a, 5, 7);
    printf("index = %d\\n", pos);
    return 0;
}`
  },
  {
    id: 'c-stack',
    lang: 'c',
    title: '数组实现栈',
    code:
`#include <stdio.h>

#define MAX 100

int stack[MAX];
int top = -1;

void push(int x) {
    if (top < MAX - 1) {
        stack[++top] = x;
    }
}

int pop(void) {
    if (top >= 0) {
        return stack[top--];
    }
    return -1;
}

int main(void) {
    push(10);
    push(20);
    push(30);
    printf("%d\\n", pop());
    printf("%d\\n", pop());
    return 0;
}`
  },

  // ---- Go：排序与哈希查找 ----
  {
    id: 'go-selection',
    lang: 'go',
    title: '选择排序',
    code:
`package main

import "fmt"

func selectionSort(a []int) []int {
    n := len(a)
    for i := 0; i < n-1; i++ {
        min := i
        for j := i + 1; j < n; j++ {
            if a[j] < a[min] {
                min = j
            }
        }
        a[i], a[min] = a[min], a[i]
    }
    return a
}

func main() {
    nums := []int{8, 3, 6, 1, 9}
    fmt.Println(selectionSort(nums))
}`
  },
  {
    id: 'go-twosum',
    lang: 'go',
    title: '两数之和（哈希查找）',
    code:
`package main

import "fmt"

func twoSum(nums []int, target int) []int {
    seen := make(map[int]int)
    for i, v := range nums {
        if j, ok := seen[target-v]; ok {
            return []int{j, i}
        }
        seen[v] = i
    }
    return nil
}

func main() {
    fmt.Println(twoSum([]int{2, 7, 11, 15}, 9))
}`
  },

  // ---- HTML：页面结构（练习大量 <> 与属性引号） ----
  {
    id: 'html-form',
    lang: 'html',
    title: '登录表单',
    code:
`<form action="/login" method="post">
  <label for="email">Email</label>
  <input id="email" name="email" type="email" required>
  <label for="pass">Password</label>
  <input id="pass" name="pass" type="password" minlength="8">
  <button type="submit">Sign in</button>
</form>`
  },
  {
    id: 'html-page',
    lang: 'html',
    title: 'HTML5 页面骨架',
    code:
`<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Hello World</title>
</head>
<body>
  <h1>Hello, World!</h1>
  <p>This is a minimal HTML5 page.</p>
</body>
</html>`
  },

  // ---- CSS：布局（练习 {} : ; 等高频符号） ----
  {
    id: 'css-flex',
    lang: 'css',
    title: 'Flex 卡片布局',
    code:
`.card {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 24px;
  border: 1px solid #333;
  border-radius: 8px;
  background: #1a1a2e;
}

.card .title {
  font-size: 18px;
  font-weight: 700;
  color: #e0e0e0;
}`
  },
  {
    id: 'css-grid',
    lang: 'css',
    title: 'Grid 响应式布局',
    code:
`.grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

@media (max-width: 768px) {
  .grid {
    grid-template-columns: 1fr;
  }
}`
  },

  // ---- SQL：查询语句 ----
  {
    id: 'sql-join',
    lang: 'sql',
    title: '多表连接查询',
    code:
`SELECT u.name, o.total
FROM users AS u
JOIN orders AS o
  ON o.user_id = u.id
WHERE o.total > 100
ORDER BY o.total DESC;`
  },
  {
    id: 'sql-group',
    lang: 'sql',
    title: '分组统计',
    code:
`SELECT category, COUNT(*) AS cnt
FROM products
WHERE price > 0
GROUP BY category
HAVING cnt >= 3
ORDER BY cnt DESC;`
  },

  // ---- 其他：JSON / 命令行 ----
  {
    id: 'other-json',
    lang: 'other',
    title: 'JSON 配置',
    code:
`{
  "name": "keyforge",
  "version": "1.0.0",
  "scripts": {
    "start": "node index.js"
  },
  "tags": ["typing", "practice"]
}`
  },
  {
    id: 'other-git',
    lang: 'other',
    title: 'Git 与命令行',
    code:
`git init
git add .
git commit -m "feat: add typing trainer"
git push -u origin main
npm install && npm run dev`
  }
];
```

