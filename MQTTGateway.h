#ifndef MQTT_GATEWAY_H
#define MQTT_GATEWAY_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "MQTTConfig.h"
#include "MeshTypes.h"

class MQTTGateway {
public:
    MQTTGateway();

    void begin();
    void loop();
    void publishSensorFrame(const uint8_t *mac, const SFM_SensorFramePayload &payload);
    // 🔥 NEW: MQTT Discovery
    void publishDiscovery(const String &macStr, const SFM_SensorValue &v);
    // Thêm hàm thêm uni
    void getSensorMeta(const String &name, String &unit, String &deviceClass);
    
private:
    WiFiClient wifiClient;
    PubSubClient mqtt;

    void reconnect();
    String macToString(const uint8_t *mac);
};

#endif