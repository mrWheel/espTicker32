#include "SettingsClass.h"

SettingsClass::SettingsClass() {
    // Constructor can initialize default settings if needed
}

void SettingsClass::setDebug(Stream* debugPort) {
  debug = debugPort;
}

DeviceSettings& SettingsClass::getSettings()
{
  return settings;
}

const SettingsAttributes& SettingsClass::getSettingsAttributes()
{
  return settingsAttributes;
}

/**** 
std::string SettingsClass::getHostname() {
    return settings.hostname;
}

uint8_t SettingsClass::getScrollSnelheid() {
    return settings.scrollSnelheid;
}

uint8_t SettingsClass::getLDRMinWaarde() {
    return settings.LDRMinWaarde;
}

uint8_t SettingsClass::getLDRMaxWaarde() {
    return settings.LDRMaxWaarde;
}

uint8_t SettingsClass::getMaxIntensiteitLeds() {
    return settings.maxIntensiteitLeds;
}

std::string SettingsClass::getWeerliveAuthToken() {
    return settings.weerliveAuthToken;
}

std::string SettingsClass::getWeerlivePlaats() {
    return settings.weerlivePlaats;
}

uint8_t SettingsClass::getWeerliveRequestInterval() {
    return settings.weerliveRequestInterval;
}

std::string SettingsClass::getSkipItems() {
    return settings.skipItems;
}

void SettingsClass::setHostname(const std::string& hostname) {
    settings.hostname = hostname;
}

void SettingsClass::setScrollSnelheid(uint8_t scrollSnelheid) {
    settings.scrollSnelheid = scrollSnelheid;
}

void SettingsClass::setLDRMinWaarde(uint8_t LDRMinWaarde) {
    settings.LDRMinWaarde = LDRMinWaarde;
}

void SettingsClass::setLDRMaxWaarde(uint8_t LDRMaxWaarde) {
    settings.LDRMaxWaarde = LDRMaxWaarde;
}

void SettingsClass::setMaxIntensiteitLeds(uint8_t maxIntensiteitLeds) {
    settings.maxIntensiteitLeds = maxIntensiteitLeds;
}

void SettingsClass::setWeerliveAuthToken(const std::string& authToken) {
    settings.weerliveAuthToken = authToken;
}

void SettingsClass::setWeerlivePlaats(const std::string& plaats) {
    settings.weerlivePlaats = plaats;
}

void SettingsClass::setWeerliveRequestInterval(uint8_t interval) {
    settings.weerliveRequestInterval = interval;
}

void SettingsClass::setSkipItems(const std::string& skipItems) {
    settings.skipItems = skipItems;
}
****/

void SettingsClass::readSettings() {
    File file = LittleFS.open("/settings.ini", "r");

    if (!file) {
        debug->println("Failed to open settings.ini file for reading");
        return;
    }

    String line;
    while (file.available()) {
        line = file.readStringUntil('\n');

        // Read and parse settings from each line
        if (line.startsWith("hostname=")) {
            settings.hostname = std::string(line.substring(9).c_str());
        }
        else if (line.startsWith("scrollSnelheid=")) {
            settings.scrollSnelheid = line.substring(15).toInt();
        }
        else if (line.startsWith("LDRMinWaarde=")) {
            settings.LDRMinWaarde = line.substring(14).toInt();
        }
        else if (line.startsWith("LDRMaxWaarde=")) {
            settings.LDRMaxWaarde = line.substring(14).toInt();
        }
        else if (line.startsWith("maxIntensiteitLeds=")) {
            settings.maxIntensiteitLeds = line.substring(19).toInt();
        }
        else if (line.startsWith("weerliveAuthToken=")) {
            settings.weerliveAuthToken = std::string(line.substring(18).c_str());
        }
        else if (line.startsWith("weerlivePlaats=")) {
            settings.weerlivePlaats = std::string(line.substring(15).c_str());
        }
        else if (line.startsWith("weerliveRequestInterval=")) {
            settings.weerliveRequestInterval = line.substring(23).toInt();
        }
        else if (line.startsWith("skipItems=")) {
            settings.skipItems = std::string(line.substring(10).c_str());
        }
    }

    file.close();
    debug->println("Settings read successfully");

} // readSettings()


void SettingsClass::writeSettings() 
{
    File file = LittleFS.open("/settings.ini", "w");

    if (!file) {
        debug->println("Failed to open settings.ini file for writing");
        return;
    }

    // Validate and write hostname
    if (settings.hostname.length() > settingsAttributes.hostnameMaxLength) {
        debug->println("Error: Hostname exceeds maximum length, truncating");
        settings.hostname = settings.hostname.substr(0, settingsAttributes.hostnameMaxLength);
    }
    file.printf("hostname=%s\n", settings.hostname.c_str());

    // Validate and write scrollSnelheid
    if (settings.scrollSnelheid > 255) {
        debug->println("Error: Scroll Snelheid is above maximum (255), setting to 255");
        settings.scrollSnelheid = 255;
    }
    file.printf("scrollSnelheid=%d\n", settings.scrollSnelheid);

    // Validate and write LDRMinWaarde
    if (settings.LDRMinWaarde < settingsAttributes.LDRMinWaardeMin) {
        debug->println("Error: LDRMinWaarde below minimum, setting to minimum");
        settings.LDRMinWaarde = settingsAttributes.LDRMinWaardeMin;
    } else if (settings.LDRMinWaarde > settingsAttributes.LDRMinWaardeMax) {
        debug->println("Error: LDRMinWaarde above maximum, setting to maximum");
        settings.LDRMinWaarde = settingsAttributes.LDRMinWaardeMax;
    }
    file.printf("LDRMinWaarde=%d\n", settings.LDRMinWaarde);

    // Validate and write LDRMaxWaarde
    if (settings.LDRMaxWaarde < settingsAttributes.LDRMaxWaardeMin) {
        debug->println("Error: LDRMaxWaarde below minimum, setting to minimum");
        settings.LDRMaxWaarde = settingsAttributes.LDRMaxWaardeMin;
    } else if (settings.LDRMaxWaarde > settingsAttributes.LDRMaxWaardeMax) {
        debug->println("Error: LDRMaxWaarde above maximum, setting to maximum");
        settings.LDRMaxWaarde = settingsAttributes.LDRMaxWaardeMax;
    }
    file.printf("LDRMaxWaarde=%d\n", settings.LDRMaxWaarde);

    // Validate and write maxIntensiteitLeds
    if (settings.maxIntensiteitLeds < settingsAttributes.maxIntensiteitLedsMin) {
        debug->println("Error: maxIntensiteitLeds below minimum, setting to minimum");
        settings.maxIntensiteitLeds = settingsAttributes.maxIntensiteitLedsMin;
    } else if (settings.maxIntensiteitLeds > settingsAttributes.maxIntensiteitLedsMax) {
        debug->println("Error: maxIntensiteitLeds above maximum, setting to maximum");
        settings.maxIntensiteitLeds = settingsAttributes.maxIntensiteitLedsMax;
    }
    file.printf("maxIntensiteitLeds=%d\n", settings.maxIntensiteitLeds);

    // Validate and write weerliveAuthToken
    if (settings.weerliveAuthToken.length() > settingsAttributes.weerliveAuthTokenMaxLength) {
        debug->println("Error: WeerliveAuthToken exceeds maximum length, truncating");
        settings.weerliveAuthToken = settings.weerliveAuthToken.substr(0, settingsAttributes.weerliveAuthTokenMaxLength);
    }
    file.printf("weerliveAuthToken=%s\n", settings.weerliveAuthToken.c_str());

    // Validate and write weerlivePlaats
    if (settings.weerlivePlaats.length() > settingsAttributes.weerlivePlaatsMaxLength) {
        debug->println("Error: WeerlivePlaats exceeds maximum length, truncating");
        settings.weerlivePlaats = settings.weerlivePlaats.substr(0, settingsAttributes.weerlivePlaatsMaxLength);
    }
    file.printf("weerlivePlaats=%s\n", settings.weerlivePlaats.c_str());

    // Validate and write weerliveRequestInterval
    if (settings.weerliveRequestInterval < settingsAttributes.weerliveRequestIntervalMin) {
        debug->println("Error: WeerliveRequestInterval below minimum, setting to minimum");
        settings.weerliveRequestInterval = settingsAttributes.weerliveRequestIntervalMin;
    } else if (settings.weerliveRequestInterval > settingsAttributes.weerliveRequestIntervalMax) {
        debug->println("Error: WeerliveRequestInterval above maximum, setting to maximum");
        settings.weerliveRequestInterval = settingsAttributes.weerliveRequestIntervalMax;
    }
    file.printf("weerliveRequestInterval=%d\n", settings.weerliveRequestInterval);

    // Validate and write skipItems
    if (settings.skipItems.length() > settingsAttributes.skipItemsMaxLength) {
        debug->println("Error: SkipItems exceeds maximum length, truncating");
        settings.skipItems = settings.skipItems.substr(0, settingsAttributes.skipItemsMaxLength);
    }
    file.printf("skipItems=%s\n", settings.skipItems.c_str());

    file.close();
    debug->println("Settings saved successfully");
} // writeSettings()