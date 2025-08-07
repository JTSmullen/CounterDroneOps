/**
 * @file LD2412.cpp
 * @author Trent Tobias
 * @version 1.0
 * @date August 07, 2025
 * @brief LD2412 serial communication implementation
 */

#include "LD2412.h"

LD2412::LD2412(Stream& ld_serial) : serial(ld_serial) {
}

/*-----MISC Functions-----*/
void LD2412::sendCommand(uint8_t* data) {
    this->data_len[0] = sizeof(data);

    this->serial.write(FRAME_HEADER, 4);
    this->serial.write(this->data_len, 2);
    this->serial.write(data, this->data_len[0]);
    this->serial.write(FRAME_FOOTER, 4);
}

uint8_t* LD2412::getAck(uint8_t respData, uint8_t len) {
    unsigned long time = CURRENT_TIME;
    int i = 0;

    while (CURRENT_TIME - time < ACK_TIMEOUT)
        while (this->serial.available() && i < BUFFER_SIZE)
            this->buffer[i++] = this->serial.read();

    if (i >= len) {
        for (i=0; i<len; i++)
            if (i<4 && this->buffer[i] != FRAME_HEADER[i]                       //Verifies header
                || i==4 && this->buffer[i] != len-12                            //Verifies expected length
                || i==5 && this->buffer[i] != 0x00                              //Verifies spacing (0x00)
                || i==6 && this->buffer[i] != respData                          //Verifies command word for ack
                || i==7 && this->buffer[i] != 0x01                              //Verifies response acknowledgement (0x01)
                || len-4<i<len && this->buffer[i] != FRAME_FOOTER[i-(len-4)])   //Verifies footer
                return nullptr;
    }
    return this->buffer;
}

bool LD2412::enableConfig() {
    uint8_t data[] = {0xFF, 0x00, 0x01, 0x00};
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 18); ack != nullptr && ack[8] == 0x00)
        return true;
    return false;
}

bool LD2412::disableConfig() {
    uint8_t data[] = {0xFE, 0x00};
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 18); ack != nullptr && ack[8] == 0x00)
        return true;
    return false;
}

bool LD2412::readSerial() {
    //If serial was already successfully read within the past threshold, this function is skipped
    if (this->serialLastRead != NULL && CURRENT_TIME - this->serialLastRead < this->refresh_threshold)
        return true;

    int i = 0;
    long int timeRef = CURRENT_TIME;
    while (this->serial.available() && i < 21) {
        for (i=0; i<21; i++) {
            this->buffer[i] = this->serial.read();

            //Ensures packet capture is properly aligned at header
            if (this->buffer[0] != 0xF4
                || i>0 && this->buffer[1] != 0xF3
                || i>1 && this->buffer[2] != 0xF2
                || i>2 && this->buffer[3] != 0xF1)
                i=-1;

            //Ensures the packet was completely and properly captured by verifying footer
            if (i>16 && this->buffer[17] != 0xF8
                || i>17 && this->buffer[18] != 0xF7
                || i>18 && this->buffer[19] != 0xF6
                || i>19 && this->buffer[20] != 0xF5)
                return false;

            else if (CURRENT_TIME - timeRef > ACK_TIMEOUT)
                return false;
        }
    }
    this->serialLastRead = CURRENT_TIME;
    for (i=0; i<21; i++)
        this->serialBuffer[i] = this->buffer[i];
    return true;
}

bool LD2412::enterCalibrationMode() {
    uint8_t data[] = {0x0B, 0x00};
    bool success = false;

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 14); ack != nullptr && ack[8] == 0x00)
        success = true;
    disableConfig();
    return success;
}

int LD2412::checkCalibrationMode() {
    uint8_t data[] = {0x1B, 0x00};

    if (!enableConfig())
        return -1;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 16); ack != nullptr && ack[8] == 0x00) {
        disableConfig();
        return ack[10];
    }
    disableConfig();
    return -1;
}

int* LD2412::readFirmwareVersion() {
    uint8_t data[] = {0x12, 0x00};

    if (!enableConfig())
        return nullptr;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 22); ack != nullptr && ack[8] == 0x00) {
        this->firmwareResponse[0] = ack[10] + (ack[11] << 8);
        this->firmwareResponse[1] = ack[12] + (ack[13] << 8);
        this->firmwareResponse[2] = ack[14] + (ack[15] << 8) + (ack[16] << 16) + (ack[17] << 24);
        disableConfig();
        return this->firmwareResponse;
    }
    disableConfig();
    return nullptr;
}

bool LD2412::resetDeviceSettings() {
    uint8_t data[] = {0xA2, 0x00};
    bool success = false;

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 14); ack != nullptr && ack[8] == 0x00)
        success = true;
    disableConfig();
    return success;
}

bool LD2412::restartModule() {
    uint8_t data[] = {0xA3, 0x00};
    bool success = false;

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 14); ack != nullptr && ack[8] == 0x00)
        success = true;
    disableConfig();
    return success;
}

/*-----SET Functions-----*/
bool LD2412::setParamConfig(uint8_t min, uint8_t max, uint8_t duration, uint8_t outPinPolarity) {
    uint8_t data[] = {0x02, 0x00, min, max, duration, 0x00, outPinPolarity};
    bool success = false;

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 14); ack != nullptr && ack[8] == 0x00)
        success = true;
    disableConfig();
    return success;
}

bool LD2412::setMotionSensitivity(uint8_t sen) {
    uint8_t data[16] = {0x03, 0x00};
    for (int i=2; i<16; i++)
        data[i] = sen;
    bool success = false;

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 14); ack != nullptr && ack[8] == 0x00)
        success = true;
    disableConfig();
    return success;
}
bool LD2412::setMotionSensitivity(uint8_t sen[14]) {
    uint8_t data[16] = {0x03, 0x00};
    for (int i=2; i<16; i++)
        data[i] = sen[i-2];
    bool success = false;

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 14); ack != nullptr && ack[8] == 0x00)
        success = true;
    disableConfig();
    return success;
}

bool LD2412::setStaticSensitivity(uint8_t sen) {
    uint8_t data[16] = {0x04, 0x00};
    for (int i=2; i<16; i++)
        data[i] = sen;
    bool success = false;

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 14); ack != nullptr && ack[8] == 0x00)
        success = true;
    disableConfig();
    return success;
}
bool LD2412::setStaticSensitivity(uint8_t sen[14]) {
    uint8_t data[16] = {0x04, 0x00};
    for (int i=2; i<16; i++)
        data[i] = sen[i-2];
    bool success = false;

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 14); ack != nullptr && ack[8] == 0x00)
        success = true;
    disableConfig();
    return success;
}

bool LD2412::setBaudRate(int baud) {
    uint8_t data[] = {0x05, 0x00};
    switch (baud) {
        case 9600:
            data[0] = 0x01;
            break;
        case 19200:
            data[0] = 0x02;
            break;
        case 38400:
            data[0] = 0x03;
            break;
        case 57600:
            data[0] = 0x04;
            break;
        case 115200:
            data[0] = 0x05;
            break;
        case 230400:
            data[0] = 0x06;
            break;
        case 256000:
            data[0] = 0x07;
            break;
        case 460800:
            data[0] = 0x08;
            break;
        default:
            return false;
    }
    bool success = false;

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 14); ack != nullptr && ack[8] == 0x00)
        success = true;
    disableConfig();
    return success;
}

void LD2412::setSerialRefreshThres(unsigned int refreshTime) {
    this->refresh_threshold = refreshTime;
}

/*-----GET Functions-----*/
int* LD2412::getParamConfig() {
    uint8_t data[] = {0x12, 0x00};

    if (!enableConfig())
        return nullptr;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 19); ack != nullptr && ack[8] == 0x00) {
        for (int i=10; i<15; i++)
            this->paramResponse[i-10] = static_cast<int>(ack[i]);
        disableConfig();
        return this->paramResponse;
    }
    disableConfig();
    return nullptr;
}

int* LD2412::getMotionSensitivity(RETURN_ARRAY_TRUE) {
    uint8_t data[] = {0x13, 0x00};

    if (!enableConfig())
        return nullptr;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 28); ack != nullptr && ack[8] == 0x00) {
        for (int i=10; i<24; i++)
            this->sensResponse[i-10] = static_cast<int>(ack[i]);
        disableConfig();
        return this->sensResponse;
    }
    disableConfig();
    return nullptr;
}
int LD2412::getMotionSensitivity(RETURN_ARRAY_FALSE) {
    uint8_t data[] = {0x13, 0x00};

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 28); ack != nullptr && ack[8] == 0x00) {
        int min = 100;
        for (int i=10; i<24; i++)
            if (ack[i] < min)
                min = ack[i];
        disableConfig();
        return min;
    }
    disableConfig();
    return -1;
}

int* LD2412::getStaticSensitivity(RETURN_ARRAY_TRUE) {
    uint8_t data[] = {0x14, 0x00};

    if (!enableConfig())
        return nullptr;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 28); ack != nullptr && ack[8] == 0x00) {
        for (int i=10; i<24; i++)
            this->sensResponse[i-10] = static_cast<int>(ack[i]);
        disableConfig();
        return this->sensResponse;
    }
    disableConfig();
    return nullptr;
}
int LD2412::getStaticSensitivity(RETURN_ARRAY_FALSE) {
    uint8_t data[] = {0x14, 0x00};

    if (!enableConfig())
        return false;
    sendCommand(data);

    if (const uint8_t* ack = getAck(data[0], 28); ack != nullptr && ack[8] == 0x00) {
        int min = 100;
        for (int i=10; i<24; i++)
            if (ack[i] < min)
                min = ack[i];
        disableConfig();
        return min;
    }
    disableConfig();
    return -1;
}

unsigned int LD2412::getSerialRefreshThres() {
    return this->refresh_threshold;
}

/*-----READ DATA Functions-----*/
int LD2412::targetState() {
    if (!readSerial())
        return -1;
    return this->serialBuffer[8];
}

int LD2412::movingDistance() {
    if (!readSerial())
        return -1;
    return this->serialBuffer[9] + (this->serialBuffer[10] << 8);
}

int LD2412::movingEnergy() {
    if (!readSerial())
        return -1;
    return this->serialBuffer[11];
}

int LD2412::staticDistance() {
    if (!readSerial())
        return -1;
    return this->serialBuffer[12] + (this->serialBuffer[13] << 8);
}

int LD2412::staticEnergy() {
    if (!readSerial())
        return -1;
    return this->serialBuffer[14];
}