#include "SettingsClass.h"

SettingsClass::SettingsClass() {
    // Constructor can initialize default settings if needed
}

void SettingsClass::setDebug(Stream* debugPort) {
  debug = debugPort;
}

DeviceSettings& SettingsClass::getDeviceSettings()
{
  return deviceSettings;
}

const DeviceAttributes& SettingsClass::getDeviceAttributes()
{
  return deviceAttributes;
}

WeerliveSettings& SettingsClass::getWeerliveSettings()
{
  return weerliveSettings;
}

const WeerliveAttributes& SettingsClass::getWeerliveAttributes()
{
  return weerliveAttributes;
}

std::string SettingsClass::buildDeviceFieldsJson()
{
  // Estimate the size (you can also use the ArduinoJson Assistant online)
  //-- to big for the stack: StaticJsonDocument<4096> doc;
  //-- move to the heap
  DynamicJsonDocument doc(4096);

  // Set the root-level keys
  doc["type"] = "update";
  doc["target"] = "deviceSettings";
  doc["settingsName"] = "Device Settings";

  // Add the "fields" array - CHANGE FROM "devFields" to "fields"
  JsonArray fields = doc.createNestedArray("fields");

  //-- std::string hostname;
  JsonObject field1 = fields.createNestedObject();
  field1["fieldName"] = "hostname";
  field1["fieldPrompt"] = "hostname";
  field1["fieldValue"] = deviceSettings.hostname.c_str();
  field1["fieldType"] = "s";
  field1["fieldLen"] = deviceAttributes.hostnameLen;

  //-- uint8_t scrollSnelheid;
  JsonObject field2 = fields.createNestedObject();
  field2["fieldName"] = "scrollSnelheid";
  field2["fieldPrompt"] = "Scroll Snelheid";
  field2["fieldValue"] = deviceSettings.scrollSnelheid;
  field2["fieldType"] = "n";
  field2["fieldMin"] = deviceAttributes.scrollSnelheidMin;
  field2["fieldMax"] = deviceAttributes.scrollSnelheidMax;
  field2["fieldStep"] = 1;

  //-- uint8_t LDRMinWaarde;
  JsonObject field3 = fields.createNestedObject();
  field3["fieldName"] = "LDRMinWaarde";
  field3["fieldPrompt"] = "LDR Min. Waarde";
  field3["fieldValue"] = deviceSettings.LDRMinWaarde;
  field3["fieldType"] = "n";
  field3["fieldMin"] = deviceAttributes.LDRMinWaardeMin;
  field3["fieldMax"] = deviceAttributes.LDRMinWaardeMax;
  field3["fieldStep"] = 1;

  //-- uint8_t LDRMaxWaarde;
  JsonObject field4 = fields.createNestedObject();
  field4["fieldName"] = "LDRMaxWaarde";
  field4["fieldPrompt"] = "LDR Max. Waarde";
  field4["fieldValue"] = deviceSettings.LDRMaxWaarde;
  field4["fieldType"] = "n";
  field4["fieldMin"] = deviceAttributes.LDRMaxWaardeMin;
  field4["fieldMax"] = deviceAttributes.LDRMaxWaardeMax;
  field4["fieldStep"] = 1;

  //-- uint8_t maxIntensiteitLeds;
  JsonObject field5 = fields.createNestedObject();
  field5["fieldName"] = "maxIntensiteitLeds";
  field5["fieldPrompt"] = "Max. Intensiteit LEDS";
  field5["fieldValue"] = deviceSettings.maxIntensiteitLeds;
  field5["fieldType"] = "n";
  field5["fieldMin"] = deviceAttributes.maxIntensiteitLedsMin;
  field5["fieldMax"] = deviceAttributes.maxIntensiteitLedsMax;
  field5["fieldStep"] = 1;

  //-- std::string skipItems;
  JsonObject field6 = fields.createNestedObject();
  field6["fieldName"] = "skipItems";
  field6["fieldPrompt"] = "Words to skip Items";
  field6["fieldValue"] = deviceSettings.skipItems.c_str();
  field6["fieldType"] = "s";
  field6["fieldLen"] = deviceAttributes.skipItemsLen;

  // Serialize to a string and return it
  std::string jsonString;
  serializeJson(doc, jsonString);
  debug->printf("buildDeviceFieldsJson(): JSON string: %s\n", jsonString.c_str());
  // Return the JSON string
  return jsonString;

} // buildDeviceFieldsJson()

std::string SettingsClass::buildWeerliveFieldsJson()
{
  // Estimate the size (you can also use the ArduinoJson Assistant online)
  //-- to big for the stack: StaticJsonDocument<4096> doc;
  //-- move to the heap
  DynamicJsonDocument doc(4096);

  // Set the root-level keys
  doc["type"] = "update";
  doc["target"] = "weerliveSettings";
  doc["settingsName"] = "Weerlive Settings";

  // Add the "fields" array - CHANGE FROM "devFields" to "fields"
  JsonArray fields = doc.createNestedArray("fields");

  //-- std::string weerliveAuthToken;
  JsonObject field1 = fields.createNestedObject();
  field1["fieldName"] = "authToken";
  field1["fieldPrompt"] = "weerlive Auth. Tokend";
  field1["fieldValue"] = weerliveSettings.authToken.c_str();
  field1["fieldType"] = "s";
  field1["fieldLen"] = weerliveAttributes.authTokenLen;

  //-- std::string weerlivePlaats;
  JsonObject field2 = fields.createNestedObject();
  field2["fieldName"] = "plaats";
  field2["fieldPrompt"] = "Plaats";
  field2["fieldValue"] = weerliveSettings.plaats;
  field2["fieldType"] = "s";
  field2["fieldLen"] = weerliveAttributes.plaatsLen;

  //-- uint8_t requestInterval;
  JsonObject field3 = fields.createNestedObject();
  field3["fieldName"] = "requestIntervals";
  field3["fieldPrompt"] = "Request Interval (minuten)";
  field3["fieldValue"] = weerliveSettings.requestInterval;
  field3["fieldType"] = "n";
  field3["fieldMin"] = weerliveAttributes.requestIntervalMin;
  field3["fieldMax"] = weerliveAttributes.requestIntervalMax;
  field3["fieldStep"] = 1;
    
  // Serialize to a string and return it
  std::string jsonString;
  serializeJson(doc, jsonString);
  debug->printf("buildDeviceFieldsJson(): JSON string: %s\n", jsonString.c_str());
  // Return the JSON string
  return jsonString;

} // buildWeerliveFieldsJson()


void SettingsClass::readDeviceSettings() 
{
    File file = LittleFS.open("/settings.ini", "r");

    if (!file) {
        debug->println("readDeviceSettings(): Failed to open settings.ini file for reading");
        return;
    }

    String line;
    while (file.available()) 
    {
        line = file.readStringUntil('\n');

        // Read and parse settings from each line
        debug->printf("readDeviceSettings(): line: [%s]\n", line.c_str());

        if (line.startsWith("hostname=")) {
          deviceSettings.hostname = std::string(line.substring(9).c_str());
        }
        else if (line.startsWith("scrollSnelheid=")) {
          deviceSettings.scrollSnelheid = line.substring(15).toInt();
        }
        else if (line.startsWith("LDRMinWaarde=")) {
          deviceSettings.LDRMinWaarde = line.substring(13).toInt();
        }
        else if (line.startsWith("LDRMaxWaarde=")) {
          deviceSettings.LDRMaxWaarde = line.substring(13).toInt();
        }
        else if (line.startsWith("maxIntensiteitLeds=")) {
          deviceSettings.maxIntensiteitLeds = line.substring(19).toInt();
        }
        else if (line.startsWith("skipItems=")) {
          deviceSettings.skipItems = std::string(line.substring(10).c_str());
        }
    }

    file.close();
    debug->println("readDeviceSettings(): read successfully");

} // readDeviceSettings()


void SettingsClass::writeDeviceSettings() 
{
    File file = LittleFS.open("/settings.ini", "w");

    if (!file) {
        debug->println("writeDeviceSettings(): Failed to open settings.ini file for writing");
        return;
    }

    // Validate and write hostname
    if (deviceSettings.hostname.length() > deviceAttributes.hostnameLen) {
        debug->println("Error: Hostname exceeds maximum length, truncating");
        deviceSettings.hostname = deviceSettings.hostname.substr(0, deviceAttributes.hostnameLen);
    }
    file.printf("hostname=%s\n", deviceSettings.hostname.c_str());

    // Validate and write scrollSnelheid
    if (deviceSettings.scrollSnelheid > 255) {
        debug->println("Error: Scroll Snelheid is above maximum (255), setting to 255");
        deviceSettings.scrollSnelheid = 255;
    }
    file.printf("scrollSnelheid=%d\n", deviceSettings.scrollSnelheid);

    // Validate and write LDRMinWaarde
    if (deviceSettings.LDRMinWaarde < deviceAttributes.LDRMinWaardeMin) {
        debug->println("Error: LDRMinWaarde below minimum, setting to minimum");
        deviceSettings.LDRMinWaarde = deviceAttributes.LDRMinWaardeMin;
    } else if (deviceSettings.LDRMinWaarde > deviceAttributes.LDRMinWaardeMax) {
        debug->println("Error: LDRMinWaarde above maximum, setting to maximum");
        deviceSettings.LDRMinWaarde = deviceAttributes.LDRMinWaardeMax;
    }
    file.printf("LDRMinWaarde=%d\n", deviceSettings.LDRMinWaarde);

    // Validate and write LDRMaxWaarde
    if (deviceSettings.LDRMaxWaarde < deviceAttributes.LDRMaxWaardeMin) {
        debug->println("Error: LDRMaxWaarde below minimum, setting to minimum");
        deviceSettings.LDRMaxWaarde = deviceAttributes.LDRMaxWaardeMin;
    } else if (deviceSettings.LDRMaxWaarde > deviceAttributes.LDRMaxWaardeMax) {
        debug->println("Error: LDRMaxWaarde above maximum, setting to maximum");
        deviceSettings.LDRMaxWaarde = deviceAttributes.LDRMaxWaardeMax;
    }
    file.printf("LDRMaxWaarde=%d\n", deviceSettings.LDRMaxWaarde);

    // Validate and write maxIntensiteitLeds
    if (deviceSettings.maxIntensiteitLeds < deviceAttributes.maxIntensiteitLedsMin) {
        debug->println("Error: maxIntensiteitLeds below minimum, setting to minimum");
        deviceSettings.maxIntensiteitLeds = deviceAttributes.maxIntensiteitLedsMin;
    } else if (deviceSettings.maxIntensiteitLeds > deviceAttributes.maxIntensiteitLedsMax) {
        debug->println("Error: maxIntensiteitLeds above maximum, setting to maximum");
        deviceSettings.maxIntensiteitLeds = deviceAttributes.maxIntensiteitLedsMax;
    }
    file.printf("maxIntensiteitLeds=%d\n", deviceSettings.maxIntensiteitLeds);

    // Validate and write weerliveAuthToken
    if (deviceSettings.weerliveAuthToken.length() > deviceAttributes.weerliveAuthTokenLen) {
        debug->println("Error: WeerliveAuthToken exceeds maximum length, truncating");
        deviceSettings.weerliveAuthToken = deviceSettings.weerliveAuthToken.substr(0, deviceAttributes.weerliveAuthTokenLen);
    }
    file.printf("weerliveAuthToken=%s\n", deviceSettings.weerliveAuthToken.c_str());

    // Validate and write weerlivePlaats
    if (deviceSettings.weerlivePlaats.length() > deviceAttributes.weerlivePlaatsLen) {
        debug->println("Error: WeerlivePlaats exceeds maximum length, truncating");
        deviceSettings.weerlivePlaats = deviceSettings.weerlivePlaats.substr(0, deviceAttributes.weerlivePlaatsLen);
    }
    file.printf("weerlivePlaats=%s\n", deviceSettings.weerlivePlaats.c_str());

    // Validate and write weerliveRequestInterval
    if (deviceSettings.weerliveRequestInterval < deviceAttributes.weerliveRequestIntervalMin) {
        debug->println("Error: WeerliveRequestInterval below minimum, setting to minimum");
        deviceSettings.weerliveRequestInterval = deviceAttributes.weerliveRequestIntervalMin;
    } else if (deviceSettings.weerliveRequestInterval > deviceAttributes.weerliveRequestIntervalMax) {
        debug->println("Error: WeerliveRequestInterval above maximum, setting to maximum");
        deviceSettings.weerliveRequestInterval = deviceAttributes.weerliveRequestIntervalMax;
    }
    debug->printf("weerliveRequestInterval=[%d]\n", deviceSettings.weerliveRequestInterval);
    file.printf("weerliveRequestInterval=%d\n", deviceSettings.weerliveRequestInterval);

    // Validate and write skipItems
    if (deviceSettings.skipItems.length() > deviceAttributes.skipItemsLen) {
        debug->println("Error: SkipItems exceeds maximum length, truncating");
        deviceSettings.skipItems = deviceSettings.skipItems.substr(0, deviceAttributes.skipItemsLen);
    }
    file.printf("skipItems=%s\n", deviceSettings.skipItems.c_str());

    file.close();
    debug->println("writeDeviceSettings(): Settings saved successfully");

} // writeDeviceSettings()


void SettingsClass::readWeerliveSettings() 
{
    File file = LittleFS.open("/weerlive.ini", "r");

    if (!file) {
        debug->println("readWeerliveSettings(): Failed to open weerlive.ini file for reading");
        return;
    }

    String line;
    while (file.available()) 
    {
        line = file.readStringUntil('\n');

        // Read and parse settings from each line
        debug->printf("readWeerliveSettings(): line: [%s]\n", line.c_str());

        if (line.startsWith("authToken=")) {
          weerliveSettings.authToken = std::string(line.substring(10).c_str());
        }
        else if (line.startsWith("plaats=")) {
          weerliveSettings.plaats = std::string(line.substring(7).c_str());
        }
        else if (line.startsWith("requestInterval=")) {
          weerliveSettings.requestInterval = line.substring(16).toInt();
        }
    }

    file.close();
    debug->println("readWeerliveSettings(): read successfully");

} // readWeerliveSettings()


void SettingsClass::writeWeerliveSettings() 
{
    File file = LittleFS.open("/weerlive.ini", "w");

    if (!file) {
        debug->println("writeWeerliveSettings(): Failed to open weerlive.ini file for writing");
        return;
    }

    // Validate and write authToken
    if (weerliveSettings.authToken.length() > weerliveAttributes.authTokenLen) {
        debug->println("Error: authToken exceeds maximum length, truncating");
        weerliveSettings.authToken = weerliveSettings.authToken.substr(0, weerliveAttributes.authTokenLen);
    }
    file.printf("authToken=%s\n", weerliveSettings.authToken.c_str());

    // Validate and write weerlivePlaats
    if (weerliveSettings.plaats.length() > weerliveAttributes.plaatsLen) {
        debug->println("Error: plaats exceeds maximum length, truncating");
        weerliveSettings.plaats = weerliveSettings.plaats.substr(0, weerliveAttributes.plaatsLen);
    }
    file.printf("plaats=%s\n", weerliveSettings.plaats.c_str());

    // Validate and write weerliveRequestInterval
    if (weerliveSettings.requestInterval < weerliveAttributes.requestIntervalMin) {
        debug->println("Error: requestInterval below minimum, setting to minimum");
        weerliveSettings.requestInterval = weerliveAttributes.requestIntervalMin;
    } else if (weerliveSettings.requestInterval > weerliveAttributes.requestIntervalMax) {
        debug->println("Error: requestInterval above maximum, setting to maximum");
        weerliveSettings.requestInterval = weerliveAttributes.requestIntervalMax;
    }
    debug->printf("requestInterval=[%d]\n", weerliveSettings.requestInterval);
    file.printf("requestInterval=%d\n", weerliveSettings.requestInterval);

    file.close();
    debug->println("Settings saved successfully");

} // writeSettings()

void SettingsClass::saveSettings(const std::string& target)
{
  debug->printf("saveSettings(): Saving settings for target: %s\n", target.c_str());
  
  if (target == "deviceSettings") {
    writeDeviceSettings();
  } 
  else if (target == "weerliveSettings") {
    writeWeerliveSettings();
  }
  else {
    debug->printf("saveSettings(): Unknown target: %s\n", target.c_str());
  }
} // saveSettings()
