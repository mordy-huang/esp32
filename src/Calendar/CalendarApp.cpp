#include "CalendarApp.h"
#include "../Ap_29demo.h"              // 底图
#include "../FontDriver/HanZiDriver.h" // 底图
#include "CalendarClient.h"            // 引入日历模块
#include <config.h>

CalendarClient calendarClient(SERVER_URL);
// 时区配置
#define GMT_OFFSET_SEC 8 * 3600
#define DAYLIGHT_OFFSET_SEC 0
// 定义画点函数（给 HanziDriver 用的）
static EPD_Driver *_global_screen_ptr = nullptr;
void myAppDrawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (_global_screen_ptr)
    {
        // 这里调用屏幕驱动画一个黑点
        _global_screen_ptr->drawPixel(x, y, color);
    }
}

// 实例化驱动对象
HanziDriver myFont(myAppDrawPixel);
// ==========================================

// 构造函数：接收驱动对象
CalendarApp::CalendarApp(EPD_Driver &drv) : screen(drv)
{
    // 3. 在构造时，把屏幕对象的地址赋给全局指针 <--- 新增
    _global_screen_ptr = &drv;
}

void CalendarApp::begin()
{
    // 这里只初始化业务相关的，比如绑定 U8g2
    // 屏幕硬件初始化已经在 driver.begin() 做过了，或者在这里调也可以
    u8g2.begin(screen); // 把屏幕驱动交给 U8g2
}

void CalendarApp::connectWiFi(const char *ssid, const char *password)
{
    // ... (保留原有的 WiFi 连接代码) ...
    // 比如 Serial.printf("Connecting to %s...", ssid); WiFi.begin...
}

bool CalendarApp::syncTime()
{
    // ... (保留原有的 NTP 代码) ...
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "ntp.aliyun.com", "pool.ntp.org");
    struct tm timeinfo;
    return getLocalTime(&timeinfo, 10000);
}

// 🎨 纯粹的 UI 绘制逻辑
void CalendarApp::drawUI(struct tm *now)
{
    // 1. 贴底图
    if (screen.getBuffer())
    {
        memcpy(screen.getBuffer(), gImage_1, 800 * 480 / 4);
    }

    bool success = calendarClient.update(2026, 2, 2);

    if (success)
    {
        Serial.println("✅ 数据更新成功！");

        // ------------------------------------------------------
        // 3. 获取并使用【详情数据】 (DailyInfo)
        // ------------------------------------------------------
        DailyInfo info = calendarClient.getDailyInfo();

        Serial.println("\n--- 今日黄历详情 ---");
        Serial.println("日期: " + info.dateString + " (" + info.weekDay + ")");
        Serial.println("农历: " + info.lunarString);
        Serial.println("干支: " + info.ganZhi);
        Serial.println("生肖: " + info.shengXiao);
        Serial.println("宜: " + info.yi);
        Serial.println("忌: " + info.ji);
        Serial.println("财神: " + info.caiShen);

        // ------------------------------------------------------
        // 4. 获取并使用【网格数据】 (CalendarCell 数组)
        // ------------------------------------------------------
        CalendarCell *cells = calendarClient.getGridData();

        Serial.println("\n--- 日历网格数据 (前7天示例) ---");
        // 遍历 42 个格子 (这里只打印前 7 个作为演示)
        for (int i = 0; i < 7; i++)
        {
            Serial.printf("[%d] 公历:%d | 农历:%s | 状态:%d | 本月:%d\n",
                          i,
                          cells[i].solarDay,
                          cells[i].lunarText.c_str(),
                          cells[i].status,
                          cells[i].isCurrentMonth);
        }

        // === 在这里调用你的墨水屏绘制函数 ===
        drawMyScreen(cells, info);
    }
    else
    {
        Serial.println("❌ 数据更新失败，请检查 Python 服务是否开启");
    }

    // 2. 用 U8g2 写字 (针对 screen 对象操作)
    u8g2.setForegroundColor(EPD_WHITE); // 假设 3 是黑色
    u8g2.setBackgroundColor(EPD_RED);   // 白色

    // --- 天气 ---
    u8g2.setFont(u8g2_font_logisoso30_tr);
    //     u8g2.setForegroundColor(EPD_WHITE); // 假设 3 是黑色
    // u8g2.setBackgroundColor(EPD_RED); // 白色rsor(90, 250);
    // u8g2.setCursor(105,110);
    myFont.drawText(85, 72, "26", EPD_WHITE);

    // --- 日期 ---
    u8g2.setFont(u8g2_font_logisoso92_tn);
    u8g2.setCursor(90, 250);
    u8g2.print(now->tm_mday);

    u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    //     u8g2.setForegroundColor(EPD_WHITE); // 假设 3 是黑色
    // u8g2.setBackgroundColor(EPD_RED); // 白色rsor(90, 250);
    u8g2.setCursor(90, 334);
    u8g2.print("冬月十一");
    u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    //     u8g2.setForegroundColor(EPD_WHITE); // 假设 3 是黑色
    // u8g2.setBackgroundColor(EPD_RED); // 白色rsor(90, 250);
    u8g2.setCursor(70, 395);
    u8g2.print("祭祀 祈福 纳财");

    // 星期
    myFont.drawText(82, 260, "星期五267", EPD_YELLOW);
    // --- 时间 ---
    // char timeStr[10];
    // sprintf(timeStr, "%02d:%02d", now->tm_hour, now->tm_min);
    // u8g2.setFont(u8g2_font_helvB24_tf);
    // u8g2.setCursor(400, 220);
    // u8g2.print(timeStr);

    // ... 其他绘制代码 ...
}

bool CalendarApp::run(const char *ssid, const char *password)
{
    // 1. 联网对时
    connectWiFi(ssid, password);
    syncTime();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        // 获取失败也没事，给个默认值防止崩溃
        timeinfo.tm_year = 2025 - 1900;
        timeinfo.tm_mday = 1;
    }

    WiFi.disconnect(true); // 断网省电

    // 2. 唤醒屏幕硬件
    screen.begin();

    // 3. 绘制内容到显存
    drawUI(&timeinfo);

    // 4. 刷屏
    screen.display();

    return true;
}