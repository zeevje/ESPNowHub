// SPDX-License-Identifier: MIT
// Author: Zeev Elias
// ESPNowHub — Arduino library abstracting ESP-NOW peer-to-peer communication.
// Requires ESP32 core (esp_now.h, WiFi.h) by Espressif Systems.
#pragma once

#include <Arduino.h>
#include <functional>
#include <esp_now.h>
#include <WiFi.h>

static constexpr uint8_t  ESPNOWHUB_MAX_PEERS           = 7;
static constexpr uint8_t  ESPNOWHUB_NAME_LEN            = 15;
static constexpr uint8_t  ESPNOWHUB_SERIAL_BUF_LEN      = 255;  // max payload size
static constexpr uint8_t  ESPNOWHUB_SERIAL_START_MARKER = 0xAA;
static constexpr uint32_t ESPNOWHUB_DEFAULT_HB_TIMEOUT  = 3000; // ms

using ESPNowHubReceiveCb = std::function<void(const char *, const uint8_t *, int)>;
using ESPNowHubSerialCb  = std::function<void(const uint8_t *payload, uint8_t len)>;

struct ESPNowPeer
{
    char     name[ESPNOWHUB_NAME_LEN + 1]; // name up to 15 chars incl. null terminator
    uint8_t  mac[6];                       // like (0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6)
    int8_t   gatePin;                      // HIGH = send allowed, LOW = blocked, -1 = no gate
    bool     connected;
    uint32_t lastHeartbeatMs;              // time of last received packet from this peer
    uint32_t heartbeatTimeoutMs;           // heartbeat timeout duration
};

class ESPNowHub
{
public:
    ESPNowHub();

    // sets up wifi and espnow, call this first.    
    bool begin(); 

    // register a peer with its MAC address and gate pin.
    bool addPeer(const char*  name,
                 const uint8_t* mac,
                 int8_t         gatePin            = -1,
                 uint32_t       heartbeatTimeoutMs = ESPNOWHUB_DEFAULT_HB_TIMEOUT);

    // passes raw bytes and target peer to esp_now_send()
    bool send(const char* name, const void* data, size_t len);

    // checks if peer has sent any packet within hb window.
    bool isConnected(const char* name) const;

    // register the receive callback for incoming ESP-NOW packets.
    void onReceive(ESPNowHubReceiveCb cb);

    // iterates all peers and checks hb status. Call once per loop. 
    void update();

    // parse incoming serial bytes. Frame format: [0xAA][LEN][payload]
    void parseSerial(Stream& stream, ESPNowHubSerialCb cb);

private:
    ESPNowPeer _peers[ESPNOWHUB_MAX_PEERS];
    uint8_t    _peerCount;

    ESPNowHubReceiveCb _receiveCb;

    enum class SerialParseState : uint8_t { IDLE, LENGTH, PAYLOAD };
    SerialParseState _serialState;
    uint8_t          _serialPayloadLen;
    uint8_t          _serialBuf[ESPNOWHUB_SERIAL_BUF_LEN];
    uint8_t          _serialBufIdx;

    // Internal helpers 
    int8_t _findPeerByMac (const uint8_t* mac) const;
    int8_t _findPeerByName(const char*    name) const;
    void   _handleReceive (const uint8_t* mac, const uint8_t* data, int len);

    static ESPNowHub* _instance;
    static void       _espNowRecvCb(const uint8_t* mac, const uint8_t* data, int len);
};