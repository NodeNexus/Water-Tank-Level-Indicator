# Water Tank Level Indicator with Pump Control 💦

An automated system using an ESP32 microcontroller to monitor the water level in a tank and control a pump to maintain the water supply, accessible via a local Wi-Fi web interface.

## Overview

This project provides reliable monitoring and autonomous control of a water pump. It uses an **Ultrasonic Sensor** for continuous level measurement and a **Float Sensor** as a crucial safety backup to prevent overflow. Users can monitor the water level and switch between automatic and manual pump control modes via a local web page.

## Features

* **Dual-Sensor Monitoring:** Combines an Ultrasonic sensor for percentage-based level tracking and a Float sensor for failsafe high-level detection.
* **Automatic Pumping:** Automatically starts the pump when the water level drops below a set **start threshold** (e.g., 20%) and stops it when it reaches the **stop threshold** (e.g., 95%).
* **Failsafe:** The Float sensor overrides the ultrasonic readings and turns the pump off immediately if the tank is full, ensuring no overflow.
* **Web Interface:** Real-time visual gauge of the water level and status display via a local Wi-Fi web server.
* **Manual Override:** Ability to temporarily disable auto-mode and control the pump manually.

## Hardware Requirements

| Component | Quantity | Notes |
| :--- | :--- | :--- |
| **ESP32** or **ESP8266** | 1 | Microcontroller with built-in Wi-Fi. |
| **HC-SR04 Ultrasonic Sensor** | 1 | Used for distance/level measurement. |
| **Float Sensor** | 1 | Placed at the desired "Full" level. |
| **1-Channel Relay Module** | 1 | To control the AC/DC water pump. |
| **Water Pump** | 1 | The device being controlled. |
| **Jumper Wires** | Various | |

## Wiring Diagram

1.  **Ultrasonic Sensor (HC-SR04):**
    * **VCC** $\rightarrow$ **ESP32 5V**
    * **GND** $\rightarrow$ **ESP32 GND**
    * **Trig** $\rightarrow$ **ESP32 GPIO 5**
    * **Echo** $\rightarrow$ **ESP32 GPIO 18**

2.  **Float Sensor:**
    * **VCC** $\rightarrow$ **ESP32 3.3V** (or VCC)
    * **GND** $\rightarrow$ Not needed if using internal pull-up/grounding
    * **Signal** $\rightarrow$ **ESP32 GPIO 19** (Configured as `INPUT_PULLUP` in code)
    *(Note: Connect the signal pin to GND when the water level is high/full to trigger the safety stop.)*

3.  **Pump Relay Module:**
    * **VCC** $\rightarrow$ **ESP32 5V**
    * **GND** $\rightarrow$ **ESP32 GND**
    * **IN** $\rightarrow$ **ESP32 GPIO 27** (`PUMP_RELAY_PIN` in code)
    * **Pump Control:** Connect the pump power line through the relay's **COM** and **NO** (Normally Open) terminals.

## Software Setup and Installation

1.  **Arduino IDE:** Ensure you have the ESP32/ESP8266 boards installed.
2.  **Libraries:** `WiFi.h` and `WebServer.h` (Standard ESP core libraries).
3.  **Configuration:**
    * Update the `ssid` and `password` variables for your local Wi-Fi network.
    * **Crucially, set the `TANK_HEIGHT_CM`, `MAX_DISTANCE_CM`, and `MIN_DISTANCE_CM`** to match your specific tank dimensions and sensor placement for accurate percentage calculation.
    * Adjust `AUTO_START_LEVEL` and `AUTO_STOP_LEVEL` as needed.
4.  **Upload:** Upload the code to the ESP32.
5.  **Access:** Check the Serial Monitor for the assigned **IP Address**. Open this IP in a web browser on your local network to view the water level and control the pump.
