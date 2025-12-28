#ifndef SMART_FARM_CONFIG_H
#define SMART_FARM_CONFIG_H

#include <Arduino.h>

#define ROLE_GATEWAY   1
#define ROLE_NODE      2

//#define DEVICE_ROLE ROLE_GATEWAY
#define DEVICE_ROLE ROLE_GATEWAY


#define SFM_WIFI_CHANNEL       6
#define SFM_MAX_NODES          32
#define SFM_HEARTBEAT_INTERVAL 5000   // ✅ thêm dòng này
#define SFM_LOG_PREFIX        "[SFM] "

#define SFM_DEBUG_SERIAL_BAUD 115200
#define SFM_ENABLE_LOG        1

#if SFM_ENABLE_LOG
  #define SFM_LOG(level, msg)  \
    do { Serial.print(SFM_LOG_PREFIX); Serial.print(level); Serial.print(": "); Serial.println(msg); } while(0)
  #define SFM_LOGF(level, fmt, ...) \
    do { Serial.print(SFM_LOG_PREFIX); Serial.print(level); Serial.print(": "); Serial.printf(fmt, __VA_ARGS__); Serial.println(); } while(0)
#else
  #define SFM_LOG(level, msg)
  #define SFM_LOGF(level, fmt, ...)
#endif

// ✅ MAC Gateway bạn cung cấp
static uint8_t GATEWAY_MAC[6] = {0xFC, 0x01, 0x2C, 0x2D, 0x7B, 0xA8};

#endif