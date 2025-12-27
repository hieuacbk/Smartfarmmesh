#include "MeshNode.h"
#include "MQTTGateway.h"
#include "MeshTypes.h"

MeshNode* MeshNode::instance = nullptr;
extern MQTTGateway mqttGateway;   // mqttGateway nằm trong .ino

MeshNode::MeshNode() {
    instance = this;
}

bool MeshNode::begin() {
    Serial.begin(SFM_DEBUG_SERIAL_BAUD);
    delay(200);

    SFM_LOG("INFO", "MeshNode begin()");

    // Set WiFi to STA + correct channel
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(SFM_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    delay(100);

    // Print MAC
    uint8_t mac[6];
    WiFi.macAddress(mac);
    SFM_LOGF("INFO", "STA MAC = %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    WiFi.disconnect();

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        SFM_LOG("ERROR", "Error initializing ESP-NOW");
        return false;
    }

    esp_now_register_send_cb(MeshNode::onDataSentStatic);
    esp_now_register_recv_cb(MeshNode::onDataRecvStatic);

    // NODE: add gateway peer
    if (DEVICE_ROLE == ROLE_NODE) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, GATEWAY_MAC, 6);
        peerInfo.channel = SFM_WIFI_CHANNEL;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            SFM_LOG("ERROR", "Failed to add gateway peer");
            return false;
        }
        SFM_LOG("INFO", "Added gateway peer");
    }

    nodeCount = 0;

    SFM_LOGF("INFO", "Device role = %s",
             DEVICE_ROLE == ROLE_GATEWAY ? "GATEWAY" : "NODE");

    return true;
}

void MeshNode::loop() {
    uint32_t now = millis();

    // NODE: send heartbeat
    if (DEVICE_ROLE == ROLE_NODE) {
        if (now - lastHeartbeatMs >= SFM_HEARTBEAT_INTERVAL) {
            lastHeartbeatMs = now;
            sendHeartbeat();
        }
    }
}

void MeshNode::sendHeartbeat() {
    SFM_HeartbeatPayload payload;
    payload.uptimeMs = millis();
    payload.rssi = 0;
    payload.nodeId = 0;

    SFM_PacketHeader header;
    header.msgType = SFM_MSG_HEARTBEAT;
    header.reserved = 0;
    header.payloadLen = sizeof(SFM_HeartbeatPayload);

    uint8_t buffer[sizeof(header) + sizeof(payload)];
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), &payload, sizeof(payload));

    sendTo(GATEWAY_MAC, buffer, sizeof(buffer));
}

void MeshNode::sendSensorFrame(SFM_SensorValue* values, uint8_t count) {
    SFM_SensorFramePayload payload;
    payload.uptimeMs = millis();
    payload.nodeId = 0;
    payload.count = count;

    for (int i = 0; i < count; i++) {
        payload.values[i] = values[i];
    }

    SFM_PacketHeader header;
    header.msgType = SFM_MSG_SENSOR_FRAME;
    header.reserved = 0;
    header.payloadLen = sizeof(SFM_SensorFramePayload);

    uint8_t buffer[sizeof(header) + sizeof(payload)];
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), &payload, sizeof(payload));

    sendTo(GATEWAY_MAC, buffer, sizeof(buffer));
}

// ==== static callbacks ====

void MeshNode::onDataSentStatic(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (instance) instance->handleDataSent(mac_addr, status);
}

void MeshNode::onDataRecvStatic(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (instance) instance->handleDataRecv(mac, incomingData, len);
}

// ==== instance handlers ====

void MeshNode::handleDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    SFM_LOGF("DEBUG", "Send cb, status=%d", status);
}

void MeshNode::handleDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    SFM_LOGF("DEBUG", "Data recv len=%d", len);
    processPacket(mac, incomingData, len);
}

void MeshNode::processPacket(const uint8_t *mac, const uint8_t *data, int len) {
    if (len < (int)sizeof(SFM_PacketHeader)) {
        SFM_LOG("WARN", "Packet too short");
        return;
    }

    SFM_PacketHeader header;
    memcpy(&header, data, sizeof(header));

    const uint8_t* payloadPtr = data + sizeof(header);
    int payloadLen = len - sizeof(header);

    if (payloadLen != header.payloadLen) {
        SFM_LOG("WARN", "Payload length mismatch");
        return;
    }

    switch (header.msgType) {
        case SFM_MSG_HEARTBEAT: {
            if (payloadLen != sizeof(SFM_HeartbeatPayload)) return;
            SFM_HeartbeatPayload hb;
            memcpy(&hb, payloadPtr, sizeof(hb));
            handleHeartbeat(mac, hb);
            break;
        }

        case SFM_MSG_SENSOR_FRAME: {
            if (payloadLen != sizeof(SFM_SensorFramePayload)) return;
            SFM_SensorFramePayload sf;
            memcpy(&sf, payloadPtr, sizeof(sf));
            handleSensorFrame(mac, sf);
            break;
        }

        case SFM_MSG_HEARTBEAT_ACK:
            SFM_LOG("INFO", "Received HEARTBEAT_ACK");
            break;

        default:
            SFM_LOGF("WARN", "Unknown msgType=%d", header.msgType);
            break;
    }
}

void MeshNode::handleHeartbeat(const uint8_t *mac, const SFM_HeartbeatPayload &payload) {
    SFM_LOG("INFO", "Received HEARTBEAT");

    if (DEVICE_ROLE == ROLE_GATEWAY) {
        int idx = addOrUpdateNode(mac, payload.nodeId, payload.rssi);
        if (idx >= 0) logNodeBrief(idx);

        if (!ensurePeer(mac)) return;

        SFM_PacketHeader header;
        header.msgType = SFM_MSG_HEARTBEAT_ACK;
        header.reserved = 0;
        header.payloadLen = 0;

        sendTo(mac, (uint8_t*)&header, sizeof(header));
    }
}

/* ============ OLD HANDLE SENSOR (chỉ log) ============
void MeshNode::handleSensorFrame(const uint8_t *mac, const SFM_SensorFramePayload &payload) {
    int idx = addOrUpdateNode(mac, payload.nodeId, 0);

    for (int i = 0; i < payload.count; i++) {
        SFM_LOGF("INFO",
                 "SensorData nodeIdx=%d %02X:%02X:%02X:%02X:%02X:%02X %s v1=%.2f v2=%.2f",
                 idx,
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                 payload.values[i].name,
                 payload.values[i].value1,
                 payload.values[i].value2);
    }
}
===================================================== */

void MeshNode::handleSensorFrame(const uint8_t *mac, const SFM_SensorFramePayload &payload) {
    int idx = addOrUpdateNode(mac, payload.nodeId, 0);

    // Giữ nguyên log cũ
    for (int i = 0; i < payload.count; i++) {
        SFM_LOGF("INFO",
                 "SensorData nodeIdx=%d %02X:%02X:%02X:%02X:%02X:%02X %s v1=%.2f v2=%.2f",
                 idx,
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                 payload.values[i].name,
                 payload.values[i].value1,
                 payload.values[i].value2);
    }

    // 🔥 NEW: đẩy sang MQTT
    onSensorFrameReceived(mac, payload);
}

// ==== Peer + Registry ====

bool MeshNode::ensurePeer(const uint8_t *mac) {
    if (esp_now_is_peer_exist(mac)) return true;

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = SFM_WIFI_CHANNEL;
    peerInfo.encrypt = false;

    esp_err_t err = esp_now_add_peer(&peerInfo);
    if (err == ESP_OK) {
        SFM_LOGF("INFO", "Added new peer %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return true;
    }

    SFM_LOGF("ERROR", "esp_now_add_peer failed, err=%d", err);
    return false;
}

bool MeshNode::sendTo(const uint8_t *mac, const uint8_t *data, size_t len) {
    if (DEVICE_ROLE == ROLE_GATEWAY) {
        if (!ensurePeer(mac)) return false;
    }

    esp_err_t result = esp_now_send(mac, data, len);
    if (result == ESP_OK) {
        SFM_LOG("DEBUG", "esp_now_send OK");
        return true;
    }

    SFM_LOGF("ERROR", "esp_now_send failed, err=%d", result);
    return false;
}

int MeshNode::findNodeIndexByMac(const uint8_t *mac) {
    for (int i = 0; i < nodeCount; i++) {
        if (memcmp(nodes[i].mac, mac, 6) == 0) return i;
    }
    return -1;
}

int MeshNode::addOrUpdateNode(const uint8_t *mac, uint16_t nodeId, int8_t rssi) {
    int idx = findNodeIndexByMac(mac);

    if (idx < 0) {
        if (nodeCount >= SFM_MAX_NODES) return -1;
        idx = nodeCount++;
        memcpy(nodes[idx].mac, mac, 6);
    }

    nodes[idx].lastSeenMs = millis();
    nodes[idx].lastRssi = rssi;
    nodes[idx].nodeId = nodeId;

    return idx;
}

void MeshNode::logNodeBrief(int idx) {
    if (idx < 0 || idx >= nodeCount) return;

    SFM_NodeInfo &n = nodes[idx];
    SFM_LOGF("INFO",
             "Node[%d] MAC=%02X:%02X:%02X:%02X:%02X:%02X nodeId=%u lastSeen=%lu rssi=%d",
             idx,
             n.mac[0], n.mac[1], n.mac[2],
             n.mac[3], n.mac[4], n.mac[5],
             n.nodeId,
             (unsigned long)n.lastSeenMs,
             n.lastRssi);
}

/* ============ OLD THỬ NGHIỆM DÙNG SFM_Frame + parseFrame ============
extern MQTTGateway mqttGateway; // nếu mqttGateway nằm trong .ino

void MeshNode::onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
    Serial.printf("[SFM] DEBUG: Data recv len=%d\n", len);

    SFM_Frame parsedFrame;
    if (!parseFrame(data, len, &parsedFrame)) {
        Serial.println("[SFM] ERROR: Failed to parse frame");
        return;
    }

    if (parsedFrame.msgType == SFM_MSG_HEARTBEAT) {
        Serial.println("[SFM] INFO: Received HEARTBEAT");
        return;
    }

    if (parsedFrame.msgType == SFM_MSG_SENSOR_FRAME) {
        Serial.println("[SFM] INFO: Received SENSOR_FRAME");
        onSensorFrameReceived(mac, parsedFrame.payload.sensorFrame);
        return;
    }

    Serial.println("[SFM] WARN: Unknown frame type");
}
================================================================= */

void MeshNode::onSensorFrameReceived(const uint8_t *mac, const SFM_SensorFramePayload &payload) {
    Serial.println("[MeshNode] SensorFrame received");
    mqttGateway.publishSensorFrame(mac, payload);
}