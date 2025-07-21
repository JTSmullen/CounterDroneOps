/**
    * @file DroneTracker.cpp
    * @author Joshua Smullen
    * @brief Aggregator of data from the Sensor nodes. 
    * @version 1.0
    * @date 2025-07-21
    *
    * This file is responsible for receiving distance measurements from multiple sensors,
    * storing the latest reading from each, and triggering a trilateration calculation
    * once a complete set of data is available. 
    *
    * Designed to be thread-safe to handle concurrent updates from different NodeManager threads.
*/

// --- Imports ---
#include "DroneTracker.h"
// --- End Imports ---

/**
    * @brief Map to hold the fixed positions of the sensors
    *
    * This method should be called whenever you need to validate a sensors position
    *
    * @param Map of sensors current positions
*/
DroneTracker::DroneTracker(const std::map<std::string, Point>& sensor_positions)
    : sensor_positions_(sensor_positions) {

        // populate the map
        for (const auto& pair : sensor_positions_) {
            required_sensor_ids_.push_back(pair.first);
        }
    }

/**
    * @brief Aggregates data to ensure consitency and accurate calculation of positon.
    * This method is called when a new distance measurement is received
    * from a sensor. It stores the distance and then checks if a reading has been
    * received from all the required sensors. If a complete dataset is available,
    * it performs the trilateration calculation.
    *
    * @param full_sensor_id The unique identifier for the sensor
    * @param distance The measured distance from the sensor to the target, in meters (change to cm maybe?)
    *
    * @return std::optional<Point> If the calculation is successful, it returns an
    * optional contained the calculated (x,y) Point. If there is not enough
    * data, or if the calculation fails (mayber collinear sensors) it
    * returns std::nullopt.
*/
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
