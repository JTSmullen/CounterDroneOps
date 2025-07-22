#!/bin/bash

echo "--- Compiling Drone Tracker (Pi Portion) ---"

g++ -std=c++17 \
    main.cpp \
    NodeManager.cpp \
    DroneTracker.cpp \
    Trilateration.cpp \
    -o drone_tracker \
    -I/usr/include/nlohmann \
    -lpaho-mqttpp3 -lpaho-mqtt3as -pthread

if [ $? -eq 0 ]; then
    echo "--- Compiled Succesfully! ---"
    echo "Run with : ./drone_tracker"
else
    echo "--- Compilation Failed! ---"
fi