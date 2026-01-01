#include "EPD_Driver.h"

// 引脚定义 (根据你的 CalendarApp.cpp 提取)
#define PIN_SCK  15
#define PIN_MOSI 16
// CS, DC, RST, BUSY 已经在 Display_EPD_W21_spi.h 定义，这里不用重写

EPD_Driver::EPD_Driver(int16_t w, int16_t h) : Adafruit_GFX(w, h) {
    // 1. 申请显存
    // 4色屏 2-bit (一个字节存4个像素)
    buffer = (uint8_t *)malloc(w * h / 4);
    if (buffer) {
        clearBuffer();
    } else {
        Serial.println("❌ [Driver] 显存申请失败!");
    }
}

EPD_Driver::~EPD_Driver() {
    if (buffer) free(buffer);
}

void EPD_Driver::begin() {
    // 2. 初始化引脚 (提取自原 CalendarApp::begin)
    pinMode(EPD_PIN_BUSY, INPUT);  
    pinMode(EPD_PIN_RST, OUTPUT); 
    pinMode(EPD_PIN_DC, OUTPUT);    
    pinMode(EPD_PIN_CS, OUTPUT);  

    // 初始化 SPI
    SPI.end();
    SPI.begin(PIN_SCK, -1, PIN_MOSI, EPD_PIN_CS);
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    
    Serial.println("✅ [Driver] 屏幕硬件初始化完成");
}

void EPD_Driver::clearBuffer() {
    if (buffer) memset(buffer, 0xFF, _width * _height / 4); // 0xFF 是全白
}

uint8_t* EPD_Driver::getBuffer() {
    return buffer;
}

// 核心画点逻辑 (提取自原 CalendarApp::drawPixel)
void EPD_Driver::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;
    if (!buffer) return;

    // 4色屏内存映射算法
    long index = y * (_width / 4) + (x / 4);
    int shift = (3 - (x % 4)) * 2;
    
    // 00=白, 01=黄, 10=红, 11=黑 (具体值取决于你的厂商定义)
    // 这里做位操作：先清零对应位，再写入新颜色
    buffer[index] &= ~(0x03 << shift);
    buffer[index] |= ((color & 0x03) << shift);
}

void EPD_Driver::display() {
    if (!buffer) return;
    Serial.println("⚡ [Driver] 开始刷新屏幕 (约15秒)...");

    // 调用厂商驱动流程 (提取自原 flushScreen)
    EPD_init();           // 唤醒/复位
    PIC_display(buffer);  // 发送数据
    EPD_sleep();          // 休眠
    
    Serial.println("✅ [Driver] 刷新完成");
}