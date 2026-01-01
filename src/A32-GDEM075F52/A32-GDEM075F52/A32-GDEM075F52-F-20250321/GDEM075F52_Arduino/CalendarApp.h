#ifndef CALENDAR_APP_H
#define CALENDAR_APP_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <time.h>
#include <SPI.h>

// 引入字体
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

// 引入底层驱动
#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"

// 颜色定义 (适配你的屏幕硬件映射: 0x03是白, 0x01是黄)
#define C_BLACK   0x00
#define C_WHITE   0x03
#define C_YELLOW  0x01
#define C_RED     0x02

class CalendarApp : public Adafruit_GFX {
public:
    // 构造函数
    CalendarApp();
    
    // 初始化硬件 (SPI, 引脚)
    void begin();

    // 运行一次完整的日历流程 (连网->对时->绘图->刷屏)
    // 返回 true 表示成功，false 表示中间出错
    bool run(const char* ssid, const char* password);

    // 核心画点函数 (必须实现 Adafruit_GFX 的虚函数)
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;

    // 绘制整张图片（2bpp 打包，每字节包含 4 个像素，MSB 为左侧像素，每行字节数 = (w+3)/4）
    // 如果宽高等于屏幕尺寸（800x480），会直接 memcpy 到内部显存以提高速度。
    void drawFullImage(const uint8_t *data, int16_t w, int16_t h);
    
    void test();

private:
    uint8_t *buffer; // 显存指针

    // 内部功能函数
    void connectWiFi(const char* ssid, const char* password);
    bool syncTime();
    void drawCalendarLayout(struct tm *now);
    void flushScreen();
};

#endif