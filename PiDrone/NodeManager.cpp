#include "NodeManager.h"
#include "SensorData.h"
#include <iostream>
#include <iomanip>

void process_sensor_update(const std::string& esp_id, const TrackedSensor& sensor);
void process_drone_location(const Point& drone_pos);

NodeManager::NodeManager(std::string esp_id, DroneTracker& tracker)
    : esp_id_(esp_id), drone_tracker_(tracker), worker_(&NodeManager::process_loop, this) {}

NodeManager::~NodeManager() {
    stop_flag_ = true;
    cv_.notify_one();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void NodeManager::add_message(mqtt::const_message_ptr msg) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        msg_queue_.push(msg);
    }
    cv_.notify_one();
}

void NodeManager::process_loop() {
    while (!stop_flag_) {
        mqtt::const_message_ptr msg;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { return !msg_queue_.empty() || stop_flag_; });
            if (stop_flag_ && msg_queue_.empty()) return;
            msg = msg_queue_.front();
            msg_queue_.pop();
        }

        try {
            std::string topic = msg->get_topic();
            size_t last_slash = topic.find_last_of('/');
            std::string sensor_id = topic.substr(last_slash + 1);
            std::string full_sensor_id = esp_id_ + "/" + sensor_id;

            if (sensors_.find(sensor_id) == sensors_.end()) {
                sensors_.emplace(sensor_id, TrackedSensor(sensor_id));
            }

            auto& sensor = sensors_.at(sensor_id);
            auto data_json = nlohmann::json::parse(msg->get_payload_str());
            SensorData point = SensorData::fromJson(data_json);
            sensor.addDataPoint(point);

            process_sensor_update(esp_id_, sensor);

            if (point.presence) {
                auto calculated_pos = drone_tracker_.updateAndCalculate(full_sensor_id, point.range);

                if (calculated_pos) {
                    process_drone_location(*calculated_pos);
                }
            } else {
                drone_tracker_.clearSensorDistance(full_sensor_id);
            }


        } catch (const std::exception& e) {
            std::cerr << "Error in process_loop for node " << esp_id_ << ": " << e.what() << std::endl;
        }
    }
}