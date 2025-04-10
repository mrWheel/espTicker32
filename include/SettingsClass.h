#ifndef SETTINGS_CLASS_H
#define SETTINGS_CLASS_H

#include <string>
#include "LittleFS.h"
#include "Arduino.h"
#include <ArduinoJson.h>

struct DeviceSettings {
    std::string hostname;
    uint8_t scrollSnelheid;
    uint8_t LDRMinWaarde;
    uint8_t LDRMaxWaarde;
    uint8_t maxIntensiteitLeds;
    std::string weerliveAuthToken;
    std::string weerlivePlaats;
    uint8_t weerliveRequestInterval;
    std::string skipItems;
};

struct DeviceAttributes {
    size_t hostnameLen = 32;
    uint8_t scrollSnelheidMin = 0;
    uint8_t scrollSnelheidMax = 255;
    uint8_t LDRMinWaardeMin = 0;
    uint8_t LDRMinWaardeMax = 255;
    uint8_t LDRMaxWaardeMin = 0;
    uint8_t LDRMaxWaardeMax = 255;
    uint8_t maxIntensiteitLedsMin = 0;
    uint8_t maxIntensiteitLedsMax = 255;
    uint8_t weerliveRequestIntervalMin = 10;
    uint8_t weerliveRequestIntervalMax = 120;
    size_t weerliveAuthTokenLen = 128;
    size_t weerlivePlaatsLen = 64;
    size_t skipItemsLen = 256;
};

class SettingsClass {
private:
    DeviceSettings settings;
    Stream* debug = nullptr; // Optional, default to nullptr
    DeviceAttributes deviceAttributes;

public:
    SettingsClass();
    void setDebug(Stream* debugPort);

    // Getters
    DeviceSettings& getSettings();
    const DeviceAttributes& getDeviceAttributes();
    std::string buildDeviceFieldsJson();
    // Methods for reading and writing settings
    void readSettings();
    void writeSettings();
};

#endif // SETTINGS_CLASS_H