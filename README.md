HomeOrbit Smart Home System - ReadMe
A Personalized IoT-Based Home Automation System

Table of Contents
Overview

Hardware Requirements

Circuit Setup

Software Setup

Configuration

Usage Instructions

Troubleshooting

Future Enhancements

Overview
HomeOrbit is an IoT-based home automation system that uses environmental sensors (temperature, humidity, light) to trigger automated actions (e.g., activating LEDs for "Coffee Mode"). It features:

Real-time sensor monitoring via a web interface.

Manual override controls for LEDs.

MQTT integration for remote communication.

Over-the-air (OTA) firmware updates.

Hardware Requirements
Components
ESP32 Development Board (central controller).

DHT11 Sensor (temperature/humidity).

Photoresistor (LDR) (light detection).

LEDs (3x: Coffee, Music, Rumba).

Breadboard & Jumper Wires.

Resistors (10kΩ for LDR, 220Ω for LEDs).

Circuit Setup
Wiring Diagram
Circuit Configuration
Refer to Figure 3.8 in the thesis for details.

DHT11:

VCC → ESP32 3.3V.

DATA → GPIO 32.

GND → GND.

LDR:

One leg → GPIO 33 (ADC input).

Other leg → GND via 10kΩ resistor (voltage divider).

LEDs:

LED_COFFEE → GPIO 27.

LED_MUSIC → GPIO 19.

LED_RUMBA → GPIO 25.

Each LED’s cathode → GND via 220Ω resistor.

Software Setup
Prerequisites
Install Arduino IDE (Download).

Install ESP32 Board Support:

In Arduino IDE: File → Preferences → Additional Boards Manager URLs.

Add: https://dl.espressif.com/dl/package_esp32_index.json.

Go to Tools → Board → Boards Manager, search for ESP32, and install.

Library Installation
Install these libraries via Sketch → Include Library → Manage Libraries:

WiFi

PubSubClient (for MQTT)

DHT sensor library

AsyncTCP

ESPAsyncWebServer

ArduinoOTA

Configuration
Step 1: Update Credentials
In the code, modify the following:

cpp
Copy
// WiFi Configuration  
const char* ssid = "TP-Link_07B8";     // Replace with your WiFi SSID  
const char* password = "30258877";     // Replace with your WiFi password  

// MQTT Configuration (optional)  
const char* mqtt_server = "broker.hivemq.com";  // Default public broker  
Step 2: Upload the Code
Connect the ESP32 via USB.

In Arduino IDE:

Tools → Board → ESP32 Dev Module.

Tools → Port → Select the correct COM port.

Click Upload.

Usage Instructions
Web Interface
After uploading, open the Serial Monitor (Tools → Serial Monitor) to see the ESP32’s IP address.

Open a web browser and navigate to http://[ESP32_IP].
Web Interface

Sensor Dashboard: Displays real-time temperature, humidity, and light values.

Control Buttons: Manually toggle LEDs or activate "Auto Mode".

Active Conditions: Shows triggered automation rules.

MQTT Commands
Use an MQTT client (e.g., MQTT Explorer) to interact with topics:

Subscribe to home/automation/sensor for sensor data.

Publish commands to home/automation/action:

coffee_on, music_off, auto_mode, etc.

OTA Updates
Ensure the ESP32 is connected to WiFi.

In Arduino IDE: Tools → Port → [ESP32 IP Address].

Upload new firmware wirelessly.

Troubleshooting
Issue	Solution
Sensors not reading	Check wiring and GPIO pins.
WiFi connection failed	Verify SSID/password.
Web interface not loading	Restart ESP32 or check AsyncWebServer code.
LEDs not blinking	Ensure manual override is off (use "Auto Mode").
Future Enhancements
Add more sensors (e.g., motion, CO₂).

Integrate voice control (Alexa/Google Assistant).

Implement machine learning for predictive automation.

Use a local MQTT broker (e.g., Mosquitto) for security.
