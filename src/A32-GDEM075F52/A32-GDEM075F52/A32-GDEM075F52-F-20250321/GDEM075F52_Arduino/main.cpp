#include <Arduino.h>
#include <SPI.h>

// 引入官方驱动头文件
#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"
// #include "Ap_29demo.h"  
#include "Ap_29demo1.h"  

// 定义 SPI 引脚 (必须与 Display_EPD_W21_spi.h 里的定义配合)
#define PIN_SCK  15
#define PIN_MOSI 16
// CS, DC, RST, BUSY 已经在 spi.h 里定义了

void setup() {
   Serial.begin(115200);
   delay(1000);
   Serial.println("Starting GDEM075F52 Demo...");

   // 1. 初始化引脚
   // 必须显式设置为 OUTPUT，否则电平拉不动
   pinMode(EPD_PIN_BUSY, INPUT);  
   pinMode(EPD_PIN_RST, OUTPUT); 
   pinMode(EPD_PIN_DC, OUTPUT);    
   pinMode(EPD_PIN_CS, OUTPUT);    

   // 2. 初始化 SPI (ESP32-S3 关键一步)
   // 原厂代码里只有 SPI.begin()，这在 S3 上是跑不通的！
   SPI.end();
   SPI.begin(PIN_SCK, -1, PIN_MOSI, EPD_PIN_CS);
   SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0)); 

   Serial.println("Hardware initialized. Sending Display Commands...");
      Serial.println("EPD Init...");
   EPD_init(); 
      // 2. 全屏刷图片 (gImage_1 来自 Ap_29demo.h)
   Serial.println("Displaying Image...");
   PIC_display(gImage_1);
   
   // 3. 进入睡眠
   Serial.println("Sleep.");
   EPD_sleep();

}

void loop() {
   // --- 演示流程 ---
   
   // 1. 初始化屏幕



   // 4. 等待很久再刷，避免频繁刷新伤屏
   delay(20000); 
   
   PIC_display(gImage_1);
      EPD_init(); 

   // 3. 进入睡眠
   Serial.println("Sleep.");
   EPD_sleep();
   // Serial.println("Display RED...");
   // EPD_init();
   // Display_All_Red();
   // EPD_sleep();
   // delay(5000);

   // Serial.println("Display White...");
   // EPD_init();
   // Display_All_White();
   // EPD_sleep();
   // delay(5000);

}