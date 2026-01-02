#include <Arduino.h>
#include "RGB_LED/RGBLight.h"     // 引入 LED 模块
#include "Calendar/CalendarApp.h" // 引入日历模块
#include <config.h>


// ================= 配置 =================

// S3 板载灯引脚
#define LED_PIN 48

// 更新间隔：1分钟 (60 * 1000 毫秒)
// 注意：墨水屏刷新本身需要约 15-20 秒，所以实际间隔是 1分钟 + 刷新时间
#define UPDATE_INTERVAL_MS 60000

// 创建模块实例
RGBLight led(LED_PIN);
EPD_Driver driver(800, 480);
CalendarApp calendar(driver);

// 记录上次更新的时间
unsigned long lastUpdateTime = 0;
void setup()
{
  // 1. 系统启动缓冲
  delay(3000);
  Serial.begin(115200);
  Serial.println(">>> 系统启动 (常亮模式) <<<");
  // 2. 直接使用 Secrets.h 里定义的变量名
  // WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Serial.print("正在连接 WiFi: ");
  // Serial.println(WIFI_SSID); // 打印一下名字确认

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // 2. 初始化模块
  led.begin();
  calendar.begin(); // 这里面会初始化 SPI 和 屏幕引脚

  // 3. 开机先立即运行一次，不要傻等1分钟
  Serial.println("开机首次更新...");
  led.runBreathingEffect(1); // 快速让灯动一下

  // 运行日历 (这会阻塞约 15-20 秒)
  // calendar.test();

  // 记录当前时间，作为下一次计时的起点
  calendar.run(WIFI_SSID, WIFI_PASS);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  //

  lastUpdateTime = millis();


}

void loop()
{
  // 1. 让呼吸灯一直运行
  // 只要下面的 if 没触发，这个函数就会飞速运行，灯光非常丝滑
  led.runBreathingEffect(5);

  // 2. 检查时间：是不是过去 1 分钟了？
  if (millis() - lastUpdateTime > UPDATE_INTERVAL_MS)
  {
    Serial.println("时间到！开始更新屏幕...");

    // 在更新屏幕前，可以把灯设为某种状态（可选）
    // 因为 calendar.run 是阻塞的，更新期间呼吸灯会“卡住”不动
    // 这是单线程单片机的特性，无法避免（除非上 FreeRTOS）

    // 执行日历任务 (连网 -> 对时 -> 绘图 -> 刷屏)
    // 过程约 15-20 秒

    // 更新完成后，重置计时器
    lastUpdateTime = millis();
    Serial.println("更新完成，等待下一次...");
  }
}