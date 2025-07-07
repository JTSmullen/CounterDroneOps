// --- Import Statements ---
#include <WiFi.h> // Connect to the wifi for mqtt communication
#include <PubSubClient.h> // mqtt communication library

// --- Pin Definitions ---
// RCWL-0516 Simple Radar
#define RCWL_IN_PIN 13 // RCWL-0516 'OUT' PIN

// --- WiFi Config ---
const char* WIFI_SSID = "TCBTDJ"; // Your WiFi network
const char* WIFI_PASSWORD = "wooD2Chair1!"; // Your WiFi password

// --- MQTT Config ---
const char* MQTT_SERVER = "192.168.68.64"; // Rasp Pi's IP
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "sensors/radar/status";

// --- Sensor Node Config ---
const int NODE_ID = 3; // sensor array/board ID

// --- Global Objects ---
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

// --- Main Setup ---
void setup() {
  Serial.begin(115200);
  while (!Serial) {delay(10);}

  pinMode(RCWL_IN_PIN, INPUT);
  
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