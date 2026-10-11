# keyforge-qt / qml —— 界面层说明

## 文件清单
- `Main.qml` —— 唯一的 QML 文件：CRT 荧光终端全界面 + 特效层（约 1230 行，UTF-8，中文注释）。
- 不依赖任何外部资源：图标均为字符，扫描线为 Canvas 绘制，辉光为多层半透明模拟（未用 QtQuick.Effects，避免 58 键效果层的性能/裁切风险）。

## 对接模块
- `import Keyforge 1.0`，单例 `Engine`；属性/方法/信号名与契约逐字一致（qmllint 已通过）。
- QML 不接收键盘输入，按键由 C++ 事件过滤器处理。

## 给 C++ 端的说明（含存疑点，请核对）
1. **报告数值格式**：`reportTime` / `reportWpm` / `reportAcc` 按「引擎已格式化为网页同款显示串」直接原样显示（QML 未再拼接单位）；`reportKeys` / `reportErr` 由 QML `String()` 转字符串；`reportBest` 原样显示。若引擎给的是裸数值（如 12.3、45、98.7），显示会与网页版不一致，请在 C++ 侧格式化或告知后改 QML。
2. **codeHtml / hintHtml 必须是带内联样式（inline style）的 HTML**：QML 富文本不支持 CSS class。约定：已输入字符色 `rgba(158,226,178,0.55)`、当前字符建议白底/高亮（网页为 #f0fff4 + 底边框）、错误字符 `#ff5e5b`。QML 能解析的 HTML 子集见 Qt `supported HTML subset` 文档（div/span/font 的 color 均可用，建议都用 span style）。
3. **占位模式**：`placeholderMode == true` 时 QML 显示本地占位文案「◇ 请选择上方代码片段…」，此时 codeHtml 被忽略；`false` 时显示 codeHtml。
4. **已知限制（ComboBox 同步）**：`randomSnippet()` / `nextSnippet()` 后 ComboBox 无法自动高亮当前片段（引擎未暴露当前片段 id）。如需同步，可在引擎加一个 `currentSnippetId` 属性，QML 端可据此设 `snippetCombo.currentIndex`。
5. **keyRows 约定**：5 行嵌套 QVariantList，每行 flex 之和须为 15（键宽 = flex × 面板宽/15，与 tools/gen-keyboard.js 的校验一致）；元素字段：code/cap/shift/label/fingerColor/hand/flex/isNext/isPressed（isNext/isPressed 须带变更通知，QML 依赖其刷新高亮/下沉态）。
6. **customText 双向同步**：文本框改动 → `setCustomTextFromUi(text)`；引擎侧改动（如加载存档）→ QML 监听 `customTextChanged` 回写文本框。
7. **字体**：全局固定 `Consolas`（QML 无 CSS 字体回退链；Consolas 为 Windows 全版本内置等宽字体）。若想用网页原版 Cascadia Code，把 Main.qml 中 `monoFont` 改为 `"Cascadia Mono"` 即可（需目标机安装）。
8. **窗口/缩放**：1280×860、minimum 900×620；整体 ScrollView 纵向滚动；键盘区窄屏横向 Flickable（内容 min-width 680）。

## 特效层 z 序（勿动）
暗角 60 < 报告弹层 90 < 帮助面板 100 < 扫描线 120 < 开机爆闪 200；背景与内容在 0–1。装饰层均 `enabled: false`，不拦截鼠标；报告弹层/帮助面板自带 MouseArea 吞点击。

## 自检记录
- `qmllint`（Qt 6.12.0）：无语法错误；仅剩 Engine 单例访问的 [unqualified] 风格警告（运行期合法，无法进一步限定，属预期）。
- 冒烟测试要求（verify.ps1 --smoke）：offscreen 加载 2.5s 无 fatal 输出；如 stderr 出现绑定循环警告请回传行号。
