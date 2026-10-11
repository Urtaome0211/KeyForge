// ============================================================
// 本文件由 tools/gen-keyboard.js 自动生成 —— 请勿手工修改！
// 数据来源：网页版 app.js 的 ROWS 键位表（58 键 5 行，逐字符一致）
// 重新生成：node tools/gen-keyboard.js
// ============================================================
#include "keyboarddata.h"

namespace Keyforge {

const QString kShiftCodeLeft = QStringLiteral("ShiftLeft");
const QString kShiftCodeRight = QStringLiteral("ShiftRight");

const QVector<QVector<KeyDef>> kRows = {
    // 第 0 行
    {
    KeyDef{ QStringLiteral("`"), QStringLiteral("~"), QString(), QString(), 0, QStringLiteral("L"), QStringLiteral("Backquote"), 1 },
    KeyDef{ QStringLiteral("1"), QStringLiteral("!"), QString(), QString(), 0, QStringLiteral("L"), QStringLiteral("Digit1"), 1 },
    KeyDef{ QStringLiteral("2"), QStringLiteral("@"), QString(), QString(), 1, QStringLiteral("L"), QStringLiteral("Digit2"), 1 },
    KeyDef{ QStringLiteral("3"), QStringLiteral("#"), QString(), QString(), 2, QStringLiteral("L"), QStringLiteral("Digit3"), 1 },
    KeyDef{ QStringLiteral("4"), QStringLiteral("$"), QString(), QString(), 3, QStringLiteral("L"), QStringLiteral("Digit4"), 1 },
    KeyDef{ QStringLiteral("5"), QStringLiteral("%"), QString(), QString(), 3, QStringLiteral("L"), QStringLiteral("Digit5"), 1 },
    KeyDef{ QStringLiteral("6"), QStringLiteral("^"), QString(), QString(), 3, QStringLiteral("R"), QStringLiteral("Digit6"), 1 },
    KeyDef{ QStringLiteral("7"), QStringLiteral("&"), QString(), QString(), 3, QStringLiteral("R"), QStringLiteral("Digit7"), 1 },
    KeyDef{ QStringLiteral("8"), QStringLiteral("*"), QString(), QString(), 2, QStringLiteral("R"), QStringLiteral("Digit8"), 1 },
    KeyDef{ QStringLiteral("9"), QStringLiteral("("), QString(), QString(), 1, QStringLiteral("R"), QStringLiteral("Digit9"), 1 },
    KeyDef{ QStringLiteral("0"), QStringLiteral(")"), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("Digit0"), 1 },
    KeyDef{ QStringLiteral("-"), QStringLiteral("_"), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("Minus"), 1 },
    KeyDef{ QStringLiteral("="), QStringLiteral("+"), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("Equal"), 1 },
    KeyDef{ QString(), QString(), QStringLiteral("⌫"), QStringLiteral("Backspace"), 0, QStringLiteral("R"), QStringLiteral("Backspace"), 2 },
    },
    // 第 1 行
    {
    KeyDef{ QString(), QString(), QStringLiteral("Tab"), QStringLiteral("Tab"), 0, QStringLiteral("L"), QStringLiteral("Tab"), 1.5 },
    KeyDef{ QStringLiteral("q"), QStringLiteral("Q"), QString(), QString(), 0, QStringLiteral("L"), QStringLiteral("KeyQ"), 1 },
    KeyDef{ QStringLiteral("w"), QStringLiteral("W"), QString(), QString(), 1, QStringLiteral("L"), QStringLiteral("KeyW"), 1 },
    KeyDef{ QStringLiteral("e"), QStringLiteral("E"), QString(), QString(), 2, QStringLiteral("L"), QStringLiteral("KeyE"), 1 },
    KeyDef{ QStringLiteral("r"), QStringLiteral("R"), QString(), QString(), 3, QStringLiteral("L"), QStringLiteral("KeyR"), 1 },
    KeyDef{ QStringLiteral("t"), QStringLiteral("T"), QString(), QString(), 3, QStringLiteral("L"), QStringLiteral("KeyT"), 1 },
    KeyDef{ QStringLiteral("y"), QStringLiteral("Y"), QString(), QString(), 3, QStringLiteral("R"), QStringLiteral("KeyY"), 1 },
    KeyDef{ QStringLiteral("u"), QStringLiteral("U"), QString(), QString(), 3, QStringLiteral("R"), QStringLiteral("KeyU"), 1 },
    KeyDef{ QStringLiteral("i"), QStringLiteral("I"), QString(), QString(), 2, QStringLiteral("R"), QStringLiteral("KeyI"), 1 },
    KeyDef{ QStringLiteral("o"), QStringLiteral("O"), QString(), QString(), 1, QStringLiteral("R"), QStringLiteral("KeyO"), 1 },
    KeyDef{ QStringLiteral("p"), QStringLiteral("P"), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("KeyP"), 1 },
    KeyDef{ QStringLiteral("["), QStringLiteral("{"), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("BracketLeft"), 1 },
    KeyDef{ QStringLiteral("]"), QStringLiteral("}"), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("BracketRight"), 1 },
    KeyDef{ QStringLiteral("\\"), QStringLiteral("|"), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("Backslash"), 1.5 },
    },
    // 第 2 行
    {
    KeyDef{ QString(), QString(), QStringLiteral("Caps"), QStringLiteral("Caps Lock"), 0, QStringLiteral("L"), QStringLiteral("CapsLock"), 1.8 },
    KeyDef{ QStringLiteral("a"), QStringLiteral("A"), QString(), QString(), 0, QStringLiteral("L"), QStringLiteral("KeyA"), 1 },
    KeyDef{ QStringLiteral("s"), QStringLiteral("S"), QString(), QString(), 1, QStringLiteral("L"), QStringLiteral("KeyS"), 1 },
    KeyDef{ QStringLiteral("d"), QStringLiteral("D"), QString(), QString(), 2, QStringLiteral("L"), QStringLiteral("KeyD"), 1 },
    KeyDef{ QStringLiteral("f"), QStringLiteral("F"), QString(), QString(), 3, QStringLiteral("L"), QStringLiteral("KeyF"), 1 },
    KeyDef{ QStringLiteral("g"), QStringLiteral("G"), QString(), QString(), 3, QStringLiteral("L"), QStringLiteral("KeyG"), 1 },
    KeyDef{ QStringLiteral("h"), QStringLiteral("H"), QString(), QString(), 3, QStringLiteral("R"), QStringLiteral("KeyH"), 1 },
    KeyDef{ QStringLiteral("j"), QStringLiteral("J"), QString(), QString(), 3, QStringLiteral("R"), QStringLiteral("KeyJ"), 1 },
    KeyDef{ QStringLiteral("k"), QStringLiteral("K"), QString(), QString(), 2, QStringLiteral("R"), QStringLiteral("KeyK"), 1 },
    KeyDef{ QStringLiteral("l"), QStringLiteral("L"), QString(), QString(), 1, QStringLiteral("R"), QStringLiteral("KeyL"), 1 },
    KeyDef{ QStringLiteral(";"), QStringLiteral(":"), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("Semicolon"), 1 },
    KeyDef{ QStringLiteral("'"), QStringLiteral("\""), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("Quote"), 1 },
    KeyDef{ QString(), QString(), QStringLiteral("⏎"), QStringLiteral("Enter 回车"), 0, QStringLiteral("R"), QStringLiteral("Enter"), 2.2 },
    },
    // 第 3 行
    {
    KeyDef{ QString(), QString(), QStringLiteral("Shift"), QStringLiteral("Shift"), 0, QStringLiteral("L"), QStringLiteral("ShiftLeft"), 2.3 },
    KeyDef{ QStringLiteral("z"), QStringLiteral("Z"), QString(), QString(), 0, QStringLiteral("L"), QStringLiteral("KeyZ"), 1 },
    KeyDef{ QStringLiteral("x"), QStringLiteral("X"), QString(), QString(), 1, QStringLiteral("L"), QStringLiteral("KeyX"), 1 },
    KeyDef{ QStringLiteral("c"), QStringLiteral("C"), QString(), QString(), 2, QStringLiteral("L"), QStringLiteral("KeyC"), 1 },
    KeyDef{ QStringLiteral("v"), QStringLiteral("V"), QString(), QString(), 3, QStringLiteral("L"), QStringLiteral("KeyV"), 1 },
    KeyDef{ QStringLiteral("b"), QStringLiteral("B"), QString(), QString(), 3, QStringLiteral("L"), QStringLiteral("KeyB"), 1 },
    KeyDef{ QStringLiteral("n"), QStringLiteral("N"), QString(), QString(), 3, QStringLiteral("R"), QStringLiteral("KeyN"), 1 },
    KeyDef{ QStringLiteral("m"), QStringLiteral("M"), QString(), QString(), 3, QStringLiteral("R"), QStringLiteral("KeyM"), 1 },
    KeyDef{ QStringLiteral(","), QStringLiteral("<"), QString(), QString(), 2, QStringLiteral("R"), QStringLiteral("Comma"), 1 },
    KeyDef{ QStringLiteral("."), QStringLiteral(">"), QString(), QString(), 1, QStringLiteral("R"), QStringLiteral("Period"), 1 },
    KeyDef{ QStringLiteral("/"), QStringLiteral("?"), QString(), QString(), 0, QStringLiteral("R"), QStringLiteral("Slash"), 1 },
    KeyDef{ QString(), QString(), QStringLiteral("Shift"), QStringLiteral("Shift"), 0, QStringLiteral("R"), QStringLiteral("ShiftRight"), 2.7 },
    },
    // 第 4 行
    {
    KeyDef{ QString(), QString(), QStringLiteral("Ctrl"), QStringLiteral("Control"), 0, QStringLiteral("L"), QStringLiteral("ControlLeft"), 1.6 },
    KeyDef{ QString(), QString(), QStringLiteral("Alt"), QStringLiteral("Alt"), 4, QStringLiteral("L"), QStringLiteral("AltLeft"), 1.6 },
    KeyDef{ QString(), QString(), QStringLiteral("Space"), QStringLiteral("空格"), 4, QStringLiteral("L"), QStringLiteral("Space"), 8.6 },
    KeyDef{ QString(), QString(), QStringLiteral("Alt"), QStringLiteral("Alt"), 4, QStringLiteral("R"), QStringLiteral("AltRight"), 1.6 },
    KeyDef{ QString(), QString(), QStringLiteral("Ctrl"), QStringLiteral("Control"), 0, QStringLiteral("R"), QStringLiteral("ControlRight"), 1.6 },
    },
};

} // namespace Keyforge
