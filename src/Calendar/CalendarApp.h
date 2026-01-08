#ifndef CALENDAR_APP_H
#define CALENDAR_APP_H

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "../LcdDriver/EPD_Driver.h" // 引入刚才写的驱动
#include <U8g2_for_Adafruit_GFX.h>   // 如果你要用 U8g2
#include "CalendarClient.h"

class CalendarApp
{
public:
    CalendarApp(EPD_Driver &drv); // 构造时传入驱动

    void begin();
    bool run(const char *ssid, const char *password);

private:
    EPD_Driver &screen;         // 引用驱动对象
    // --- 新增：用于缓存 API 数据的变量 ---
    CalendarCell* _cachedGrid; // 指向 Grid 数组的指针
    DailyInfo     _cachedInfo; // 黄历详情
    WeatherInfo   _cachedWeather; // 天气详情
    bool          _isDataLoaded = false; // 标记数据是否加载成功
    // ----------------------------------
    U8G2_FOR_ADAFRUIT_GFX u8g2; // 字体工具
    void drawSingleCell(int index, CalendarCell cell);
    

    // 内部功能函数
    void connectWiFi(const char *ssid, const char *password);
    bool syncTime();
    void drawUI_HuangLi(struct tm *now);      // 以前的 drawCalendarLayout
    void drawUI_MoonCalendar(struct tm *now); // 以前的 drawCalendarLayout
    void drawUI_Clock(struct tm *now);        // 以前的 drawCalendarLayout

    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
    void myDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t width, uint16_t color, boolean isX = true);
    void fillGrayRect(int16_t x, int16_t y, int16_t w, int16_t h);
    // 新增这个声明
    void drawNeedle(int16_t cx, int16_t cy, float angle, int16_t len, int16_t width, uint16_t color);
    void drawTempBatteryUI();
    void setupWebServer();
    void handleClient(); // 必须在 loop 中调用
    void drawGalleryPage();   
    const uint8_t* getWeatherIcon(String weatherStr,boolean isNight);
 };

#endif