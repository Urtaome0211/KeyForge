// ============================================================
// 本文件由 tools/gen-keyboard.js 自动生成 —— 请勿手工修改！
// 数据来源：网页版 app.js 的 ROWS 键位表（58 键 5 行，逐字符一致）
// 重新生成：node tools/gen-keyboard.js
// ============================================================
#pragma once

#include <QString>
#include <QVector>

namespace Keyforge {

// 单个键位的静态定义（与 app.js ROWS 里的键对象一一对应）
struct KeyDef
{
    QString cap;     // 主字符（按一下打出的字符；特殊键为空）
    QString shift;   // 上档字符（Shift+键 打出的字符；特殊键为空）
    QString label;   // 特殊键帽标签（如 ⌫/Tab/⏎；普通键为空，显示 cap+shift）
    QString title;   // 悬浮提示文本
    int finger;      // 手指编号：0小指 1无名指 2中指 3食指 4拇指
    QString hand;    // 'L' 左手 / 'R' 右手
    QString code;    // 键位 code（KeyboardEvent.code 命名，如 KeyQ/Digit1）
    double w;        // 键宽单位，每行总和 15，各行宽度天然对齐
};

// 五行键位（58 键），与网页版 app.js ROWS 完全一致
extern const QVector<QVector<KeyDef>> kRows;

// 两个 Shift 键的 code（下一键需要 Shift 时两个 Shift 同步高亮）
extern const QString kShiftCodeLeft;
extern const QString kShiftCodeRight;

} // namespace Keyforge
