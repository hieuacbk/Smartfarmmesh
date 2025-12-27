#ifndef MESH_NODE_H
#define MESH_NODE_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "SmartFarmConfig.h"
#include "MeshTypes.h"

class MeshNode {
public:
  MeshNode();

  bool begin();
  void loop();

  // ==== MQTT hook mới: xử lý SensorFrame đã parse xong ====
  void onSensorFrameReceived(const uint8_t *mac, const SFM_SensorFramePayload &payload);

  // ==== Gửi từ NODE lên GATEWAY ====
  void sendHeartbeat();
  void sendSensorFrame(SFM_SensorValue* values, uint8_t count);

  // ==== ESP-NOW callbacks ====
  static void onDataSentStatic(const uint8_t *mac_addr, esp_now_send_status_t status);
  static void onDataRecvStatic(const uint8_t * mac, const uint8_t *incomingData, int len);

  // ==== (OLD – bạn từng thêm, hiện không cần dùng) ====
  // void onDataReceived(const uint8_t *mac, const uint8_t *data, int len);

private:
  static MeshNode* instance;

  uint32_t lastHeartbeatMs = 0;

  SFM_NodeInfo nodes[SFM_MAX_NODES];
  int nodeCount = 0;

  void handleDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
  void handleDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

  void processPacket(const uint8_t *mac, const uint8_t *data, int len);
  void handleHeartbeat(const uint8_t *mac, const SFM_HeartbeatPayload &payload);
  void handleSensorFrame(const uint8_t *mac, const SFM_SensorFramePayload &payload);

  bool sendTo(const uint8_t *mac, const uint8_t *data, size_t len);
  bool ensurePeer(const uint8_t *mac);

  int findNodeIndexByMac(const uint8_t *mac);
  int addOrUpdateNode(const uint8_t *mac, uint16_t nodeId, int8_t rssi);
  void logNodeBrief(int idx);
};

#endif