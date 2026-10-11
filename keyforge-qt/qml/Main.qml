// ================================================================
// 键锋 KEYFORGE —— Qt 6 QML 界面（CRT 荧光终端风格）
// ----------------------------------------------------------------
// 模块职责：纯界面层。所有状态与逻辑都来自 C++ 引擎单例 Engine
//           （模块 URI "Keyforge" 1.0）。QML 不接收键盘输入，
//           按键由 C++ 事件过滤器统一处理。
//
// 对接契约（本文件用到的部分，属性/方法/信号名与契约逐字一致）：
//   属性：snippetTitle / category / wrongText / codeHtml / hintHtml /
//         reportGrade / reportNote / reportTime / reportWpm /
//         reportAcc / reportKeys / reportErr / reportBest /
//         customText(可写) / errors / wpm / cpm / elapsedMs /
//         progressPct / accuracy / finished / reportVisible /
//         helpVisible(可写) / soundOn / placeholderMode /
//         categories / snippets / keyRows
//   方法：setCategory / loadSnippet / restart / nextSnippet /
//         randomSnippet / startCustom / toggleSound / closeReport /
//         setCustomTextFromUi
//   信号：codeHtmlChanged / snippetLoaded / wrongTextChanged /
//         customTextChanged（经 Connections 监听）
//
// 图层结构（z）：内容 1 < 暗角 60 < 报告弹层 90 < 帮助 100
//               < 扫描线 120 < 开机爆闪 200
// ================================================================

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Keyforge 1.0

Window {
    id: win
    width: 1280
    height: 860
    minimumWidth: 900
    minimumHeight: 620
    visible: true
    title: "键锋 KEYFORGE"
    color: "#050a06"

    Item {
        id: root
        anchors.fill: parent

        // ---------------- 主题常量（照抄网页 :root） ----------------
        readonly property color cGreen: "#4afc7f"        // 主荧光绿
        readonly property color cGreenSoft: "#804afc7f"  // 半透明绿 0.5
        readonly property color cGreenLine: "#384afc7f"  // 细线绿 0.22
        readonly property color cAmber: "#ffb454"        // 琥珀
        readonly property color cRed: "#ff5e5b"          // 红
        readonly property color cPanel: "#c709160d"      // 面板底 rgba(9,22,13,0.78)
        // QML 无法写 CSS 式字体回退链，只能选单一 family；
        // 选 Consolas：Windows 全版本内置、必为等宽（详见 qml/README.md）
        readonly property string monoFont: "Consolas"

        // 用时格式 m:ss.t（与网页 fmtTime 一致；手写补零，避免依赖 padStart）
        function fmtTime(ms) {
            var m = Math.floor(ms / 60000)
            var s = ((ms % 60000) / 1000).toFixed(1)
            if (s.length < 4) s = "0" + s
            return m + ":" + s
        }

        // ==================== 内联组件 ====================

        // 通用面板：半透明暗绿底 + 绿细边（尺寸由内容 Column 撑开）
        component Panel: Rectangle {
            color: root.cPanel
            border.color: root.cGreenLine
            radius: 10
            implicitHeight: childrenRect.height
            implicitWidth: childrenRect.width
        }

        // 通用按钮：暗底绿边；primary 反色填充；dim 为音效关闭态
        component CrButton: Rectangle {
            property string text: ""
            property bool primary: false
            property bool dim: false
            property bool hovered: false
            signal clicked()

            implicitWidth: btnText.implicitWidth + 28
            implicitHeight: btnText.implicitHeight + 14
            radius: 6
            color: primary ? (hovered ? "#6dff97" : root.cGreen)
                           : (hovered ? "#1f4afc7f" : "#0f4afc7f")
            border.width: 1
            border.color: dim ? "#33a0c8aa"
                              : (primary ? root.cGreen
                                         : (hovered ? root.cGreenSoft : root.cGreenLine))

            // 悬停微光
            Rectangle {
                anchors.fill: parent
                radius: 7
                color: root.cGreen
                opacity: (hovered && !dim) ? 0.08 : 0
            }

            Text {
                id: btnText
                anchors.centerIn: parent
                text: parent.text
                color: dim ? "#80a0c8aa" : (primary ? "#03130a" : root.cGreen)
                font { family: root.monoFont; pixelSize: 13; bold: primary; letterSpacing: 1 }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onContainsMouseChanged: hovered = containsMouse
                onClicked: parent.clicked()
            }
        }

        // 分类标签：胶囊形，激活态反色填充
        component PillTab: Rectangle {
            property string text: ""
            property bool active: false
            property bool hovered: false
            signal clicked()

            implicitWidth: tabTxt.implicitWidth + 30
            implicitHeight: 30
            radius: implicitHeight / 2
            color: active ? root.cGreen : "transparent"
            border.width: 1
            border.color: active ? root.cGreen : (hovered ? root.cGreenSoft : root.cGreenLine)

            // 激活态辉光
            Rectangle {
                anchors.fill: parent
                radius: implicitHeight / 2
                color: root.cGreen
                opacity: active ? 0.15 : 0
            }

            Text {
                id: tabTxt
                anchors.centerIn: parent
                text: parent.text
                color: active ? "#03130a" : root.cGreenSoft
                font { family: root.monoFont; pixelSize: 13; bold: active; letterSpacing: 1 }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onContainsMouseChanged: hovered = containsMouse
                onClicked: parent.clicked()
            }
        }

        // 统计卡片：数值 + 标签；准确率分级着色（>=95 绿 / >=85 琥珀 / 其余红）
        component StatCard: Rectangle {
            property string labelText: ""
            property string valueText: ""
            property color valueColor: root.cGreen

            color: root.cPanel
            border.color: root.cGreenLine
            radius: 10
            implicitHeight: cardCol.implicitHeight

            Column {
                id: cardCol
                width: parent.width
                topPadding: 14
                bottomPadding: 12
                spacing: 4

                Item {
                    width: parent.width
                    height: 34
                    // 辉光模拟：同色放大微透文字垫底（不依赖效果模块）
                    Text {
                        anchors.centerIn: parent
                        text: valueText
                        color: valueColor
                        opacity: 0.45
                        scale: 1.05
                        font { family: root.monoFont; pixelSize: 27; bold: true }
                    }
                    Text {
                        anchors.centerIn: parent
                        text: valueText
                        color: valueColor
                        font { family: root.monoFont; pixelSize: 26; bold: true }
                    }
                }

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: labelText
                    color: root.cGreenSoft
                    font { family: root.monoFont; pixelSize: 11; letterSpacing: 1.5 }
                }
            }
        }

        // 键帽：宽 = flex * 单位宽（每行总 flex = 15）；isNext 辉光脉冲 / isPressed 下沉
        component KeyCap: Rectangle {
            property real unit: 1
            readonly property bool next: !!modelData.isNext
            readonly property bool pressed: !!modelData.isPressed
            readonly property string finger: modelData.fingerColor ? modelData.fingerColor : "#4afc7f"
            readonly property string cap: modelData.cap !== undefined ? modelData.cap : ""
            readonly property string shift: modelData.shift ? modelData.shift : ""
            readonly property string label: modelData.label ? modelData.label : ""
            property bool hovered: false

            implicitHeight: 46
            implicitWidth: 46
            Layout.preferredHeight: 46
            Layout.preferredWidth: (modelData.flex !== undefined ? modelData.flex : 1) * unit
            Layout.minimumWidth: 28

            radius: 7
            color: "transparent"
            gradient: Gradient {
                GradientStop { position: 0.0; color: pressed ? "#12301b" : "#0c1a11" }
                GradientStop { position: 1.0; color: pressed ? "#0c1a11" : "#07100a" }
            }
            border.width: 1
            border.color: next ? finger : (hovered ? finger : "#404afc7f")

            // 按下下沉 2px（transform 不影响布局）
            transform: Translate { y: pressed ? 2 : 0 }

            // isNext：同色外圈辉光 + 0.9s 呼吸脉冲（两层矩形模拟，无需效果模块）
            Rectangle {
                id: halo
                anchors.fill: parent
                radius: 9
                color: "transparent"
                border.width: 1.5
                border.color: finger
                visible: next
                opacity: 0.4
                scale: 1.02
            }
            SequentialAnimation {
                running: next
                loops: Animation.Infinite
                NumberAnimation { target: halo; property: "opacity"; from: 0.25; to: 0.85; duration: 450; easing.type: Easing.InOutSine }
                NumberAnimation { target: halo; property: "opacity"; from: 0.85; to: 0.25; duration: 450; easing.type: Easing.InOutSine }
            }

            // isNext 键面提亮（同网页 rgba(255,255,255,0.06)）
            Rectangle {
                anchors.fill: parent
                radius: 6
                color: next ? "#0fffffff" : "transparent"
            }

            // 下边框加厚模拟键程：默认 3px，按下 1px
            Rectangle {
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                height: pressed ? 1 : 3
                radius: 3
                color: "#594afc7f"
            }

            // 左上角手指色圆点（6px 圆点 + 同色辉光）
            Rectangle { x: 3; y: 3; width: 10; height: 10; radius: 5; color: finger; opacity: pressed ? 0.55 : 0.35 }
            Rectangle { x: 5; y: 5; width: 6; height: 6; radius: 3; color: finger; opacity: pressed ? 1.0 : 0.7 }

            // 键帽主字符 / 特殊键标签
            Text {
                anchors.centerIn: parent
                text: label !== "" ? label : cap
                color: next ? finger : "#a7e6bc"
                opacity: label !== "" ? 0.9 : 1.0
                font { family: root.monoFont; pixelSize: label !== "" ? 12 : 15; letterSpacing: label !== "" ? 0.5 : 0 }
            }

            // 右上角上档字符
            Text {
                visible: label === "" && shift !== ""
                anchors { top: parent.top; topMargin: 4; right: parent.right; rightMargin: 7 }
                text: shift
                color: next ? finger : "#a7e6bc"
                opacity: 0.5
                font { family: root.monoFont; pixelSize: 10 }
            }

            // 悬停反馈 + ToolTip（纯视觉，不处理点击；按键由 C++ 过滤器负责）
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
                onContainsMouseChanged: hovered = containsMouse
            }
            ToolTip.visible: hovered
            ToolTip.delay: 400
            ToolTip.text: label !== "" ? label : (cap + (shift !== "" ? " / Shift+" + shift : ""))
        }

        // 任务报告里的一行：标签左、数值右、底部分隔线
        component ReportRow: Item {
            property string labelText: ""
            property string valueText: ""
            width: parent.width
            height: 32
            Rectangle {
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                height: 1
                color: root.cGreenLine
            }
            Text {
                anchors { left: parent.left; leftMargin: 2; verticalCenter: parent.verticalCenter }
                text: labelText
                color: root.cGreenSoft
                font { family: root.monoFont; pixelSize: 13 }
            }
            Text {
                anchors { right: parent.right; rightMargin: 2; verticalCenter: parent.verticalCenter }
                text: valueText
                color: "#d8ffe2"
                font { family: root.monoFont; pixelSize: 13 }
            }
        }

        // ============ 背景（z0）：垂直渐变 + 顶部径向亮斑 ============
        Rectangle {
            z: 0
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#07130a" }
                GradientStop { position: 0.6; color: "#050a06" }
                GradientStop { position: 1.0; color: "#030603" }
            }
        }
        // 顶部径向亮斑：用 Canvas 手绘径向渐变（本机 kit 无 Qt5Compat.GraphicalEffects，
        // QML 原生也没有 RadialGradient 类型，Canvas 的 createRadialGradient 是等效替代）
        Canvas {
            z: 0
            anchors.fill: parent
            enabled: false
            onPaint: {
                const ctx = getContext("2d")
                const g = ctx.createRadialGradient(width * 0.5, 0, 0,
                                                   width * 0.5, 0,
                                                   Math.max(width * 0.6, height * 0.45))
                g.addColorStop(0.0, "#8c14321a")
                g.addColorStop(0.55, "#00060d07")
                g.addColorStop(1.0, "#00060d07")
                ctx.fillStyle = g
                ctx.fillRect(0, 0, width, height)
            }
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
        }

        // ============ 主内容（z1）整体纵向滚动 ============
        ScrollView {
            id: mainScroll
            z: 1
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Column {
                id: page
                width: Math.min(1040, mainScroll.availableWidth)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                spacing: 14
                topPadding: 22
                bottomPadding: 48

                // ---- 1. 头部：Logo + 光标闪烁 + 音效/帮助按钮 ----
                RowLayout {
                    width: parent.width
                    spacing: 14

                    Column {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 6

                        Item {
                            id: logoBox
                            width: logoMain.width
                            height: logoMain.height
                            property string logoHtml: "<span style='font-size:26px'>▚ KEYFORGE </span><span style='font-size:18px'>键锋</span>"

                            // 辉光/色散层：绿光放大垫底 + 红蓝错位（模拟 CRT 色散）
                            Text {
                                text: logoBox.logoHtml + "▮"
                                textFormat: Text.RichText
                                anchors.centerIn: parent
                                scale: 1.05
                                color: root.cGreen
                                opacity: 0.5
                                font { family: root.monoFont; pixelSize: 26; bold: true; letterSpacing: 3 }
                            }
                            Text {
                                text: logoBox.logoHtml + "▮"
                                textFormat: Text.RichText
                                anchors { left: parent.left; leftMargin: 1.5; top: parent.top }
                                color: "#59ff4a7a"
                                font { family: root.monoFont; pixelSize: 26; bold: true; letterSpacing: 3 }
                            }
                            Text {
                                text: logoBox.logoHtml + "▮"
                                textFormat: Text.RichText
                                anchors { left: parent.left; leftMargin: -1.5; top: parent.top }
                                color: "#5954e0ff"
                                font { family: root.monoFont; pixelSize: 26; bold: true; letterSpacing: 3 }
                            }
                            Text {
                                id: logoMain
                                text: logoBox.logoHtml
                                textFormat: Text.RichText
                                anchors { left: parent.left; top: parent.top }
                                color: root.cGreen
                                font { family: root.monoFont; pixelSize: 26; bold: true; letterSpacing: 3 }
                            }
                            // 光标方块：Timer 硬切闪烁（同网页 steps(1)）
                            Text {
                                id: logoCursor
                                text: "▮"
                                anchors { baseline: logoMain.baseline; left: logoMain.right }
                                color: root.cGreen
                                opacity: root.cursorOn ? 1 : 0
                                font { family: root.monoFont; pixelSize: 26; bold: true; letterSpacing: 3 }
                            }
                        }

                        Text {
                            text: "键盘千锤百炼 · 指法键键生锋 · CRT TERMINAL v1.0"
                            color: root.cGreenSoft
                            font { family: root.monoFont; pixelSize: 12; letterSpacing: 1 }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Row {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 8
                        CrButton {
                            text: "♪ 音效 " + (Engine.soundOn ? "ON" : "OFF")
                            dim: !Engine.soundOn
                            onClicked: Engine.toggleSound()
                        }
                        CrButton {
                            text: "? 帮助"
                            onClicked: Engine.helpVisible = !Engine.helpVisible
                        }
                    }
                }

                // ---- 2. 分类标签栏（胶囊按钮，active 随 Engine.category） ----
                Flow {
                    width: parent.width
                    spacing: 8
                    topPadding: 18
                    bottomPadding: 12

                    Repeater {
                        model: Engine.categories
                        delegate: PillTab {
                            text: modelData.name
                            active: Engine.category === modelData.id
                            onClicked: Engine.setCategory(modelData.id)
                        }
                    }
                }

                // ---- 3. 片段选择行（custom 分类时整行隐藏） ----
                RowLayout {
                    width: parent.width
                    spacing: 10
                    visible: Engine.category !== 'custom'

                    ComboBox {
                        id: snippetCombo
                        Layout.preferredWidth: 280
                        model: Engine.snippets
                        textRole: "title"
                        font.family: root.monoFont
                        font.pixelSize: 13
                        onActivated: (index) => Engine.loadSnippet(Engine.snippets[index].id)

                        // 引擎侧切换片段（random/next/分类切换）时，同步下拉框选中项；
                        // 自定义文本（currentSnippetId 为空）时置为无选中。
                        Connections {
                            target: Engine
                            function onSnippetLoaded() {
                                for (let i = 0; i < Engine.snippets.length; ++i) {
                                    if (Engine.snippets[i].id === Engine.currentSnippetId) {
                                        if (snippetCombo.currentIndex !== i)
                                            snippetCombo.currentIndex = i
                                        return
                                    }
                                }
                                snippetCombo.currentIndex = -1
                            }
                        }

                        background: Rectangle {
                            color: "#08140c"
                            border.color: snippetCombo.activeFocus ? root.cGreenSoft : root.cGreenLine
                            radius: 6
                        }
                        contentItem: Text {
                            leftPadding: 12
                            rightPadding: 30
                            verticalAlignment: Text.AlignVCenter
                            text: snippetCombo.currentIndex >= 0 && snippetCombo.currentIndex < Engine.snippets.length
                                  ? Engine.snippets[snippetCombo.currentIndex].title : ""
                            color: root.cGreen
                            elide: Text.ElideRight
                            font: snippetCombo.font
                        }
                        indicator: Text {
                            x: snippetCombo.width - width - 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: "▾"
                            color: root.cGreenSoft
                            font.pixelSize: 11
                        }
                        popup: Popup {
                            y: snippetCombo.height + 4
                            width: snippetCombo.width
                            implicitHeight: contentItem.implicitHeight + 8
                            padding: 4
                            background: Rectangle { color: "#08140c"; border.color: root.cGreenLine; radius: 6 }
                            contentItem: ListView {
                                clip: true
                                implicitHeight: Math.min(contentHeight, 300)
                                model: snippetCombo.popup.visible ? snippetCombo.delegateModel : null
                                currentIndex: snippetCombo.highlightedIndex
                                ScrollIndicator.vertical: ScrollIndicator { }
                                delegate: ItemDelegate {
                                    width: ListView.view.width
                                    highlighted: snippetCombo.highlightedIndex === index
                                    contentItem: Text {
                                        leftPadding: 12
                                        text: "[" + (modelData.lang ? modelData.lang : "?") + "] " + modelData.title
                                        color: root.cGreen
                                        elide: Text.ElideRight
                                        font.family: root.monoFont
                                        font.pixelSize: 13
                                    }
                                    background: Rectangle { color: highlighted ? "#0d2a17" : "transparent"; radius: 4 }
                                }
                            }
                        }
                    }

                    CrButton { text: "⟳ 随机一篇"; onClicked: Engine.randomSnippet() }
                    CrButton { text: "↺ 重新开始"; onClicked: Engine.restart() }
                    Item { Layout.fillWidth: true }
                }

                // ---- 4. 代码练习面板：进度条 + 代码区 + 元信息 ----
                Panel {
                    width: parent.width
                    Column {
                        width: parent.width

                        // 顶部进度条：宽度随 progressPct，150ms 缓动
                        Item {
                            width: parent.width
                            height: 5
                            clip: true
                            Rectangle { anchors.fill: parent; color: "#144afc7f" }
                            Rectangle {
                                id: progressBar
                                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                                width: Math.max(0, parent.width * Engine.progressPct / 100)
                                radius: 3
                                gradient: Gradient {
                                    orientation: Gradient.Horizontal
                                    GradientStop { position: 0.0; color: "#1d7a3d" }
                                    GradientStop { position: 1.0; color: root.cGreen }
                                }
                                Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
                            }
                        }

                        // 代码区：富文本 20px 等宽 1.75 行高；滚动位置按 progressPct 比例定位
                        Flickable {
                            id: codeFlick
                            width: parent.width
                            height: 300
                            clip: true
                            contentWidth: width
                            boundsBehavior: Flickable.StopAtBounds
                            Text {
                                id: codeText
                                width: parent.width
                                padding: 18
                                wrapMode: Text.WrapAnywhere
                                textFormat: Engine.placeholderMode ? Text.PlainText : Text.RichText
                                text: Engine.placeholderMode
                                      ? "◇ 请选择上方代码片段\n◇ 或在「自定义」分类里粘贴自己的代码练习"
                                      : Engine.codeHtml
                                color: Engine.placeholderMode ? "#594afc7f" : "#8c9ee2b2"
                                lineHeight: 1.75
                                font { family: root.monoFont; pixelSize: 20 }
                            }
                        }

                        // 底部元信息：进度 / 错误数 / 片段标题
                        Item {
                            width: parent.width
                            height: 40
                            Rectangle {
                                anchors { left: parent.left; right: parent.right; top: parent.top }
                                height: 1
                                color: root.cGreenLine
                            }
                            RowLayout {
                                anchors { left: parent.left; right: parent.right; leftMargin: 20; rightMargin: 20; verticalCenter: parent.verticalCenter }
                                spacing: 18
                                Text { text: Math.round(Engine.progressPct) + "%"; color: root.cGreenSoft; font { family: root.monoFont; pixelSize: 12; letterSpacing: 1 } }
                                Text { text: "错误 " + Engine.errors; color: root.cGreenSoft; font { family: root.monoFont; pixelSize: 12; letterSpacing: 1 } }
                                Item { Layout.fillWidth: true }
                                Text { text: Engine.snippetTitle; color: root.cGreenSoft; font { family: root.monoFont; pixelSize: 12; letterSpacing: 1 } }
                            }
                        }
                    }
                }

                // ---- 5. 实时统计：四张卡片 ----
                RowLayout {
                    width: parent.width
                    spacing: 12

                    StatCard {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 200
                        labelText: "准确率 ACCURACY"
                        valueText: Engine.accuracy.toFixed(1) + "%"
                        valueColor: Engine.accuracy >= 95 ? root.cGreen : (Engine.accuracy >= 85 ? root.cAmber : root.cRed)
                    }
                    StatCard {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 200
                        labelText: "速度 WPM"
                        valueText: String(Engine.wpm)
                        valueColor: root.cGreen
                    }
                    StatCard {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 200
                        labelText: "击键 CPM"
                        valueText: String(Engine.cpm)
                        valueColor: root.cGreen
                    }
                    StatCard {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 200
                        labelText: "用时 TIME"
                        valueText: root.fmtTime(Engine.elapsedMs)
                        valueColor: root.cGreen
                    }
                }

                // ---- 6. 指法键盘面板：键位 + 图例 + 提示 ----
                Panel {
                    width: parent.width
                    Column {
                        width: parent.width

                        // 标题行
                        Item {
                            width: parent.width
                            height: kbTitle.implicitHeight + 20
                            Text {
                                id: kbTitle
                                x: 20
                                y: 14
                                text: "指法 FINGERING"
                                color: root.cGreen
                                font { family: root.monoFont; pixelSize: 14; letterSpacing: 3 }
                            }
                            Text {
                                anchors { baseline: kbTitle.baseline; left: kbTitle.right; leftMargin: 12 }
                                text: "键上圆点颜色 = 该用哪根手指 · 高亮键 = 下一个要按的键"
                                color: root.cGreenSoft
                                font { family: root.monoFont; pixelSize: 11; letterSpacing: 1 }
                            }
                        }

                        // 键盘区：min-width 680，窄屏横向滚动（同网页 kb-scroll）
                        Flickable {
                            id: kbFlick
                            width: parent.width
                            height: 276
                            clip: true
                            contentWidth: kbInner.width
                            contentHeight: kbFlick.height
                            flickableDirection: Flickable.HorizontalFlick
                            boundsBehavior: Flickable.StopAtBounds

                            Item {
                                id: kbInner
                                x: 16
                                y: 14
                                width: Math.max(680, kbFlick.width - 32)
                                height: 254
                                property real keyUnit: width / 15   // 每行总 flex = 15

                                Column {
                                    anchors.fill: parent
                                    spacing: 6
                                    Repeater {
                                        model: Engine.keyRows
                                        delegate: RowLayout {
                                            width: parent.width
                                            spacing: 6
                                            Repeater {
                                                model: modelData
                                                delegate: KeyCap { unit: kbInner.keyUnit }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // 图例：五指五色 + 说明
                        Flow {
                            width: parent.width
                            leftPadding: 20
                            rightPadding: 20
                            topPadding: 4
                            bottomPadding: 14
                            spacing: 16
                            Repeater {
                                model: [
                                    ["#ff6ad5", "小指"],
                                    ["#52e5ff", "无名指"],
                                    ["#ffb454", "中指"],
                                    ["#3afb6f", "食指"],
                                    ["#c9d6c9", "拇指"]
                                ]
                                delegate: Row {
                                    spacing: 7
                                    Item {
                                        width: 16
                                        height: 16
                                        Rectangle { anchors.centerIn: parent; width: 16; height: 16; radius: 8; color: modelData[0]; opacity: 0.35 }
                                        Rectangle { anchors.centerIn: parent; width: 12; height: 12; radius: 3; color: modelData[0] }
                                    }
                                    Text {
                                        text: modelData[1]
                                        color: root.cGreenSoft
                                        font { family: root.monoFont; pixelSize: 12; letterSpacing: 1 }
                                    }
                                }
                            }
                            Text {
                                text: "（左右手同色 · 空格用拇指）"
                                color: root.cGreenSoft
                                opacity: 0.65
                                font { family: root.monoFont; pixelSize: 12 }
                            }
                        }

                        // 实时提示行（内容由 C++ 生成 hintHtml）
                        Item {
                            width: parent.width
                            implicitHeight: Math.max(42, hintText.implicitHeight + 22)
                            Rectangle {
                                anchors { left: parent.left; right: parent.right; top: parent.top }
                                height: 1
                                color: root.cGreenLine
                            }
                            Text {
                                id: hintText
                                anchors { left: parent.left; right: parent.right; leftMargin: 20; rightMargin: 20; verticalCenter: parent.verticalCenter }
                                textFormat: Text.RichText
                                text: Engine.hintHtml
                                color: "#cc4afc7f"
                                wrapMode: Text.Wrap
                                font { family: root.monoFont; pixelSize: 13; letterSpacing: 0.5 }
                            }
                        }
                    }
                }

                // ---- 7. 自定义练习面板（仅 custom 分类显示） ----
                Panel {
                    width: parent.width
                    visible: Engine.category === 'custom'
                    Column {
                        width: parent.width

                        Item {
                            width: parent.width
                            height: customTitle.implicitHeight + 20
                            Text {
                                id: customTitle
                                x: 20
                                y: 14
                                text: "自定义练习 CUSTOM"
                                color: root.cGreen
                                font { family: root.monoFont; pixelSize: 14; letterSpacing: 3 }
                            }
                            Text {
                                anchors { baseline: customTitle.baseline; left: customTitle.right; leftMargin: 12 }
                                text: "粘贴任意代码 / 文本，练习自己的内容"
                                color: root.cGreenSoft
                                font { family: root.monoFont; pixelSize: 11; letterSpacing: 1 }
                            }
                        }

                        // 文本框：与 Engine.customText 双向同步（用户改动时回写引擎）
                        Item {
                            width: parent.width
                            height: 130
                            TextArea {
                                id: customArea
                                x: 20
                                width: parent.width - 40
                                height: 130
                                color: root.cGreen
                                font { family: root.monoFont; pixelSize: 14 }
                                wrapMode: TextArea.Wrap
                                selectByMouse: true
                                text: Engine.customText
                                placeholderText: "把想练的代码粘贴到这里，例如：\nfunction hello() {\n  console.log('hello');\n}"
                                placeholderTextColor: "#594afc7f"
                                background: Rectangle {
                                    color: "#061009"
                                    border.color: customArea.activeFocus ? root.cGreenSoft : root.cGreenLine
                                    radius: 8
                                }
                                onTextChanged: {
                                    if (text !== Engine.customText)
                                        Engine.setCustomTextFromUi(text)
                                }
                            }
                        }

                        // 开始按钮（点击后交出焦点，避免按键落回文本框）
                        Item {
                            width: parent.width
                            height: 40
                            CrButton {
                                x: 20
                                y: 5
                                text: "▶ 开始自定义练习"
                                primary: true
                                onClicked: {
                                    customArea.focus = false
                                    Engine.startCustom()
                                }
                            }
                        }

                        Item { width: parent.width; height: 18 }
                    }
                }

                // ---- 8. 页脚：两行提示（同网页版） ----
                Column {
                    width: parent.width
                    spacing: 0
                    topPadding: 12
                    Text {
                        width: parent.width
                        text: "提示：错误字符显示为红色，需退格修正；速度 = 正确字符数 ÷ 5 ÷ 分钟；成绩与设置保存在本机（QSettings）。"
                        color: "#734afc7f"
                        lineHeight: 1.9
                        font { family: root.monoFont; pixelSize: 12; letterSpacing: 0.5 }
                    }
                    Text {
                        width: parent.width
                        text: "美术风格为原创 CRT 荧光终端 · 数据仅存于本机，不联网"
                        color: "#734afc7f"
                        lineHeight: 1.9
                        font { family: root.monoFont; pixelSize: 12; letterSpacing: 0.5 }
                    }
                }
            }
        }

        // ============ 特效层：暗角（z60，Canvas 手绘径向渐变） ============
        Canvas {
            z: 60
            anchors.fill: parent
            enabled: false
            onPaint: {
                const ctx = getContext("2d")
                const cx = width / 2
                const cy = height / 2
                const g = ctx.createRadialGradient(cx, cy, 0,
                                                   cx, cy,
                                                   Math.sqrt(cx * cx + cy * cy))
                g.addColorStop(0.0, "transparent")
                g.addColorStop(0.58, "transparent")
                g.addColorStop(1.0, "#80000000")
                ctx.fillStyle = g
                ctx.fillRect(0, 0, width, height)
            }
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
        }

        // ============ 任务报告弹层（z90，finished && reportVisible） ============
        Item {
            id: reportOverlay
            z: 90
            anchors.fill: parent
            visible: Engine.finished && Engine.reportVisible

            // 全屏遮罩：rgba(1,4,2,0.85)，点击空白处关闭
            Rectangle {
                anchors.fill: parent
                color: "#d9010402"
                MouseArea {
                    anchors.fill: parent
                    onClicked: Engine.closeReport()
                }
            }

            // 居中报告卡片
            Rectangle {
                id: reportCard
                width: Math.min(480, parent.width * 0.92)
                implicitHeight: reportCol.implicitHeight
                anchors.centerIn: parent
                color: "#07130b"
                border.color: root.cGreenSoft
                radius: 12

                // 外辉光
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -4
                    radius: 16
                    color: root.cGreen
                    opacity: 0.1
                    z: -1
                }

                // 入场动画：缩放入场（每次变为可见时重放）
                onVisibleChanged: {
                    if (visible) {
                        scale = 0.92
                        opacity = 0
                        reportIn.restart()
                    }
                }

                Column {
                    id: reportCol
                    width: parent.width
                    leftPadding: 28
                    rightPadding: 28
                    topPadding: 26
                    bottomPadding: 24
                    spacing: 6

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: "SYS·REPORT 任务报告"
                        color: root.cGreen
                        font { family: root.monoFont; pixelSize: 16; letterSpacing: 5 }
                    }

                    // 评级大字（76px）+ 呼吸辉光
                    Item {
                        width: parent.width
                        height: 92
                        Text {
                            id: gradeHalo
                            anchors.centerIn: parent
                            text: Engine.reportGrade
                            color: root.cGreen
                            opacity: 0.4
                            scale: 1.08
                            font { family: root.monoFont; pixelSize: 76; bold: true }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: Engine.reportGrade
                            color: root.cGreen
                            font { family: root.monoFont; pixelSize: 76; bold: true }
                        }
                    }

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: Engine.reportNote
                        color: root.cGreenSoft
                        font { family: root.monoFont; pixelSize: 12; letterSpacing: 3 }
                    }

                    // 六行报告
                    Column {
                        width: parent.width
                        topPadding: 10
                        bottomPadding: 6
                        spacing: 0
                        ReportRow { labelText: "用时 TIME"; valueText: Engine.reportTime }
                        ReportRow { labelText: "速度 SPEED"; valueText: Engine.reportWpm }
                        ReportRow { labelText: "准确率 ACCURACY"; valueText: Engine.reportAcc }
                        ReportRow { labelText: "击键 KEYSTROKES"; valueText: String(Engine.reportKeys) }
                        ReportRow { labelText: "错误 ERRORS"; valueText: String(Engine.reportErr) }
                        ReportRow { labelText: "历史最佳 BEST"; valueText: Engine.reportBest }
                    }

                    // 四个按钮
                    Item {
                        width: parent.width
                        height: reportBtnRow.implicitHeight
                        Row {
                            id: reportBtnRow
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 8
                            CrButton { text: "↺ 再来一次"; primary: true; onClicked: Engine.restart() }
                            CrButton { text: "→ 下一篇"; onClicked: Engine.nextSnippet() }
                            CrButton { text: "⟳ 随机"; onClicked: Engine.randomSnippet() }
                            CrButton { text: "✕ 关闭"; onClicked: Engine.closeReport() }
                        }
                    }

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: "Enter 再来一次 · Esc 关闭"
                        color: "#664afc7f"
                        font { family: root.monoFont; pixelSize: 11; letterSpacing: 1 }
                    }
                }

                // 吞掉点击，避免误触遮罩关闭
                MouseArea { anchors.fill: parent }
            }
        }
        SequentialAnimation {
            id: reportIn
            ParallelAnimation {
                NumberAnimation { target: reportCard; property: "scale"; to: 1; duration: 250; easing.type: Easing.OutCubic }
                NumberAnimation { target: reportCard; property: "opacity"; to: 1; duration: 250 }
            }
        }
        SequentialAnimation {
            id: gradeGlow
            running: reportOverlay.visible
            loops: Animation.Infinite
            NumberAnimation { target: gradeHalo; property: "opacity"; from: 0.25; to: 0.7; duration: 800; easing.type: Easing.InOutSine }
            NumberAnimation { target: gradeHalo; property: "opacity"; from: 0.7; to: 0.25; duration: 800; easing.type: Easing.InOutSine }
        }

        // ============ 帮助面板（z100，右上浮层） ============
        Rectangle {
            id: helpPanel
            z: 100
            visible: Engine.helpVisible
            width: Math.min(320, win.width - 32)
            implicitHeight: helpCol.implicitHeight
            anchors { top: parent.top; topMargin: 64; right: parent.right; rightMargin: 16 }
            color: "#07130b"
            border.color: root.cGreenSoft
            radius: 10

            // 外辉光
            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                radius: 13
                color: root.cGreen
                opacity: 0.08
                z: -1
            }

            // 防点击穿透（空 MouseArea 吞掉点击）
            MouseArea { anchors.fill: parent }

            Column {
                id: helpCol
                width: parent.width
                leftPadding: 18
                rightPadding: 18
                topPadding: 16
                bottomPadding: 16
                spacing: 6

                Text {
                    text: "▸ 玩法说明"
                    color: root.cGreen
                    font { family: root.monoFont; pixelSize: 13; letterSpacing: 3 }
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    lineHeight: 1.9
                    text: "▸ 输入上方代码：绿色为已输入，高亮字符是当前要按的键。"
                    color: "#d94afc7f"
                    font { family: root.monoFont; pixelSize: 12 }
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    lineHeight: 1.9
                    text: "▸ 按错显示为红色，需先按退格键修正。"
                    color: "#d94afc7f"
                    font { family: root.monoFont; pixelSize: 12 }
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    lineHeight: 1.9
                    text: "▸ 键盘上发光的键 = 下一个要按的键，颜色代表该用的手指。"
                    color: "#d94afc7f"
                    font { family: root.monoFont; pixelSize: 12 }
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    lineHeight: 1.9
                    text: "▸ 大写与符号需同时按 Shift，Shift 也会同步发光提示。"
                    color: "#d94afc7f"
                    font { family: root.monoFont; pixelSize: 12 }
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    lineHeight: 1.9
                    text: "▸ 完成整段后查看任务报告，可挑战更高评级（S/A/B/C/D）。"
                    color: "#d94afc7f"
                    font { family: root.monoFont; pixelSize: 12 }
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    lineHeight: 1.9
                    text: "▸ 在「自定义」分类可粘贴自己的代码练习。"
                    color: "#d94afc7f"
                    font { family: root.monoFont; pixelSize: 12 }
                }
            }

            Text {
                id: helpClose
                text: "✕"
                anchors { top: parent.top; topMargin: 8; right: parent.right; rightMargin: 10 }
                color: closeMa.containsMouse ? root.cGreen : root.cGreenSoft
                font { family: root.monoFont; pixelSize: 14 }
                MouseArea {
                    id: closeMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Engine.helpVisible = false
                }
            }
        }

        // ============ 扫描线（z120）：Canvas 画 1px/3px 栅格 + flicker ============
        Canvas {
            id: scanlines
            z: 120
            anchors.fill: parent
            enabled: false
            opacity: 1
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "rgba(0,0,0,0.16)"
                for (var y = 0; y < height; y += 3)
                    ctx.fillRect(0, y, width, 1)
            }
            SequentialAnimation on opacity {
                running: true
                loops: Animation.Infinite
                NumberAnimation { to: 1; duration: 4550 }     // 0%→91% 保持 1
                NumberAnimation { to: 0.82; duration: 50 }    // 92%
                NumberAnimation { to: 1; duration: 100 }      // 94%
                NumberAnimation { to: 0.9; duration: 50 }     // 95%
                NumberAnimation { to: 1; duration: 100 }      // 97%
                NumberAnimation { to: 1; duration: 150 }      // 100%
            }
        }

        // ============ 开机爆闪（z200）：暗屏 → 荧光爆闪 → 淡入（约 0.7s） ============
        Rectangle {
            id: bootFlash
            z: 200
            anchors.fill: parent
            color: "#020503"
            enabled: false
            SequentialAnimation {
                running: true
                PauseAnimation { duration: 280 }                                                    // 0%→40% 保持暗屏
                PropertyAnimation { target: bootFlash; property: "color"; to: "#a8ffc8"; duration: 70 }  // →50% 爆闪
                PropertyAnimation { target: bootFlash; property: "color"; to: "#020503"; duration: 56 }  // →58% 回暗
                PropertyAnimation { target: bootFlash; property: "opacity"; to: 0; duration: 294 }        // →100% 淡出
                ScriptAction { script: bootFlash.visible = false }
            }
        }

        // ============ 全局计时器 / 引擎信号连接 ============

        // Logo 光标：550ms 硬切闪烁（同网页 1.1s steps(1) 的 50% 开关）
        property bool cursorOn: true
        Timer {
            interval: 550
            repeat: true
            running: true
            onTriggered: root.cursorOn = !root.cursorOn
        }

        // 代码区滚动：codeHtml 每次变化后，把 contentY 定位到 progressPct 比例处
        // （内容高度超出视口时 contentY = (contentHeight - height) * progressPct / 100）
        Timer {
            id: scrollTimer
            interval: 30
            repeat: false
            onTriggered: {
                var h = codeFlick.contentHeight - codeFlick.height
                codeFlick.contentY = (h > 0) ? h * Engine.progressPct / 100 : 0
            }
        }

        // 按错时代码区抖动（同网页 .shake，两次 0.15s）
        SequentialAnimation {
            id: codeShake
            running: false
            loops: 2
            NumberAnimation { target: codeText; property: "x"; from: 0; to: -3; duration: 37 }
            NumberAnimation { target: codeText; property: "x"; from: -3; to: 3; duration: 75 }
            NumberAnimation { target: codeText; property: "x"; from: 3; to: 0; duration: 37 }
        }

        Connections {
            target: Engine
            function onCodeHtmlChanged() { scrollTimer.restart() }
            function onSnippetLoaded() { scrollTimer.restart() }
            function onWrongTextChanged() {
                if (Engine.wrongText && Engine.wrongText.length > 0)
                    codeShake.restart()
            }
            function onCustomTextChanged() {
                // 引擎侧改动（如恢复存档）时同步回文本框
                if (customArea.text !== Engine.customText)
                    customArea.text = Engine.customText
            }
        }
    }
}
