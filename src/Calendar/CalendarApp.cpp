#include "CalendarApp.h"
#include "../BackgroundImage.h"        // 底图
#include "../FontDriver/HanZiDriver.h" // 底图
#include "CalendarClient.h"            // 引入日历模块
#include <config.h>
#include <./FontDriver/WeatherIcons.h>
#include <./FontDriver/Font40px.h>
#include <./FontDriver/Font120px.h>
#include <math.h> // 确保引用了 math 库
#include "FontEndCode.h"
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h> // 或者 SPIFFS，建议用 LittleFS
#include <TempertureAndAirPressure/TempertureAndAirPressure.h>

// 时区配置
#define GMT_OFFSET_SEC 8 * 3600
#define DAYLIGHT_OFFSET_SEC 0

CalendarClient calendarClient(SERVER_URL);
WebServer server(80); // 定义 Web 服务器端口 80
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
#define GRID_START_Y (HEADER_H + WEEK_H - 10)

// 计算格子大小 (800宽 / 7列 ≈ 114)
#define CELL_W (SCREEN_W / 7)
#define CELL_H ((SCREEN_H - GRID_START_Y) / 6) // 剩余高度分给6行

// 颜色定义 (请根据你的屏幕驱动修改，通常 0x00黑, 0x03白, 0x02红)
#define C_BLACK EPD_BLACK
#define C_WHITE EPD_WHITE
#define C_RED EPD_RED
#define C_YELLOW EPD_YELLOW
#define C_GRAY EPD_GRAY

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
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("连接成功");
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

  // 1. 确定颜色逻辑
  // 周末(1) 或 节假日(2) -> 红色，否则 -> 黑色
  uint16_t numColor = (cell.status == 1 || cell.status == 2) ? C_RED : C_BLACK;
  uint16_t lunarColor = C_GRAY; // 如果没有定义C_GRAY，就用 C_BLACK

  // 2. 🔴 “今天”的高亮处理
  if (cell.isToday)
  {
    // 画正方形背景
    int cx = x + CELL_W / 2 - 50;
    int cy = y + CELL_H / 2 - 30;
    int r = (CELL_W < CELL_H ? CELL_W : CELL_H) / 2 - 4;
    screen.fillRect(cx, cy, 105, 60, EPD_BLACK);
    screen.fillRect(cx + 2, cy + 2, 105 - 4, 60 - 4, EPD_WHITE);
    fillGrayRect(cx, cy, 105, 60);
    // 今天文字全部反白
    numColor = C_WHITE;
    lunarColor = C_WHITE;
  }

  // 设置背景色为透明或白色 (取决于具体驱动，通常设为白色比较稳妥)
  // u8g2.setBackgroundColor(C_WHITE);

  // ==========================================
  // 📅 1. 绘制公历数字 (使用 u8g2 大字体)
  // ==========================================
  if (cell.solarDay > 0)
  {
    // 字体推荐：u8g2_font_helvB24_tf (粗体) 或 u8g2_font_logisoso24_tf (大数字)
    u8g2.setFont(u8g2_font_inb33_mf);
    u8g2.setForegroundColor(EPD_BLACK);
    u8g2.setBackgroundColor(EPD_WHITE);
    u8g2.setFontMode(true);
    // u8g2.setForegroundColor(numColor);

    String dayStr = String(cell.solarDay);

    // 自动计算宽度并居中
    int w = u8g2.getUTF8Width(dayStr.c_str());
    int h = u8g2.getFontAscent() - u8g2.getFontDescent(); // 获取字体高度

    int numX = x + (CELL_W - w) / 2;
    // 垂直位置：居中偏上一点
    int numY = y + (CELL_H / 2) + (h / 2) - 12;

    u8g2.setCursor(numX, numY);
    u8g2.print(dayStr);
  }

  // ==========================================
  // 🏮 2. 绘制农历/节日 (使用 u8g2 中文字体)
  // ==========================================
  if (cell.lunarText.length() > 0)
  {
    // ⚠️ 关键：必须使用支持中文的 u8g2 字体
    // u8g2_font_wqy12_t_gb2312 是最常用的 12px 中文字体
    u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2.setFontMode(1);
    // u8g2.setForegroundColor(lunarColor);

    String bottomText = cell.lunarText;

    // 自动计算中文宽度
    int lunW = u8g2.getUTF8Width(bottomText.c_str());
    int lunX = x + (CELL_W - lunW) / 2;
    int lunY = y + CELL_H - 8; // 靠底部

    u8g2.setCursor(lunX, lunY);
    u8g2.print(bottomText);
  }

  // ==========================================
  // 🏷️ 3. 绘制 "休/班" (使用 u8g2 中文字体)
  // ==========================================
  if (cell.status == 2 || cell.status == 3)
  {
    u8g2.setFont(u8g2_font_wqy15_t_gb2312); // 继续用中文字体

    String tag = (cell.status == 2) ? "休" : "班";
    uint16_t tagColor = (cell.status == 2) ? C_RED : C_BLACK;
    // if (cell.isToday)
    //     tagColor = C_WHITE;

    u8g2.setForegroundColor(EPD_WHITE);
    u8g2.setBackgroundColor(EPD_RED);

    // 计算坐标：右上角
    int tagW = u8g2.getUTF8Width(tag.c_str());
    int tagX = x + CELL_W - tagW - 7;
    int tagY = y + 18; // u8g2 的坐标是基线，所以要往下一点，否则字会跑到格子上面去

    u8g2.setCursor(tagX, tagY);
    u8g2.print(tag);
  }
}

void CalendarApp::drawTempBatteryUI()
{

  SensorData data = readSensors();
  // 电池
  myDrawLine(718, 7, 750, 7, 2, EPD_BLACK, true);
  myDrawLine(718, 21, 750, 21, 2, EPD_BLACK, true);
  myDrawLine(718, 7, 718, 21, 2, EPD_BLACK, false);
  myDrawLine(749, 7, 749, 21, 2, EPD_BLACK, false);
  myDrawLine(751, 12, 751, 17, 2, EPD_BLACK, false);
  // 电量显示
  myDrawLine(749, 13, 749, 16, 1, EPD_WHITE, false);
  myDrawLine(720, 8, 720, 20, 15, EPD_BLACK, false);
  // screen.drawRect(450, 50, 800, 50, EPD_WHITE);
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.setForegroundColor(C_BLACK);
  u8g2.setCursor(410, 20);
  u8g2.print("气压:" + String(data.pressure, 0) + " hPa");
  // 当前温度
  u8g2.setCursor(530, 20);
  u8g2.print("温度" + String(data.temperature, 1) + "℃");
  // 湿度
  u8g2.setCursor(630, 20);
  u8g2.print("湿度" + String(data.humidity, 1) + "%");
  // 电量
  u8g2.setCursor(755, 20);
  u8g2.print("80%");
}
void CalendarApp::drawUI_HuangLi(struct tm *now)
{

  // 1. 贴底图

  drawTempBatteryUI();
  // 如果数据没加载成功，显示个错误或者默认值
  if (!_isDataLoaded)
  {
    myFont.setFont(Font_40px);
    myFont.drawText(200, 200, "数据加载失败", C_BLACK);
    return;
  }
  if (screen.getBuffer())
  {
    memcpy(screen.getBuffer(), Huang_Li_Image, 800 * 480 / 4);
  }
  u8g2.setForegroundColor(C_BLACK);
  // --- 动态显示日期 ---
  // 例如：2026年1月
  String yearMonth = String(now->tm_year + 1900) + "年" + String(now->tm_mon + 1) + "月";
  myFont.setFont(Font_40px);
  myFont.drawText(35, 120, yearMonth, EPD_WHITE);

  String dayStr = (now->tm_mday < 10 ? "0" : "") + String(now->tm_mday);
  myFont.setFont(Font_120px);
  myFont.drawText(40, 170, dayStr, EPD_WHITE);

  // 星期
  myFont.setFont(Font_40px);
  myFont.drawText(70, 310, _cachedInfo.weekDay, EPD_WHITE); // 使用接口返回的 "星期X"

  // --- 动态显示黄历信息 ---
  // 农历日期
  myFont.drawText(300, 45, _cachedInfo.lunarString, EPD_ORANGE); // "腊月初八"

  // 干支 + 纳音 (或者生肖)
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.setCursor(305, 105);
  // 比如显示: "乙巳年 戊子月 丙午日"
  u8g2.print(_cachedInfo.ganZhiShengXiaoFull);

  myDrawLine(302, 115, 774, 115, 2, EPD_BLACK);

  // 生肖 & 冲煞
  myFont.drawText(360, 135, _cachedInfo.shengXiao, EPD_BLACK);    // "蛇"
  myFont.drawText(360, 195, "冲" + _cachedInfo.chong, EPD_BLACK); // "冲鼠"

  // 财神/喜神等方位
  myFont.drawText(600, 135, _cachedInfo.caiShen, EPD_BLACK); // "正北" (财神)

  // 彭祖百忌 或 纳音
  // 注意：font40px可能显示不下太长的字，建议换小字体
  u8g2.setCursor(600, 195);
  u8g2.print(_cachedInfo.naYin); // "覆灯火"

  // 宜
  u8g2.setCursor(350, 295);
  // 截取一部分显示，防止太长超出屏幕
  String yiShort = _cachedInfo.yi.length() > 100 ? _cachedInfo.yi.substring(0, 100) + "..." : _cachedInfo.yi;
  u8g2.print(yiShort);

  // 忌
  u8g2.setCursor(350, 337);
  String jiShort = _cachedInfo.ji.length() > 100 ? _cachedInfo.ji.substring(0, 100) + "..." : _cachedInfo.ji;
  u8g2.print(jiShort);

  // --- 动态显示天气 ---
  // drawBitmap... (可以根据天气现象 weather.currentWeather 选不同的图标)
  bool isNight = (now->tm_hour >= 18 || now->tm_hour < 6);
  myFont.drawText(310, 410, _cachedWeather.currentTemp + "℃", EPD_BLACK);

  u8g2.setCursor(670, 425);
  u8g2.print(_cachedWeather.currentWeather); // "多云"
  drawBitmap(405, 400, getWeatherIcon(_cachedWeather.currentWeather, isNight), IMAGE_WIDTH, IMAGE_HEIGHT, C_BLACK);

  // 显示今天白天的风向
  if (_cachedWeather.forecastCount > 0)
  {
    u8g2.setCursor(670, 450);
    String windInfo = _cachedWeather.forecasts[0].dayWind + "风 " + _cachedWeather.forecasts[0].dayPower + " 级";
    u8g2.print(windInfo);
  }
  u8g2.setCursor(310, 450);
  u8g2.print(_cachedWeather.province+"-"+_cachedWeather.city); // 城市
  // myFont.drawText(305, 45, "冬月十五", EPD_ORANGE);
  // u8g2.setCursor(305, 105);
  // u8g2.print("乙巳年 戊子月 丙午日");
  // myDrawLine(302, 115, 774, 115, 2, EPD_BLACK);
  // myFont.drawText(360, 135, "蛇", EPD_BLACK);
  // myFont.drawText(360, 195, "冲鼠", EPD_BLACK);

  // myFont.drawText(600, 135, "正西", EPD_BLACK);
  // myFont.drawText(600, 195, "丙不修道", EPD_BLACK);

  // u8g2.setCursor(350, 295);
  // u8g2.print("祭祀 祈福 求嗣 开光 出行 解除 伐木 拆卸");
  // u8g2.setCursor(350, 337);
  // u8g2.print("嫁娶 移徙 入宅 安葬");

  // // 天气
  // myFont.drawText(310, 410, "20℃", EPD_BLACK);
  // u8g2.setCursor(670, 425);
  // u8g2.print("空气优");
  // u8g2.setCursor(670, 450);
  // u8g2.print("西北风 3 级");
}

void CalendarApp::myDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t width, uint16_t color, boolean isX)
{
  for (size_t i = 0; i < width; i++)
  {
    if (isX)
    {
      screen.drawLine(x0, y0 + i, x1, y1 + i, color);
    }
    else
    {
      screen.drawLine(x0 + i, y0, x1 + i, y1, color);
    }
  }
}
// 🎨 纯粹的 UI 绘制逻辑
void CalendarApp::drawUI_MoonCalendar(struct tm *now)
{
  // 1. 贴底图
  // if (screen.getBuffer())
  // {
  //     memcpy(screen.getBuffer(), Huang_Li_Image, 800 * 480 / 4);
  // }
  screen.fillScreen(C_WHITE);

  drawTempBatteryUI();

  myFont.setFont(Font_40px);
  myFont.drawText(20, 0, "2026年1月", EPD_ORANGE);
  myDrawLine(0, 45, 800, 45, 2, EPD_RED, true);

  const String weekNames[] = {"一", "二", "三", "四", "五", "六", "日"};
  // struct CalendarCell {
  //   int solarDay;           // 公历日期
  //   String lunarText;       // 农历/节日
  //   int status;             // 0=班, 1=休, 2=法定休, 3=补班
  //   bool isCurrentMonth;    // 是否本月
  //   bool isToday;           // 是否今天
  // };
  CalendarCell cell = {9, "春节", 2, true, true};
  for (int i = 0; i < 7; i++)
  {
    // 1. 计算居中坐标
    // i * CELL_W : 当前格子的左边界
    // (CELL_W - fontWidth) / 2 : 在格子内居中
    int x = i * CELL_W + (CELL_W - 40) / 2;
    // 2. 颜色逻辑
    // 数组下标 0~4 是周一到周五 (黑)
    // 数组下标 5~6 是周六、周日 (红)
    uint16_t color = (i >= 5) ? EPD_RED : EPD_BLACK;
    // 3. 绘制文字
    myFont.drawText(x, 50, weekNames[i], color);
  }
  for (int i = 0; i < 42; i++)
  {
    drawSingleCell(i, cell);
  }
}

void CalendarApp::drawUI_Clock(struct tm *now)
{
  // 1. 贴底图
  if (screen.getBuffer())
  {
    memcpy(screen.getBuffer(), Clock_Image, 800 * 480 / 4);
  }
  // screen.fillScreen(C_WHITE);
  drawTempBatteryUI();
  myFont.setFont(Font_40px);
  myFont.drawText(0, 0, "2026年1月5日", EPD_ORANGE);
  myFont.drawText(0, 50, "星期二", EPD_ORANGE);

  int cx = 400;
  int cy = 240;

  // ==========================================
  // 🕒 2. 计算角度
  // ==========================================

  // 分针角度：0~59分 -> 0~360度 秒的计算加上屏幕刷新时间
  float m_angle = (now->tm_min * 6.0) + ((now->tm_sec + 15) * 0.1);

  // 时针角度：0~11时 -> 0~360度
  // 重要：加上 (分/60 * 30) 让时针平滑移动，不要指在两个数字中间跳变
  // 基础：每小时走 30度
  float hour_part = (now->tm_hour % 12) * 30.0;
  // 分钟影响：每分钟时针走 0.5度
  float min_part = now->tm_min * 0.5;

  // 秒影响：每秒钟时针走 (0.5度 / 60) ≈ 0.00833度
  // 虽然肉眼几乎看不见，但为了逻辑完整加上它
  float sec_part = (now->tm_sec + 15) * 0.008333;

  float h_angle = hour_part + min_part + sec_part;
  Serial.print(m_angle);

  // ==========================================
  // 🖌️ 3. 绘制指针
  // ==========================================

  // 画时针 (黑色，短粗)
  // 长度 100，宽度 16
  drawNeedle(cx, cy, h_angle, 100, 16, EPD_BLACK);

  // 画分针 (黑色，细长)
  // 长度 130，宽度 8
  drawNeedle(cx, cy, m_angle, 130, 8, EPD_RED);

  // ==========================================
  // 🔴 4. 绘制中心装饰盖 (Center Cap)
  // ==========================================
  // 为了盖住两个三角形重叠的乱七八糟的部分，最后画一个红点

  // 先画一个白圈做边框 (模拟 border)
  screen.fillCircle(cx, cy, 14, C_WHITE);
  // 再画红点
  screen.fillCircle(cx, cy, 10, C_RED);
  // myDrawLine(0, 45, 800, 45, 2, EPD_RED, true);
}
static bool hasNewImage = false;
bool CalendarApp::run(const char *ssid, const char *password)
{
  // 1. 联网对时
  connectWiFi(ssid, password);
  syncTime();
  // 3. 🌟 【核心修改】只调用一次 API，获取所有数据
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    // 调用 update，传入当前的年月日
    // 注意：tm_year 是从1900开始的，tm_mon 是 0-11
    if (calendarClient.update(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday))
    {
      Serial.println("Data fetch success!");

      // 将数据缓存到成员变量中
      _cachedGrid = calendarClient.getGridData();
      _cachedInfo = calendarClient.getDailyInfo();
      _cachedWeather = calendarClient.getWeatherInfo();
      _isDataLoaded = true;

      // 2. 🔍【新增】串口详细打印调试信息
      Serial.println("\n========== [API 数据调试] ==========");

      // --- 打印黄历详情 ---
      Serial.println("--- 📅 日期与黄历 ---");
      Serial.println("公历: " + _cachedInfo.dateString + " " + _cachedInfo.weekDay + " " + _cachedInfo.xingZuo);
      Serial.println("农历: " + _cachedInfo.lunarYear + " " + _cachedInfo.lunarString);
      Serial.println("干支: " + _cachedInfo.ganZhi);
      Serial.println("生肖: " + _cachedInfo.shengXiaoFull);
      Serial.println("宜: " + _cachedInfo.yi);
      Serial.println("忌: " + _cachedInfo.ji);
      Serial.printf("杂项: %s | %s | %s | %s\n", _cachedInfo.jieQi.c_str(), _cachedInfo.wuHou.c_str(), _cachedInfo.shuJiu.c_str(), _cachedInfo.fu.c_str());
      Serial.printf("方位: 喜神%s | 财神%s | 福神%s | 胎神%s\n", _cachedInfo.xiShen.c_str(), _cachedInfo.caiShen.c_str(), _cachedInfo.fuShen.c_str(), _cachedInfo.taiShen.c_str());

      // --- 打印天气 ---
      Serial.println("\n--- 🌤️ 天气信息 ---");
      Serial.println("城市: " + _cachedWeather.city);
      Serial.println("实况: " + _cachedWeather.currentWeather + " " + _cachedWeather.currentTemp + "℃");

      Serial.println("未来预报 (" + String(_cachedWeather.forecastCount) + "天):");
      for (int i = 0; i < _cachedWeather.forecastCount; i++)
      {
        ForecastItem f = _cachedWeather.forecasts[i];
        // 使用 printf 格式化输出更整齐
        Serial.printf("  [%s %s] 白天:%s(%s℃) 晚上:%s(%s℃) 风:%s%s级\n",
                      f.date.c_str(), f.week.c_str(),
                      f.dayWeather.c_str(), f.dayTemp.c_str(),
                      f.nightWeather.c_str(), f.nightTemp.c_str(),
                      f.dayWind.c_str(), f.dayPower.c_str());
      }
      Serial.println("====================================\n");
    }
    else
    {
      Serial.println("Data fetch failed!");
      _isDataLoaded = false;
    }
  }
  // 初始化变量
  int page = 1; // 默认直接进画廊页测试
  if (!getLocalTime(&timeinfo))
  {
    timeinfo.tm_year = 2025 - 1900;
    timeinfo.tm_mday = 1;
  }

  // 🌟 初始化 Web Server

  // 2. 唤醒屏幕硬件
  screen.begin();

  // 先画一次当前页面
  switch (page)
  {
  case 1:
    drawUI_HuangLi(&timeinfo);
    break;
  case 2:
    drawUI_MoonCalendar(&timeinfo);
    break;
  case 3:
    drawUI_Clock(&timeinfo);
    break;
  case 4:
    setupWebServer();
    drawGalleryPage();
    break;
  default:
    break;
  }
  screen.display(); // 第一次显示

  // 4. 进入主循环
  while (1)
  {
    // 🌟 A. 处理 Web 请求
    handleClient();

    // 🌟 B. 检查是否有新图片上传成功
    if (hasNewImage)
    {
      Serial.println("Refreshing Gallery...");

      // 只有当前在第4页(画廊页)才立即刷新，防止在看时钟时突然跳图
      if (page == 4)
      {
        // 建议先清屏，防止残影
        screen.fillScreen(C_WHITE);

        // 重画画廊
        drawGalleryPage();

        // 刷新屏幕硬件
        screen.display();
      }

      // 复位标志位
      hasNewImage = false;
    }

    // 🌟 C. 必须加延时！
    // 如果不加 delay，CPU 会被死循环占满，导致 WiFi 没时间收发数据，
    // 表现就是网页点发送后一直转圈圈，或者“连接被重置”。
    delay(5);
  }
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
void CalendarApp::fillGrayRect(int16_t x, int16_t y, int16_t w, int16_t h)
{
  for (int i = 0; i < w; i++)
  {
    for (int j = 0; j < h; j++)
    {
      // 计算绝对坐标
      int pixelX = x + i;
      int pixelY = y + j;

      // 🧮 核心算法：检查坐标之和的奇偶性
      // 结果是 0 就画黑，是 1 就画白
      if ((pixelX + pixelY) % 9 == 0)
      {
        screen.drawPixel(pixelX, pixelY, C_BLACK);
      }
      else
      {
        screen.drawPixel(pixelX, pixelY, C_WHITE);
      }
    }
  }
}

// 🎨 通用画指针函数
// angle: 角度 (0~360，0点是12点方向)
// len: 指针长度
// width: 指针底部的宽度 (粗细)
// color: 颜色
void CalendarApp::drawNeedle(int16_t cx, int16_t cy, float angle, int16_t len, int16_t width, uint16_t color)
{
  // 1. 将角度转换为弧度 (减90度是因为 0度通常指右边，而时钟0度是上面)
  float rad = (angle - 90) * PI / 180.0;

  // 2. 计算针尖坐标 (Tip)
  int16_t xTip = cx + cos(rad) * len;
  int16_t yTip = cy + sin(rad) * len;

  // 3. 计算针尾宽度 (Base)
  // 我们需要找到与指针垂直的那条线，角度相差 90度 (PI/2)
  float radBase = rad + PI / 2.0;

  // 针尾左侧点
  int16_t xBase1 = cx + cos(radBase) * (width / 2);
  int16_t yBase1 = cy + sin(radBase) * (width / 2);

  // 针尾右侧点
  int16_t xBase2 = cx - cos(radBase) * (width / 2);
  int16_t yBase2 = cy - sin(radBase) * (width / 2);

  // 4. 画实心三角形
  // 注意：如果有尾巴需求，可以让 Base 点往反方向延伸一点，这里简化为以中心为底
  screen.fillTriangle(xBase1, yBase1, xBase2, yBase2, xTip, yTip, color);

  // 5. (可选) 画一个小圆修正锯齿，让根部圆润一点
  screen.fillCircle(cx, cy, width / 2, color);
}

// 🚀 初始化 Web 服务器
// 🚀 初始化 Web 服务器 (修复版)
void CalendarApp::setupWebServer()
{
  // 启动文件系统
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS Mount Failed, trying to format...");
    LittleFS.format();
    if (!LittleFS.begin(true))
      return;
  }
  Serial.println("LittleFS Mounted Successfully");

  // 1. 首页
  server.on("/", HTTP_GET, []()
            { server.send(200, "text/html", html_upload_page); });

  // 2. 处理图片上传 (核心修复部分)
  server.on("/upload", HTTP_POST, []()
            {
              // A. 请求结束时的回调：发送成功响应
              server.send(200, "text/plain", "Upload Success");
              Serial.println("Image Received & Saved!");
              hasNewImage = true; // 通知主循环刷屏
            },
            []()
            {
        // B. 数据传输过程中的回调
        HTTPUpload& upload = server.upload();
        
        // 🌟 核心修复：定义一个静态文件对象，保持连接
        static File uploadFile; 

        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Start Upload: %s\n", upload.filename.c_str());
            
            // 为了防止空间不足，先删除旧文件
            if(LittleFS.exists("/gallery.bin")) {
                LittleFS.remove("/gallery.bin");
            }

            // 打开文件，模式为 "w" (写入/覆盖)
            uploadFile = LittleFS.open("/gallery.bin", "w");
            if (!uploadFile) {
                Serial.println("Failed to open file for writing");
            }
        } 
        else if (upload.status == UPLOAD_FILE_WRITE) {
            // 🌟 只有文件打开成功才写入，保持文件开启状态
            if (uploadFile) {
                uploadFile.write(upload.buf, upload.currentSize);
            }
        } 
        else if (upload.status == UPLOAD_FILE_END) {
            // 🌟 上传结束时才关闭文件
            if (uploadFile) {
                uploadFile.close();
                Serial.printf("Upload End. Size: %u\n", upload.totalSize);
            }
        } });

  server.begin();
  Serial.println("Web Server Started");

  // 打印 IP
  Serial.println("Upload: " + WiFi.localIP().toString());

  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor(10, 470);
  u8g2.print("Upload: " + WiFi.localIP().toString());
}

// 在 run() 的循环里调用
void CalendarApp::handleClient()
{
  server.handleClient();
}

// 🖼️ 绘制画廊页面
void CalendarApp::drawGalleryPage()
{
  // 1. 检查文件是否存在
  if (!LittleFS.exists("/gallery.bin"))
  {
    myFont.setFont(Font_40px);
    myFont.drawText(200, 200, "请通过 WiFi 上传图片", C_RED);
    myFont.setFont(Font_40px);
    String ipMsg = "访问: http://" + WiFi.localIP().toString();
    return;
  }

  // 2. 打开文件
  File file = LittleFS.open("/gallery.bin", "r");
  if (!file)
    return;

  // 3. 读取并绘制 (使用缓冲区加速)
  uint8_t buf[800]; // 每次读一行

  for (int y = 0; y < 480; y++)
  {
    // 读满一行 800 个像素
    if (file.read(buf, 800) == 800)
    {
      for (int x = 0; x < 800; x++)
      {
        uint8_t colorVal = buf[x];
        // 这里的 colorVal 就是网页传过来的 0,1,2,3
        // 直接画点
        myAppDrawPixel(x, y, colorVal);
      }
    }
  }

  file.close();

  // 4. 显示 IP 地址提示，方便用户知道去哪里上传
  // 可以画在角落里
  u8g2.setForegroundColor(EPD_BLACK);
  u8g2.setFontMode(1);
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor(10, 470);
  u8g2.print("Upload: " + WiFi.localIP().toString());
}
const uint8_t *CalendarApp::getWeatherIcon(String weatherStr, boolean isNight)
{

  // 1. 定义扩展后的枚举 (共 11 种基础类型)
  enum WeatherID
  {
    ID_UNKNOWN = 0,
    ID_QING,   // 晴
    ID_DUOYUN, // 多云
    ID_YIN,    // 阴
    ID_YU,     // 雨
    ID_LEI,    // 雷
    ID_XUE,    // 雪
    ID_SLEET,  // 🌧️雨夹雪 (新增)
    ID_WU,     // 🌫️雾/霾
    ID_SAND,   // 🏜️沙尘 (新增)
    ID_WIND    // 🍃大风 (新增)
  };

  int typeId = ID_UNKNOWN;

  // 2. 字符串匹配逻辑 (🌟注意优先级：字数多/特殊的在前)
  // 例如：必须先判断 "雨夹雪"，再判断 "雨" 或 "雪"

  if (weatherStr.indexOf("雨夹雪") >= 0)
    typeId = ID_SLEET;
  else if (weatherStr.indexOf("雷") >= 0)
    typeId = ID_LEI;
  else if (weatherStr.indexOf("沙") >= 0 || weatherStr.indexOf("尘") >= 0)
    typeId = ID_SAND;
  else if (weatherStr.indexOf("雾") >= 0 || weatherStr.indexOf("霾") >= 0)
    typeId = ID_WU;

  else if (weatherStr.indexOf("雪") >= 0)
    typeId = ID_XUE;
  else if (weatherStr.indexOf("雨") >= 0)
    typeId = ID_YU;

  else if (weatherStr.indexOf("阴") >= 0)
    typeId = ID_YIN;
  else if (weatherStr.indexOf("多云") >= 0)
    typeId = ID_DUOYUN;
  else if (weatherStr.indexOf("晴") >= 0)
    typeId = ID_QING;

  // 风的判断 (只有明确写了"风"且通常不是微风时才显示风图标，视API返回而定)
  else if (weatherStr.indexOf("风") >= 0)
    typeId = ID_WIND;

  // 3. 图标选择逻辑 (支持夜间模式)
  switch (typeId)
  {
  case ID_QING:
    // 如果是夜晚，返回月亮图标，否则返回太阳
    return isNight ? image_qing_n : image_qing;

  case ID_DUOYUN:
    // 多云也有夜间模式
    return isNight ? image_duoyun_n : image_duoyun;

  case ID_YIN:
    return image_yin;
  case ID_YU:
    return image_yu;
  case ID_LEI:
    return image_lei;
  case ID_XUE:
    return image_xue;
  case ID_SLEET:
    return image_sleet; // 对应 WeatherIcons.h 中的 image_sleet
  case ID_WU:
    return image_wu;
  case ID_SAND:
    return image_sand; // 对应 WeatherIcons.h 中的 image_sand
  case ID_WIND:
    return image_wind; // 对应 WeatherIcons.h 中的 image_wind

  default:
    return image_unknown; // 未知天气
  }
}