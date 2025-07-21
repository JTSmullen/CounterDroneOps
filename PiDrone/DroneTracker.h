/**
    * @file DroneTracker.h
    * @author Joshua Smullen
    * @brief Defines the DroneTracker class, the central aggregator for sensor data.
    * @version 1.0
    * @date 2025-07-21
    *
    * This file contains the declaration of the DroneTracker class. This class is
    * responsible for receiving distance measurements from multiple sensors,
    * storing the latest reading from each, and triggering a trilateration
    * calculation once a complete set of data is available. It is designed to be
    * thread-safe to handle concurrent updates from different NodeManager threads.
*/

// --- ensure single compilation ---
#pragma once

// --- import statements ---
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "SensorModel.h"
#include "Trilateration.h"

/**
    * @class DroneTracker
    * @brief Aggregates sensor data and calculates the drone's 2D position.
    *
    * The DroneTracker class acts as the central brain for the trilateration system.
    * It maintains the state of the latest distance reading from each requuired sensor.
    * Upon receiving a new data point, it checks if it has a complete set of readings
    * and if so, invokes the trilateration algorithm to compute the drone's (x,y) coordinates
    * This class is designed to be thread-safe.
*/
class DroneTracker {
    // --- Private var declaration to be used ---
    private:
        std::mutex data_mutex_;
        std::map<std::string, Point> sensor_positions_;
        std::map<std::string, double> latest_distances_;
        std::vector<std::string> required_sensor_ids_;

    // --- Public method declarations ---
    public:
        DroneTracker(const std::map<std::string, Point>& sensor_positions);

        std::optional<Point> updateAndCalculate(const std::string& full_sensor_id, double distance);
};
