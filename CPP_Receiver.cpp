// Compile on the Pi with g++

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <memory>
#include <atomic>
#include <cstdlib>
#include "mqtt/async_client.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

const std::string MQTT_SERVER   = ""; // IP address of your PI
const int         MQTT_PORT     = 1883;
const std::string MQTT_TOPIC    = "sensors/radar/status";
const int         QOS           = 1;

const std::string FORE_GREEN    = "\033[32m";
const std::string FORE_YELLOW   = "\033[33m";
const std::string FORE_BLUE     = "\033[34m";
const std::string FORE_MAGENTA  = "\033[35m";
const std::string FORE_CYAN     = "\033[36m";
const std::string FORE_RED      = "\033[31m";
const std::string STYLE_BRIGHT  = "\033[1m";
const std::string STYLE_RESET   = "\033[0m";

const std::vector<std::string> NODE_COLORS = {FORE_GREEN, FORE_YELLOW, FORE_BLUE, FORE_MAGENTA, FORE_CYAN};

void process_message_data(const std::string& payload, int node_id);

class NodeProcessor {
private:
    int node_id_;
    std::queue<std::string> msg_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_flag_{false};
    std::thread worker_;

    void process_loop() {
        while (!stop_flag_) {
            std::string payload;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [this] { return !msg_queue_.empty() || stop_flag_; });

                if (stop_flag_ && msg_queue_.empty()) {
                    return;
                }

                payload = msg_queue_.front();
                msg_queue_.pop();
            }
            process_message_data(payload, node_id_);
        }
    }

public:
    NodeProcessor(int node_id) : node_id_(node_id), worker_(&NodeProcessor::process_loop, this) {
        std::cout << FORE_CYAN << "--> Created dedicated processing thread for NODE ID: " << node_id_ << STYLE_RESET << std::endl;
    }

    ~NodeProcessor() {
        stop_flag_ = true;
        cv_.notify_one();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    void add_message(const std::string& msg) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            msg_queue_.push(msg);
        }
        cv_.notify_one();
    }
};

std::map<int, std::unique_ptr<NodeProcessor>> g_node_processors;
std::mutex g_map_mutex;

class callback : public virtual mqtt::callback {
public:
    void connection_lost(const std::string& cause) override {
        std::cout << FORE_RED << "\n---> Connection lost" << std::endl;
        if (!cause.empty()) {
            std::cout << FORE_RED << "\tcause: " << cause << std::endl;
        }
    }

    void connected(const std::string& cause) override {
        std::cout << FORE_CYAN << "---> Successfully connected to MQTT Broker at " << MQTT_SERVER << STYLE_RESET << std::endl;
        std::cout << FORE_CYAN << "---> Subscribed to topic '" << MQTT_TOPIC << "'. Waiting for messages..." << STYLE_RESET << std::endl;
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        try {
            json data = json::parse(msg->get_payload_str());
            if (!data.contains("nodeId")) {
                std::cout << FORE_YELLOW << "[WARNING] Malformed message (no nodeId): " << msg->get_payload_str() << STYLE_RESET << std::endl;
                return;
            }
            int node_id = data["nodeId"];

            std::lock_guard<std::mutex> lock(g_map_mutex);
            if (g_node_processors.find(node_id) == g_node_processors.end()) {
                g_node_processors[node_id] = std::make_unique<NodeProcessor>(node_id);
            }
            g_node_processors[node_id]->add_message(msg->get_payload_str());

        } catch (const std::exception& e) {
            std::cout << FORE_RED << "[ERROR] in message_arrived: " << e.what() << STYLE_RESET << std::endl;
        }
    }
};

void process_message_data(const std::string& payload, int node_id) {
    try {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss_ts;
        ss_ts << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        std::string timestamp = ss_ts.str();

        json data = json::parse(payload);
        
        if (!data.contains("status")) {
            std::cout << FORE_YELLOW << "[WARNING] Malformed message received: " << payload << STYLE_RESET << std::endl;
            return;
        }

        std::string status = data["status"];
        std::string sensor_type = data.value("sensorType", "Unknown");

        std::string color = NODE_COLORS[(node_id - 1) % NODE_COLORS.size()];
        
        std::cout << color << "NODE ARRAY ID: " << node_id
                  << " | SENSOR: " << std::left << std::setw(9) << sensor_type << " |";
        
        if (status == "motion_detected") {
            std::string status_text = "Presence Detected";
            std::cout << " " << STYLE_BRIGHT << std::left << std::setw(17) << status_text << STYLE_RESET;

            if (sensor_type == "C4001" && data.contains("range_cm") && data.contains("speed_m_s")) {
                double range_cm = data["range_cm"];
                double speed_ms = data["speed_m_s"];
                std::cout << std::fixed << std::setprecision(1)
                          << "| Range: " << range_cm << " cm"
                          << std::setprecision(2)
                          << " | Speed: " << speed_ms << " m/s";
            }
            std::cout << " | Time: " << timestamp << STYLE_RESET << std::endl;

        } else if (status == "no_motion") {
            std::string status_text = "No Presence";
            std::cout << " " << std::left << std::setw(17) << status_text
                      << " | Time: " << timestamp << STYLE_RESET << std::endl;
        }

    } catch (const json::parse_error& e) {
        std::cout << FORE_YELLOW << "[WARNING] Could not decode JSON from payload: " << payload << STYLE_RESET << std::endl;
    } catch (const std::exception& e) {
        std::cout << FORE_RED << "[ERROR] An error occurred in process_message_data: " << e.what() << STYLE_RESET << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::string server_address = "tcp://" + MQTT_SERVER + ":" + std::to_string(MQTT_PORT);
    
    std::cout << "--- Multi-Sensor MQTT Logger Initializing ---" << std::endl;

    if (MQTT_SERVER.empty()) {
        std::cerr << FORE_RED << "---> CRITICAL: MQTT_SERVER IP address is not set. Please edit the source file." << STYLE_RESET << std::endl;
        return 1;
    }

    mqtt::async_client client(server_address, "cpp_multithread_client");
    callback cb;
    client.set_callback(cb);

    mqtt::connect_options conn_opts;
    conn_opts.set_clean_session(true);

    try {
        client.connect(conn_opts)->wait();
        client.subscribe(MQTT_TOPIC, QOS)->wait();
    } catch (const mqtt::exception& exc) {
        std::cerr << FORE_RED << "---> CRITICAL: Could not connect to " << MQTT_SERVER << ". Error: " << exc.what() << STYLE_RESET << std::endl;
        return 1;
    }

    std::cout << "--- Main thread is now idle. Press Ctrl+C to exit. ---" << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    return 0;
}
