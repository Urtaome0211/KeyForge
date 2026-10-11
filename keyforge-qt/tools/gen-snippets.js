#!/usr/bin/env node
// ============================================================
// gen-snippets.js —— 练习片段数据生成器
//
// 职责：加载网页版 snippets.js（定义 window.SNIPPETS），校验后
//       生成 C++ 片段数据 src/snippetdata.h / .cpp。
//
// 校验规则（与网页版注释约定一致，逐条断言）：
//   1. 共 32 段；id 全局唯一且非空
//   2. lang 属于 js/py/java/c/go/html/css/sql/other
//   3. code 非空；只含可打印 ASCII（0x20-0x7E）+ 换行 \n
//      （无 \r、无 Tab——缩进必须用空格，Tab 会被当练习字符）
//   4. 无反引号与 ${（模板字符串解析陷阱）
//   5. 不含 "KFG" 子串（code 用 C++ raw string 定界符 R"KFG(...)KFG"，
//      内容里出现定界符会提前终止字符串）
//
// 实现要点：global.window = {} 后直接 require 网页版 snippets.js，
// 它只做 window.SNIPPETS = [...] 赋值，无其他副作用。
// 代码正文用自定义定界符 raw string，内容逐字节保留（反斜杠、
// 引号、换行都不转义），与网页版运行时字符串完全一致。
//
// 用法：node tools/gen-snippets.js（输出到 keyforge-qt/src/）
// ============================================================
'use strict';

const fs = require('fs');
const path = require('path');

// ---- 路径：相对本脚本定位网页版与输出目录 ----
const snippetsJsPath = path.resolve(__dirname, '..', '..', 'snippets.js');
const outDir = path.resolve(__dirname, '..', 'src');

// ---- 加载网页版片段数据（window.SNIPPETS） ----
global.window = {};
require(snippetsJsPath);
const list = window.SNIPPETS;

// ---- 校验 ----
const LANGS = new Set(['js', 'py', 'java', 'c', 'go', 'html', 'css', 'sql', 'other']);
function fail(msg) { throw new Error('snippets.js 校验失败：' + msg); }
if (!Array.isArray(list)) fail('window.SNIPPETS 不是数组');
if (list.length !== 32) fail('片段数量 ' + list.length + ' ≠ 32');

const ids = new Set();
for (const s of list) {
  if (!s || typeof s.id !== 'string' || !s.id) fail('存在缺失或空的 id');
  if (ids.has(s.id)) fail('id 重复：' + s.id);
  ids.add(s.id);
  if (!LANGS.has(s.lang)) fail('未知分类 lang：' + s.lang + '（id ' + s.id + '）');
  if (typeof s.title !== 'string' || !s.title) fail('id ' + s.id + ' 的 title 为空');
  if (typeof s.code !== 'string' || !s.code) fail('id ' + s.id + ' 的 code 为空');
  for (const ch of s.code) {
    const c = ch.codePointAt(0);
    if (c !== 0x0a && (c < 0x20 || c > 0x7e)) {
      fail('id ' + s.id + ' 的 code 含非打印字符 0x' + c.toString(16));
    }
  }
  if (s.code.includes('`')) fail('id ' + s.id + ' 的 code 含反引号');
  if (s.code.includes('${')) fail('id ' + s.id + ' 的 code 含 ${');
  if (s.code.includes('KFG')) fail('id ' + s.id + ' 的 code 含 raw string 定界符 KFG');
  if (/[\x00-\x1f\x7f]/.test(s.title)) fail('id ' + s.id + ' 的 title 含控制字符');
}

// ---- C++ 字符串转义（id/lang/title 用普通字面量，需转义） ----
function cppStr(s) {
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

// ---- 生成 C++ ----
// code 用 QString::fromUtf8(R"KFG(...)KFG")：内容已校验为纯 ASCII，
// 无长度限制（QStringLiteral 有 4096 字符上限），raw string 逐字节保留。
const entries = list.map((s) => {
  return '    { ' + [cppStr(s.id), cppStr(s.lang), cppStr(s.title)].join(', ') + ',\n' +
    '      QString::fromUtf8(R"KFG(' + s.code + ')KFG") },';
}).join('\n');

const header = `// ============================================================
// 本文件由 tools/gen-snippets.js 自动生成 —— 请勿手工修改！
// 数据来源：网页版 snippets.js 的 window.SNIPPETS（32 段练习代码）
// 重新生成：node tools/gen-snippets.js
// ============================================================
#pragma once

#include <QString>
#include <QVector>

namespace Keyforge {

// 单段练习的静态定义（与 snippets.js 的片段对象一一对应）
struct SnippetDef
{
    QString id;     // 全局唯一标识（同时用作历史最佳成绩的存储键）
    QString lang;   // 语言分类（js/py/java/c/go/html/css/sql/other）
    QString title;  // 下拉框显示的标题
    QString code;   // 练习代码原文（可打印 ASCII + 换行）
};

// 全部 32 段内置练习，顺序与网页版一致
extern const QVector<SnippetDef> kSnippets;

} // namespace Keyforge
`;

const source = `// ============================================================
// 本文件由 tools/gen-snippets.js 自动生成 —— 请勿手工修改！
// 数据来源：网页版 snippets.js 的 window.SNIPPETS（32 段练习代码）
// 重新生成：node tools/gen-snippets.js
// ============================================================
#include "snippetdata.h"

namespace Keyforge {

const QVector<SnippetDef> kSnippets = {
${entries}
};

} // namespace Keyforge
`;

fs.writeFileSync(path.join(outDir, 'snippetdata.h'), header, 'utf8');
fs.writeFileSync(path.join(outDir, 'snippetdata.cpp'), source, 'utf8');
console.log('已生成 src/snippetdata.h / src/snippetdata.cpp：' + list.length + ' 段，校验全部通过');
