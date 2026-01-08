#include "TempertureAndAirPressure.h" // 引用头文件

// 🌟 关键修改：在这里真正定义变量
// 只有在 .cpp 里才能不加 extern
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
bool isAhtReady = false;
bool isBmpReady = false;

// 🔧 定义 I2C 引脚 (根据你的板子修改)
#define I2C_SDA 17
#define I2C_SCL 18


// 🛠️ 2. 初始化函数 (放在 setup 中调用)
void initSensors() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // 初始化 AHT20
    if (aht.begin()) {
        isAhtReady = true;
        Serial.println("✅ AHT20 Init Success");
    } else {
        Serial.println("❌ AHT20 Init Failed!");
    }

    // 初始化 BMP280 (尝试两个常见地址)
    if (bmp.begin(0x76) || bmp.begin(0x77)) {
        isBmpReady = true;
        // 设置采样模式以提高精度
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                        Adafruit_BMP280::SAMPLING_X2,
                        Adafruit_BMP280::SAMPLING_X16,
                        Adafruit_BMP280::FILTER_X16,
                        Adafruit_BMP280::STANDBY_MS_500);
        Serial.println("✅ BMP280 Init Success");
    } else {
        Serial.println("❌ BMP280 Init Failed!");
    }
}

// 📡 3. 读取函数 (这就是你要的单独函数)
SensorData readSensors() {
    SensorData data = {0, 0, 0, false}; // 默认初始化

    // 读取 AHT20 (主要负责温湿度)
    if (isAhtReady) {
        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);
        data.temperature = temp.temperature;
        data.humidity = humidity.relative_humidity;
        data.valid = true; 
    }

    // 读取 BMP280 (主要负责气压，也可以辅助测温)
    if (isBmpReady) {
        // 如果 AHT20 坏了，用 BMP280 的温度顶替
        if (!isAhtReady) {
            data.temperature = bmp.readTemperature();
        }
        // 读取气压并转换为 hPa (百帕)
        data.pressure = bmp.readPressure() / 100.0F;
        data.valid = true;
    }

    return data;
}