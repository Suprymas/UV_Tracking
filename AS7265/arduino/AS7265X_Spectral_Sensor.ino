#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include "SparkFun_AS7265X.h"  // SparkFun AS7265x spectral sensor library

// ========== Configuration ==========
// Change these to match your actual wiring (ESP32-C3 example pins)
#define I2C_SDA 8
#define I2C_SCL 9

// WiFi Configuration
const char* WIFI_SSID = "Internetas";        // Change to your WiFi SSID
const char* WIFI_PASSWORD = "12345678";      // Change to your WiFi password

// WebSocket Configuration
const char* WS_HOST = "10.31.204.51";        // WebSocket server IP address
const int WS_PORT = 8765;                     // WebSocket server port
const char* WS_PATH = "/";                    // WebSocket path (root path)
const char* WS_PROTOCOL = "ws://";            // Protocol: ws:// (plain) or wss:// (secure)

// Sampling interval
unsigned long samplingInterval = 500;         // Default: 500ms (0.5 seconds)
unsigned long lastSampleTime = 0;

AS7265X sensor;
WebSocketsClient webSocket;

// ========== WebSocket Functions ==========
void sendSpectralDataOverWebSocket(float data[18]) {
  if (!webSocket.isConnected()) {
    return;  // Not connected, skip sending
  }

  // Create JSON message with 18-channel spectral data
  // Format matches server expectations from WebSocket.py
  StaticJsonDocument<512> doc;
  doc["type"] = "spectral_data";
  doc["device_id"] = "esp32-as7265x";
  
  // Add all 18 channels with wavelength labels
  JsonArray channels = doc.createNestedArray("channels");
  const char* channelNames[] = {"A", "B", "C", "D", "E", "F", "G", "H", "R", "I", "S", "J", "T", "U", "V", "W", "K", "L"};
  const int wavelengths[] = {410, 435, 460, 485, 510, 535, 560, 585, 610, 645, 680, 705, 730, 760, 810, 860, 900, 940};
  
  for (int i = 0; i < 18; i++) {
    JsonObject channel = channels.createNestedObject();
    channel["name"] = channelNames[i];
    channel["wavelength_nm"] = wavelengths[i];
    channel["value"] = data[i];
  }
  
  doc["timestamp"] = millis();
  
  // Serialize JSON to string
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send via WebSocket
  webSocket.sendTXT(jsonString);
  
  Serial.print("Sent: ");
  Serial.println(jsonString);
}

void handleWebSocketMessages(String message) {
  Serial.print("Received: ");
  Serial.println(message);
  
  // Parse incoming JSON message
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Check message type
  const char* type = doc["type"];
  
  if (strcmp(type, "command") == 0) {
    // Handle commands from server
    const char* action = doc["action"];
    
    if (strcmp(action, "read_sensor") == 0) {
      // Server requested immediate reading
      float ch[18];
      sensor.takeMeasurementsWithBulb();
      ch[0] = sensor.getCalibratedA(); ch[1] = sensor.getCalibratedB();
      ch[2] = sensor.getCalibratedC(); ch[3] = sensor.getCalibratedD();
      ch[4] = sensor.getCalibratedE(); ch[5] = sensor.getCalibratedF();
      ch[6] = sensor.getCalibratedG(); ch[7] = sensor.getCalibratedH();
      ch[8] = sensor.getCalibratedR(); ch[9] = sensor.getCalibratedI();
      ch[10] = sensor.getCalibratedS(); ch[11] = sensor.getCalibratedJ();
      ch[12] = sensor.getCalibratedT(); ch[13] = sensor.getCalibratedU();
      ch[14] = sensor.getCalibratedV(); ch[15] = sensor.getCalibratedW();
      ch[16] = sensor.getCalibratedK(); ch[17] = sensor.getCalibratedL();
      sendSpectralDataOverWebSocket(ch);
    }
    else if (strcmp(action, "set_interval") == 0) {
      // Change sampling interval
      if (doc.containsKey("interval_ms")) {
        samplingInterval = doc["interval_ms"];
        Serial.print("Sampling interval set to: ");
        Serial.print(samplingInterval);
        Serial.println(" ms");
      }
    }
    else if (strcmp(action, "bulb_on") == 0) {
      Serial.println("Bulb ON command received");
      // You can add bulb control here if needed
    }
    else if (strcmp(action, "bulb_off") == 0) {
      Serial.println("Bulb OFF command received");
      // You can add bulb control here if needed
    }
  }
  else if (strcmp(type, "ack") == 0) {
    Serial.println("Acknowledgment received from server");
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket disconnected");
      break;
      
    case WStype_CONNECTED:
      Serial.print("WebSocket connected to: ");
      Serial.println((char*)payload);
      
      // Send status message on connection
      {
        StaticJsonDocument<128> doc;
        doc["type"] = "status";
        doc["message"] = "ESP32-C3 AS7265X connected";
        doc["device_id"] = "esp32-as7265x";
        String jsonString;
        serializeJson(doc, jsonString);
        webSocket.sendTXT(jsonString);
      }
      break;
      
    case WStype_TEXT:
      handleWebSocketMessages((char*)payload);
      break;
      
    case WStype_ERROR:
      Serial.print("WebSocket error: ");
      Serial.println((char*)payload);
      break;
      
    default:
      break;
  }
}

// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-C3 + AS7265x starting...");

  // Initialize WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize WebSocket
  // Build WebSocket URL: ws://IP:PORT/PATH
  String wsUrl = String(WS_PROTOCOL) + String(WS_HOST) + ":" + String(WS_PORT) + String(WS_PATH);
  Serial.print("Connecting to WebSocket: ");
  Serial.println(wsUrl);
  
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);  // Reconnect every 5 seconds if disconnected

  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize AS7265x sensor
  if (sensor.begin() == false) {
    Serial.println("AS7265x not detected. Check wiring.");
    while (1) {
      delay(1000);  // Stay here if sensor is not detected
    }
  }

  // Optional: disable indicator LED on the board
  sensor.disableIndicator();
  Serial.println("AS7265x initialized.");
  Serial.println("Channels: A,B,C,D,E,F,G,H,R,I,S,J,T,U,V,W,K,L");
  Serial.println("Ready to send spectral data via WebSocket!");
}

void loop() {
  // Handle WebSocket events (must be called regularly)
  webSocket.loop();
  
  // Check if it's time to take a sample
  unsigned long currentTime = millis();
  if (currentTime - lastSampleTime >= samplingInterval) {
    lastSampleTime = currentTime;
    
    // Take measurements using the onboard bulb
    sensor.takeMeasurementsWithBulb();  
    // If you don't want to use the bulb, use: sensor.takeMeasurements();

    float ch[18];

    // Read the 18 calibrated spectral channels (410–940 nm)
    ch[0]  = sensor.getCalibratedA(); // 410 nm
    ch[1]  = sensor.getCalibratedB(); // 435 nm
    ch[2]  = sensor.getCalibratedC(); // 460 nm
    ch[3]  = sensor.getCalibratedD(); // 485 nm
    ch[4]  = sensor.getCalibratedE(); // 510 nm
    ch[5]  = sensor.getCalibratedF(); // 535 nm
    ch[6]  = sensor.getCalibratedG(); // 560 nm
    ch[7]  = sensor.getCalibratedH(); // 585 nm
    ch[8]  = sensor.getCalibratedR(); // 610 nm
    ch[9]  = sensor.getCalibratedI(); // 645 nm
    ch[10] = sensor.getCalibratedS(); // 680 nm
    ch[11] = sensor.getCalibratedJ(); // 705 nm
    ch[12] = sensor.getCalibratedT(); // 730 nm
    ch[13] = sensor.getCalibratedU(); // 760 nm
    ch[14] = sensor.getCalibratedV(); // 810 nm
    ch[15] = sensor.getCalibratedW(); // 860 nm
    ch[16] = sensor.getCalibratedK(); // 900 nm
    ch[17] = sensor.getCalibratedL(); // 940 nm

    // Print all 18 channels as one CSV line (easy to parse later)
    for (int i = 0; i < 18; i++) {
      Serial.print(ch[i], 4);
      if (i < 17) Serial.print(", ");
    }
    Serial.println();

    // Send data through WebSocket
    sendSpectralDataOverWebSocket(ch);
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

