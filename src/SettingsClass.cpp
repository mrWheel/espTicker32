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

const DeviceAttributes& SettingsClass::getDeviceAttributes()
{
  return deviceAttributes;
}

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
    if (settings.hostname.length() > deviceAttributes.hostnameMaxLength) {
        debug->println("Error: Hostname exceeds maximum length, truncating");
        settings.hostname = settings.hostname.substr(0, deviceAttributes.hostnameMaxLength);
    }
    file.printf("hostname=%s\n", settings.hostname.c_str());

    // Validate and write scrollSnelheid
    if (settings.scrollSnelheid > 255) {
        debug->println("Error: Scroll Snelheid is above maximum (255), setting to 255");
        settings.scrollSnelheid = 255;
    }
    file.printf("scrollSnelheid=%d\n", settings.scrollSnelheid);

    // Validate and write LDRMinWaarde
    if (settings.LDRMinWaarde < deviceAttributes.LDRMinWaardeMin) {
        debug->println("Error: LDRMinWaarde below minimum, setting to minimum");
        settings.LDRMinWaarde = deviceAttributes.LDRMinWaardeMin;
    } else if (settings.LDRMinWaarde > deviceAttributes.LDRMinWaardeMax) {
        debug->println("Error: LDRMinWaarde above maximum, setting to maximum");
        settings.LDRMinWaarde = deviceAttributes.LDRMinWaardeMax;
    }
    file.printf("LDRMinWaarde=%d\n", settings.LDRMinWaarde);

    // Validate and write LDRMaxWaarde
    if (settings.LDRMaxWaarde < deviceAttributes.LDRMaxWaardeMin) {
        debug->println("Error: LDRMaxWaarde below minimum, setting to minimum");
        settings.LDRMaxWaarde = deviceAttributes.LDRMaxWaardeMin;
    } else if (settings.LDRMaxWaarde > deviceAttributes.LDRMaxWaardeMax) {
        debug->println("Error: LDRMaxWaarde above maximum, setting to maximum");
        settings.LDRMaxWaarde = deviceAttributes.LDRMaxWaardeMax;
    }
    file.printf("LDRMaxWaarde=%d\n", settings.LDRMaxWaarde);

    // Validate and write maxIntensiteitLeds
    if (settings.maxIntensiteitLeds < deviceAttributes.maxIntensiteitLedsMin) {
        debug->println("Error: maxIntensiteitLeds below minimum, setting to minimum");
        settings.maxIntensiteitLeds = deviceAttributes.maxIntensiteitLedsMin;
    } else if (settings.maxIntensiteitLeds > deviceAttributes.maxIntensiteitLedsMax) {
        debug->println("Error: maxIntensiteitLeds above maximum, setting to maximum");
        settings.maxIntensiteitLeds = deviceAttributes.maxIntensiteitLedsMax;
    }
    file.printf("maxIntensiteitLeds=%d\n", settings.maxIntensiteitLeds);

    // Validate and write weerliveAuthToken
    if (settings.weerliveAuthToken.length() > deviceAttributes.weerliveAuthTokenMaxLength) {
        debug->println("Error: WeerliveAuthToken exceeds maximum length, truncating");
        settings.weerliveAuthToken = settings.weerliveAuthToken.substr(0, deviceAttributes.weerliveAuthTokenMaxLength);
    }
    file.printf("weerliveAuthToken=%s\n", settings.weerliveAuthToken.c_str());

    // Validate and write weerlivePlaats
    if (settings.weerlivePlaats.length() > deviceAttributes.weerlivePlaatsMaxLength) {
        debug->println("Error: WeerlivePlaats exceeds maximum length, truncating");
        settings.weerlivePlaats = settings.weerlivePlaats.substr(0, deviceAttributes.weerlivePlaatsMaxLength);
    }
    file.printf("weerlivePlaats=%s\n", settings.weerlivePlaats.c_str());

    // Validate and write weerliveRequestInterval
    if (settings.weerliveRequestInterval < deviceAttributes.weerliveRequestIntervalMin) {
        debug->println("Error: WeerliveRequestInterval below minimum, setting to minimum");
        settings.weerliveRequestInterval = deviceAttributes.weerliveRequestIntervalMin;
    } else if (settings.weerliveRequestInterval > deviceAttributes.weerliveRequestIntervalMax) {
        debug->println("Error: WeerliveRequestInterval above maximum, setting to maximum");
        settings.weerliveRequestInterval = deviceAttributes.weerliveRequestIntervalMax;
    }
    file.printf("weerliveRequestInterval=%d\n", settings.weerliveRequestInterval);

    // Validate and write skipItems
    if (settings.skipItems.length() > deviceAttributes.skipItemsMaxLength) {
        debug->println("Error: SkipItems exceeds maximum length, truncating");
        settings.skipItems = settings.skipItems.substr(0, deviceAttributes.skipItemsMaxLength);
    }
    file.printf("skipItems=%s\n", settings.skipItems.c_str());

    file.close();
    debug->println("Settings saved successfully");
} // writeSettings()