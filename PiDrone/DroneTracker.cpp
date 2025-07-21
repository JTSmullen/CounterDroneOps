#include "DroneTracker.h"

DroneTracker::DroneTracker(const std::map<std::string, Point>& sensor_positions)
    : sensor_positions_(sensor_positions) {

        for (const auto& pair : sensor_positions_) {
            required_sensor_ids_.push_back(pair.first);
        }
    }

std::optional<Point> DroneTracker::updateAndCalculate(const std::string& full_sensor_id, double distance) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (sensor_positions_.find(full_sensor_id) == sensor_positions_.end()) {
        return std::nullopt;
    }

    latest_distances_[full_sensor_id] = distance;

    if (latest_distance_.size() < required_sensor)ids_.size() {
        return std::nullopt;
    }

    for (const auto& id : required_sensor_ids_) {
        if (latest_distances.find(id) == latest_distances_.end()) {
            return std::nullopt;
        }
    }

    const auto& id1 = required_sensor_ids_[0];
    const auto& id2 = required_sensor_ids_[1];
    const auto& id3 = required_sensor_ids_[2];

    return trilaterate(
        sensor_positions_.at(id1), latest_distances_.at(id1),
        sensor_positions_.at(id2), latest_distances_.at(id2),
        sensor_positions_.at(id3), latest_distances_.at(id3)
    );
}