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