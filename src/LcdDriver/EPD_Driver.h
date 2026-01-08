#ifndef EPD_DRIVER_H
#define EPD_DRIVER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <SPI.h>

// 引入厂商的底层头文件 (确保这些文件在你的 src 里)
#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"

// 继承 Adafru#define EPD_WHITE   0  // 00 -> 白色
#define EPD_WHITE        0  // 00 -> 白色
#define EPD_YELLOW       1  // 01 -> 黄色
#define EPD_RED          2  // 10 -> 红色
#define EPD_BLACK        3  // 11 -> 黑色
#define EPD_GRAY         4  // 11 -> 灰色
#define EPD_ORANGE       5  // 11 -> 橙色
#define EPD_PICK         6  // 11 -> 粉色
#define EPD_DARK_YELLOW  7  // 11 -> 暗黄
#define EPD_DARK_RED     8  // 11 -> 暗红
class EPD_Driver : public Adafruit_GFX {
public:
    // 构造函数：指定宽高
    EPD_Driver(int16_t w, int16_t h);
    ~EPD_Driver();

    // 初始化硬件 (SPI, 引脚)
    void begin();

    // 核心：将显存的数据发送给屏幕 (封装厂商的 PIC_display)
    void display();

    // 必选：实现 Adafruit_GFX 的画点函数
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;

    // 清空显存 (变白)
    void clearBuffer();
    
    // 获取显存指针 (给 memcpy 贴图用)
    uint8_t* getBuffer();

private:
    uint8_t *buffer; // 显存数组
};

#endif