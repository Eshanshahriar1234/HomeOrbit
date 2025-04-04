
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// WiFi Configuration
const char* ssid = "TP-Link_07B8";         // Your WiFi SSID
const char* password = "30258877";          // Your WiFi Password

// MQTT Configuration
const char* mqtt_server = "broker.hivemq.com";
const char* topic_sensor = "home/automation/sensor";
const char* topic_light  = "home/automation/light";
const char* topic_action = "home/automation/action";

// DHT Configuration
#define DHTPIN 32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LDR Configuration
#define LDR_PIN 33

// LED Pins
#define LED_COFFEE 27
#define LED_MUSIC  19
#define LED_RUMBA  25

// Condition thresholds (merged from second code)
// Cold & Dry condition for Coffee
#define TEMP_COLD 20.0
#define HUMIDITY_DRY 50.0
// Moderate condition for Music
#define TEMP_MODERATE_LOW 20.0
#define TEMP_MODERATE_HIGH 25.0
#define HUMIDITY_MODERATE_LOW 40.0
#define HUMIDITY_MODERATE_HIGH 60.0
// Hot & Humid condition for Rumba
#define TEMP_HOT 25.0
#define HUMIDITY_WET 60.0

// Light thresholds
#define LIGHT_LOW 500
#define LIGHT_MODERATE_LOW 500
#define LIGHT_MODERATE_HIGH 4095
#define LIGHT_HIGH 4095
#define COMPLETE_DARKNESS 100

// LED Blinking variables
unsigned long previousMillis = 0;
const long blinkInterval = 200;  // 200ms blink interval
bool ledState = LOW;
int blinkCount = 0;
const int maxBlinkCount = 10;    // Maximum number of blinks

// LED control flags
bool coffeeBlinking = false;
bool musicBlinking = false;
bool rumbaBlinking = false;

// Manual override flags
bool coffeeManualOverride = false;
bool musicManualOverride = false;
bool rumbaManualOverride = false;

// Condition flags
bool coffeeConditionMet = false;
bool musicConditionMet = false;
bool rumbaConditionMet = false;

WiFiClient espClient;
PubSubClient client(espClient);

// Create AsyncWebServer on port 80 and AsyncWebSocket on "/ws"
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Web page to serve (chat interface with threshold settings and condition info)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>HomeOrbit Smart Home </title>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; background: #f4f4f4; padding: 20px; }
    .container { width: 100%; max-width: 500px; background: #fff; margin: auto; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    .messages { height: 200px; overflow-y: scroll; border: 1px solid #ddd; padding: 10px; margin-bottom: 10px; text-align: left; }
    .input { display: flex; margin-bottom: 20px; }
    input[type="text"] { flex: 1; padding: 8px; border: 1px solid #ddd; border-radius: 4px 0 0 4px; }
    button { padding: 8px 15px; background: #4CAF50; color: white; border: none; border-radius: 0 4px 4px 0; cursor: pointer; }
    button:hover { background: #45a049; }
    .sensor-data { background: #eef; padding: 10px; border-radius: 4px; margin-bottom: 15px; }
    .controls { display: flex; flex-wrap: wrap; justify-content: space-between; margin-bottom: 15px; }
    .control-btn { padding: 8px 12px; margin: 5px; border: none; border-radius: 4px; cursor: pointer; }
    .coffee { background: #8B4513; color: white; }
    .music { background: #1E90FF; color: white; }
    .rumba { background: #FF4500; color: white; }
    .conditions { background: #f9f9f9; padding: 10px; border-radius: 4px; margin-top: 15px; margin-bottom: 15px; }
    .condition-title { font-weight: bold; margin-bottom: 5px; }
    .condition-item { text-align: left; margin: 5px 0; }
    .active { background: #4CAF50; }
  </style>
</head>
<body>
  <div class="container">
    <h2>HomeOrbit SmartHome Control</h2>
    
    <div class="sensor-data" id="sensors">
      Loading sensor data...
    </div>
    
    <div class="controls">
      <button class="control-btn coffee" onclick="sendCommand('start coffee')">Start Coffee</button>
      <button class="control-btn coffee" onclick="sendCommand('stop coffee')">Stop Coffee</button>
      <button class="control-btn music" onclick="sendCommand('start music')">Start Music</button>
      <button class="control-btn music" onclick="sendCommand('stop music')">Stop Music</button>
      <button class="control-btn rumba" onclick="sendCommand('start rumba')">Start Rumba</button>
      <button class="control-btn rumba" onclick="sendCommand('stop rumba')">Stop Rumba</button>
    </div>
    
    <div class="controls">
      <button class="control-btn" onclick="sendCommand('auto mode')" style="background: #9C27B0; width: 100%;">Auto Mode</button>
    </div>
    
    <div class="conditions">
      <div class="condition-title">Active Conditions:</div>
      <div id="active-conditions">Checking conditions...</div>
    </div>
    
    <div class="messages" id="messages"></div>
    
    <div class="input">
      <input type="text" id="msg" placeholder="Enter command...">
      <button onclick="sendMsg()">Send</button>
    </div>
  </div>

<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  
  window.addEventListener('load', onLoad);
  
  function onLoad() {
    initWebSocket();
    document.getElementById('msg').addEventListener('keypress', function(e) {
      if (e.key === 'Enter') {
        sendMsg();
      }
    });
  }
  
  function initWebSocket() {
    console.log('Connecting to WebSocket...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
  }
  
  function onOpen(event) {
    console.log('WebSocket connected');
  }
  
  function onClose(event) {
    console.log('WebSocket disconnected');
    setTimeout(initWebSocket, 2000);
  }
  
  function onMessage(event) {
    var message = event.data;
    
    // Check if message is JSON (for sensor data or conditions)
    try {
      var jsonData = JSON.parse(message);
      
      if (jsonData.type === 'sensor_data') {
        updateSensorDisplay(jsonData);
        return;
      } else if (jsonData.type === 'conditions') {
        updateConditionsDisplay(jsonData);
        return;
      }
    } catch (e) {
      // Not JSON, treat as regular message
    }
    
    var messages = document.getElementById('messages');
    messages.innerHTML += '<p>' + message + '</p>';
    messages.scrollTop = messages.scrollHeight;
  }
  
  function updateSensorDisplay(data) {
    var sensorElement = document.getElementById('sensors');
    sensorElement.innerHTML = `
      <strong>Temperature:</strong> ${data.temperature}°C 
      <strong>Humidity:</strong> ${data.humidity}% 
      <strong>Light:</strong> ${data.light}
    `;
  }
  
  function updateConditionsDisplay(data) {
    var conditionsElement = document.getElementById('active-conditions');
    var conditions = [];
    
    if (data.coffee) conditions.push('<span style="color:#8B4513">Coffee Condition</span> (Cold & Dry): Active');
    if (data.music) conditions.push('<span style="color:#1E90FF">Music Condition</span> (Moderate): Active');
    if (data.rumba) conditions.push('<span style="color:#FF4500">Rumba Condition</span> (Hot & Humid): Active');
    
    if (data.darkness) conditions.push('<span style="color:purple">Complete Darkness Mode</span>: Active');
    
    if (conditions.length === 0) {
      conditionsElement.innerHTML = 'No conditions currently active';
    } else {
      conditionsElement.innerHTML = conditions.join('<br>');
    }
  }
  
  function sendMsg() {
    var input = document.getElementById('msg');
    if (input.value.trim() !== '') {
      sendCommand(input.value);
      input.value = '';
    }
  }
  
  function sendCommand(command) {
    websocket.send(command);
  }
</script>
</body>
</html>
)rawliteral";

// Function Prototypes
void setupWiFi();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setupOTA();
void sendMQTTMessage(const char* topic, const char* message);
void turnOffAllLEDs();
void handleLEDBlinking();
void checkSensorConditions(float temperature, float humidity, int lightIntensity);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void notifyClients(String message);
void broadcastSensorData(float temperature, float humidity, int lightIntensity);
void broadcastConditionStatus();

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Initialize LED pins and LDR pin
  pinMode(LED_COFFEE, OUTPUT);
  pinMode(LED_MUSIC, OUTPUT);
  pinMode(LED_RUMBA, OUTPUT);
  turnOffAllLEDs();
  pinMode(LDR_PIN, INPUT);

  setupWiFi();
  
  // Setup MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  reconnectMQTT();
  
  // Setup OTA updates
  setupOTA();

  // Setup AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Serve the chat interface page at "/"
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  ArduinoOTA.handle();
  ws.cleanupClients();

  // Handle LED blinking if active
  if (coffeeBlinking || musicBlinking || rumbaBlinking) {
    handleLEDBlinking();
  }

  // Read sensors every 2 seconds
  static unsigned long lastSensorReadTime = 0;
  if (currentMillis - lastSensorReadTime >= 2000) {
    lastSensorReadTime = currentMillis;
    
    // Sensor readings
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int lightIntensity = analogRead(LDR_PIN);

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Check conditions based on sensor readings
    checkSensorConditions(temperature, humidity, lightIntensity);

    // Broadcast sensor data to WebSocket clients
    broadcastSensorData(temperature, humidity, lightIntensity);
    
    // Broadcast condition status
    broadcastConditionStatus();

    // Send to MQTT
    char sensorData[128];
    snprintf(sensorData, sizeof(sensorData), "Temperature: %.2f°C, Humidity: %.2f%%, Light: %d", temperature, humidity, lightIntensity);
    sendMQTTMessage(topic_sensor, sensorData);
    sendMQTTMessage(topic_light, String(lightIntensity).c_str());
    Serial.println("Sensor Data Published: " + String(sensorData));
  }
}

void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client_" + String(millis());
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(topic_action);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Received MQTT message: ");
  Serial.println(message);

  // Control LEDs based on MQTT command
  if (message == "coffee_on") {
    coffeeManualOverride = true;
    coffeeBlinking = false;
    digitalWrite(LED_COFFEE, HIGH);
    notifyClients("Coffee turned ON via MQTT");
  } else if (message == "coffee_off") {
    coffeeManualOverride = true;
    coffeeBlinking = false;
    digitalWrite(LED_COFFEE, LOW);
    notifyClients("Coffee turned OFF via MQTT");
  } else if (message == "music_on") {
    musicManualOverride = true;
    musicBlinking = false;
    digitalWrite(LED_MUSIC, HIGH);
    notifyClients("Music turned ON via MQTT");
  } else if (message == "music_off") {
    musicManualOverride = true;
    musicBlinking = false;
    digitalWrite(LED_MUSIC, LOW);
    notifyClients("Music turned OFF via MQTT");
  } else if (message == "rumba_on") {
    rumbaManualOverride = true;
    rumbaBlinking = false;
    digitalWrite(LED_RUMBA, HIGH);
    notifyClients("Rumba turned ON via MQTT");
  } else if (message == "rumba_off") {
    rumbaManualOverride = true;
    rumbaBlinking = false;
    digitalWrite(LED_RUMBA, LOW);
    notifyClients("Rumba turned OFF via MQTT");
  } else if (message == "auto_mode") {
    // Reset all manual overrides
    coffeeManualOverride = false;
    musicManualOverride = false;
    rumbaManualOverride = false;
    notifyClients("Automatic mode activated via MQTT");
  }
}

void setupOTA() {
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready. IP: " + WiFi.localIP().toString());
}

void sendMQTTMessage(const char* topic, const char* message) {
  if (client.connected()) {
    client.publish(topic, message);
  }
}

void turnOffAllLEDs() {
  digitalWrite(LED_COFFEE, LOW);
  digitalWrite(LED_MUSIC, LOW);
  digitalWrite(LED_RUMBA, LOW);
  coffeeBlinking = false;
  musicBlinking = false;
  rumbaBlinking = false;
}

void handleLEDBlinking() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    ledState = !ledState;
    
    // Increment blink counter when LED turns ON
    if (ledState == HIGH) {
      blinkCount++;
    }
    
    // Apply blinking to appropriate LEDs
    if (coffeeBlinking && !coffeeManualOverride) {
      digitalWrite(LED_COFFEE, ledState);
    }
    
    if (musicBlinking && !musicManualOverride) {
      digitalWrite(LED_MUSIC, ledState);
    }
    
    if (rumbaBlinking && !rumbaManualOverride) {
      digitalWrite(LED_RUMBA, ledState);
    }
    
    // Reset blinking after maxBlinkCount complete cycles
    if (blinkCount >= maxBlinkCount * 2) { // Multiply by 2 because each cycle has ON and OFF
      blinkCount = 0;
      
      if (!coffeeConditionMet && coffeeBlinking && !coffeeManualOverride) {
        coffeeBlinking = false;
        digitalWrite(LED_COFFEE, LOW);
      }
      
      if (!musicConditionMet && musicBlinking && !musicManualOverride) {
        musicBlinking = false;
        digitalWrite(LED_MUSIC, LOW);
      }
      
      if (!rumbaConditionMet && rumbaBlinking && !rumbaManualOverride) {
        rumbaBlinking = false;
        digitalWrite(LED_RUMBA, LOW);
      }
    }
  }
}

void checkSensorConditions(float temperature, float humidity, int lightIntensity) {
  // Reset condition flags
  coffeeConditionMet = false;
  musicConditionMet = false;
  rumbaConditionMet = false;
  bool darknessMode = false;
  
  // Check if it's complete darkness
  if (lightIntensity == COMPLETE_DARKNESS) {
    darknessMode = true;
    notifyClients("Complete darkness detected!");
    sendMQTTMessage(topic_action, "Action: Complete darkness detected");
    
    // In complete darkness, check for specific conditions
    if (temperature < TEMP_COLD && humidity < HUMIDITY_DRY) {
      coffeeConditionMet = true;
      if (!coffeeManualOverride) {
        coffeeBlinking = true;
        blinkCount = 0;
      }
      notifyClients("Coffee condition met in darkness!");
      sendMQTTMessage(topic_action, "Action: Coffee triggered in darkness");
    } 
    else if (temperature >= TEMP_MODERATE_LOW && temperature <= TEMP_MODERATE_HIGH && 
             humidity >= HUMIDITY_MODERATE_LOW && humidity <= HUMIDITY_MODERATE_HIGH) {
      musicConditionMet = true;
      if (!musicManualOverride) {
        musicBlinking = true;
        blinkCount = 0;
      }
      notifyClients("Music condition met in darkness!");
      sendMQTTMessage(topic_action, "Action: Music triggered in darkness");
    } 
    else if (temperature > TEMP_HOT && humidity > HUMIDITY_WET) {
      rumbaConditionMet = true;
      if (!rumbaManualOverride) {
        rumbaBlinking = true;
        blinkCount = 0;
      }
      notifyClients("Rumba condition met in darkness!");
      sendMQTTMessage(topic_action, "Action: Rumba triggered in darkness");
    } 
    else {
      // If no specific condition is met in darkness, turn all LEDs on
      if (!coffeeManualOverride) digitalWrite(LED_COFFEE, HIGH);
      if (!musicManualOverride) digitalWrite(LED_MUSIC, HIGH);
      if (!rumbaManualOverride) digitalWrite(LED_RUMBA, HIGH);
      notifyClients("All actions triggered in darkness!");
      sendMQTTMessage(topic_action, "Action: All LEDs ON (Complete Darkness)");
      Serial.println("Condition: All LEDs ON (Complete Darkness)");
    }
  } 
  else {
    // In normal light conditions, check for specific triggers
    if (temperature < TEMP_COLD && humidity < HUMIDITY_DRY && lightIntensity < LIGHT_LOW) {
      coffeeConditionMet = true;
      if (!coffeeManualOverride) {
        coffeeBlinking = true;
        blinkCount = 0;
      }
      notifyClients("Coffee condition met! (Cold & Dry, Low Light)");
      sendMQTTMessage(topic_action, "Action: Coffee triggered");
    } 
    else if (temperature >= TEMP_MODERATE_LOW && temperature <= TEMP_MODERATE_HIGH && 
             humidity >= HUMIDITY_MODERATE_LOW && humidity <= HUMIDITY_MODERATE_HIGH &&
             lightIntensity >= LIGHT_MODERATE_LOW && lightIntensity <= LIGHT_MODERATE_HIGH) {
      musicConditionMet = true;
      if (!musicManualOverride) {
        musicBlinking = true;
        blinkCount = 0;
      }
      notifyClients("Music condition met! (Moderate Temp & Humidity, Moderate Light)");
      sendMQTTMessage(topic_action, "Action: Music triggered");
    } 
    else if (temperature > TEMP_HOT && humidity > HUMIDITY_WET && lightIntensity > LIGHT_HIGH) {
      rumbaConditionMet = true;
      if (!rumbaManualOverride) {
        rumbaBlinking = true;
        blinkCount = 0;
      }
      notifyClients("Rumba condition met! (Hot & Humid, Bright Light)");
      sendMQTTMessage(topic_action, "Action: Rumba triggered");
    } 
    else {
      // If no condition is met and not manually overridden, turn LEDs off
      if (!coffeeManualOverride) {
        coffeeBlinking = false;
        digitalWrite(LED_COFFEE, LOW);
      }
      if (!musicManualOverride) {
        musicBlinking = false;
        digitalWrite(LED_MUSIC, LOW);
      }
      if (!rumbaManualOverride) {
        rumbaBlinking = false;
        digitalWrite(LED_RUMBA, LOW);
      }
      
      if (!coffeeManualOverride && !musicManualOverride && !rumbaManualOverride) {
        notifyClients("No conditions met, all automatic actions inactive");
        sendMQTTMessage(topic_action, "Action: No automatic actions triggered");
      }
    }
  }
}

// Broadcast a message to all WebSocket clients
void notifyClients(String message) {
  ws.textAll(message);
}

// Broadcast sensor data as JSON to all WebSocket clients
void broadcastSensorData(float temperature, float humidity, int lightIntensity) {
  String json = "{\"type\":\"sensor_data\",\"temperature\":";
  json += String(temperature, 1);
  json += ",\"humidity\":";
  json += String(humidity, 1);
  json += ",\"light\":";
  json += String(lightIntensity);
  json += "}";
  
  ws.textAll(json);
}

// Broadcast condition status as JSON to all WebSocket clients
void broadcastConditionStatus() {
  String json = "{\"type\":\"conditions\",\"coffee\":";
  json += coffeeConditionMet ? "true" : "false";
  json += ",\"music\":";
  json += musicConditionMet ? "true" : "false";
  json += ",\"rumba\":";
  json += rumbaConditionMet ? "true" : "false";
  json += ",\"darkness\":";
  json += (analogRead(LDR_PIN) == COMPLETE_DARKNESS) ? "true" : "false";
  json += "}";
  
  ws.textAll(json);
}

// AsyncWebSocket event handler
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    client->text("Connected to ESP32 Smart Home");
    
  } else if(type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  } else if(type == WS_EVT_DATA) {
    String msg = "";
    for(size_t i = 0; i < len; i++) {
      msg += (char)data[i];
    }
    msg.trim(); // Remove any extra whitespace/newline characters
    Serial.printf("Received WebSocket message: '%s'\n", msg.c_str());
    
    // Process regular commands
    if(msg == "start coffee") {
      coffeeManualOverride = true;
      coffeeBlinking = false;
      digitalWrite(LED_COFFEE, HIGH);
      client->text("Coffee started!");
      
    } else if(msg == "stop coffee") {
      coffeeManualOverride = true;
      coffeeBlinking = false;
      digitalWrite(LED_COFFEE, LOW);
      client->text("Coffee stopped!");
      
    } else if(msg == "start music") {
      musicManualOverride = true;
      musicBlinking = false;
      digitalWrite(LED_MUSIC, HIGH);
      client->text("Music started!");
      
    } else if(msg == "stop music") {
      musicManualOverride = true;
      musicBlinking = false;
      digitalWrite(LED_MUSIC, LOW);
      client->text("Music stopped!");
      
    } else if(msg == "start rumba") {
      rumbaManualOverride = true;
      rumbaBlinking = false;
      digitalWrite(LED_RUMBA, HIGH);
      client->text("Rumba started!");
      
    } else if(msg == "stop rumba") {
      rumbaManualOverride = true;
      rumbaBlinking = false;
      digitalWrite(LED_RUMBA, LOW);
      client->text("Rumba stopped!");
      
    } else if(msg == "auto mode") {
      // Reset all manual overrides
      coffeeManualOverride = false;
      musicManualOverride = false;
      rumbaManualOverride = false;
      client->text("Automatic mode activated - LEDs will respond to sensor conditions");
      
      // Force immediate condition check
      float temperature = dht.readTemperature();
      float humidity = dht.readHumidity();
      int lightIntensity = analogRead(LDR_PIN);
      checkSensorConditions(temperature, humidity, lightIntensity);
      broadcastConditionStatus();
      
    } else {
      client->text("Unknown command: " + msg);
    }
  }
}
