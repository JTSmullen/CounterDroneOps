#pragma once

#include <string>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "mqtt/async_client.h"
#include "SensorModel.h"
#include "DroneTracker.h"

class NodeManager {
private:
    std::string esp_id_;
    DroneTracker& drone_tracker_;
    std::map<std::string, TrackedSensor> sensors_;
    std::queue<mqtt::const_message_ptr> msg_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_flag_{false};
    std::thread worker_;

    void process_loop();

public:
    NodeManager(std::string esp_id, DroneTracker& tracker);
    ~NodeManager();

    void add_message(mqtt::const_message_ptr msg);
};