/**
 * @file LD2412.h
 * @author Trent Tobias
 * @version 1.0
 * @date August 07, 2025
 * @brief LD2412 serial communication implementation
 */

#ifndef LD2412_H
#define LD2412_H

#include <Arduino.h>
#include <cstdint>
#include <type_traits>

#define CURRENT_TIME millis()
#define RETURN_ARRAY_TRUE std::true_type
#define RETURN_ARRAY_FALSE std::false_type

class LD2412 {

public:
    /**
     * @brief Constructor which uses the passed-in Serial for the object
     * @param ld_serial HardwareSerial or SoftwareSerial object reference
     */
    LD2412(Stream& ld_serial);

private:
    /*-----Variables & Objects-----*/
    //Reference for the passed in Serial object
    Stream& serial;

    //Determines when a response takes too long
    const int ACK_TIMEOUT = 200;

    //Buffer used in various functions
    static constexpr int BUFFER_SIZE = 32;
    uint8_t buffer[BUFFER_SIZE];

    //Arrays for array responses
    int paramResponse[5];
    int sensResponse[14];
    int firmwareResponse[3];

    //For use by readSerial()
    unsigned int refresh_threshold = 5;             //Forces serial to be read if 5 ms have passed since last reading
    unsigned long serialLastRead = NULL;            //Latest time serial was read
    static constexpr int serialBuffer_SIZE = 21;
    uint8_t serialBuffer[serialBuffer_SIZE];

    //Frame structure
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
     * @return Array ptr of the data information or nullptr if failed
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
     * Enters calibration mode after 10 seconds from function call
     * @return Success status
     */
    bool enterCalibrationMode();

    /**
     * Queries whether the sensor is in calibration mode or not
     * @return 1 if in calibration mode, 0 if not, -1 if status retrieval failed
     */
    int checkCalibrationMode();

    /**
     * @brief Reads firmware version information
     * @return Array pointer: [0] Firmware type, [1] major version number, [2] minor version number
     */
    int* readFirmwareVersion();

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
     * @brief Sets the motion sensitivity for all gates.
     * Detections only count as presence when energy is above set sensitivity
     * @overload Pass in 1 value to set that sensitivity across all gates
     * @overload Pass in 14 size array to set an individual sensitivity per gate
     * @param sen Motion sensitivity (0-100)
     * @return Success status
     */
    bool setMotionSensitivity(uint8_t sen);
    bool setMotionSensitivity(uint8_t sen[14]);

    /**
     * @brief Sets the static sensitivity for all gates.
     * Detections only count as presence when energy is above set sensitivity
     * @overload Pass in 1 value to set that sensitivity across all gates
     * @overload Pass in 14 size array to set an individual sensitivity per gate
     * @param sen Static sensitivity (0-100)
     * @return Success status
     */
    bool setStaticSensitivity(uint8_t sen);
    bool setStaticSensitivity(uint8_t sen[14]);

    /**
     * @brief Sets the baud rate
     * @param baud Baud rate
     * @return Success status
     */
    bool setBaudRate(int baud);

    /**
     * @brief Sets the threshold time (in ms) of how often the serial should be read.
     * This is to ensure get data calls are from one specific reading and not multiple, different readings.
     * Should only be adjusted when baud rate is adjusted from 115200 (default: 5 ms)
     * @param refreshTime Threshold time
     */
    void setSerialRefreshThres(unsigned int refreshTime);

    /*-----GET Functions-----*/
    /**
     * @brief Reads basic parameters of the radar
     * @return Array pointer: [0] Success status, [1] min distance gate (m), [2] max distance gate (m),
     * [3] unmanned duration (s), [4] OUT pin polarity
     */
    int* getParamConfig();

    /**
     * @brief Gets the motion sensitivity.
     * Detections only count as presence when energy is above set sensitivity
     * @overload Pass in RETURN_ARRAY_TRUE to return a 14-size array pointer containing each single gate's sensitivity
     * @overload Pass in RETURN_ARRAY_FALSE to return the lowest sensitivity found from across all gates
     * @return An array ptr or the lowest motion sensitivity between all gates, -1/nullptr if failed
     */
    int* getMotionSensitivity(RETURN_ARRAY_TRUE);
    int getMotionSensitivity(RETURN_ARRAY_FALSE);

    /**
     * @brief Gets the static sensitivity.
     * Detections only count as presence when energy is above set sensitivity
     * @overload Pass in RETURN_ARRAY_TRUE to return a 14-size array pointer containing each single gate's sensitivity
     * @overload Pass in RETURN_ARRAY_FALSE to return the lowest sensitivity found from across all gates
     * @return An array ptr or the lowest static sensitivity between all gates, -1/nullptr if failed
     */
    int* getStaticSensitivity(RETURN_ARRAY_TRUE);
    int getStaticSensitivity(RETURN_ARRAY_FALSE);

    /**
     * @brief Gets the threshold time (in ms) of how often the serial should be read
     * @return Serial refresh threshold time
     */
    unsigned int getSerialRefreshThres();

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