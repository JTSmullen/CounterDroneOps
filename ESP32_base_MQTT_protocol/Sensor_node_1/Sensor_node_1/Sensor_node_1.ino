/**
  * @file ESP32_Node_1.ino
  * @brief Main firmware for a single ESP32 sensor node.
  * @author Joshua Smullen
  * @version 1.0
  * @date 2025-07-22
  *
  * This firmware runs on an ESP32 and manages two distinct radar sensors:
  * - C4001 DFRobot for range and speed detection.
  * - An RCWL-0516 for simple presence detection.
  *
  * It uses an OOP approach to handle sensors and FreeRTOS to manage sensor polling
  * and network communication independently. Data is published to a central MQTT
  * broker in a structured JSON format.
*/

// --- Dependencies ---
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Sensors.h" // custom header with sensor class implementations
// --- End dependencies ---

// --- Hardware pin definitions ---
#define C4001_RX_PIN 19 // C4001 RX Pin
#define C4001_TX_PIN 18 // C4001 TX PIn
#define RCWL_IN_PIN 13 // RCWL-0516 'OUT' Pin
// --- End Hardware pin definitions ---

// --- WiFi Config ---
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";
// --- End WiFI config ---

// --- MQTT config ---
const char* MQTT_SERVER = ""; // rasp pi IP
const int MQTT_PORT = 1883; // base mqtt port
// --- End MQTT config

// --- Node Config ---
// Each ESP32 must have a unique Identifier
const char* ESP_ID = "esp32_1";
// --- End Node Config ---

// --- Sensor Config ---
const int NUM_SENSORS = 2;
DroneSensor* sensors[NUM_SENSORS]; // Array of pointers to base sensor classes
// --- End sensor config ---

// --- Global Objects ---
WiFiClient espClient;
PubSubClient mqttClient(espClient);
// --- End Global Objects ---

// --- Functions ---

/**
  * @brief Connects the ESP32 to the wifi network
*/
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

/**
  * @brief Connects or reconnects to the MQTT broker.
*/
void reconnect_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Node-";
    clientId += ESP_ID;
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

/**
  * @brief FreeRTOS task to poll all sensors and publish data.
  *
  * This task runs in a infinite loop, independtly of the main loop().
  * It iterates through the global sensors array, reads data from each
  * and publishes any state changes to the MQTT broker.
  *
  * @param pvParameters Standard FreeRTOS task parameter (unused)
*/
void sensorProcessingTask(void *pvParameters) {
  Serial.println("Sensor Processing Task started.");
  for (;;) { // infinite loop
    for (int i = 0; i < NUM_SENSORS; i++) {
      if (sensors[i]->readData()) {
        // 1 create the MQTT topic string
        String topic = "drones/data/";
        topic += ESP_ID;
        topic += "/";
        topic += sensors[i]->getSensorId();

        // 2 Buld the payload
        StaticJsonDocument<256> doc;
        sensors[i]->buildJsonPayload(doc);
        
        char payload[256];
        serializeJson(doc, payload);

        // 3 publish data
        Serial.print("Publishing to ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(payload);
        mqttClient.publish(topic.c_str(), payload);
      }
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// --- Setup and Loop functions ---

/**
  * @brief Main setup function, run once at boot.
*/
void setup() {
  Serial.begin(115200);
  while (!Serial) {delay(10);}

  Serial.println("--- Drone SEnsor Node Booting ---");
  Serial.print("Node ID: ");
  Serial.println(ESP_ID);

  // init our radar objects
  sensors[0] = new RadarC4001("radar_C4001", Serial1, 9600, C4001_RX_PIN, C4001_TX_PIN);
  sensors[1] = new RadarRCWL("radar_RCWL", RCWL_IN_PIN);

  // init sensors
  for (int i = 0; i < NUM_SENSORS; i++) {
    if(sensors[i]->initialize()){
      Serial.printf("Sensor '%s' initialized successfully\n", sensors[i]->getSensorId());
    } else {
      Serial.printf("!!! FAILED to initialize sensor '%s'. Halting.\n", sensors[i]->getSensorId());
      while(1);
    }
  }

  setup_wifi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  xTaskCreate(
    sensorProcessingTask,
    "SensorTask",
    4096,
    NULL,
    1,
    NULL
  );

  Serial.println("Setup complete. Main loop is now handling MQTT connection.");
}

/**
  * @brief Main loop, runs infinitely after setup.
  *
  * Its only job is to maintain MQTT connection.
  * Sensor logic is handled by the dedicated RTOS task.
*/
void loop() {
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop();
  vTaskDelay(10 / portTICK_PERIOD_MS); // Task delay to yield cpu usage
}



























