/**
  * @file ESP32_Node_3.ino
  * @brief Main firmware for a single ESP32 sensor node.
  * @authors Joshua Smullen, Trent Tobias
  * @version 1.0
  * @date 2025-08-11
  *
  * This firmware runs on an ESP32 and manages two distinct radar sensors:
  * - An NRF24L01 transceiver to detect noise on a given channel by a drone's presence.
  * - An RCWL-0516 for simple presence detection.
  *
  * It uses an OOP approach to handle sensors and FreeRTOS to manage sensor polling
  * and network communication independently. Data is published to a central MQTT
  * broker in a structured JSON format.
*/

// --- Import Statements ---
#include <WiFi.h> // Connect to the wifi for mqtt communication
#include <PubSubClient.h> // mqtt communication library
#include <RF24.h> // nrf24 library

// --- Pin Definitions ---
// RCWL-0516 Simple Radar
#define RCWL_IN_PIN 13 // RCWL-0516 'OUT' PIN
// NRF24L01 Transceiver
#define CE_PIN 16
#define CSN_PIN 17
#define SCK_PIN 18
#define MOSI_PIN 23
#define MISO_PIN 19

// --- WiFi Config ---
const char* WIFI_SSID = ""; // Your WiFi network
const char* WIFI_PASSWORD = ""; // Your WiFi password

// --- MQTT Config ---
const char* MQTT_SERVER = ""; // Rasp Pi's IP
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "sensors/radar/status";

// --- Sensor Node Config ---
const int NODE_ID = 3; // sensor array/board ID
const uint8_t CHANNEL = 76; // channel for NRF24 to scan for your own drown

// --- Global Objects ---
RF24 radio(CE_PIN, CSN_PIN);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- FreeRTOS Handles ---
SemaphoreHandle_t mqttMutex;

// --- WiFi and MQTT connection Functions ---
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "RadarNode-" + String(NODE_ID);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// --- RTOS Task for RCWL-0516 Sensor ---
void rcwl_Task(void *pvParameters) {
  Serial.println("RCWL-0516 Task started");
  int lastState = LOW; // Track the previous state to only send changes
  
  for (;;) { // Infinite loop
    int currentState = digitalRead(RCWL_IN_PIN);
    
    // Check if the state has changed since the last reading
    if (currentState != lastState) {
      String payload = "";
      if (currentState == HIGH) {
        // Motion just started
        payload = "{\"nodeId\":" + String(NODE_ID) + 
                  ",\"sensorType\":\"RCWL-0516\"" + 
                  ",\"status\":\"motion_detected\"}";
      } else {
        // Motion just stopped
        payload = "{\"nodeId\":" + String(NODE_ID) + 
                  ",\"sensorType\":\"RCWL-0516\"" +
                  ",\"status\":\"no_motion\"}";
      }

      // Use the Mutex to publish to MQTT
      if (xSemaphoreTake(mqttMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("[RCWL] Publishing: " + payload);
        mqttClient.publish(MQTT_TOPIC, payload.c_str());
        xSemaphoreGive(mqttMutex); // Release
      }
      
      lastState = currentState; // Update the state
    }
    
    // Check for state changes frequently
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}

// --- RTOS Task for NRF24L01 Transceiver ---
void nrf24_Task(void *pvParameters) {
  Serial.println("NRF24L01 Task started");

  for(;;) {
    String payload = "";

    if (radio.testCarrier()) {  // If activity is present on channel that drone communicates on
      payload = "{\"nodeId\":" + String(NODE_ID) + 
                ",\"sensorType\":\"NRF24L01\"" + 
                ",\"status\":\"noise_detected\"}";
    } else {
      String payload = "";
      payload = "{\"nodeId\":" + String(NODE_ID) + 
                ",\"sensorType\":\"NRF24L01\"" + 
                ",\"status\":\"no_noise_detected\"}";
    }

    // Use the Mutex to publish to MQTT
    if (xSemaphoreTake(mqttMutex, portMAX_DELAY) == pdTRUE) {
      Serial.println("[NRF24] Publishing: " + payload);
      mqttClient.publish(MQTT_TOPIC, payload.c_str());
      xSemaphoreGive(mqttMutex); // Release
    }
  }

    // Wait before the next check to not overload the MQTT Broker
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
}

// --- Main Setup ---
void setup() {
  Serial.begin(115200);
  while (!Serial) {delay(10);}

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
  pinMode(RCWL_IN_PIN, INPUT);

  // --- Configure NRF24L01 Sensor ---
  // Set up to only detect activity on a channel; it doesn't translate, store, or transmit bytes
  radio.setAutoAck(false);                
  radio.setDataRate(RF24_2MBPS);          
  radio.setChannel(CHANNEL);
  radio.setCRCLength(RF24_CRC_DISABLED);  
  radio.disableDynamicPayloads();
  radio.setPayloadSize(32);
  radio.openReadingPipe(1, 0xFFFFFFFFEE);
  radio.powerUp();
  radio.startListening();
  
  setup_wifi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  mqttMutex = xSemaphoreCreateMutex();

  xTaskCreate(
    rcwl_Task, // Task Function
    "RCWL Task", // Name of the task
    2048, // Stack size
    NULL, // Task input Parameter
    1, // Task priority
    NULL // Task handle
  );

  xTaskCreate(
    nrf24_Task, // Task Function
    "NRF24 Task", // Name of the task
    2048, // Stack size
    NULL, // Task input Parameter
    1, // Task priority
    NULL // Task handle
  );

  Serial.println("Setupd complete. Tasks are running.");
}

void loop() {
  // This loop is now only responsible for maintaining the MQTT connection.
  // The sensor logic is handled by the RTOS tasks.
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop(); // Allow the MQTT client to process messages
  delay(10); // Small delay to yield to other tasks
}
