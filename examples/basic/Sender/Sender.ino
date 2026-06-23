// ESPNowHub — Sender Example
// Sends a data struct to a named peer every second.
// 1. Flash Receiver.ino to your first ESP32 and note its MAC address.
// 2. Paste that MAC into receiverMac below.
// 3. Flash this sketch to your second ESP32.

#include "ESPNowHub.h"
#include <WiFi.h>

ESPNowHub hub;

// replace with the MAC address printed by Receiver.ino
uint8_t receiverMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// example payload — replace with your own struct
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

    Serial.print("Sender MAC: ");
    Serial.println(WiFi.macAddress());

    hub.addPeer("receiver", receiverMac);

    hub.onReceive([](const char* peer, const uint8_t* data, int len) {
        Serial.printf("got %d bytes from %s\n", len, peer);
    });
}

void loop() {
    hub.update();
    SensorData payload = {1, 3.14f, true};
    bool ok = hub.send("receiver", &payload, sizeof(payload));
    Serial.println(ok ? "sent" : "send failed");
    delay(1000);
}