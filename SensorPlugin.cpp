#include "SensorPlugin.h"

SensorPluginManager::SensorPluginManager() {
  pluginCount = 0;
}

bool SensorPluginManager::registerPlugin(SensorPlugin* plugin) {
  if (pluginCount >= MAX_SENSOR_PLUGINS) return false;
  plugins[pluginCount++] = plugin;
  return true;
}

void SensorPluginManager::beginAll() {
  for (int i = 0; i < pluginCount; i++) {
    plugins[i]->begin();
  }
}

uint8_t SensorPluginManager::readAll(SFM_SensorValue* outValues) {
  uint8_t count = 0;

  for (int i = 0; i < pluginCount; i++) {
    float v1, v2;
    if (plugins[i]->read(v1, v2)) {
      strncpy(outValues[count].name, plugins[i]->getName(), 11);
      outValues[count].name[11] = 0;
      outValues[count].value1 = v1;
      outValues[count].value2 = v2;
      count++;
    }
  }

  return count;
}