#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "SensorModel.h"
#include "Trilateration.h"

class DroneTracker {
    private:
        std::mutex data_mutex_;
        std::map<std::string, Point> sensor_positions_;
        std::map<std::string, double> latest_distances_;
        std::vector<std::string> required_sensor_ids_;

    public:
        DroneTracker(const std::map<std::string, Point>& sensor_positions);

        std::optional<Point> updateAndCalculate(const std::string& full_sensor_id, double distance);
};