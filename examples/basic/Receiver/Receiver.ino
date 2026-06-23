// ESPNowHub — Receiver Example
// 1. Flash this sketch to your first ESP32.
// 2. Open Serial Monitor and copy the MAC address printed here.
// 3. Paste that MAC into Sender.ino, then flash the sender.

#include "ESPNowHub.h"
#include <WiFi.h>

ESPNowHub hub;

// replace with the MAC address printed by Sender.ino
uint8_t senderMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// match this struct to the one in Sender.ino
struct SensorData {
    int   id;
    float value;
    bool  active;
};

void setup() {
    Serial.begin(115200);
    delay(3000);

    if (!hub.begin()) {
        Serial.println("ESP-NOW init failed");
        while (true);
    }

    Serial.print("Receiver MAC: ");
    Serial.println(WiFi.macAddress());

    hub.addPeer("sender", senderMac);

    hub.onReceive([](const char* peer, const uint8_t* data, int len) {
        if (len != sizeof(SensorData)) return;
        SensorData payload;
        memcpy(&payload, data, sizeof(payload));
        Serial.printf("from %s — id: %d, value: %.2f, active: %s\n",
            peer, payload.id, payload.value, payload.active ? "true" : "false");
    });
}

void loop() {
    hub.update();
}