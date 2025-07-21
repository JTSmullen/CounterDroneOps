#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <memory>
#include <csignal>
#include "mqtt/async_client.h"
#include "NodeManager.h"
#include "DroneTracker.h"

const std::string MQTT_SERVER   = ""; // IP of your pi
const int         MQTT_PORT     = 1883;
const std::string MQTT_BASE_TOPIC = "drones/data";
const std::string MQTT_SUB_TOPIC  = MQTT_BASE_TOPIC + "/+/+";
const int         QOS           = 1;

const std::string FORE_GREEN    = "\033[32m";
const std::string FORE_YELLOW   = "\033[33m";
const std::string FORE_CYAN     = "\033[36m";
const std::string FORE_RED      = "\033[31m";
const std::string STYLE_BRIGHT  = "\033[1m";
const std::string STYLE_RESET   = "\033[0m";

std::unique_ptr<mqtt::async_client> g_client;
std::map<std::string, std::unique_ptr<NodeManager>> g_node_managers;
std::mutex g_map_mutex;

void process_sensor_update(const std::string& esp_id, const TrackedSensor& sensor) {
    SensorData latest = sensor.getLatestData();
    std::cout << FORE_CYAN << "UPDATE | ESP: " << std::left << std::setw(10) << esp_id
              << " | Sensor: " << std::left << std::setw(9) << sensor.getId()
              << std::fixed << std::setprecision(2)
              << " | Range: " << std::setw(6) << latest.range << " m"
              << " | Speed: " << std::setw(5) << latest.speed << " m/s"
              << STYLE_RESET << std::endl;
}

void process_drone_location(const Point& drone_pos) {
    std::cout << STYLE_BRIGHT << FORE_GREEN << ">>>>>> LOCATION (X,Y): (" 
              << std::fixed << std::setprecision(2) << std::setw(6) << drone_pos.x << ", " 
              << std::setw(6) << drone_pos.y << ")"
              << STYLE_RESET << std::endl;
}

class callback : public virtual mqtt::callback {
    DroneTracker& tracker_;

public:
    callback(DroneTracker& tracker) : tracker_(tracker) {}

    void connection_lost(const std::string& cause) override {
        std::cerr << FORE_RED << "\n---> Connection lost: " << cause << std::endl;
        exit(1);
    }

    void connected(const std::string& cause) override {
        std::cout << FORE_CYAN << "---> Successfully connected to MQTT Broker." << STYLE_RESET << std::endl;
        g_client->subscribe(MQTT_SUB_TOPIC, QOS);
        std::cout << FORE_CYAN << "---> Subscribed to '" << MQTT_SUB_TOPIC << "'. Waiting for data..." << STYLE_RESET << std::endl;
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        try {
            std::string topic = msg->get_topic();
            if (topic.find(MQTT_BASE_TOPIC) != 0) return;

            std::string topic_path = topic.substr(MQTT_BASE_TOPIC.length() + 1);
            size_t first_slash = topic_path.find('/');
            if (first_slash == std::string::npos) return;
            
            std::string esp_id = topic_path.substr(0, first_slash);

            std::lock_guard<std::mutex> lock(g_map_mutex);
            if (g_node_managers.find(esp_id) == g_node_managers.end()) {
                std::cout << STYLE_BRIGHT << FORE_YELLOW << "--> Discovered new ESP node: " << esp_id << STYLE_RESET << std::endl;
                g_node_managers[esp_id] = std::make_unique<NodeManager>(esp_id, tracker_);
            }
            g_node_managers[esp_id]->add_message(msg);

        } catch (const std::exception& e) {
            std::cerr << FORE_RED << "[ERROR] in message_arrived: " << e.what() << STYLE_RESET << std::endl;
        }
    }
};

void signal_handler(int signum) {
    std::cout << "\nCaught signal, shutting down..." << std::endl;
    g_node_managers.clear();
    if (g_client && g_client->is_connected()) {
        g_client->disconnect()->wait();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "--- Multi-Sensor Drone Tracker Initializing ---" << std::endl;

    // Define sensor positions here, This is dependant on your room layout / each person. You need to measure the distance in meters between your esp32s
    std::map<std::string, Point> sensor_positions = {
        {"esp32_1/radar_A", {0.0, 0.0}},
        {"esp32_2/radar_A", {5.0, 0.0}},
        {"esp32_3/radar_A", {2.5, 4.33}}
    };
    std::cout << "---> " << sensor_positions.size() << " sensor positions loaded for trilateration." << std::endl;

    DroneTracker tracker(sensor_positions);
    
    std::string server_address = "tcp://" + MQTT_SERVER + ":" + std::to_string(MQTT_PORT);
    g_client = std::make_unique<mqtt::async_client>(server_address, "drone_tracker_client");
    
    callback cb(tracker);
    g_client->set_callback(cb);

    mqtt::connect_options conn_opts;
    conn_opts.set_clean_session(true);

    try {
        g_client->connect(conn_opts)->wait();
    } catch (const mqtt::exception& exc) {
        std::cerr << FORE_RED << "---> CRITICAL: Could not connect to " << MQTT_SERVER << ". Error: " << exc.what() << STYLE_RESET << std::endl;
        return 1;
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    return 0;
}