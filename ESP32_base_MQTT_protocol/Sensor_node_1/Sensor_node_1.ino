// --- Import Statements ---
#include <DFRobot_C4001.h> // Connect to the C4001 sensor
#include <WiFi.h> // Connect to the wifi for mqtt communication
#include <PubSubClient.h> // mqtt communication library

// --- PIN DEFINITIONS ---
#define RX_PIN 19 // C4001 RX (C/R) PIN
#define TX_PIN 18 // C4001 TX (D/T) PIN

// --- WIFI CONFIG ---
const char* WIFI_SSID = ""; // Your WiFi network
const char* WIFI_PASSWORD = ""; // Your WiFi password

// --- MQTT CONFIG ---
const char* MQTT_SERVER = ""; // Your Raspberry Pi's IP address on your home network
// To get the this run 'hostname -I' on your Pi's terminal
const int MQTT_PORT = 1883; // General port for MQTT communication

// --- SENSOR NODE CONFIG ---
const int NODE_ID = 1; // Which sensor node of the array
const char* MQTT_TOPIC = "sensors/radar/status"; // Topic to publish to

// --- GLOBAL OBJECTS ---
DFRobot_C4001_UART radar(&Serial1, 9600, RX_PIN, TX_PIN);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- Connect to WiFi ---
void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connection to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() 
  {
  while (!mqttClient.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "RadarNode-" + String(NODE_ID);

    if (mqttClient.connect(clientId.c_str())) 
    {
      Serial.println("connected!");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void publishMotionData()
{
  String payload = "{\"nodeId\":" + String(NODE_ID) + ",\"status\":\"motion_detected\"}";

  Serial.print("Publishing message to topic: ");
  Serial.println(MQTT_TOPIC);
  Serial.println(payload);

  mqttClient.publish(MQTT_TOPIC, payload.c_str());
}

void publishStatus(const char* status)
{
  String payload = "{\"nodeId\":" + String(NODE_ID) + ",\"status\":\"" + status + "\"}";
  mqttClient.publish(MQTT_TOPIC, payload.c_str());
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10);
  }
  
  // connect to WiFi
  setup_wifi();

  // Config MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  // init radar
  while (!radar.begin())
  {
    Serial.println("C4001 not detected!");
    delay(100);
  }
  Serial.println("C4001 connected!");

  radar.setSensorMode(eExitMode);

  sSensorStatus_t status = radar.getStatus();
  Serial.print("Work status: ");
  Serial.println(status.workStatus);
  Serial.print("Work Mode: ");
  Serial.println(status.workMode);
  Serial.print("Init status: ");
  Serial.print(status.initStatus);
}

void loop()
{
  if (!mqttClient.connected())
  {
    reconnect_mqtt();
  }

  mqttClient.loop();

  if (radar.motionDetection())
  {
    publishStatus("motion_detected");
  }
  else
  {
    publishStatus("no_motion");
  }

  delay(1000);
}