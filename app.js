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
