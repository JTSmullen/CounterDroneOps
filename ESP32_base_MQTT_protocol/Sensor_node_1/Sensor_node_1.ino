// --- Import Statements ---
#include <DFRobot_C4001.h> // Connect to the C4001 sensor
#include <WiFi.h> // Connect to the wifi for mqtt communication
#include <PubSubClient.h> // mqtt communication library

// --- Pin Definitions ---
// C4001 Advanced Radar
#define C4001_RX_PIN 19 // C4001 RX (C/R) PIN
#define C4001_TX_PIN 18 // C4001 TX (D/T) PIN

// --- WiFi Config ---
const char* WIFI_SSID = ""; // Wifi Network
const char* WIFI_PASSWORD = ""; // Wifi password

// --- MQTT config ---
const char* MQTT_SERVER = ""; // Raspberry Pi's IP address
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "sensors/radar/status";

// --- Sensor Node Config ---
const int NODE_ID = 1; // sensor array / board ID

// --- Global Objects ---
DFRobot_C4001_UART c4001_radar(&Serial1, 9600, C4001_RX_PIN, C4001_TX_PIN); // C4001 declaration
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- FreeRTOS Handles ---
SemaphoreHandle_t mqttMutex;

// --- WiFi and MQTT connection functions ---
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

// --- RTOS Task for C4001 ---
void c4001_Task(void *pvParameters) {
  Serial.println("C4001 Task started");
  for (;;) { // Infinite loop
    String payload = "";
    // Check if the radar sensor detects a target
    if (c4001_radar.getTargetNumber() > 0) {
      // Motion detected, get data from the sensor
      int targetNum = c4001_radar.getTargetNumber();
      float targetRange = c4001_radar.getTargetRange();
      float targetSpeed = c4001_radar.getTargetSpeed();

      // Create a detailed JSON Payload, identifying the sensor type
      payload = "{\"nodeId\":" + String(NODE_ID) +
                ",\"sensorType\":\"C4001\"" + 
                ",\"status\":\"motion_detected\"" +
                ",\"targetNumber\":" + String(targetNum) +
                ",\"range_cm\":" + String(targetRange) +
                ",\"speed_m_s\":" + String(targetSpeed) + "}";
    } else {
      // No motion is detected
      payload = "{\"nodeId\":" + String(NODE_ID) + 
                ",\"sensorType\":\"C4001\"" +
                ",\"status\":\"no_motion\"}";
    }
    
    // Use the Mutex to publish to MQTT
    if (xSemaphoreTake(mqttMutex, portMAX_DELAY) == pdTRUE) {
      Serial.println("[C4001] Publishing: " + payload);
      mqttClient.publish(MQTT_TOPIC, payload.c_str());
      xSemaphoreGive(mqttMutex); // Release
    }
    
    // Wait before the next check to not overload the MQTT Broker
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {delay(10);}

  // Init Hardware
  while (!c4001_radar.begin()) {
    Serial.println("C4001 not detected! Retrying...");
    delay(1000);
  }
  Serial.println("C4001 Connected!");

  c4001_radar.setSensorMode(eSpeedMode);
  c4001_radar.setDetectThres(60, 1200, 10);
  c4001_radar.setDetectionRange(60, 1200, 1200);
  c4001_radar.setTrigSensitivity(3);
  c4001_radar.setKeepSensitivity(1);

  // --- Network and MQTT Setup ---
  setup_wifi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  // --- Create RTOS ---
  // create a mutex for MQTT publishing
  mqttMutex = xSemaphoreCreateMutex();

  // Create the tasks, one for each type of sensor
  xTaskCreate(
    c4001_Task, // Task Function
    "C4001 Task", // Name of the Task
    4096, // Stack size
    NULL, // Task input Parameter
    1, // Priority
    NULL // Task Handle
  );

  Serial.println("Setup Complete, Tasks are running.");
}

// --- Main loop ---
void loop() {
  // this loop is now only respondible for maintaining the MQTT connection.
  // The sensor logic is handled by the RTOS tasks.
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop(); // allow the mqtt client to process messages
  delay(10);
}
