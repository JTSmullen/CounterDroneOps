#ifndef LD2412_H
#define LD2412_H

#include <Arduino.h>
#include <cstdint>
#include "esp_timer.h"
#include "HardwareSerial.h"

#define CURRENT_TIME esp_timer_get_time()/1000ULL

class LD2412 {

public:
    /**
     * @brief: Constructor which uses the passed in Serial for the object
     * @param hSerial HardwareSerial object reference
     */
    LD2412(HardwareSerial& hSerial);

private:
    //Reference for the passed in Serial
    HardwareSerial& serial;

    //Determines when a response takes too long
    const int ACK_TIMEOUT = 200;

    //Buffer used in various functions
    static constexpr int BUFFER_SIZE = 32;
    uint8_t buffer[BUFFER_SIZE];

    //Array for array responses
    int arrayResponse[5];

    //For use by readSerial()
    static const int REFRESH_THRESHOLD = 1000;      //Forces serial to be read if 1000 ms have passed since last reading
    unsigned long serialLastRead = NULL;            //Latest time serial was read
    static constexpr int SERIAL_BUFFER_SIZE = 21;
    uint8_t serialBuffer[SERIAL_BUFFER_SIZE];

    /*-----Frame Structure-----*/
    const uint8_t FRAME_HEADER[4] = {0xFD, 0xFC, 0xFB, 0xFA};
    const uint8_t FRAME_FOOTER[4] = {0x04, 0x03, 0x02, 0x01};
    uint8_t data_len[2] = {0x00, 0x00};

    /*-----MISC Functions-----*/
    /**
     * @brief Sends a command to the radar
     * @param data The data (command word and command value)
     */
    void sendCommand(uint8_t* data);

    /**
     * @brief Gets the ACK after a command is sent
     * @param respData Response data (command word byte)
     * @param len Total length of expected response
     * @return Array of the data information or nullptr if failed
     */
    uint8_t* getAck(uint8_t respData, uint8_t len);

    /**
     * @brief Enables configuration mode
     * @return Success status
     */
    bool enableConfig();

    /**
     * @brief Disables configuration mode
     * @return Success status
     */
    bool disableConfig();

    /**
     * @brief Reads serial and puts data in serial buffer
     * @return Success status
     */
    bool readSerial();

public:
    /**
     * @brief Restores factory settings
     * @return Success status
     */
    bool resetDeviceSettings();

    /**
     * @brief Restarts the module
     * @return Success status
     */
    bool restartModule();

    /*-----SET Functions-----*/
    /**
     * @brief Sets basic parameter configuration
     * @param min Minimum distance gate in meters (1-14)
     * @param max Maximum distance gate in meters (1-14)
     * @param duration Unmanned duration in seconds
     * @param outPinPolarity OUT pin polarity (0 manned output HIGH, 1 unmanned output LOW)
     * @return Success status
     */
    bool setParamConfig(uint8_t min, uint8_t max, uint8_t duration, uint8_t outPinPolarity);

    /**
     * @brief Sets the motion sensitivity; detections only count as presence when energy is above set sensitivity
     * @param sen Motion sensitivity (0-100)
     * @return Success status
     */
    bool setMotionSensitivity(uint8_t sen);

    /**
     * @brief Sets the static sensitivity; detections only count as presence when energy is above set sensitivity
     * @param sen Static sensitivity (0-100)
     * @return Success status
     */
    bool setStaticSensitivity(uint8_t sen);

    /**
     * @brief Sets the baud rate
     * @param baud Baud rate
     * @return Success status
     */
    bool setBaudRate(int baud);

    /*-----GET Functions-----*/
    /**
     * @brief Reads basic parameters of the radar
     * @return Array pointer: [0] Success status, [1] min distance gate (m), [2] max distance gate (m), [3] unmanned duration (s), [4] OUT pin polarity
     */
    int* getParamConfig();

    /**
     * @brief Gets the motion sensitivity; detections only count as presence when energy is above set sensitivity
     * @return Lowest motion sensitivity, -1 if failed
     */
    int getMotionSensitivity();

    /**
     * @brief Gets the static sensitivity; detections only count as presence when energy is above set sensitivity
     * @return Lowest static sensitivity, -1 if failed
     */
    int getStaticSensitivity();

    /*-----READ DATA Functions-----*/
    /**
     * @brief Gets target status (0 none, 1 moving, 2 stationary, 3 both)
     * @return Target status, -1 if failed
     */
    int targetState();

    /**
     * @brief Gets moving target distance
     * @return Moving target distance (cm), -1 if failed
     */
    int movingDistance();

    /**
     * @brief Gets moving target energy
     * @return Moving target energy, -1 if failed
     */
    int movingEnergy();

    /**
     * @brief Gets static target distance
     * @return Static target distance (cm), -1 if failed
     */
    int staticDistance();

    /**
     * @brief Gets static target energy
     * @return Static target energy, -1 if failed
     */
    int staticEnergy();
};

#endif //LD2412_H
