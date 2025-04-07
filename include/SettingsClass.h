#ifndef SETTINGS_CLASS_H
#define SETTINGS_CLASS_H

#include <string>
#include "LittleFS.h"
#include "Arduino.h"

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

struct SettingsAttributes {
    uint8_t LDRMinWaardeMin = 0;
    uint8_t LDRMinWaardeMax = 255;
    uint8_t LDRMaxWaardeMin = 0;
    uint8_t LDRMaxWaardeMax = 255;
    uint8_t maxIntensiteitLedsMin = 0;
    uint8_t maxIntensiteitLedsMax = 255;
    uint8_t weerliveRequestIntervalMin = 10;
    uint8_t weerliveRequestIntervalMax = 120;
    size_t hostnameMaxLength = 64;
    size_t weerliveAuthTokenMaxLength = 128;
    size_t weerlivePlaatsMaxLength = 64;
    size_t skipItemsMaxLength = 256;
};

class SettingsClass {
private:
    DeviceSettings settings;
    Stream* debug = nullptr; // Optional, default to nullptr
    SettingsAttributes settingsAttributes;

public:
    SettingsClass();
    void setDebug(Stream* debugPort);

    // Getters
    DeviceSettings& getSettings();
    const SettingsAttributes& getSettingsAttributes();
    std::string getHostname();
    uint8_t getScrollSnelheid();
    uint8_t getLDRMinWaarde();
    uint8_t getLDRMaxWaarde();
    uint8_t getMaxIntensiteitLeds();
    std::string getWeerliveAuthToken();
    std::string getWeerlivePlaats();
    uint8_t getWeerliveRequestInterval();
    std::string getSkipItems();

    // Setters
    /***** 
    void setHostname(const std::string& hostname);
    void setScrollSnelheid(uint8_t scrollSnelheid);
    void setLDRMinWaarde(uint8_t LDRMinWaarde);
    void setLDRMaxWaarde(uint8_t LDRMaxWaarde);
    void setMaxIntensiteitLeds(uint8_t maxIntensiteitLeds);
    void setWeerliveAuthToken(const std::string& authToken);
    void setWeerlivePlaats(const std::string& plaats);
    void setWeerliveRequestInterval(uint8_t interval);
    void setSkipItems(const std::string& skipItems);
    *****/

    // Methods for reading and writing settings
    void readSettings();
    void writeSettings();
};

#endif // SETTINGS_CLASS_H