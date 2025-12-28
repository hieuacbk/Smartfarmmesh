#ifndef MQTT_GATEWAY_H
#define MQTT_GATEWAY_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "MQTTConfig.h"
#include "MeshTypes.h"
#include <vector>

class MQTTGateway {
public:
    MQTTGateway();

    void begin();
    void loop();

    // Publish sensor frame (value1/value2)
    void publishSensorFrame(const uint8_t *mac, const SFM_SensorFramePayload &payload);

    // MQTT Discovery for sensor
    void publishDiscovery(const String &macStr, const SFM_SensorValue &v);

    // NEW: Node Auto-Discovery (V1.8)
    void publishNodeDiscovery(const String &macStr);
    void publishNodeStatus(const String &macStr, const SFM_HeartbeatPayload &hb);

    // Helper
    void getSensorMeta(const String &name, String &unit, String &deviceClass);

    // Make macToString public so MeshNode can use it
    String macToString(const uint8_t *mac);


    
private:
    WiFiClient wifiClient;
    PubSubClient mqtt;

    //void reconnect();
    //String macToString(const uint8_t *mac);

    // ==== NEW V1.8 – MQTT QUEUE ====
    struct PendingMsg {
        String topic;
        String payload;
        bool retain;
    };

    std::vector<PendingMsg> mqttQueue;

    void reconnect();

    // Queue helpers
    void enqueue(const String &topic, const String &payload, bool retain = false);
    void processQueue();


};

#endif