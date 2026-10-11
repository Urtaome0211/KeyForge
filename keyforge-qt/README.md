# 键锋 KEYFORGE（Qt 版）

编程打字练习桌面应用。把练打字做成一台 CRT 荧光终端机：输入真实代码片段，实时统计准确率 / WPM / CPM，按键盘指法分色提示，完成评级 S~D。

- 技术栈：Qt 6.12（QML / C++20，llvm-mingw 工具链），CMake + Ninja
- 原网页版（纯前端）：同目录上一级 `index.html` 双击可开

## 目录结构

```
keyforge-qt/
├── CMakeLists.txt        # 工程定义（Qt6 Quick/Qml/Multimedia/QuickControls2）
├── src/
│   ├── main.cpp          # 入口：--selftest 自测 / --smoke 无头冒烟 / 正常启动
│   ├── typingengine.*    # 打字状态机、统计、评级、HTML 渲染、事件过滤
│   ├── keyboarddata.*    # 58 键 5 行键位表（tools/gen-keyboard.js 生成）
│   ├── snippetdata.*     # 32 段练习代码（tools/gen-snippets.js 生成）
│   ├── soundengine.*     # QAudioSink 程序内合成音效（按键/错误/完成）
│   └── settingsstore.*   # QSettings 持久化（设置/进度/历史最佳）
├── qml/Main.qml          # CRT 终端界面（扫描线/暗角/开机爆闪/辉光）
└── tools/
    ├── gen-keyboard.js   # 从网页版 app.js 生成 keyboarddata.cpp
    ├── gen-snippets.js   # 从网页版 snippets.js 生成 snippetdata.cpp
    ├── verify.ps1        # 集成验证流水线（配置→构建→自测→冒烟→可选打包）
    └── package.ps1       # windeployqt 打包成独立可分发目录
```

## 构建

```powershell
$env:PATH = "D:\Qt\Tools\llvm-mingw2217_64\bin;D:\Qt\6.12.0\llvm-mingw_64\bin;D:\Qt\Tools\CMake_64\bin;D:\Qt\Tools\Ninja;$env:PATH"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=D:/Qt/6.12.0/llvm-mingw_64
cmake --build build
```

## 运行与验证

```powershell
build\Keyforge.exe            # 正常启动
build\Keyforge.exe --selftest # 核心逻辑自测（打字状态机/统计/评级/纪录），输出 SELFTEST OK
build\Keyforge.exe --smoke    # 无头冒烟：offscreen 加载 QML，2.5 秒退出，输出 SMOKE OK
pwsh -File tools\verify.ps1           # 一键：配置→构建→自测→冒烟
pwsh -File tools\verify.ps1 -Package  # 一键：上述全部 + 打包 + 打包产物自测
```

## 打包独立可分发目录

```powershell
pwsh -File tools\package.ps1
# 产物：dist\Keyforge\Keyforge.exe（双击即用，整个目录拷贝到没装 Qt 的机器也能运行）
```

打包脚本做了两件保证可移植的事（缺一不可）：

1. **复制应用自身 QML 模块**：exe 内嵌的 `:/Keyforge/` 模块在 Qt 6.12 下运行时解析不到
   `Main` 类型（构建目录里靠 `build\Keyforge\` 文件系统目录兜底），而 windeployqt 不会
   复制应用自己的模块，所以脚本把 `build\Keyforge\{qmldir,Main.qml,Keyforge.qmltypes}`
   手工复制到 `dist\Keyforge\Keyforge\`。
2. **写 qt.conf**（`[Paths] Prefix=.`）：让插件/QML 库路径全部相对 dist 目录解析，
   脱离编译期内嵌的 `D:\Qt\...` 前缀。

## 功能对照（与网页版一致）

- 32 段练习：JavaScript / Python / Java / C / Go / HTML / CSS / SQL / 其他 / 自定义
- 状态机：错误串须退格修正，Enter/Tab/Space 即换行/制表/空格；长按连发不计
- 统计：准确率、WPM、CPM、用时；评级 S(98/45) A(95/35) B(90/25) C(85) D
- 指法键盘：58 键，五指五色（小指/无名指/中指/食指/拇指），下一键辉光脉冲 + Shift 联动
- 音效：程序内实时合成（方波按键音随进度升调、锯齿错误音、完成琶音），可开关
- 持久化：QSettings 保存音效开关、上次分类/片段、自定义文本、每段历史最佳（WPM 更优即纪录）
- CRT 特效：磷光绿辉光、扫描线闪烁、暗角、开机爆闪

## 已知实现说明

- QML 使用 Qt Quick Controls 的 **Basic** 样式（`main.cpp` 里 `QQuickStyle::setStyle("Basic")`），
  Windows 原生样式会忽略 background/contentItem/indicator 的手写定制。
- 内联组件里的主题常量引用统一写成 `root.cGreen` 这样的 **id 限定**形式：
  Qt 6.12 编译期 QML 下，内联组件解析不到外层 Item 的裸引用属性（qmllint 静态能解析，
  运行期才报 ReferenceError）。
- 音效为程序内 WebAudio 风格的方波/锯齿/琶音合成（QAudioSink），无任何音频资源文件。
