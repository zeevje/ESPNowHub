# ESPNowHub

ESPNowHub is an Arduino library for ESP32 that lets you communicate with ESP-NOW peers by name, monitor connection status, and exchange data through a simple callback-based API.

Register peers once and communicate with them by name instead of managing MAC addresses throughout your application.

## Why ESPNowHub?

ESP-NOW is fast and lightweight, but managing peers, tracking connection state, handling heartbeats, and coordinating communication with a host application adds boilerplate to every project.

ESPNowHub lets you:

- Register peers by name instead of MAC address
- Track peer connectivity based on packet activity
- Gate communication using hardware pins
- Receive packets through a simple callback API
- Exchange structured commands with a PC over a framed serial protocol

## Architecture

```
MATLAB / Python
        │
     Serial
        │
   ESPNowHub
    /     \
sensor   servo
ESP32    ESP32
```

## Installation

1. Download or clone this repo
2. Copy the `ESPNowHub` folder into your Arduino `libraries/` directory
3. Restart Arduino IDE

## Quick Start

```cpp
#include "ESPNowHub.h"

ESPNowHub hub;

uint8_t peerMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

struct SensorData {
    float temperature;
};

void setup() {
    if (!hub.begin()) {
        Serial.println("Failed to initialize ESP-NOW");
        while (true);
    }
    hub.addPeer("sensor", peerMac);
    hub.onReceive([](const char* peer, const uint8_t* data, int len) {
        // handle incoming data
    });
}

void loop() {
    hub.update();

    SensorData data;
    data.temperature = 25.4f;
    hub.send("sensor", &data, sizeof(data));

    delay(1000);
}
```

## API

### `bool begin()`
Initializes WiFi in station mode and starts ESP-NOW. Call once in `setup()`. Returns `false` on failure.

### `bool addPeer(const char* name, const uint8_t* mac, int8_t gatePin = -1, uint32_t heartbeatTimeoutMs = 3000)`
Registers a named peer. `gatePin = -1` disables gate checking. Returns `false` if at max capacity (7 peers) or if registration fails.

### `bool send(const char* name, const void* data, size_t len)`
Sends raw bytes to a named peer. Returns `false` if the peer is not found, gate pin is LOW, or the send fails. Point-to-point only — broadcast addresses are not supported.

### `bool isConnected(const char* name) const`
Returns `true` if the peer has sent at least one packet within its configured timeout window.

### `void onReceive(ESPNowHubReceiveCb cb)`
Registers a callback fired on every incoming ESP-NOW packet. Keep callback code short and avoid blocking operations.

### `void update()`
Checks heartbeat timeouts and marks peers disconnected if expired. Call once per `loop()`.

### `void parseSerial(Stream& stream, ESPNowHubSerialCb cb)`
Non-blocking serial frame parser. Frame format: `[0xAA][LEN][payload]`. Call once per `loop()`.

## Gate Pins

Each peer can optionally be assigned a gate pin:

- **HIGH** → packets can be transmitted
- **LOW** → transmission is blocked

This is useful when communication should only occur while an external device is enabled, armed, or ready.

```cpp
hub.addPeer("servo", servoMac, GATE_PIN);
```

## Connection Monitoring

A peer is considered connected if it has sent at least one packet within its configured timeout window. Any received packet refreshes the peer's heartbeat timer — no dedicated heartbeat packets required.

```cpp
if (hub.isConnected("sensor")) {
    // peer is online
}
```

## Serial Framing Protocol

| Byte | Description |
|------|-------------|
| `0xAA` | Start marker |
| `LEN` | Payload length (1 byte, max 255) |
| `...` | Payload bytes |

Bytes before a valid start marker are silently discarded.

## Examples

- `Sender` — periodically transmits a data struct to a named peer once per second
- `Receiver` — receives packets, decodes the struct, and prints values to Serial

See `examples/Sender` and `examples/Receiver` for the full two-board demo.

## Limitations

- Maximum of 7 registered peers (ESP-NOW hardware limit)
- Requires ESP32 Arduino core by Espressif
- Payload size limited by ESP-NOW constraints (250 bytes max)
- Point-to-point only — broadcast not supported
- Only one ESPNowHub instance may exist per device

## License

MIT