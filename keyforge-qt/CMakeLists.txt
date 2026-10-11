# ============================================================
# 键锋 KEYFORGE —— Qt 6 QML 打字练习应用
#
# 结构：
#   - 核心逻辑（C++，本文件编译）：src/*.cpp
#   - QML 界面：qml/*.qml（他人并行编写；qml/ 为空时也能配置成功）
#   - 数据生成：tools/gen-keyboard.js、tools/gen-snippets.js
#     （产物 src/keyboarddata.*、src/snippetdata.* 已提交，构建不依赖 Node）
# ============================================================
cmake_minimum_required(VERSION 3.16)
project(Keyforge VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 模块：Quick（界面）、Qml（模块注册）、Multimedia（音效）、
# QuickControls2（QQuickStyle::setStyle 强制 Basic 样式，让手写控件外观生效）
find_package(Qt6 REQUIRED COMPONENTS Quick Qml Multimedia QuickControls2)
qt_standard_project_setup()

# QML 文件：qml/ 目录可能尚不存在或为空——GLOB 天然容忍空结果。
# RELATIVE 得到相对路径；再对每个文件设置 QT_RESOURCE_ALIAS=文件名，
# 保证 Main.qml 落在 :/qt/qml/Keyforge/Main.qml（loadFromModule 按模块根查找）。
file(GLOB_RECURSE QML_SOURCES CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "qml/*.qml")
foreach(qml_file IN LISTS QML_SOURCES)
    get_filename_component(_qml_name "${qml_file}" NAME)
    set_source_files_properties("${qml_file}" PROPERTIES QT_RESOURCE_ALIAS "${_qml_name}")
endforeach()

qt_add_executable(Keyforge
    src/main.cpp
    src/typingengine.h src/typingengine.cpp
    src/soundengine.h src/soundengine.cpp
    src/settingsstore.h src/settingsstore.cpp
    src/keyboarddata.h src/keyboarddata.cpp
    src/snippetdata.h src/snippetdata.cpp
)

# Qml 由 qt_add_qml_module 自动链接；Quick（QGuiApplication/渲染）、
# Multimedia（QAudioSink 音效）、QuickControls2（QQuickStyle）需显式链接。
target_link_libraries(Keyforge PRIVATE Qt6::Quick Qt6::Multimedia Qt6::QuickControls2)

# QML 模块：URI "Keyforge"，main.cpp 用 loadFromModule("Keyforge","Main") 加载
qt_add_qml_module(Keyforge
    URI "Keyforge"
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
)
