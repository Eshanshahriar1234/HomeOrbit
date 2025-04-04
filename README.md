# HomeOrbit Smart Home 
**A Personalized IoT-Based Home Automation System**  

---

## Table of Contents  
1. [Overview](#overview)  
2. [Hardware Requirements](#hardware-requirements)  
3. [Circuit Setup](#circuit-setup)  
4. [Software Setup](#software-setup)  
5. [Configuration](#configuration)  
6. [Usage Instructions](#usage-instructions)  
7. [Troubleshooting](#troubleshooting)  
8. [Future Enhancements](#future-enhancements)  


---

## Overview  
HomeOrbit is an IoT-based home automation system that uses environmental sensors (temperature, humidity, light) to trigger automated actions (e.g., activating LEDs for "Coffee Mode"). It features:  
- Real-time sensor monitoring via a web interface  
- Manual override controls for LEDs  
- MQTT integration for remote communication  
- Over-the-air (OTA) firmware updates  

---

## Hardware Requirements  
### Components  
1. **ESP32 Development Board** (central controller)  
2. **DHT11 Sensor** (temperature/humidity)  
3. **Photoresistor (LDR)** (light detection)  
4. **LEDs** (3x: Coffee, Music, Rumba)  
5. **Breadboard & Jumper Wires**  
6. **Resistors** (10kΩ for LDR, 220Ω for LEDs)  

---

## Circuit Setup  
### Wiring Diagram  
![Circuit Configuration](docs/circuit_diagram.png)  
*Refer to Figure 3.8 in the thesis for details.*  

1. **DHT11**:  
   - `VCC` → ESP32 3.3V  
   - `DATA` → GPIO 32  
   - `GND` → GND  

2. **LDR**:  
   - One leg → GPIO 33 (ADC input)  
   - Other leg → GND via 10kΩ resistor (voltage divider)  

3. **LEDs**:  
   - `LED_COFFEE` → GPIO 27  
   - `LED_MUSIC` → GPIO 19  
   - `LED_RUMBA` → GPIO 25  
   - Each LED's cathode → GND via 220Ω resistor  

---

## Software Setup  
### Prerequisites  
1. Install **[Arduino IDE](https://www.arduino.cc/en/software)**  
2. Install **ESP32 Board Support**:  
   - In Arduino IDE: **File → Preferences → Additional Boards Manager URLs**  
   - Add: `https://dl.espressif.com/dl/package_esp32_index.json`  
   - Go to **Tools → Board → Boards Manager**, search for `ESP32`, and install  

### Library Installation  
Install these libraries via **Sketch → Include Library → Manage Libraries**:  
- `WiFi`  
- `PubSubClient` (for MQTT)  
- `DHT sensor library`  
- `AsyncTCP`  
- `ESPAsyncWebServer`  
- `ArduinoOTA`  

---

## Configuration  
### Step 1: Update Credentials  
In the code, modify the following:  
```cpp
// WiFi Configuration  
const char* ssid = "TP-Link_07B8";     // Replace with your WiFi SSID  
const char* password = "30258877";     // Replace with your WiFi password  
```
// MQTT Configuration (optional)  
const char* mqtt_server = "broker.hivemq.com";  // Default public broker

## Step 2: Upload the Code

1. **Connect the ESP32 via USB** to your computer
2. **In Arduino IDE**:
   - Navigate to:  
     ```arduino
     Tools → Board → ESP32 Dev Module
     ```
   - Select the correct COM port:  
     ```arduino
     Tools → Port → [Your COM Port]
     ```
3. **Click the Upload button** (→ arrow icon) or press `Ctrl+U`

## Usage Instructions

### Web Interface
1. After uploading:
   - Open **Serial Monitor** (`Tools → Serial Monitor`)
   - Note the ESP32's IP address (e.g., `192.168.1.100`)
2. Access the web interface:
   - Open a browser and navigate to:  
     ```
     http://[ESP32_IP]
     ```

**Interface Features**:
- **Sensor Dashboard**:
  - Real-time temperature (°C)
  - Humidity (%)
  - Light intensity (0-4095)
- **Control Buttons**:
  - Toggle Coffee/Music/Rumba LEDs
  - Activate "Auto Mode"
- **Active Conditions Panel**:
  - Displays currently triggered automation rules

### MQTT Commands
1. **Using MQTT Explorer**:
   - Subscribe to:  
     ```plaintext
     home/automation/sensor
     ```
     *(receives JSON sensor data)*
   - Publish to:  
     ```plaintext
     home/automation/action
     ```
     **Valid commands**:
     ```
     coffee_on, coffee_off, 
     music_on, music_off, 
     rumba_on, rumba_off, 
     auto_mode
     ```

### OTA Updates
1. **Prerequisites**:
   - ESP32 must be connected to WiFi
   - Initial USB upload completed
2. **Update Process**:
   - In Arduino IDE:  
     ```arduino
     Tools → Port → [ESP32_IP]
     ```
   - Upload as normal (firmware transfers wirelessly)

## Troubleshooting

### Common Issues and Solutions

| Symptom | Possible Cause | Solution |
|---------|---------------|----------|
| **No serial output** | Incorrect COM port selected | Select correct port in `Tools → Port` |
| **WiFi connection fails** | Wrong credentials/2.4GHz network | Verify SSID/password, use 2.4GHz network |
| **Web interface unreachable** | Firewall blocking | Allow port 80 in firewall settings |
| **DHT11 shows no values** | Loose connections | Check wiring (3.3V, GND, DATA pins) |
| **LEDs not lighting up** | Incorrect GPIO pin | Verify pin assignments in code |

## Future Enhancements

- Add more sensors (e.g., motion, CO₂)
- Integrate voice control (Alexa/Google Assistant)
- Implement machine learning for predictive automation
- Use a local MQTT broker (e.g., Mosquitto) for security
