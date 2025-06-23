// --- Import Statements ---
#include <WiFi.h>
#include <PubSubClient.h>
#include <DFRobot_C4001.h>
// --- End of Import Statements ---

// --- Pin Definitions ---
#define RX_PIN 19 // C4001 RX (C/R) PIN
#define TX_PIN 18 // C4001 TX (C/T) PIN
// --- End of Pin Definitions ---

// --- WiFi and MQTT Config ---
const char* WIFI_SSID = ""; // Your WiFi Network
const char* WIFI_PASSWORD = ""; // Your WiFi Password

const char* MQTT_SERVER = ""; // Your Raspberry Pi's IP
const int MQTT_PORT = 1883; // Default
// --- End of WiFi and MQTT Config ---

// --- Sensor Node Config ---
const int NODE_ID = 2; // Which sensor node of the array
const char* MQTT_TOPIC = "sensors/radar/status"; // Topic to publish to
// --- End of Sensor Node Config ---

// --- Global Variables ---
DFRobot_C4001_UART radar(&Serial1, 9600, RX_PIN, TX_PIN);
WiFiClient espClient;
PubSubClient mqttClient(espClient);
// --- End of Global Variables ---

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
    delay(10); // Short delay to allow serial monitor to init
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID); // Print which SSID you are connecting to 

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Connect to the WiFi network

    // Continuously attempt to connect until successful
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // Once connected, print the IP address
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
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
    // Loop until the connection is established
    while (!mqttClient.connected())
    {
        // Attempt to connect to the MQTT broker
        Serial.print("Attempting MQTT connection...");
        String clientId = "RadarNode-" + String(NODE_ID);

        // If connected, print success, otherwise print failure
        // and wait 5 seconds before retrying
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

    * @return void
*/
void publishMotionData()
{
    // Create a JSON payload with the node ID and status
    String payload = "{\"nodeId\": " + String(NODE_ID) + ", \"status\":\"motion_detected\"}";

    // Print the topic and payload to the serial monitor for debugging
    Serial.print("Publishing to topic: ");
    Serial.println(MQTT_TOPIC);
    Serial.print("Payload: ");
    Serial.println(payload);

    // Publish the payload to the MQTT topic
    mqttClient.publish(MQTT_TOPIC, payload.c_str());
}

/*
    * @name publishStatus

    * @brief Function to publish the current status of the sensor node.

    * @details This function will create a JSON payload with the node ID and status, then publish it to the specified MQTT topic.

    * @note This function should be called periodically to update the status.

    * @return void
*/
void publishStatus()
{
    // Generate JSON formatted payload
    String payload = "{\"nodeId\":" + String(NODE_ID) + ",\"status\":\"" + status + "\"}";

    // Publish the payload to the MQTT topic
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

    // Ensure the serial port is ready
    while (!Serial)
    {
        delay(10);
    }

    // Initializes WiFi connection
    setup_wifi();

    // Initializes MQTT client
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

    // Initializes the radar sensor and ensures its connected
    while(!radar.begin())
    {
        Serial.println("C4001 not detected");
        delay(1000);
    }
    Serial.println("C4001 Connected!");

    // Set the radar sensor mode
    radar.setSensorMode(eExitMode);

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

    // Loop the MQTT client
    mqttClient.Loop();

    // Check if the radar sensor detects motion
    if (radar.motionDetection())
    {
        // Yes motion detected
        publishStatus("motion_detected");
    }
    else
    {
        // No motion detected
        publishStatus("no_motion");
    }

    delay(1000); // Delay to not overload the MQTT broker
}