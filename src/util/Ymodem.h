#pragma once

#include <Arduino.h>

class Ymodem {
public:
    // returns true on success
    bool recvFile(const String& destPath);
    bool sendFile(const String& srcPath);

private:
    uint16_t calcCrc16(const uint8_t* data, size_t len);
};
