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
    std::string skipItems;
    uint8_t tickerSpeed;
    
};

struct DeviceAttributes {
    size_t hostnameLen = 32;
    uint8_t scrollSnelheidMin = 5;
    uint8_t scrollSnelheidMax = 255;
    uint8_t LDRMinWaardeMin = 10;
    uint8_t LDRMinWaardeMax = 100;
    uint8_t LDRMaxWaardeMin = 11;
    uint8_t LDRMaxWaardeMax = 101;
    uint8_t maxIntensiteitLedsMin = 10;
    uint8_t maxIntensiteitLedsMax = 55;
    size_t skipItemsLen = 256;
    uint8_t tickerSpeedMin = 10;
    uint8_t tickerSpeedMax = 120;
  };

struct ParolaSettings {
  uint8_t hardwareType;
  uint8_t numDevices;
  uint8_t numZones;
//uint8_t speed;
  
};

struct ParolaAttributes {
  uint8_t hardwareTypeMin = 1;
  uint8_t hardwareTypeMax = 3;
  uint8_t numDevicesMin = 1;
  uint8_t numDevicesMax = 22;
  uint8_t numZonesMin = 1;
  uint8_t numZonesMax = 2;
//uint8_t speedMin = 1;
//uint8_t speedMax = 100;
};

struct WeerliveSettings {
  std::string authToken;
  std::string plaats;
  uint8_t requestInterval;
};

struct WeerliveAttributes {
  size_t authTokenLen = 16;
  size_t plaatsLen = 32;
  uint8_t requestIntervalMin = 10;
  uint8_t requestIntervalMax = 120;
};

struct MediastackSettings {
  std::string authToken;
  uint8_t maxMessages;
  uint8_t requestInterval;
};

struct MediastackAttributes {
  size_t authTokenLen = 16;
  uint8_t requestIntervalMin = 60;
  uint8_t requestIntervalMax = 240;
  uint8_t maxMessagesMin = 1;
  uint8_t maxMessagesMax = 10;
};

class SettingsClass {
private:
    DeviceSettings deviceSettings;
    WeerliveSettings weerliveSettings;
    MediastackSettings mediastackSettings;
    ParolaSettings parolaSettings;
    Stream* debug = nullptr; // Optional, default to nullptr
    DeviceAttributes deviceAttributes;
    WeerliveAttributes weerliveAttributes;
    MediastackAttributes mediastackAttributes;
    ParolaAttributes parolaAttributes;

public:
    SettingsClass();
    void setDebug(Stream* debugPort);

    // Getters
    DeviceSettings& getDeviceSettings();
    const DeviceAttributes& getDeviceAttributes();
    WeerliveSettings& getWeerliveSettings();
    MediastackSettings& getMediastackSettings();
    const WeerliveAttributes& getWeerliveAttributes();
    const MediastackAttributes& getMediastackAttributes();
    ParolaSettings& getParolaSettings();
    const ParolaAttributes& getParolaAttributes();
    std::string buildDeviceFieldsJson();
    std::string buildWeerliveFieldsJson();
    std::string buildParolaFieldsJson();
    // Methods for reading and writing settings
    void readDeviceSettings();
    void writeDeviceSettings();
    void readWeerliveSettings();
    void writeWeerliveSettings();
    void readParolaSettings();
    void writeParolaSettings();
    // Generic method to save settings based on target
    void saveSettings(const std::string& target);

};

#endif // SETTINGS_CLASS_H