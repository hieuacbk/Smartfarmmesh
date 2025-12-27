#include "Sensor_PHAnalog.h"
#include <Arduino.h>

Sensor_PHAnalog::Sensor_PHAnalog(uint8_t pin) {
  adcPin = pin;
}

const char* Sensor_PHAnalog::getName() {
  return "PH_ANALOG";
}

bool Sensor_PHAnalog::begin() {
  pinMode(adcPin, INPUT);
  return true;
}

bool Sensor_PHAnalog::read(float &v1, float &v2) {
  int raw = analogRead(adcPin);
  float voltage = raw * 3.3 / 4095.0;
  float ph = 7 + (2.5 - voltage) * 3.5;

  v1 = ph;
  v2 = voltage;
  return true;
}