#!/usr/bin/env node
// ============================================================
// gen-keyboard.js —— 键位数据生成器
//
// 职责：从网页版 app.js 的文本中提取 "const ROWS = [" 开头的键位表
//       数组块，生成 C++ 键位数据 src/keyboarddata.h / .cpp。
//
// 实现要点：
//   1. 不用完整 JS 解析器：自写迷你 tokenizer，遇到单/双引号跳过整个
//      字符串（正确处理 \' 与 \\ 转义），其余位置做 [ ] 括号配对，
//      深度归零即数组结束——字符串里的括号不会干扰配对。
//   2. 在本地作用域定义 K()（参数与网页版逐字一致）后 eval 该数组块，
//      得到 58 键 5 行的真实数据，与网页版运行时完全一致。
//   3. 输出前校验：5 行、58 键、键码唯一、每行键宽之和 = 15。
//
// 用法：node tools/gen-keyboard.js（任何工作目录均可，输出到 keyforge-qt/src/）
// 重新生成后内容与网页数据逐字符一致（幂等）。
// ============================================================
'use strict';

const fs = require('fs');
const path = require('path');

// ---- 路径：相对本脚本定位网页版与输出目录 ----
const appJsPath = path.resolve(__dirname, '..', '..', 'app.js');
const outDir = path.resolve(__dirname, '..', 'src');

// ---- 1. 读取 app.js 全文 ----
const text = fs.readFileSync(appJsPath, 'utf8');

// ---- 2. 定位 "const ROWS = [" ----
const mark = 'const ROWS = [';
const start = text.indexOf(mark);
if (start < 0) throw new Error('app.js 中未找到 "const ROWS = ["');

// ---- 3. 迷你 tokenizer：跳过字符串后再做方括号配对 ----
// open 指向 ROWS 的 '['。扫描时：
//   - 遇到引号：跳过整个字符串（\\ 转义跳 2 个字符）
//   - 遇到 '['：深度 +1；遇到 ']'：深度 -1，归零即数组结束
const open = start + 'const ROWS = '.length;
let depth = 0;
let i = open;
while (i < text.length) {
  const ch = text[i];
  if (ch === "'" || ch === '"') {
    const quote = ch;
    i++;
    while (i < text.length) {
      if (text[i] === '\\') { i += 2; continue; } // 跳过转义字符
      if (text[i] === quote) { i++; break; }
      i++;
    }
    continue;
  }
  if (ch === '[') depth++;
  else if (ch === ']') {
    depth--;
    if (depth === 0) break;
  }
  i++;
}
if (depth !== 0) throw new Error('ROWS 方括号不配对，提取失败');
const block = text.slice(open, i + 1);

// ---- 4. 定义 K() 后 eval 得到五行键位数据 ----
// K 的参数与网页版逐字一致：
//   cap    主字符（按一下打出的字符）
//   shift  上档字符（Shift+键 打出的字符）
//   finger 手指编号（0-3 = 小指~食指，4 = 拇指）
//   hand   'L' 左手 / 'R' 右手
//   code   KeyboardEvent.code，用于按下时定位键帽
//   w      键宽单位（默认 1）
// 注意：eval 的对象只是从本地受控文件（app.js）提取的字面量数组，
//       不包含任何外部输入，无注入风险。
function K(cap, shift, finger, hand, code, w) {
  return { cap: cap, shift: shift, finger: finger, hand: hand, code: code, w: w || 1 };
}
const ROWS = eval(block);

// ---- 5. 提取 SHIFT_CODES（下一键需要 Shift 时两个 Shift 同步高亮）----
const shiftMatch = /const SHIFT_CODES = (\[[^\]]*\])/.exec(text);
if (!shiftMatch) throw new Error('app.js 中未找到 SHIFT_CODES');
const SHIFT_CODES = eval(shiftMatch[1]);

// ---- 6. 校验数据 ----
if (!Array.isArray(ROWS) || ROWS.length !== 5) throw new Error('ROWS 不是 5 行：' + ROWS.length);
let total = 0;
const codes = new Set();
ROWS.forEach((row, r) => {
  const sum = row.reduce((a, k) => a + k.w, 0);
  if (Math.abs(sum - 15) > 1e-9) throw new Error('第 ' + r + ' 行键宽之和 ' + sum + ' ≠ 15');
  row.forEach((k) => {
    total++;
    if (codes.has(k.code)) throw new Error('键码重复：' + k.code);
    codes.add(k.code);
  });
});
if (total !== 58) throw new Error('总键数 ' + total + ' ≠ 58');
if (SHIFT_CODES.length !== 2 || SHIFT_CODES[0] !== 'ShiftLeft' || SHIFT_CODES[1] !== 'ShiftRight') {
  throw new Error('SHIFT_CODES 异常：' + JSON.stringify(SHIFT_CODES));
}

// ---- 7. C++ 字符串转义 ----
function cppStr(s) {
  if (s === undefined || s === null) return 'QString()';
  let out = '"';
  for (const ch of s) {
    const c = ch.codePointAt(0);
    if (ch === '\\') out += '\\\\';
    else if (ch === '"') out += '\\"';
    else if (ch === '\n') out += '\\n';
    else if (ch === '\t') out += '\\t';
    else if (ch === '\r') out += '\\r';
    else if (c < 0x20 || c === 0x7f) out += '\\x' + c.toString(16).padStart(2, '0');
    else out += ch;
  }
  return 'QStringLiteral(' + out + '")';
}
const numStr = (n) => (typeof n === 'number' ? String(n) : '1');

// ---- 8. 生成 C++ ----
// KeyDef{cap, shift, label, title, finger, hand, code, w}
// 普通键：cap/shift 来自 K()，label 为空（键帽显示 cap+shift 双层字符）；
// 特殊键：label/title 展示在键帽上，cap/shift 为空——与网页版对象形状一致。
function keyInit(k) {
  return '    KeyDef{ ' + [
    cppStr(k.cap),
    cppStr(k.shift),
    cppStr(k.label),
    cppStr(k.title),
    String(k.finger),
    cppStr(k.hand),
    cppStr(k.code),
    numStr(k.w),
  ].join(', ') + ' },';
}

// 每行用 {} 包裹：QVector<QVector<KeyDef>> 需要嵌套初始化列表
const rowsSrc = ROWS.map((row, r) =>
  '    // 第 ' + r + ' 行\n    {\n' + row.map((k) => keyInit(k)).join('\n') + '\n    },'
).join('\n');

const header = `// ============================================================
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
`;

const source = `// ============================================================
// 本文件由 tools/gen-keyboard.js 自动生成 —— 请勿手工修改！
// 数据来源：网页版 app.js 的 ROWS 键位表（58 键 5 行，逐字符一致）
// 重新生成：node tools/gen-keyboard.js
// ============================================================
#include "keyboarddata.h"

namespace Keyforge {

const QString kShiftCodeLeft = QStringLiteral("ShiftLeft");
const QString kShiftCodeRight = QStringLiteral("ShiftRight");

const QVector<QVector<KeyDef>> kRows = {
${rowsSrc}
};

} // namespace Keyforge
`;

fs.writeFileSync(path.join(outDir, 'keyboarddata.h'), header, 'utf8');
fs.writeFileSync(path.join(outDir, 'keyboarddata.cpp'), source, 'utf8');
console.log('已生成 src/keyboarddata.h / src/keyboarddata.cpp：' + total + ' 键 5 行，每行键宽之和 15');
