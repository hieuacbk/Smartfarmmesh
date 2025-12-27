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