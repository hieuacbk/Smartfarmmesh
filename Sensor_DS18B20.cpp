#include "Sensor_DS18B20.h"

Sensor_DS18B20::Sensor_DS18B20(uint8_t pin)
  : dataPin(pin), oneWire(pin), sensors(&oneWire) {}

const char* Sensor_DS18B20::getName() {
  return "DS18B20";
}

bool Sensor_DS18B20::begin() {
  sensors.begin();
  return true;
}

bool Sensor_DS18B20::read(float &v1, float &v2) {
  sensors.requestTemperatures();
  v1 = sensors.getTempCByIndex(0);
  v2 = 0;
  return true;
}

/*
#include "Sensor_Template.h"

Sensor_Template::Sensor_Template(int pin)
: _pin(pin)
{
}

bool Sensor_Template::begin() {
    // TODO: Khởi tạo sensor ở đây
    // pinMode(_pin, INPUT);
    // init I2C / SPI / ADC...
    return true;
}

bool Sensor_Template::read(float &v1, float &v2) {
    // TODO: Đọc sensor ở đây
    // v1 = analogRead(_pin);
    // v2 = 0;  // nếu không dùng thì để 0

    v1 = 0.0f;   // placeholder
    v2 = 0.0f;

    return true; // trả về false nếu đọc lỗi
}
*/