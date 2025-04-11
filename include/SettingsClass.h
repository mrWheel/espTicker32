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

struct WeerliveSettings {
  std::string authToken;
  std::string plaats;
  uint8_t requestInterval;
};

struct WeerliveAttributes {
  size_t authTokenLen = 16;
  size_t plaatsLen = 32;
  uint8_t requestIntervalMin = 1;
  uint8_t requestIntervalMax = 120;
};

class SettingsClass {
private:
    DeviceSettings deviceSettings;
    WeerliveSettings weerliveSettings;
    Stream* debug = nullptr; // Optional, default to nullptr
    DeviceAttributes deviceAttributes;
    WeerliveAttributes weerliveAttributes;

public:
    SettingsClass();
    void setDebug(Stream* debugPort);

    // Getters
    DeviceSettings& getDeviceSettings();
    const DeviceAttributes& getDeviceAttributes();
    WeerliveSettings& getWeerliveSettings();
    const WeerliveAttributes& getWeerliveAttributes();
    std::string buildDeviceFieldsJson();
    std::string buildWeerliveFieldsJson();
    // Methods for reading and writing settings
    void readDeviceSettings();
    void writeDeviceSettings();
    void readWeerliveSettings();
    void writeWeerliveSettings();
};

#endif // SETTINGS_CLASS_H