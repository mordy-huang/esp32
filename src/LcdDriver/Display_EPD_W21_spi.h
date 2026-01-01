#ifndef _DISPLAY_EPD_W21_SPI_
#define _DISPLAY_EPD_W21_SPI_
#include "Arduino.h"

// ============================================
//   ESP32-S3 引脚映射 (修改此处)
// ============================================
// 你的安全引脚接线：
// BUSY -> 4
// RST  -> 5
// DC   -> 6
// CS   -> 7
// SCK  -> 15 (在 main.cpp 里定义，这里不需要)
// MOSI -> 16 (在 main.cpp 里定义，这里不需要)

// 定义引脚号
#define EPD_PIN_BUSY 4
#define EPD_PIN_RST  5
#define EPD_PIN_DC   6
#define EPD_PIN_CS   7

// ============================================
//   底层宏定义 (适配 ESP32)
// ============================================
// BUSY 读取 (注意：这款屏幕 BUSY 是 LOW 忙，还是 HIGH 忙，原厂代码里是 digitalRead(A14) )
// 我们保留原厂逻辑，只替换引脚号
#define isEPD_W21_BUSY digitalRead(EPD_PIN_BUSY)

// RST 控制
#define EPD_W21_RST_0 digitalWrite(EPD_PIN_RST, LOW)
#define EPD_W21_RST_1 digitalWrite(EPD_PIN_RST, HIGH)

// DC 控制
#define EPD_W21_DC_0  digitalWrite(EPD_PIN_DC, LOW)
#define EPD_W21_DC_1  digitalWrite(EPD_PIN_DC, HIGH)

// CS 控制
#define EPD_W21_CS_0 digitalWrite(EPD_PIN_CS, LOW)
#define EPD_W21_CS_1 digitalWrite(EPD_PIN_CS, HIGH)


void SPI_Write(unsigned char value);
void EPD_W21_WriteDATA(unsigned char datas);
void EPD_W21_WriteCMD(unsigned char command);

#endif