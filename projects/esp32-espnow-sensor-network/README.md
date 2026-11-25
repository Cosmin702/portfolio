# ESP32 ESP-NOW sensor network

> 基于 ESP-NOW 的低功耗 ESP32 无线传感器网络，带网页实时图表。

<!-- TODO: photo or GIF of the nodes and the web dashboard -->

## What it does
A battery-powered ESP32 sensor node reads temperature, humidity, light and soil moisture, sends one ESP-NOW packet to a receiver ESP32 and goes back to deep sleep. The receiver shows the latest values on an OLED display and serves a web page with live charts of the last 100 readings.

## Hardware
- 2× ESP32
- Sensor node: DHT11 (temperature, humidity) on GPIO4, analog light sensor on GPIO34, Grove soil moisture sensor on GPIO35
- Receiver: SSD1306 128×64 OLED (I²C, address 0x3C)
- <!-- TODO: wiring diagram -->

## How the code works
- `sensor_node/sensor_node.ino`
  - On a cold boot it connects to Wi-Fi once, only to find out the router's channel, and stores it in NVS (`Preferences`). ESP-NOW must use the same channel as the receiver, which is connected to Wi-Fi.
  - After a timer wake-up it reads the channel from NVS instead of reconnecting, which saves time and battery.
  - It reads the sensors, sends a struct `{temp, hum, lux, ter}` to the receiver's MAC address and goes to deep sleep from the send callback.
- `receiver/receiver.ino`
  - Connects to Wi-Fi and receives the packets with ESP-NOW.
  - Keeps a history of the last 100 values for each sensor.
  - Pushes every new reading to the browser over a WebSocket (`ESPAsyncWebServer`), where Chart.js draws four line charts.
  - Prints the latest values on the OLED.

## Setup
1. In both `sensor_node/` and `receiver/`, copy `secrets.example.h` to `secrets.h` and fill in your Wi-Fi name and password. In the sensor node, also set the receiver's MAC address. `secrets.h` is git-ignored.
2. Libraries: `DHT sensor library`, `ESPAsyncWebServer`, `AsyncTCP`, `Adafruit GFX`, `Adafruit SSD1306`.
3. Flash the receiver, open the IP it prints on the serial monitor, then flash the sensor node.

## Problems I ran into
- **Soil sensor always read 0**: it was on an ADC2 pin, and ADC2 can't be used while Wi-Fi is active. I moved it to GPIO35 (ADC1).
- **Crash after deep-sleep wake-up**: a Guru Meditation `LoadProhibited` error when restoring the channel. <!-- TODO: was it fixed, and how? -->
- **Swapped values**: the receiver declared the struct as `{temp, hum, ter, lux}` while the sender sends `{temp, hum, lux, ter}`, so light and soil moisture arrived swapped. Both structs now use the same order.

## What I'd do differently
<!-- TODO -->

## Not in this folder
The door node, which sends a Telegram notification when the door opens, is part of the same project but its code isn't here yet. <!-- TODO: add it, with the bot token kept out of the code -->
