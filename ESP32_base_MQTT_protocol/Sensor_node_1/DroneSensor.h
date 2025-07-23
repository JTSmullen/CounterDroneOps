/**
  * @file DroneSensor.h
  * @brief Defines the abstract class and data structs for all sensors
  * @author Joshua Smullen
  *
  * This file provides the contract that all specific sensor classes must follow
  * ensuring a consistent interface for initialization, data reading and payload creation.
*/

#pragma once

#include <ArduinoJson.h>

/**
  * @struct SensorData
  * @brief A standardized container for a single sensor reading.
*/
struct SensorData {
  bool presence = false;
  float range_m = 0.0;
  float speed_ms = 0.0;
  unsigned long timestamp_ms = 0;
};

/**
  * @class DroneSensor
  * @brief An abstract class for all drone tracking sensors
*/
class DroneSensor {
  public:
    virtual ~DroneSensor() {}

    virtual bool initialize() = 0;

    virtual bool readData() = 0;

    virtual void buildJsonPayload(JsonDocument& doc) = 0;

    const char* getSensorId() const {
      return sensorId_;
    }

  protected:
    const char* sensorId_;
    SensorData latestData_;
};