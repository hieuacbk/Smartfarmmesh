#ifndef SENSOR_PH_ANALOG_H
#define SENSOR_PH_ANALOG_H

#include "SensorPlugin.h"

class Sensor_PHAnalog : public SensorPlugin {
public:
  Sensor_PHAnalog(uint8_t pin);

  const char* getName() override;
  bool begin() override;
  bool read(float &v1, float &v2) override;

private:
  uint8_t adcPin;
};

#endif