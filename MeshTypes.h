#ifndef MESH_TYPES_H
#define MESH_TYPES_H

#include <Arduino.h>

//
// Message Types
//
enum SFM_MessageType : uint8_t {
    SFM_MSG_HEARTBEAT      = 1,
    SFM_MSG_HEARTBEAT_ACK  = 2,
    SFM_MSG_SENSOR_FRAME   = 3
};

//
// Packet Header
//
struct SFM_PacketHeader {
    uint8_t  msgType;
    uint8_t  reserved;
    uint16_t payloadLen;
};

//
// Heartbeat Payload
//
struct SFM_HeartbeatPayload {
    uint32_t uptimeMs;
    int8_t   rssi;
    uint16_t nodeId;
};

//
// Sensor Value
//
struct SFM_SensorValue {
    char  name[12];
    float value1;
    float value2;
};

//
// Sensor Frame Payload
//
struct SFM_SensorFramePayload {
    uint32_t uptimeMs;
    uint16_t nodeId;
    uint8_t  count;
    SFM_SensorValue values[8];
};

//
// Node Info
//
struct SFM_NodeInfo {
    uint8_t  mac[6];
    uint32_t lastSeenMs;
    int8_t   lastRssi;
    uint16_t nodeId;
};

//
// Unified Frame Structure
//
struct SFM_Frame {
    uint8_t msgType;

    union {
        SFM_HeartbeatPayload      heartbeat;
        SFM_SensorFramePayload    sensorFrame;
    } payload;
};

//
// Frame Builder
//
inline int buildFrame(uint8_t *buffer, uint8_t msgType, const void *payload, uint16_t payloadLen) {
    SFM_PacketHeader header;
    header.msgType   = msgType;
    header.reserved  = 0;
    header.payloadLen = payloadLen;

    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), payload, payloadLen);

    return sizeof(header) + payloadLen;
}

//
// Frame Parser
//
inline bool parseFrame(const uint8_t *data, int len, SFM_Frame *outFrame) {
    if (len < sizeof(SFM_PacketHeader)) return false;

    SFM_PacketHeader header;
    memcpy(&header, data, sizeof(header));

    if (len < sizeof(header) + header.payloadLen) return false;

    outFrame->msgType = header.msgType;

    const uint8_t *payloadPtr = data + sizeof(header);

    switch (header.msgType) {
        case SFM_MSG_HEARTBEAT:
            memcpy(&outFrame->payload.heartbeat, payloadPtr, sizeof(SFM_HeartbeatPayload));
            return true;

        case SFM_MSG_SENSOR_FRAME:
            memcpy(&outFrame->payload.sensorFrame, payloadPtr, sizeof(SFM_SensorFramePayload));
            return true;

        default:
            return false;
    }
}

#endif