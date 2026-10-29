# Lightshow

Firmware for an ESP8266 that drives WS2812B LED strips and is controlled remotely over MQTT. It ships with a few lighting effects (rainbow, solid, ambient and a progress bar), alternates between two configurable modes, and can be updated wirelessly using ArduinoOTA.

The project is built with [PlatformIO](https://platformio.org/) on the Arduino framework.

## Table of contents
1. [Features](#features)
2. [Hardware](#hardware)
3. [Getting started](#getting-started)
4. [Project structure](#project-structure)
5. [Controlling the lights over MQTT](#controlling-the-lights-over-mqtt)
6. [Effects](#effects)
7. [How the code works](#how-the-code-works)
8. [Adding a new effect](#adding-a-new-effect)
9. [Configuration reference](#configuration-reference)
10. [Troubleshooting](#troubleshooting)
11. [Improvements](#improvements)

## Features
- Controls two LED strips independently (or any number, with some tweaking).
- Four built-in effects, each with an adjustable speed.
- Rotates between two modes every 15 seconds.
- Receives JSON commands over MQTT using mutual TLS (set up for AWS IoT Core, but any broker that supports client certificates will work).
- Over-the-air (OTA) firmware updates, so the board doesn't need to be reachable by USB after the first flash.

## Hardware
| Component | Details |
|---|---|
| Microcontroller | ESP8266, ESP-12E module (e.g. NodeMCU). The PlatformIO board is `esp12e`. |
| LEDs | 120 × WS2812B with GRB colour order, wired as one chain. |
| Data pin | `D4` (GPIO2) |

The 120 LEDs are wired as a single physical chain, but the firmware splits them into two logical strips:

| Strip | LED indices | Variable in `main.cpp` |
|---|---|---|
| Bottom | 0–59 | `strip_bottom` |
| Top | 60–119 | `strip_top` |

> **Power:** At full white, 120 WS2812B LEDs can draw several amps. Power the strip from a suitable external 5 V supply instead of the board's 5 V pin, and connect the supply's ground to the ESP8266's ground.

## Getting started

### Prerequisites
- [VS Code](https://code.visualstudio.com/) with the [PlatformIO extension](https://platformio.org/install/ide?install=vscode), or the PlatformIO CLI.
- An MQTT broker that supports TLS with client certificates, such as AWS IoT Core.
- The CA certificate, the device certificate and the private key for that broker.

All third-party libraries are vendored in `/lib`, so you don't need to install any.

### 1. Clone the repository
```bash
git clone https://github.com/Henz00/Lightshow
```

### 2. Create `env.ini`
Copy `template-env.ini` to a new file named `env.ini` in the project root and fill in your values:

```ini
[env_vars]
WIFI_SSID="your_wifi_name"
WIFI_PASS="your_wifi_password"

MQTT_SERVER_IP="your-endpoint.iot.eu-north-1.amazonaws.com"
MQTT_SERVER_PORT="8883"
```

`platformio.ini` loads this file and passes the values to the compiler as build flags (`WIFI_SSID`, `WIFI_PASS`, `MQTT_SERVER_IP` and `MQTT_SERVER_PORT`). `env.ini` is listed in `.gitignore`, so your credentials won't be committed.

### 3. Create `certs.h`
Copy `src/template-certs.h` to `src/certs.h` and paste in your certificates:

| Constant | Contents |
|---|---|
| `AWS_CERT_CA` | The broker's root CA certificate (for AWS IoT, Amazon Root CA 1) |
| `AWS_CERT_CRT` | The device certificate |
| `AWS_CERT_PRIVATE` | The device's private key |

`src/certs.h` is also listed in `.gitignore`.

### 4. Flash the board for the first time (USB)
`platformio.ini` is set up for OTA uploads by default:

```ini
upload_protocol = espota
upload_port = esp8266-lightshow.local
```

A board that doesn't have this firmware on it yet can't receive OTA updates, so the first upload has to go over USB. Comment out those two lines, connect the board, and upload:

```bash
pio run -t upload
```

Then open the serial monitor (115200 baud) to check that it connects:

```bash
pio device monitor
```

You should see the board connect to Wi-Fi, print its IP address, sync the time, and connect to MQTT.

### 5. Flash the board over Wi-Fi (OTA)
Uncomment the two upload lines again. From now on, `pio run -t upload` sends the firmware over the network to `esp8266-lightshow.local`. Your computer has to be on the same network as the board. If mDNS (`.local`) names don't resolve on your network, set `upload_port` to the IP address the board printed on the serial monitor.

## Project structure
```
Lightshow/
├── platformio.ini            # Build config; loads env.ini and sets the OTA upload
├── template-env.ini          # Template for env.ini (Wi-Fi and MQTT settings)
├── lib/                      # Vendored third-party libraries
├── src/
│   ├── main.cpp              # Entry point: Wi-Fi, MQTT, OTA, mode rotation
│   ├── template-certs.h      # Template for certs.h (TLS certificates)
│   └── ledstrip/
│       ├── led_strip.h/.cpp  # LEDStrip class: state and mode/animation parsing
│       └── ledmodes/
│           ├── effects.h/.cpp          # Maps a Mode enum to its effect function
│           ├── rainbow_effect.h/.cpp
│           ├── solid_effect.h/.cpp
│           ├── ambient_effect.h/.cpp
│           └── progress_effect.h/.cpp
├── include/                  # PlatformIO default (unused)
└── test/                     # PlatformIO default (unused)
```

### Libraries
| Library | Used for |
|---|---|
| FastLED 3.10.3 | Driving the WS2812B LEDs |
| PubSubClient | MQTT client |
| ArduinoJson 7.x | Parsing MQTT payloads and storing the modes |
| ArduinoOTA, ESP8266WiFi | Wireless updates and Wi-Fi (part of the ESP8266 Arduino core) |

`lib/` also contains `ArduinoWebsockets` and `ESP32_WiFiClientSecure`, which the firmware doesn't use at the moment.

## Controlling the lights over MQTT

The device connects with the client ID `esp8266-office`.

| Direction | Topic | Payload |
|---|---|---|
| Subscribes | `office/ledstrip` | JSON command (see below) |
| Publishes | `server/logger` | `esp: connected`, sent each time it connects to the broker |

### Message format
```json
{
  "command": "start",
  "top": {
    "mode": "progress_bar",
    "speed": 30,
    "progress": 75,
    "animation": "center_out"
  },
  "bottom": {
    "mode": "rainbow",
    "speed": 40,
    "progress": 0,
    "animation": "fill"
  }
}
```

| Field | Values | Description |
|---|---|---|
| `command` | `start`, `pause`, `stop` | `start` runs the effects. `pause` freezes the LEDs on their current frame. `stop` freezes the effects and turns off all LEDs. Send `start` to resume after `pause` or `stop`. |
| `mode` | `rainbow`, `solid`, `ambient`, `progress_bar` | The effect to show on that strip. See [Effects](#effects). |
| `speed` | integer (ms) | Milliseconds between animation steps. **A lower value is faster.** |
| `progress` | 0–100 | How full the bar is, in percent. Only used by `progress_bar`. |
| `animation` | `instant`, `fill`, `center_out` | How the progress bar moves to a new value. Only used by `progress_bar`. |

**Send both `top` and `bottom`, with all four fields each.** The firmware overwrites every field on every message, so a missing field doesn't keep its old value. A missing `mode` becomes unknown and falls back to rainbow, and a missing `speed` becomes `0`, which makes the effect run as fast as possible.

### How a message interacts with the rotation
The firmware stores two modes, `mode_1` and `mode_2`, and switches between them every 15 seconds. A new message:
1. replaces `mode_1` with the `top` and `bottom` settings from the message,
2. resets the rotation timer, so the new `mode_1` is shown straight away,
3. runs the `command`.

`mode_2` can't be changed over MQTT. It stays at its default (ambient, speed 60, set in `InitializeLEDs()`), so the lights keep alternating between your settings and ambient. To change this behaviour, edit `InitializeLEDs()` and `loop()` in `main.cpp`.

### Example with `mosquitto_pub`
```bash
mosquitto_pub -h <your-endpoint> -p 8883 --cafile AmazonRootCA1.pem --cert device.pem.crt --key private.pem.key -t office/ledstrip -m '{"command":"start","top":{"mode":"ambient","speed":50,"progress":0,"animation":"fill"},"bottom":{"mode":"progress_bar","speed":20,"progress":60,"animation":"fill"}}'
```

## Effects
All effects are non-blocking. Each one checks how long it has been since its last step and returns straight away if it isn't time for the next one yet.

| Mode | What it looks like |
|---|---|
| `rainbow` | A single dot runs along the strip and leaves a fading trail. The dot's hue changes with each step, so the trail shows a rainbow. |
| `solid` | The same running dot with a fading trail, but always in one colour (aqua, hue 128). |
| `ambient` | The whole strip shows one colour that slowly cycles through the hues. |
| `progress_bar` | A green bar that shows `progress` percent of the strip. |

Progress bar animations:

| Animation | Behaviour |
|---|---|
| `instant` | Jumps straight to the new value. This is also the fallback for unknown animation names. |
| `fill` | Grows or shrinks one LED at a time from the start of the strip, one step every `speed` ms. |
| `center_out` | Grows or shrinks symmetrically from the middle of the strip, one step every `speed` ms. |

## How the code works

### `main.cpp`
**`setup()`** runs once at boot:
1. `InitializeLEDs()` registers the LEDs with FastLED and fills `mode_1` and `mode_2` with their defaults.
2. `ConnectToWifi()` blocks until the board is connected to Wi-Fi.
3. ArduinoOTA starts with the hostname `esp8266-lightshow`.
4. `configTime()` and `timeSync()` get the time from NTP. TLS needs the correct time to validate certificates, so this has to happen before MQTT connects.
5. `SetupMQTT()` loads the certificates into `WiFiClientSecure` and configures the broker, the message callback and a 120-second keep-alive.

**`loop()`** runs continuously and must never block:
1. `ConnectMQTT()` tries to reconnect every 5 seconds if the connection has dropped, and subscribes to `office/ledstrip` again after reconnecting.
2. `client.loop()` handles incoming MQTT messages. When one arrives, it calls `callback()`, which parses the JSON, updates `mode_1` and runs `handleCommand()`.
3. Every 15 seconds, it switches between `mode_1` and `mode_2` using `updateLEDStrips()`.
4. Every 20 ms (about 50 FPS), it calls `loop()` on each strip and then `FastLED.show()`, unless the effects are paused or stopped.
5. `yield()` and `ArduinoOTA.handle()` give the ESP8266 time for Wi-Fi tasks and check for OTA updates.

### `LEDStrip` (`ledstrip/led_strip.h` / `.cpp`)
Each `LEDStrip` controls a section of the shared `leds` array. The constructor takes a pointer to the strip's first LED and the number of LEDs in the strip:

```cpp
LEDStrip strip_bottom(&leds[0], 60);
LEDStrip strip_top(&leds[60], 60);
```

- **`loop()`** checks whether the mode or the progress animation has changed. If it has, it resets the strip. Then it calls the effect for the current mode. You shouldn't need to change this function.
- **`handleMode()`, `handleSpeed()`, `handleProgress()` and `handleAnimation()`** set the strip's state. `parseMode()` and `parseAnimation()` convert the JSON strings into the `Mode` and `ProgressAnimation` enums.
- **`resetLEDStrip()`** clears the LEDs and resets the variables the effects use to track their state.

The public members are split into two groups:
- **Set by the user:** `LED_mode`, `loop_speed`, `progress`, `progressAnimation`, as well as `colour` and `colourDelta`, which can't be set over MQTT yet.
- **Used by the effects to track state:** `lastUpdate`, `ledIndex`, `lastProgress`, `currentLit`, `lastLit`.

### Effects (`ledstrip/ledmodes/`)
An effect is a free function with the signature `void effect(LEDStrip& strip)`. `getEffectFunction()` in `effects.cpp` maps each `Mode` to its function and falls back to `rainbowEffect` for unknown modes. Effects only write to `strip.leds`. `main.cpp` calls `FastLED.show()` once per frame for all strips.

## Adding a new effect
Here's how you'd add a hypothetical `sparkle` mode:

1. **Create the files** `src/ledstrip/ledmodes/sparkle_effect.h` and `sparkle_effect.cpp`:
   ```cpp
   // sparkle_effect.h
   #pragma once
   #include "./ledstrip/led_strip.h"

   void sparkleEffect(LEDStrip& strip);
   ```
   ```cpp
   // sparkle_effect.cpp
   #include "sparkle_effect.h"

   void sparkleEffect(LEDStrip& strip){
     if (millis() - strip.lastUpdate < strip.loop_speed) {
       return;
     }
     strip.lastUpdate = millis();

     fadeToBlackBy(strip.leds, strip.num_leds, 64);
     strip.leds[random16(strip.num_leds)] = CRGB::White;
   }
   ```
   Use the timing check at the top of the function and never call `delay()`. Any delay would block MQTT and OTA.

2. **Add the mode to the enum** in `led_strip.h`, before `MODE_UNKNOWN`:
   ```cpp
   MODE_SPARKLE,
   ```

3. **Map the string to the enum** in `LEDStrip::parseMode()` in `led_strip.cpp`:
   ```cpp
   if (modeStr == "sparkle") return MODE_SPARKLE;
   ```

4. **Register the function** in `effects.cpp`:
   ```cpp
   #include "sparkle_effect.h"
   // ...
   case MODE_SPARKLE: return sparkleEffect;
   ```

You can now send `"mode": "sparkle"` over MQTT. `solid_effect.cpp` and `rainbow_effect.cpp` are short, good examples to start from. `progress_effect.cpp` shows how to handle sub-animations.

If your effect needs state that's kept between frames, add a member to `LEDStrip` and reset it in `resetLEDStrip()`.

## Configuration reference
These values are hardcoded. Change them in the files listed below.

| Setting | Value | Location |
|---|---|---|
| Total LEDs | `120` (`TOTAL_LEDS`) | `main.cpp` |
| Strip split | 60 + 60 | `main.cpp`, where `strip_bottom` and `strip_top` are declared |
| LED type, data pin, colour order | `WS2812B`, `D4`, `GRB` | `InitializeLEDs()` in `main.cpp` |
| Default modes | `mode_1`: rainbow at speed 40; `mode_2`: ambient at speed 60 | `InitializeLEDs()` in `main.cpp` |
| Rotation interval | 15000 ms | `loop()` in `main.cpp` |
| Frame interval | 20 ms | `loop()` in `main.cpp` |
| MQTT client ID | `esp8266-office` | `ConnectMQTT()` in `main.cpp` |
| MQTT topics | `office/ledstrip` (subscribe), `server/logger` (publish) | `ConnectMQTT()` in `main.cpp` |
| MQTT reconnect interval / keep-alive | 5 s / 120 s | `ConnectMQTT()` / `SetupMQTT()` in `main.cpp` |
| OTA hostname | `esp8266-lightshow` | `setup()` in `main.cpp` and `upload_port` in `platformio.ini` |
| NTP servers | `pool.ntp.org`, `time.nist.gov` | `setup()` in `main.cpp` |
| Serial baud rate | 115200 | `setup()` in `main.cpp` and `monitor_speed` in `platformio.ini` |

If you change the OTA hostname, update it in both `main.cpp` and `platformio.ini`. To add a third strip, increase `TOTAL_LEDS`, declare another `LEDStrip` on a new section of the `leds` array, and handle it in `loop()`, `callback()` and `InitializeLEDs()`.

## Troubleshooting
| Symptom | Likely cause |
|---|---|
| Serial output stops after `connecting...` with endless dots | Wrong Wi-Fi SSID or password in `env.ini`, or a 5 GHz-only network. The ESP8266 only supports 2.4 GHz. |
| Serial output stops at `Waiting for time sync` | NTP traffic (UDP port 123) is blocked on your network. |
| `MQTT connection... failed, rc=-2` repeated | Can't connect to the broker. Check the endpoint, the port and the certificates in `certs.h`. With AWS IoT, also check that the certificate is active and has a policy that allows connecting with the client ID `esp8266-office` and subscribing and publishing to the topics. |
| `JSON parse failed` | The MQTT payload isn't valid JSON. |
| `Unknown command: null` | The message didn't include `command`. The modes are still updated. |
| OTA upload can't find the device | Your computer isn't on the same network as the board, or `.local` names don't resolve. Use the board's IP address as `upload_port`. |
| `fatal error: certs.h: No such file or directory` | You haven't created `src/certs.h` yet (step 3). |

### Known quirks
- `resetLEDStrip()` calls `FastLED.clear(true)`, which clears **all** LEDs, not just that strip's section. When one strip changes mode, the other goes dark for a moment and then redraws.
- `mode_2` can only be changed in code, not over MQTT.

## Improvements
Suggestions and feedback are both greatly appreciated. Some ideas:
- Let MQTT messages update `mode_2` and the rotation interval.
- Make `colour` and `colourDelta` (and the progress bar colour) settable over MQTT.
- Only update the fields that are present in a message, so partial updates work.
- Clear only the strip's own section in `resetLEDStrip()`.
- Move the hardcoded values above into `env.ini`.
