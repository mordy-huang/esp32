#ifndef USER_FONT_H
#define USER_FONT_H

#include <Arduino.h>

// 定义获取位图的函数指针类型
typedef const unsigned char* (*GetBitmapFunc)(uint16_t unicode);

// 📦 定义字体结构体
struct MyFontDef {
    GetBitmapFunc getBitmap; // 获取该字号位图的函数
    uint8_t w;               // 宽 (例如 16)
    uint8_t h;               // 高 (例如 16)
    
    // 🎨 针对该字号的间距配置 (大字号通常需要更大的间距)
    int8_t gap_ascii; 
    int8_t gap_hanzi;
    int8_t gap_ascii_hanzi;
    int8_t gap_hanzi_ascii;

};

// --- 声明具体的字体对象 (在 .cpp 或 .h 下方定义) ---

// 假设这是你工具生成的两个查找函数

// 定义字体实例


// ------------------- 索引表结构 -------------------
 struct FontItem_t{
  uint16_t unicode;        // Unicode 编码 (例如 '你' = 0x4F60)
  const unsigned char* bitmap; // 对应的字模数组指针
} ;

#endif