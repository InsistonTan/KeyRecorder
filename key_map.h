#ifndef KEY_MAP_H
#define KEY_MAP_H

#include<QMap>

// 键盘按键对应硬件扫描码map
const static QMap<QString, short> VSC_MAP = {
    // 下面是键盘的硬件扫描码
    {"Esc",0x01},
    {"1",0x02},
    {"2",0x03},
    {"3",0x04},
    {"4",0x05},
    {"5",0x06},
    {"6",0x07},
    {"7",0x08},
    {"8",0x09},
    {"9",0x0A},
    {"0",0x0B},
    {"-",0x0C},
    {"=",0x0D},
    {"BackSpace",0x0E},
    {"Tab",0x0F},
    {"Q",0x10},
    {"W",0x11},
    {"E",0x12},
    {"R",0x13},
    {"T",0x14},
    {"Y",0x15},
    {"U",0x16},
    {"I",0x17},
    {"O",0x18},
    {"P",0x19},
    {"[",0x1A},
    {"]",0x1B},
    {"Enter",0x1C},
    {"Ctrl(Left)",0x1D},
    {"A",0x1E},
    {"S",0x1F},
    {"D",0x20},
    {"F",0x21},
    {"G",0x22},
    {"H",0x23},
    {"J",0x24},
    {"K",0x25},
    {"L",0x26},
    {";",0x27},
    {"'",0x28},
    {"`",0x29},
    {"Shift(Left)",0x2A},
    {"\\",0x2B},
    {"Z",0x2C},
    {"X",0x2D},
    {"C",0x2E},
    {"V",0x2F},
    {"B",0x30},
    {"N",0x31},
    {"M",0x32},
    {",",0x33},
    {".",0x34},
    {"/",0x35},
    {"Shift(Right)",0x36},
    {"*(Numpad)",0x37},
    {"Alt(Left)",0x38},
    {"Space",0x39},
    {"Caps Lock",0x3A},
    {"F1",0x3B},
    {"F2",0x3C},
    {"F3",0x3D},
    {"F4",0x3E},
    {"F5",0x3F},
    {"F6",0x40},
    {"F7",0x41},
    {"F8",0x42},
    {"F9",0x43},
    {"F10",0x44},
    {"Num Lock",0x45},
    {"Scroll Lock",0x46},
    {"7(Numpad)",0x47},
    {"8(Numpad)",0x48},
    {"9(Numpad)",0x49},
    {"-(Numpad)",0x4A},
    {"4(Numpad)",0x4B},
    {"5(Numpad)",0x4C},
    {"6(Numpad)",0x4D},
    {"+(Numpad)",0x4E},
    {"1(Numpad)",0x4F},
    {"2(Numpad)",0x50},
    {"3(Numpad)",0x51},
    {"0(Numpad)",0x52},
    {".(Numpad)",0x53},
    {"F11",0x57},
    {"F12",0x58},
    {"F13",0x64},
    {"F14",0x65},
    {"F15",0x66},
    {"$",0x7D},
    {"=",0x8D},
    {"^",0x90},
    {"@",0x91},
    {":",0x92},
    {"_",0x93},
    {"Enter(Numpad)",0x9C},
    {"Ctrl(Right)",0x9D},
    {",(Numpad)",0xB3},
    {"/(Numpad)",0xB5},
    {"Sys Rq",0xB7},
    {"Alt(Right)",0xB8},
    {"Pause",0xC5},
    {"Home",0xC7},
    {"↑",0xC8},
    {"Page Up",0xC9},
    {"←",0xCB},
    {"→",0xCD},
    {"End",0xCF},
    {"↓",0xD0},
    {"Page Down",0xD1},
    {"Insert",0xD2},
    {"Delete",0xD3},
    {"Windows",0xDB},
    {"Windows",0xDC},
    {"Menu",0xDD},
    {"Power",0xDE},
    {"Windows",0xDF}
};


// 鼠标按键虚拟键码map
// 常用的鼠标按键虚拟键码
// #define VK_LBUTTON    0x01  // 左键
// #define VK_RBUTTON    0x02  // 右键
// #define VK_MBUTTON    0x04  // 中键（滚轮按下）
// #define VK_XBUTTON1   0x05  // 侧键1（后退）
// #define VK_XBUTTON2   0x06  // 侧键2（前进）
const static QMap<QString, short> MOUSE_VK_MAP = {
    {"mouseLeft", 0x01},// 左键
    {"mouseRight", 0x02},// 右键
    {"mouseMiddle", 0x04},// 中键（滚轮按下）
    {"mouseSide1", 0x05}, // 侧键1（后退）
    {"mouseSide2", 0x06}, // 侧键2（前进）
};

#endif // KEY_MAP_H
