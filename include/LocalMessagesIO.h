#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <string>

class LocalMessagesIO {
private:
    std::string path;
    size_t recordSize;
    Stream* debug = nullptr; // Optional, default to nullptr

public:
    LocalMessagesIO(const std::string& path, size_t recordSize);
    void setDebug(Stream* debugPort);
    std::string read(uint8_t recNr);
    bool write(uint8_t recNr, const char* data);
};