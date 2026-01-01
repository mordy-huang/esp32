#include "RGBLight.h"

// 预定义 7 种颜色
const uint32_t COLORS[] = {
    0xFF0000, // 红
    0xFFA500, // 橙
    0xFFFF00, // 黄
    0x00FF00, // 绿
    0x00FFFF, // 青
    0x0000FF, // 蓝
    0x800080  // 紫
};
const int COLOR_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);

// 构造函数
RGBLight::RGBLight(int pin, int numPixels) 
    : _numPixels(numPixels), pixels(numPixels, pin, NEO_GRB + NEO_KHZ800) {
}

// 初始化
void RGBLight::begin() {
    pixels.begin();
    pixels.show(); // 关闭所有灯
    _brightness = 0;
    _fadeAmount = 1; // 初始为变亮方向
}

// 设置特定亮度的颜色
void RGBLight::setBrightnessColor(uint32_t color, int brightness) {
    pixels.setPixelColor(0, color);
    pixels.setBrightness(brightness);
    pixels.show();
}

// 核心呼吸逻辑 (非阻塞式，不会卡死程序)
void RGBLight::runBreathingEffect(int speed) {
    // 使用 millis() 代替 delay()，这样你的程序还能同时做别的事
    if (millis() - _lastUpdate > speed) {
        _lastUpdate = millis();

        // 1. 更新亮度
        _brightness += _fadeAmount;

        // 2. 检查边界
        if (_brightness <= 0 || _brightness >= 255) {
            _fadeAmount = -_fadeAmount; // 反转方向 (吸气变呼气，呼气变吸气)
            
            // 如果是从亮变暗到了0，说明一次呼吸结束，切换颜色
            if (_brightness <= 0) {
                _brightness = 0; // 修正
                _currentColorIndex++;
                if (_currentColorIndex >= COLOR_COUNT) {
                    _currentColorIndex = 0;
                }
            }
        }

        // 3. 应用颜色
        setBrightnessColor(COLORS[_currentColorIndex], _brightness);
    }
}