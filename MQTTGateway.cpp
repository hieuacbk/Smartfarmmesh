#include "MQTTGateway.h"

/* ============================================================
   CONSTRUCTOR
   ============================================================ */
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

    // ==== NEW V1.8: xử lý queue MQTT ====
    processQueue();
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
   NEW V1.8 – QUEUE HELPERS
   ============================================================ */
void MQTTGateway::enqueue(const String &topic, const String &payload, bool retain) {
    // Giới hạn queue để tránh tràn RAM
    if (mqttQueue.size() > 100) {
        Serial.println("[MQTT][QUEUE] Overflow → dropping");
        return;
    }

    // Không cho phép topic/payload rỗng
    if (topic.isEmpty() || payload.isEmpty()) {
        Serial.println("[MQTT][QUEUE] Empty topic/payload → skip");
        return;
    }

    PendingMsg msg;
    msg.topic = topic;
    msg.payload = payload;
    msg.retain = retain;

    mqttQueue.push_back(msg);
}


void MQTTGateway::processQueue() {
    if (!mqtt.connected()) return;
    if (mqttQueue.empty()) return;

    PendingMsg msg = mqttQueue.front();
    mqttQueue.erase(mqttQueue.begin());

    // Bảo vệ dữ liệu
    if (msg.topic.isEmpty() || msg.payload.isEmpty()) {
        Serial.println("[MQTT][QUEUE] Skip empty topic/payload");
        return;
    }

    // Payload quá lớn → PubSubClient crash
    if (msg.payload.length() > 512) {
        Serial.println("[MQTT][QUEUE] Payload too large → skip");
        return;
    }

    const char* topicCStr = msg.topic.c_str();
    const char* payloadCStr = msg.payload.c_str();

    if (topicCStr == nullptr || payloadCStr == nullptr) {
        Serial.println("[MQTT][QUEUE] Null pointer detected → skip");
        return;
    }

    mqtt.publish(topicCStr, payloadCStr, msg.retain);
}


/* ============================================================
   publishSensorFrame() – GIỜ CHỈ ENQUEUE MQTT
   ============================================================ */
// OLD VERSION (publish trực tiếp)
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
        const SFM_SensorValue &v = payload.values[i];

        publishDiscovery(macStr, v);

        String topic1 = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + v.name + "/value1";
        String valueStr1 = String(v.value1, 2);
        mqtt.publish(topic1.c_str(), valueStr1.c_str());

        if (v.value2 != 0.0f) {
            String topic2 = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + v.name + "/value2";
            String valueStr2 = String(v.value2, 2);
            mqtt.publish(topic2.c_str(), valueStr2.c_str());
        }
    }

    String statusTopic = String(MQTT_BASE_TOPIC) + "/" + macStr + "/status";
    mqtt.publish(statusTopic.c_str(), "online");

    String uptimeTopic = String(MQTT_BASE_TOPIC) + "/" + macStr + "/uptimeMs";
    String uptimeStr = String(payload.uptimeMs);
    mqtt.publish(uptimeTopic.c_str(), uptimeStr.c_str());
}
*/

void MQTTGateway::publishSensorFrame(const uint8_t *mac, const SFM_SensorFramePayload &payload) {
    Serial.println("[MQTT] publishSensorFrame()");

    if (!mqtt.connected()) {
        Serial.println("[MQTT] Not connected → enqueue vẫn được, nhưng MQTT chưa online");
        // Vẫn cho enqueue để khi reconnect thì gửi dần
    }

    String macStr = macToString(mac);
    Serial.printf("[MQTT] MAC=%s, count=%d\n", macStr.c_str(), payload.count);

    for (int i = 0; i < payload.count; i++) {
        const SFM_SensorValue &v = payload.values[i];

        // MQTT Discovery (config) – dùng retain, nên enqueue với retain=true
        publishDiscovery(macStr, v);

        // VALUE1
        String topic1 = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + v.name + "/value1";
        String valueStr1 = String(v.value1, 2);
        // mqtt.publish(topic1.c_str(), valueStr1.c_str());
        enqueue(topic1, valueStr1, false);
        Serial.printf("[MQTT][ENQUEUE] %s = %s\n", topic1.c_str(), valueStr1.c_str());

        // VALUE2 (nếu khác 0.0)
        if (v.value2 != 0.0f) {
            String topic2 = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + v.name + "/value2";
            String valueStr2 = String(v.value2, 2);
            // mqtt.publish(topic2.c_str(), valueStr2.c_str());
            enqueue(topic2, valueStr2, false);
            Serial.printf("[MQTT][ENQUEUE] %s = %s\n", topic2.c_str(), valueStr2.c_str());
        }
    }

    // STATUS = online
    {
        String statusTopic = String(MQTT_BASE_TOPIC) + "/" + macStr + "/status";
        // mqtt.publish(statusTopic.c_str(), "online");
        enqueue(statusTopic, "online", false);
        Serial.printf("[MQTT][ENQUEUE] %s = online\n", statusTopic.c_str());
    }

    // UPTIME
    {
        String uptimeTopic = String(MQTT_BASE_TOPIC) + "/" + macStr + "/uptimeMs";
        String uptimeStr = String(payload.uptimeMs);
        // mqtt.publish(uptimeTopic.c_str(), uptimeStr.c_str());
        enqueue(uptimeTopic, uptimeStr, false);
        Serial.printf("[MQTT][ENQUEUE] %s = %s\n", uptimeTopic.c_str(), uptimeStr.c_str());
    }
}

/* ============================================================
   SENSOR DISCOVERY – dùng enqueue + retain
   ============================================================ */
void MQTTGateway::publishDiscovery(const String &macStr, const SFM_SensorValue &v) {
    String base = String(MQTT_BASE_TOPIC) + "/" + macStr + "/" + v.name;
    String availabilityTopic = String(MQTT_BASE_TOPIC) + "/" + macStr + "/status";

    String unit, deviceClass;
    getSensorMeta(v.name, unit, deviceClass);

    // VALUE1
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

        // mqtt.publish(configTopic.c_str(), payload.c_str(), true);
        enqueue(configTopic, payload, true);
        Serial.printf("[MQTT][DISCOVERY ENQUEUE] %s\n", configTopic.c_str());
    }

    // VALUE2
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

        // mqtt.publish(configTopic.c_str(), payload.c_str(), true);
        enqueue(configTopic, payload, true);
        Serial.printf("[MQTT][DISCOVERY ENQUEUE] %s\n", configTopic.c_str());
    }
}

/* ============================================================
   NODE AUTO-DISCOVERY – dùng enqueue + retain
   ============================================================ */
void MQTTGateway::publishNodeDiscovery(const String &macStr) {
    String base = String(MQTT_BASE_TOPIC) + "/" + macStr;

    // ONLINE
    {
        String uid = macStr + "_online";
        String topic = "homeassistant/binary_sensor/" + uid + "/config";

        String payload = "{";
        payload += "\"name\": \"Node " + macStr + " Online\",";
        payload += "\"state_topic\": \"" + base + "/status\",";
        payload += "\"payload_on\": \"online\",";
        payload += "\"payload_off\": \"offline\",";
        payload += "\"unique_id\": \"" + uid + "\",";
        payload += "\"device\": {";
        payload += "\"identifiers\": [\"" + macStr + "\"],";
        payload += "\"name\": \"SmartFarm Node " + macStr + "\",";
        payload += "\"manufacturer\": \"SmartFarm\",";
        payload += "\"model\": \"MeshNode V1.8\"";
        payload += "}";
        payload += "}";

        // mqtt.publish(topic.c_str(), payload.c_str(), true);
        enqueue(topic, payload, true);
    }

    // RSSI
    {
        String uid = macStr + "_rssi";
        String topic = "homeassistant/sensor/" + uid + "/config";

        String payload = "{";
        payload += "\"name\": \"Node " + macStr + " RSSI\",";
        payload += "\"state_topic\": \"" + base + "/rssi\",";
        payload += "\"unit_of_measurement\": \"dBm\",";
        payload += "\"device_class\": \"signal_strength\",";
        payload += "\"unique_id\": \"" + uid + "\",";
        payload += "\"device\": {\"identifiers\": [\"" + macStr + "\"]}";
        payload += "}";

        // mqtt.publish(topic.c_str(), payload.c_str(), true);
        enqueue(topic, payload, true);
    }

    // UPTIME
    {
        String uid = macStr + "_uptime";
        String topic = "homeassistant/sensor/" + uid + "/config";

        String payload = "{";
        payload += "\"name\": \"Node " + macStr + " Uptime\",";
        payload += "\"state_topic\": \"" + base + "/uptimeMs\",";
        payload += "\"unit_of_measurement\": \"ms\",";
        payload += "\"unique_id\": \"" + uid + "\",";
        payload += "\"device\": {\"identifiers\": [\"" + macStr + "\"]}";
        payload += "}";

        // mqtt.publish(topic.c_str(), payload.c_str(), true);
        enqueue(topic, payload, true);
    }

    // LAST SEEN
    {
        String uid = macStr + "_last_seen";
        String topic = "homeassistant/sensor/" + uid + "/config";

        String payload = "{";
        payload += "\"name\": \"Node " + macStr + " Last Seen\",";
        payload += "\"state_topic\": \"" + base + "/last_seen\",";
        payload += "\"device_class\": \"timestamp\",";
        payload += "\"unique_id\": \"" + uid + "\",";
        payload += "\"device\": {\"identifiers\": [\"" + macStr + "\"]}";
        payload += "}";

        // mqtt.publish(topic.c_str(), payload.c_str(), true);
        enqueue(topic, payload, true);
    }

    // FIRMWARE
    {
        String uid = macStr + "_fw";
        String topic = "homeassistant/sensor/" + uid + "/config";

        String payload = "{";
        payload += "\"name\": \"Node " + macStr + " Firmware\",";
        payload += "\"state_topic\": \"" + base + "/fw_version\",";
        payload += "\"unique_id\": \"" + uid + "\",";
        payload += "\"device\": {\"identifiers\": [\"" + macStr + "\"]}";
        payload += "}";

        // mqtt.publish(topic.c_str(), payload.c_str(), true);
        enqueue(topic, payload, true);
    }
}

/* ============================================================
   NODE STATUS – dùng enqueue
   - last_seen gửi đúng định dạng ISO 8601 (Home Assistant yêu cầu cho device_class: timestamp)

   ============================================================ */
void MQTTGateway::publishNodeStatus(const String &macStr, const SFM_HeartbeatPayload &hb) {
    String base = String(MQTT_BASE_TOPIC) + "/" + macStr;

    enqueue(base + "/status", "online", false);
    enqueue(base + "/rssi", String(hb.rssi), false);
    enqueue(base + "/uptimeMs", String(hb.uptimeMs), false);

    // LAST SEEN — ISO 8601 timestamp
    time_t now = time(nullptr);
    struct tm *timeinfo = gmtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
    enqueue(base + "/last_seen", String(buf), false);

    enqueue(base + "/fw_version", "V1.8", false);
}



/* ============================================================
   SENSOR META
   ============================================================ */
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