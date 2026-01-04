#include "CalendarApp.h"
#include "../BackgroundImage.h"              // 底图
#include "../FontDriver/HanZiDriver.h" // 底图
#include "CalendarClient.h"            // 引入日历模块
#include <config.h>
#include <./FontDriver/Weather.h>
#include <./FontDriver/Font40px.h>
#include <./FontDriver/Font120px.h>
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

// ================= 布局常量配置 =================
#define SCREEN_W 800
#define SCREEN_H 480

// 顶部留白（放年份月份）
#define HEADER_H 60
// 星期栏高度
#define WEEK_H 40
// 网格起始 Y 坐标
#define GRID_START_Y (HEADER_H + WEEK_H)

// 计算格子大小 (800宽 / 7列 ≈ 114)
#define CELL_W (SCREEN_W / 7)
#define CELL_H ((SCREEN_H - GRID_START_Y) / 6) // 剩余高度分给6行

// 颜色定义 (请根据你的屏幕驱动修改，通常 0x00黑, 0x03白, 0x02红)
#define C_BLACK EPD_BLACK
#define C_WHITE EPD_WHITE
#define C_RED EPD_RED
#define C_YELLOW EPD_YELLOW

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
    // WiFi.begin(ssid, password);

    // while (WiFi.status() != WL_CONNECTED)
    // {
    //     delay(500);
    //     Serial.print(".");
    // }
    // Serial.print("连接成功");
}

bool CalendarApp::syncTime()
{
    // ... (保留原有的 NTP 代码) ...
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "ntp.aliyun.com", "pool.ntp.org");
    struct tm timeinfo;
    return getLocalTime(&timeinfo, 10000);
}
void CalendarApp::drawSingleCell(int index, CalendarCell cell)
{
    int row = index / 7;
    int col = index % 7;

    int x = col * CELL_W;
    int y = GRID_START_Y + row * CELL_H;

    // 1. 确定基础颜色
    uint16_t numColor = C_BLACK;   // 公历数字颜色
    uint16_t lunarColor = C_BLACK; // 农历颜色

    // 逻辑：周末(1) 或 法定假(2) -> 红色
    if (cell.status == 1 || cell.status == 2)
    {
        numColor = C_RED;
        lunarColor = C_RED;
    }

    // 逻辑：非本月显示淡一点 (如果没有灰色，就用黑色，或者你可以选择不显示)
    if (!cell.isCurrentMonth)
    {
        // 如果想区分，可以用黑色，或者画个框表示
        // numColor = C_BLACK;
    }

    // 2. 特殊处理：如果是“今天” (画实心红圆背景)
    bool isTodayHighLight = cell.isToday;
    if (isTodayHighLight)
    {
        // 画红圆：圆心 x + CELL_W/2, y + CELL_H/2
        int cx = x + CELL_W / 2;
        int cy = y + CELL_H / 2;
        screen.fillCircle(cx, cy, (CELL_H / 2) - 4, C_RED);

        numColor = C_WHITE; // 红底白字
        lunarColor = C_WHITE;
    }

    // 3. 绘制公历数字 (使用 U8g2)
    u8g2.setFontMode(1); // 透明背景
    u8g2.setFontDirection(0);

    if (isTodayHighLight)
    {
        u8g2.setForegroundColor(C_WHITE);
        u8g2.setBackgroundColor(C_RED);
    }
    else
    {
        u8g2.setForegroundColor(numColor);
        u8g2.setBackgroundColor(C_WHITE);
    }

    // 数字字体
    u8g2.setFont(u8g2_font_helvB18_tf);

    // 计算居中 (简单偏移，精细居中需要 measureText)
    int numOffsetX = (cell.solarDay < 10) ? (CELL_W / 2 - 8) : (CELL_W / 2 - 16);
    int numY = y + 32; // 数字的基线位置
    u8g2.setCursor(x + numOffsetX, numY);
    u8g2.print(cell.solarDay);

    // 4. 绘制农历/节日 (使用 HanZiDriver)
    // 这里的坐标是汉字左上角
    // 简单估算居中：假设每个汉字宽16，3个字宽48
    // 如果是“今天”，强制用白色，否则用上面算出的颜色
    uint16_t finalLunarColor = isTodayHighLight ? C_WHITE : lunarColor;

    // 计算文字长度大致居中
    int txtLenEstimate = cell.lunarText.length() / 3 * 16;
    int lunarX = x + (CELL_W - txtLenEstimate) / 2;
    int lunarY = y + 42;

    myFont.drawText(lunarX, lunarY, cell.lunarText, finalLunarColor);

    // 5. 绘制状态角标 (休/班) - 右上角
    int badgeX = x + CELL_W - 12;
    int badgeY = y + 12;

    if (cell.status == 2)
    {
        // 法定节假日 "休"
        // 你的图上是在数字上面写了字，这里我们可以画个小红点或者写"休"
        // 为了简单，画一个小实心字
        u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 小字体
        u8g2.setForegroundColor(C_RED);
        u8g2.setCursor(x + CELL_W - 15, y + 15);
        u8g2.print("休");
    }
    else if (cell.status == 3)
    {
        // 调休补班 "班"
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2.setForegroundColor(C_BLACK);
        u8g2.setCursor(x + CELL_W - 15, y + 15);
        u8g2.print("班");
    }
}
// 🎨 纯粹的 UI 绘制逻辑
void CalendarApp::drawUI(struct tm *now)
{
    // 1. 贴底图
    if (screen.getBuffer())
    {
        memcpy(screen.getBuffer(), Huang_Li_Image, 800 * 480 / 4);
    }
    u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2.setForegroundColor(C_BLACK);
    // 当前温度
    u8g2.setCursor(597, 20);
    u8g2.print("26.5℃");
    // 湿度
    u8g2.setCursor(682, 20);
    u8g2.print("45%");
    // 电量
    u8g2.setCursor(755, 20);
    u8g2.print("80%");
    u8g2.setForegroundColor(C_BLACK);
    myFont.setFont(Font_40px);
    myFont.drawText(35, 120, "2026年1月", EPD_WHITE);
    myFont.setFont(Font_120px);
    myFont.drawText(40, 170, "03", EPD_WHITE);
    myFont.setFont(Font_40px);
    myFont.drawText(70, 310, "星期六", EPD_WHITE);

    myFont.drawText(305, 45, "冬月十五", EPD_BLACK);
    u8g2.setCursor(305, 105);
    u8g2.print("乙巳年 戊子月 丙午日");
    myFont.drawText(360, 135, "蛇", EPD_BLACK);
    myFont.drawText(360, 195, "冲鼠", EPD_BLACK);

    myFont.drawText(600, 135, "正西", EPD_BLACK);
    myFont.drawText(600, 195, "丙不修道", EPD_BLACK);


    u8g2.setCursor(350, 273);
    u8g2.print("祭祀 祈福 求嗣 开光 出行 解除 伐木 拆卸");
    u8g2.setCursor(350, 315);
    u8g2.print("嫁娶 移徙 入宅 安葬");


    // 天气
    drawBitmap(400, 350, image_data, IMAGE_WIDTH, IMAGE_HEIGHT, C_BLACK);
    myFont.drawText(310, 410, "20℃", EPD_BLACK);

        u8g2.setCursor(670, 425);
    u8g2.print("空气优");
        u8g2.setCursor(670, 450);
    u8g2.print("西北风 3 级");
    // bool success = calendarClient.update(2026, 2, 2);

    // if (!success)
    // {
    //     u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    //     u8g2.setForegroundColor(C_BLACK);
    //     u8g2.setCursor(10, 100);
    //     u8g2.print("数据获取失败，请检查 WiFi 或 Server");
    //     return;
    // }

    DailyInfo info = calendarClient.getDailyInfo();
    CalendarCell *cells = calendarClient.getGridData();

    // ------------------------------------------------
    // 2. 绘制顶部 Header
    // ------------------------------------------------
    // 左侧：月份 (如 12月)
    u8g2.setForegroundColor(C_BLACK);
    u8g2.setBackgroundColor(C_WHITE);
    u8g2.setFont(u8g2_font_logisoso42_tn); // 大号数字
    u8g2.setCursor(20, 50);
    // 这里简单解析 info.dateString 或者用 now 里的月份
    // 假设 info.dateString 格式 "2025-12-31"
    String monthStr = info.dateString.substring(5, 7);
    u8g2.print(monthStr);

    u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2.setCursor(90, 45);
    // u8g2.print("月");

    // 右侧：年份
    u8g2.setFont(u8g2_font_logisoso24_tn);
    u8g2.setCursor(SCREEN_W - 100, 45);
    String yearStr = info.dateString.substring(0, 4);
    u8g2.print(yearStr);
    // Display_All_Black();

    /*
    // 绘制一条分割线
    screen.drawFastHLine(0, HEADER_H, SCREEN_W, C_BLACK);

    // ------------------------------------------------
    // 3. 绘制星期栏
    // ------------------------------------------------
    const char *weekNames[] = {"一", "二", "三", "四", "五", "六", "日"};
    u8g2.setFont(u8g2_font_wqy16_t_gb2312);

    for (int i = 0; i < 7; i++)
    {
        int wx = i * CELL_W + (CELL_W / 2 - 8);
        int wy = HEADER_H + 28;

        // 周末显示红色
        if (i >= 5)
            u8g2.setForegroundColor(C_RED);
        else
            u8g2.setForegroundColor(C_BLACK);

        u8g2.setCursor(wx, wy);
        u8g2.print(weekNames[i]);
    }

    // ------------------------------------------------
    // 4. 循环绘制 42 个格子
    // ------------------------------------------------
    for (int i = 0; i < 42; i++)
    {
        drawSingleCell(i, cells[i]);
    }

    // ------------------------------------------------
    // 5. (可选) 底部绘制一行宜忌详情
    // ------------------------------------------------
    // 如果格子没占满屏幕，可以在最下面画
    // int footerY = 460;
    // u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    // u8g2.setForegroundColor(C_BLACK);
    // u8g2.setCursor(10, footerY);
    // u8g2.print("宜: " + info.yi);

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

    // ... 其他绘制代码 ...*/
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

    // 2. 唤醒屏幕硬件
    screen.begin();

    // 3. 绘制内容到显存
    drawUI(&timeinfo);
    WiFi.disconnect(true); // 断网省电

    // 4. 刷屏
    screen.display();

    return true;
}

// 通用的单色位图绘制函数
void CalendarApp::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color)
{
    for (int16_t j = 0; j < h; j++)
    {
        for (int16_t i = 0; i < w; i++)
        {
            int16_t finalColor = bitmap[j * w + i];
            if (finalColor != 0xff)
            {
                screen.drawPixel(x + i, y + j, finalColor);
            }
        }
    }
}