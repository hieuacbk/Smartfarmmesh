#include "MQTTGateway.h"

MQTTGateway::MQTTGateway() : mqtt(wifiClient) {}

/* ============================================================
   BEGIN()
   ============================================================ */
void MQTTGateway::begin() {
    Serial.println("[MQTT] begin()");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("[MQTT] Connecting WiFi");
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 30) {
        delay(500);
        Serial.print(".");
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" OK");
        Serial.print("[MQTT] IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(" FAILED");
        return;
    }

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    Serial.println("[MQTT] MQTT server configured");
}

/* ============================================================
   LOOP()
   ============================================================ */
void MQTTGateway::loop() {
    if (!mqtt.connected()) {
        reconnect();
    }
    mqtt.loop();
}

/* ============================================================
   RECONNECT()
   ============================================================ */
void MQTTGateway::reconnect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MQTT] WiFi lost → cannot reconnect MQTT");
        return;
    }

    Serial.print("[MQTT] Connecting to broker...");
    if (mqtt.connect("SmartFarmGateway", MQTT_USER, MQTT_PASS)) {
        Serial.println(" OK");
    } else {
        Serial.print(" FAILED, rc=");
        Serial.println(mqtt.state());
        delay(2000);
    }
}

/* ============================================================
   MAC TO STRING
   ============================================================ */
String MQTTGateway::macToString(const uint8_t *mac) {
    char buf[13];
    sprintf(buf, "%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

/* ============================================================
   OLD VERSION (để bạn so sánh)
   ============================================================ */
/*
void MQTTGateway::publishSensorFrame(const uint8_t *mac, const SFM_SensorFramePayload &payload) {
    Serial.println("[MQTT] publishSensorFrame()");

    if (!mqtt.connected()) {
        Serial.println("[MQTT] Not connected → skip publish");
        return;
    }

    String macStr = macToString(mac);
    Serial.printf("[MQTT] MAC=%s, count=%d\n", macStr.c_str(), payload.count);

    for (int i = 0; i < payload.count; i++) {
        String topic1 = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + payload.values[i].name + "/value1";
        String topic2 = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + payload.values[i].name + "/value2";

        Serial.printf("[MQTT] → %s = %.2f\n", topic1.c_str(), payload.values[i].value1);
        Serial.printf("[MQTT] → %s = %.2f\n", topic2.c_str(), payload.values[i].value2);

        mqtt.publish(topic1.c_str(), String(payload.values[i].value1).c_str());
        mqtt.publish(topic2.c_str(), String(payload.values[i].value2).c_str());
    }

    String statusTopic = String(MQTT_BASE_TOPIC) + "/" + macStr + "/status";
    mqtt.publish(statusTopic.c_str(), "online");
    Serial.printf("[MQTT] → %s = online\n", statusTopic.c_str());
}
*/

/* ============================================================
   NEW VERSION — mở rộng publish MQTT
   ============================================================ */
void MQTTGateway::publishSensorFrame(const uint8_t *mac, const SFM_SensorFramePayload &payload) {
    Serial.println("[MQTT] publishSensorFrame()");

    if (!mqtt.connected()) {
        Serial.println("[MQTT] Not connected → skip publish");
        return;
    }

    String macStr = macToString(mac);
    Serial.printf("[MQTT] MAC=%s, count=%d\n", macStr.c_str(), payload.count);

    // Publish từng sensor
    for (int i = 0; i < payload.count; i++) {
        const SFM_SensorValue &v = payload.values[i];
        
        // 🔥 NEW: MQTT Discovery
        publishDiscovery(macStr, v);

        // value1
        String topic1 = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + v.name + "/value1";
        String valueStr1 = String(v.value1, 2);
        mqtt.publish(topic1.c_str(), valueStr1.c_str());
        Serial.printf("[MQTT] → %s = %s\n", topic1.c_str(), valueStr1.c_str());

        // value2 — chỉ publish nếu khác 0.00
        if (v.value2 != 0.0f) {
            String topic2 = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + v.name + "/value2";
            String valueStr2 = String(v.value2, 2);
            mqtt.publish(topic2.c_str(), valueStr2.c_str());
            Serial.printf("[MQTT] → %s = %s\n", topic2.c_str(), valueStr2.c_str());
        }
    }

    // Publish status = online
    String statusTopic = String(MQTT_BASE_TOPIC) + "/" + macStr + "/status";
    mqtt.publish(statusTopic.c_str(), "online");
    Serial.printf("[MQTT] → %s = online\n", statusTopic.c_str());

    // Publish uptimeMs
    String uptimeTopic = String(MQTT_BASE_TOPIC) + "/" + macStr + "/uptimeMs";
    String uptimeStr = String(payload.uptimeMs);
    mqtt.publish(uptimeTopic.c_str(), uptimeStr.c_str());
    Serial.printf("[MQTT] → %s = %s\n", uptimeTopic.c_str(), uptimeStr.c_str());



}
//Hàm publishDiscovery() — tạo config cho từng sensor
void MQTTGateway::publishDiscovery(const String &macStr, const SFM_SensorValue &v) {
    String base = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + v.name;
    String availabilityTopic = String(MQTT_BASE_TOPIC) + "/" + macStr + "/status";

    // Lấy unit + device_class
    String unit, deviceClass;
    getSensorMeta(v.name, unit, deviceClass);

    // -----------------------------
    // Discovery cho VALUE1
    // -----------------------------
    {
        String uniqueId = macStr + "_" + v.name + "_value1";
        String configTopic = "homeassistant/sensor/" + uniqueId + "/config";
        String stateTopic = base + "/value1";

        String payload = "{";
        payload += "\"name\": \"" + macStr + " " + v.name + " Value1\",";
        payload += "\"state_topic\": \"" + stateTopic + "\",";
        payload += "\"availability_topic\": \"" + availabilityTopic + "\",";
        payload += "\"unique_id\": \"" + uniqueId + "\",";
        payload += "\"state_class\": \"measurement\",";

        if (unit.length() > 0) payload += "\"unit_of_measurement\": \"" + unit + "\",";
        if (deviceClass.length() > 0) payload += "\"device_class\": \"" + deviceClass + "\",";

        payload += "\"device\": {";
        payload += "\"identifiers\": [\"" + macStr + "\"],";
        payload += "\"name\": \"SmartFarm Node " + macStr + "\",";
        payload += "\"manufacturer\": \"SmartFarm\",";
        payload += "\"model\": \"MeshNode V1.6\"";
        payload += "}";
        payload += "}";

        Serial.printf("[MQTT][DISCOVERY] → %s\n", configTopic.c_str());
        Serial.println(payload);

        mqtt.publish(configTopic.c_str(), payload.c_str(), true);
    }

    // -----------------------------
    // Discovery cho VALUE2
    // -----------------------------
    {
        String uniqueId = macStr + "_" + v.name + "_value2";
        String configTopic = "homeassistant/sensor/" + uniqueId + "/config";
        String stateTopic = base + "/value2";

        String payload = "{";
        payload += "\"name\": \"" + macStr + " " + v.name + " Value2\",";
        payload += "\"state_topic\": \"" + stateTopic + "\",";
        payload += "\"availability_topic\": \"" + availabilityTopic + "\",";
        payload += "\"unique_id\": \"" + uniqueId + "\",";
        payload += "\"state_class\": \"measurement\",";

        if (unit.length() > 0) payload += "\"unit_of_measurement\": \"" + unit + "\",";
        if (deviceClass.length() > 0) payload += "\"device_class\": \"" + deviceClass + "\",";

        payload += "\"device\": {";
        payload += "\"identifiers\": [\"" + macStr + "\"],";
        payload += "\"name\": \"SmartFarm Node " + macStr + "\",";
        payload += "\"manufacturer\": \"SmartFarm\",";
        payload += "\"model\": \"MeshNode V1.6\"";
        payload += "}";
        payload += "}";

        Serial.printf("[MQTT][DISCOVERY] → %s\n", configTopic.c_str());
        Serial.println(payload);

        mqtt.publish(configTopic.c_str(), payload.c_str(), true);
    }
}


//Get unit cho sensor
void MQTTGateway::getSensorMeta(const String &name, String &unit, String &deviceClass) {
    if (name.equalsIgnoreCase("DS18B20") || name.indexOf("temp") >= 0) {
        unit = "°C";
        deviceClass = "temperature";
    }
    else if (name.equalsIgnoreCase("PH") || name.indexOf("ph") >= 0) {
        unit = "pH";
        deviceClass = "";
    }
    else if (name.equalsIgnoreCase("EC") || name.indexOf("ec") >= 0) {
        unit = "mS/cm";
        deviceClass = "";
    }
    else if (name.indexOf("humidity") >= 0) {
        unit = "%";
        deviceClass = "humidity";
    }
    else if (name.indexOf("voltage") >= 0 || name.indexOf("volt") >= 0) {
        unit = "V";
        deviceClass = "voltage";
    }
    else {
        unit = "";
        deviceClass = "";
    }
}