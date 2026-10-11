// ============================================================
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
