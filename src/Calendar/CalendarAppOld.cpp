#include "CalendarAppOld.h"
// 引入你生成的底图文件 
// ⚠️ 如果你的文件名不是 Ap_29demo.h，请修改这里！
#include "Ap_29demo.h" 

// 引脚定义 (ESP32-S3)
#define PIN_SCK  15
#define PIN_MOSI 16
// CS, DC, RST, BUSY 已经在 Display_EPD_W21_spi.h 定义了

// 时区设置 (GMT+8)
#define GMT_OFFSET_SEC 8 * 3600 
#define DAYLIGHT_OFFSET_SEC 0

CalendarAppOld::CalendarAppOld() : Adafruit_GFX(800, 480) {
    // 申请显存 (约 96KB)
    buffer = (uint8_t *)malloc(800 * 480 / 4);
    if(!buffer) {
        Serial.println("❌ [Calendar] 内存申请失败!");
    } else {
        // 初始清空 (全白)
        memset(buffer, 0xFF, 800 * 480 / 4); 
    }
}

void CalendarAppOld::begin() {
    // 1. 初始化驱动引脚
    pinMode(EPD_PIN_BUSY, INPUT);  
    pinMode(EPD_PIN_RST, OUTPUT); 
    pinMode(EPD_PIN_DC, OUTPUT);    
    pinMode(EPD_PIN_CS, OUTPUT);  

    // 2. 初始化 SPI
    SPI.end();
    SPI.begin(PIN_SCK, -1, PIN_MOSI, EPD_PIN_CS);
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0)); 
    
    Serial.println("✅ [Calendar] 硬件初始化完成");
}

// 核心：画点函数 (Adafruit_GFX 需要)
void CalendarAppOld::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;
    if (!buffer) return;

    // 4色屏的内存映射逻辑 (2 bit per pixel)
    long index = y * (_width / 4) + (x / 4);
    int shift = (3 - (x % 4)) * 2;
    
    // 清除原有值 & 写入新颜色
    buffer[index] &= ~(0x03 << shift);
    buffer[index] |= ((color & 0x03) << shift);
}

void CalendarAppOld::connectWiFi(const char* ssid, const char* password) {
    Serial.printf("正在连接 WiFi: %s", ssid);
    WiFi.begin(ssid, password);
    int limit = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        limit++;
        if(limit > 20) {
            Serial.println("\n❌ WiFi 连接超时");
            return; 
        }
    }
    Serial.println("\n✅ WiFi 已连接");
}

bool CalendarAppOld::syncTime() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "ntp.aliyun.com", "pool.ntp.org");
    
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo, 10000)){ // 等待最多10秒
        Serial.println("❌ NTP 对时失败");
        return false;
    }
    Serial.println(&timeinfo, "✅ 当前时间: %A, %B %d %Y %H:%M:%S");
    return true;
}

// =========================================================
// 🎨 重点修改：绘制布局 (Background + Overlay)
// =========================================================
void CalendarAppOld::drawCalendarLayout(struct tm *now) {
    if (!buffer) return;
    
    // -----------------------------------------------------
    // 1. 加载底图 (Background)
    // -----------------------------------------------------
    // 直接把 gImage_1 (96KB) 拷贝到显存 buffer
    Serial.println("正在加载底图...");
    memcpy(buffer, gImage_1, 800 * 480 / 4);

    // -----------------------------------------------------
    // 2. 填入动态数据 (Overlay)
    // -----------------------------------------------------
    
    // --- 左侧：日期区 (假设是红底，我们用白色字) ---
    this->setTextColor(C_BLACK);
    
    // 年份 (Year)
    this->setFont(&FreeSansBold18pt7b);

    // this->setTextSize(1);
    // this->setCursor(720, 80); // 坐标需要对照你的设计图微调
    // this->print(now->tm_year + 1900);

    // 巨大的日期 (Day)
    this->setTextSize(4); 
    this->setCursor(90,232); 
    this->print(now->tm_mday);
    this->setTextSize(1); // 还原大小

    // 星期 (Week)
    const char* weekDays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    this->setFont(&FreeMonoBold12pt7b);
    this->setCursor(60, 350); 
    this->print(weekDays[now->tm_wday]);

    // --- 右侧：信息区 (假设是白底，我们用黑色字) ---
    this->setTextColor(C_BLACK);

    // 时间 (Time) - HH:MM
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", now->tm_hour, now->tm_min);
    
    this->setFont(&FreeSansBold18pt7b);
    this->setTextSize(2); // 放大2倍
    this->setCursor(400, 220); // 假设在右侧正中间
    this->print(timeStr);
    this->setTextSize(1); // 还原

    // 天气 (Weather) - 这里先写死，后面接了API再改变量
    this->setFont(&FreeMonoBold12pt7b);
    this->setCursor(600, 60); 
    this->print("26 C"); // 假数据

    // 电量 (Battery)
    // 假设电池框在 (720, 30)，满电宽度 40px
    int batteryPct = 85; 
    int w = (40 * batteryPct) / 100;
    this->fillRect(722, 32, w, 16, C_BLACK); 
    
    if(batteryPct < 20) {
        this->fillRect(722, 32, w, 16, C_RED); 
    }
}

void CalendarAppOld::flushScreen() {
    if (!buffer) return;
    Serial.println("正在刷新屏幕 (约15秒)...");
    
    // 调用底层驱动刷屏
    EPD_init(); 
    PIC_display(buffer);
    EPD_sleep();
}

bool CalendarAppOld::run(const char* ssid, const char* password) {
    connectWiFi(ssid, password);
    
    bool timeSynced = syncTime();
    
    // 哪怕对时失败，也获取一个时间（可能是错误的）来显示，防止黑屏
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        timeinfo.tm_year = 2025 - 1900;
        timeinfo.tm_mon = 0;
        timeinfo.tm_mday = 1;
        timeinfo.tm_hour = 12;
        timeinfo.tm_min = 0;
    }

    // 拿到时间后，立刻断开 WiFi 省电/稳定电压
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    drawCalendarLayout(&timeinfo);
    flushScreen();
    
    return true;
}


void CalendarAppOld::test() {
   
    Serial.println("正在绘制测试图");
    EPD_init(); //Full screen refresh initialization.
    PIC_display(gImage_1);//To Display one image using full screen refresh.
    EPD_sleep();//Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
    delay(5000); //Delay for 5s.
    
    // 这里的 WiFi 可以不断开，反正等会要 Deep Sleep，系统会自动断电
}

