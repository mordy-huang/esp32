#ifndef CALENDAR_APP_H
#define CALENDAR_APP_H

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "../LcdDriver/EPD_Driver.h" // 引入刚才写的驱动
#include <U8g2_for_Adafruit_GFX.h> // 如果你要用 U8g2

class CalendarApp {
public:
    CalendarApp(EPD_Driver &drv); // 构造时传入驱动
    
    void begin();
    bool run(const char* ssid, const char* password);

private:
    EPD_Driver &screen; // 引用驱动对象
    U8G2_FOR_ADAFRUIT_GFX u8g2; // 字体工具

    // 内部功能函数
    void connectWiFi(const char* ssid, const char* password);
    bool syncTime();
    void drawUI(struct tm *now); // 以前的 drawCalendarLayout
};

#endif