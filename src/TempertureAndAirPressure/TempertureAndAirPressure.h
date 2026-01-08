#ifndef TEMPERTURE_AND_AIR_PRESSURE_H
#define TEMPERTURE_AND_AIR_PRESSURE_H

#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

// 数据结构体
struct SensorData {
    float temperature;
    float humidity;
    float pressure;
    bool valid;
};

// 🌟 关键修改：加上 extern！
// 这只是告诉引用的文件：“去别的地方找 aht 和 bmp，不要在这里创建新的”
extern Adafruit_AHTX0 aht;
extern Adafruit_BMP280 bmp;

// 函数声明
void initSensors();
SensorData readSensors();

#endif