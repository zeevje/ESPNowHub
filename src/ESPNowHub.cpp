// SPDX-License-Identifier: MIT
// Author: Zeev Elias
// ESPNowHub - Arduino library abstracting ESP-NOW peer-to-peer communication.
// Requires ESP32 core (esp_now.h, WiFi.h) by Espressif Systems.
 
#include "ESPNowHub.h"

ESPNowHub* ESPNowHub::_instance = nullptr;

ESPNowHub::ESPNowHub()
{
    _peerCount       = 0;
    _serialState     = SerialParseState::IDLE;
    _serialPayloadLen = 0;
    _serialBufIdx    = 0;
    memset(_serialBuf, 0, sizeof(_serialBuf));
    _instance        = nullptr;
}

bool ESPNowHub::begin()
{
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return false;
    _instance = this;
    esp_now_register_recv_cb(_espNowRecvCb);
    return true;
}
 

bool ESPNowHub::addPeer(const char*    name,
                        const uint8_t* mac,
                        int8_t         gatePin,
                        uint32_t       heartbeatTimeoutMs)
{
    if (_peerCount >= ESPNOWHUB_MAX_PEERS) return false;
 
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;
 
    ESPNowPeer& p = _peers[_peerCount];
    strncpy(p.name, name, ESPNOWHUB_NAME_LEN);
    memcpy(p.mac, mac, 6);
    p.gatePin            = gatePin;
    p.heartbeatTimeoutMs = heartbeatTimeoutMs;
    p.connected          = false;
    p.lastHeartbeatMs    = 0;
    _peerCount++;
    return true;
}

bool ESPNowHub::send(const char *name, const void *data, size_t len)
{
    int8_t idx = _findPeerByName(name);
    if (idx == -1) return false;
    if (_peers[idx].gatePin != -1 && digitalRead(_peers[idx].gatePin) == LOW) return false;
    return esp_now_send(_peers[idx].mac, (const uint8_t*)data, len) == ESP_OK;
}

bool ESPNowHub::isConnected(const char *name) const
{
    int8_t idx = _findPeerByName(name);
    if (idx == -1) return false;
    return _peers[idx].connected;
}

void ESPNowHub::onReceive(ESPNowHubReceiveCb cb)
{
    _receiveCb = cb;
}
 
void ESPNowHub::update()
{
    uint32_t now = millis();
    for (int8_t i = 0; i < _peerCount; i++)
    {
        if (now - _peers[i].lastHeartbeatMs >= _peers[i].heartbeatTimeoutMs)
            _peers[i].connected = false;
    }
}

void ESPNowHub::parseSerial(Stream &stream, ESPNowHubSerialCb cb)
{
    while (stream.available())
    {
        uint8_t byte = stream.read();
        switch (_serialState)
        {
        case SerialParseState::IDLE:
            if (byte == ESPNOWHUB_SERIAL_START_MARKER)
                _serialState = SerialParseState::LENGTH;
            break;
            
        case SerialParseState::LENGTH:
            _serialPayloadLen = byte;
            _serialBufIdx     = 0;
            if (_serialPayloadLen > 0)
                _serialState = SerialParseState::PAYLOAD;
            else
                 _serialState = SerialParseState::IDLE;
            break;

        case SerialParseState::PAYLOAD:
            if (_serialBufIdx < _serialPayloadLen)
            {
                _serialBuf[_serialBufIdx] = byte;
                _serialBufIdx++;
            }
            if (_serialBufIdx >= _serialPayloadLen)
            {
            if (cb) cb(_serialBuf, _serialPayloadLen);
                _serialBufIdx = 0;
                _serialState = SerialParseState::IDLE;
            }
            break;
        }
    }
}

int8_t ESPNowHub::_findPeerByName(const char* name) const 
{ 
    for (int8_t i = 0; i < _peerCount; i++) 
        if (strcmp(_peers[i].name , name) == 0) return i;
    return -1;
}

int8_t ESPNowHub::_findPeerByMac(const uint8_t* mac) const 
{ 
    for (int8_t i = 0; i < _peerCount; i++) 
        if (memcmp(_peers[i].mac , mac, 6) == 0) return i;
    return -1;
}

void ESPNowHub::_espNowRecvCb(const uint8_t* mac, const uint8_t* data, int len)
{
    if (_instance == nullptr) return;
    _instance->_handleReceive(mac,data,len);
}

void ESPNowHub::_handleReceive (const uint8_t* mac, const uint8_t* data, int len)
{
    int8_t idx = _findPeerByMac(mac);
    if (idx == -1) return;
    _peers[idx].lastHeartbeatMs = millis();
    _peers[idx].connected = true;
    if (_receiveCb) _receiveCb(_peers[idx].name, data, len);
}
