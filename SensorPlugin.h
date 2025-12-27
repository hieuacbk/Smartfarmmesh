#ifndef SENSOR_PLUGIN_H
#define SENSOR_PLUGIN_H

#include <Arduino.h>
#include "MeshTypes.h"

class SensorPlugin {
public:
  virtual const char* getName() = 0;
  virtual bool begin() = 0;
  virtual bool read(float &v1, float &v2) = 0;
};

#define MAX_SENSOR_PLUGINS 8

class SensorPluginManager {
public:
  SensorPluginManager();

  bool registerPlugin(SensorPlugin* plugin);
  void beginAll();
  uint8_t readAll(SFM_SensorValue* outValues);

private:
  SensorPlugin* plugins[MAX_SENSOR_PLUGINS];
  int pluginCount;
};

#endif