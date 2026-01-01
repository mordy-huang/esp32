#ifndef RGB_LIGHT_H
#define RGB_LIGHT_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class RGBLight {
public:
    // 构造函数：传入引脚号和灯珠数量
    RGBLight(int pin, int numPixels = 1);
    
    // 初始化
    void begin();
    
    // 运行呼吸灯逻辑 (需要在 main loop 中调用)
    void runBreathingEffect(int speed = 6);

private:
    Adafruit_NeoPixel pixels;
    int _numPixels;
    
    // 内部变量用于记录状态
    int _brightness = 0;
    int _fadeAmount = 1;
    int _currentColorIndex = 0;
    unsigned long _lastUpdate = 0; // 非阻塞延时用
    
    // 辅助函数
    void setBrightnessColor(uint32_t color, int brightness);
};

#endif