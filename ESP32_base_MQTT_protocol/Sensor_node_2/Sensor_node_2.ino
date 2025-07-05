// --- Import Statements ---
#include <DFRobot_C4001.h> // Connect to the C4001 sensor
#include <WiFi.h> // Connect to the wifi for mqtt communication
#include <PubSubClient.h> // mqtt communication library
// --- End of Import Statements

// --- PIN DEFINITIONS ---
#define RX_PIN 19 // C4001 RX (C/R) PIN
#define TX_PIN 18 // C4001 TX (D/T) PIN
// --- End of Pin Definitions ---

// --- WIFI and MQTT Config ---
const char* WIFI_SSID = ""; // Your WiFi network
const char* WIFI_PASSWORD = ""; // Your WiFi password

// --- MQTT CONFIG ---
const char* MQTT_SERVER = ""; // Your Raspberry Pi's IP address on your home network
// To get the this run 'hostname -I' on your Pi's terminal
const int MQTT_PORT = 1883; // General port for MQTT communication
// ---End of WiFi and MQTT Config ---

// --- SENSOR NODE CONFIG ---
const int NODE_ID = 2; // Which sensor node of the array
const char* MQTT_TOPIC = "sensors/radar/status"; // Topic to publish to
// --- End of Sensor Node Config ---

// --- GLOBAL OBJECTS ---
DFRobot_C4001_UART radar(&Serial1, 9600, RX_PIN, TX_PIN);
WiFiClient espClient;
PubSubClient mqttClient(espClient);
// --- End of Global Objects ---

// --- Functions ---
/*
    * @name setup_wifi

    * @brief Function to connect to WiFi.

    * @details This function will connect to the specified WiFi network using the provided SSID

    * @note This function will continuously attempt until it connects.
    * @note This function will block until the connection is established.
    * @note This function should be called in the setup() function.

    * @return void
*/
void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
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

/*
    * @name reconnect_mqtt

    * @brief Function to connect to the MQTT broker.

    * @details This function will connect to the MQTT broker using the provided server and port.

    * @note This function will continuously attempt until it connects.
    * @note This function will block until the connection is established.
    * @note This function should be called in the loop() function.

    * @return void
*/
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

/*
    * @name publishMotionData

    * @brief Function to publish motion data to the MQTT broker.

    * @details This function will create a JSON payload with the node ID and status, then publish it to the specified MQTT topic.

    * @note This function should be called when motion is detected.
    * @note This functions is used to send a no motion detected Json Payload

    * @return void
*/
void publishStatus(const char* status)
{
  String payload = "{\"nodeId\":" + String(NODE_ID) + ",\"status\":\"" + status + "\"}";
  
  Serial.print("Publishing message to topic: ");
  Serial.println(MQTT_TOPIC);
  Serial.println(payload);
  
  mqttClient.publish(MQTT_TOPIC, payload.c_str());
}

/*
    * @name setup

    * @brief Function to initialize the sensor node.

    * @details This function will set up the serial communication, connect to WiFi, and initialize the MQTT client.

    * @note This function should be called once at the start of the program.

    * @return void
*/
void setup()
{
  // Baud rate for serial communication
  Serial.begin(115200);

  // Ensure the Serial port is ready
  while (!Serial)
  {
    delay(10);
  }
  
  // Initializes WiFi connection
  setup_wifi();

  // Initializes MQTT Client
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  // Initializes the radar sensor and ensures its connected
  while (!radar.begin())
  {
    Serial.println("C4001 not detected!");
    delay(100);
  }
  Serial.println("C4001 connected!");

  radar.setSensorMode(eSpeedMode);

  radar.setDetectThres(60, 1200, 50); // range an object that is detected
  radar.setDetectionRange(60, 1200, 1200); // Range the c4001 scans in for motion (in cms)
  radar.setTrigSensitivity(5); // Trigger Sensitivity
  radar.setKeepSensitivity(5); // Keep-locked-on sensitivity (determines when a moving object is no longer detected)

  // Print radar sensor status
  sSensorStatus_t status = radar.getStatus();
  Serial.print("Work status: ");
  Serial.println(status.workStatus);
  Serial.print("Work Mode: ");
  Serial.println(status.workMode);
  Serial.print("Init status: ");
  Serial.print(status.initStatus);
}

/*
    * @name loop

    * @brief Main loop function to handle sensor data and MQTT communication.

    * @details This function will continuously check for motion detection, publish status updates, and maintain the MQTT connection.

    * @note This function should be called repeatedly in the main program loop.

    * @return void
*/
void loop()
{
  // Ensure the mqtt client is connected
  // if not try to reconnect
  if (!mqttClient.connected())
  {
    reconnect_mqtt();
  }

  // Loop the MQTT Client
  mqttClient.loop();

  // Check if the radar sensor detects motion
  if (radar.getTargetNumber() > 0)
  {
    // Motion detected, get data from the sensor
    int targetNum = radar.getTargetNumber();
    float targetRange = radar.getTargetRange();
    float targetSpeed = radar.getTargetSpeed();

    // Create a detailed JSON Payload to send the receiver
    String payload = "{\"nodeId\":" + String(NODE_ID) +
                     ",\"status\":\"motion_detected\"" +
                     ",\"targetNumber\":" + String(targetNum) +
                     ",\"range_cm\":" + String(targetRange) +
                     ",\"speed_m_s\":" + String(targetSpeed) + "}";

    // Print the publishing message
    Serial.print("Publishing message to topic: ");
    Serial.println(MQTT_TOPIC);
    Serial.println(payload);
    
    mqttClient.publish(MQTT_TOPIC, payload.c_str());
  }
  else
  {
    // No motion is detected
    publishStatus("no_motion");
  }

  delay(1000); // Check for motion every 1 second to not overload the MQTT Broker
}
