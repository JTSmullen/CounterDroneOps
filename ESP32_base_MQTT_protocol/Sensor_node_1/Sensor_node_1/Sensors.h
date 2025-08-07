/**
 * @file Sensors.h
 * @brief Contains the concrete implementations for each specific radar sensor.
 * @author Joshua Smullen
 *
 * Each class in this file inherits from DroneSensor and provides the
 * hardware specific logic for initialization, reading, and data formatting
 */

#pragma once

#include "DroneSensor.h"
#include <DFRobot_C4001.h>

/**
 * @class RadarC4001
 * @brief Implementation for the DFRobot C4001 radar sensor
 */
class RadarC4001 : public DroneSensor {
private:
  DFRobot_C4001_UART radar_instance_;

public:
  RadarC4001(const char* id, HardwareSerial& serial, long baud, uint8_t rx, uint8_t tx)
    : radar_instance_(&serial, baud, rx, tx) {
    sensorId_ = id;
    latestData_.presence = false;
  }

  bool initialize() override {
    if (!radar_instance_.begin()) {
      return false;
    }

    radar_instance_.setSensorMode(eSpeedMode);
    radar_instance_.setDetectThres(60, 1200, 10);
    radar_instance_.setDetectionRange(60, 1200, 1200);
    radar_instance_.setTrigSensitivity(3);
    radar_instance_.setKeepSensitivity(1);
    return true;
  }

  bool readData() override {
    bool current_presence = (radar_instance_.getTargetNumber() > 0);
    
    if (current_presence) {
      latestData_.presence = true;
      latestData_.timestamp_ms = millis();
      latestData_.range_m = radar_instance_.getTargetRange();
      latestData_.speed_ms = radar_instance_.getTargetSpeed();
      return true;

    } else if (previous_presence && !current_presence) {
      latestData_.presence = false;
      latestData_.timestamp_ms = millis();
      latestData_.range_m = 0.0;
      latestData_.speed_ms = 0.0;
      return true;
    } else {
      return false;
    }
  }

  void buildJsonPayload(JsonDocument& doc) override {
    doc["presence"] = latestData_.presence;
    doc["ts"] = latestData_.timestamp_ms;
    
    if(latestData_.presence) {
      doc["range"] = latestData_.range_m;
      doc["speed"] = latestData_.speed_ms;
    }
  }
};


/**
 * @class RadarRCWL
 * @brief Implementation for the simple RCWL-0516 presence radar.
 */
class RadarRCWL : public DroneSensor {
private:
  uint8_t pin_;

public:
  RadarRCWL(const char* id, uint8_t pin) : pin_(pin) {
    sensorId_ = id;
    latestData_.presence = false;
  }

  bool initialize() override {
    pinMode(pin_, INPUT);
    // Set initial state at boot.
    latestData_.presence = (digitalRead(pin_) == HIGH);
    return true;
  }

  bool readData() override {
    bool current_presence = (digitalRead(pin_) == HIGH);

    if (current_presence != latestData_.presence) {
      latestData_.presence = current_presence;
      latestData_.timestamp_ms = millis();
      return true; // State has changed.
    }
    return false; // No change.
  }

  void buildJsonPayload(JsonDocument& doc) override {
    doc["presence"] = latestData_.presence;
    doc["ts"] = latestData_.timestamp_ms;
  }
};