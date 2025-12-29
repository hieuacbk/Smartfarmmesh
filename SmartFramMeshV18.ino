#include <Arduino.h>
#include "MeshNode.h"
#include "SmartFarmConfig.h"
#include "SensorPlugin.h"
#include "Sensor_DS18B20.h"
#include "Sensor_PHAnalog.h"
#include "MQTTGateway.h"

MeshNode mesh;
SensorPluginManager sensorManager;
MQTTGateway mqttGateway;

Sensor_DS18B20 sensor1(4);
Sensor_PHAnalog sensor2(36);

void setup() {
  Serial.begin(115200);
  Serial.println("[MAIN] setup() begin");

  mesh.begin();

  sensorManager.registerPlugin(&sensor1);
  sensorManager.registerPlugin(&sensor2);
  sensorManager.beginAll();

  if (DEVICE_ROLE == ROLE_GATEWAY) {
    Serial.println("[MAIN] Gateway detected → initializing MQTT");
    mqttGateway.begin();
  }

  Serial.println("[MAIN] setup() done");
}

void loop() {
  mesh.loop();

  if (DEVICE_ROLE == ROLE_GATEWAY) {
    mqttGateway.loop();
  }

  if (DEVICE_ROLE == ROLE_NODE) {
    static uint32_t lastSend = 0;
    uint32_t now = millis();

    if (now - lastSend > 5000) {
      lastSend = now;

      SFM_SensorValue values[8];
      uint8_t count = sensorManager.readAll(values);

      Serial.printf("[NODE] Sending SensorFrame (%d sensors)\n", count);
      mesh.sendSensorFrame(values, count);

      // 🔥 LOG: Node đã gửi frame đi
      Serial.println("[NODE] SensorFrame sent to Gateway");
    }
  }

  delay(10);
}