#ifndef SENSOR_DS18B20_H
#define SENSOR_DS18B20_H

#include "SensorPlugin.h"
#include <OneWire.h>
#include <DallasTemperature.h>

class Sensor_DS18B20 : public SensorPlugin {
public:
  Sensor_DS18B20(uint8_t pin);

  const char* getName() override;
  bool begin() override;
  bool read(float &v1, float &v2) override;

private:
  uint8_t dataPin;
  OneWire oneWire;
  DallasTemperature sensors;
};

#endif

/*Sensor template
#ifndef SENSOR_TEMPLATE_H
#define SENSOR_TEMPLATE_H

#include "SensorPlugin.h"

class Sensor_Template : public SensorPlugin {
public:
    // Constructor: truyền vào pin hoặc config nếu cần
    Sensor_Template(int pin);

    // Tên sensor (tối đa 15 ký tự)
    const char* getName() override { return "TEMPLATE"; }

    // Khởi tạo sensor
    bool begin() override;

    // Đọc sensor → trả về value1, value2
    bool read(float &v1, float &v2) override;

private:
    int _pin;
};
#endif



*/