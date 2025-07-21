#pragma once

// --- Import Statements ---
#include <string>
#include <deque>
#include <numeric> // For std:: accumulate
#include "nlohmann/json.hpp"
#include <iostream>

struct Point {
    double x = 0.0;
    double y = 0.0;
};

struct SensorData {
    double range = 0.0;
    double speed = 0.0;
    long long timestamp_ms = 0;

    static SensorData from_json(const nlohmann::json& j) {
        SensorData d;
        d.range = j.value("range", d.range);
        d.speed = j.value("speed", d.speed);
        d.timestamp_ms = j.value("ts", 0LL);
        return d;
    }
};

class TrackedSensor {
    private:
        std::string id_;
        std::deque<SensorData> history_;
        const size_t max_history_size_;

    public:
        TrackedSensor(std::string id, size_t history_size = 20)
            : id_(id), max_history_size_(history_size) {}

        const std::string& getId() const { return id_; }
        const SensorData& getLatestData() const { return history_.front(); }
        
        void addDataPoint(const SensorData& data) {
            history_.push_front(data);
            if (history_.size() > max_history_size_) {
                history_.pop_back();
            }
        }

        double getAverageSpeed() const {
            if (history_.empty()) return 0.0;
            double sum = std::accumulate(history_.begin(), history_.end(), 0.0,
                [](double sum, const SensorData& data) {
                    return sum + data.speed;
                });
            return sum / history_.size();
        }
};