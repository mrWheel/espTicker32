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

std::string SettingsClass::buildDeviceFieldsJson()
{
  // Estimate the size (you can also use the ArduinoJson Assistant online)
  //-- to big for the stack: StaticJsonDocument<4096> doc;
  //-- move to the heap
  DynamicJsonDocument doc(4096);

  // Set the root-level keys
  doc["type"] = "update";
  doc["target"] = "inputDevSettings";

  // Add the "fields" array - CHANGE FROM "devFields" to "fields"
  JsonArray fields = doc.createNestedArray("fields");

  //-- std::string hostname;
  JsonObject field1 = fields.createNestedObject();
  field1["fieldName"] = "hostname";
  field1["fieldPrompt"] = "hostname";
  field1["fieldValue"] = settings.hostname.c_str();
  field1["fieldType"] = "s";
  field1["fieldLen"] = deviceAttributes.hostnameLen;

  //-- uint8_t scrollSnelheid;
  JsonObject field2 = fields.createNestedObject();
  field2["fieldName"] = "scrollSnelheid";
  field2["fieldPrompt"] = "Scroll Snelheid";
  field2["fieldValue"] = settings.scrollSnelheid;
  field2["fieldType"] = "n";
  field2["fieldMin"] = deviceAttributes.scrollSnelheidMin;
  field2["fieldMax"] = deviceAttributes.scrollSnelheidMax;
  field2["fieldStep"] = 1;

  //-- uint8_t LDRMinWaarde;
  JsonObject field3 = fields.createNestedObject();
  field3["fieldName"] = "LDRMinWaarde";
  field3["fieldPrompt"] = "LDR Min. Waarde";
  field3["fieldValue"] = settings.LDRMinWaarde;
  field3["fieldType"] = "n";
  field3["fieldMin"] = deviceAttributes.LDRMinWaardeMin;
  field3["fieldMax"] = deviceAttributes.LDRMinWaardeMax;
  field3["fieldStep"] = 1;

  //-- uint8_t LDRMaxWaarde;
  JsonObject field4 = fields.createNestedObject();
  field4["fieldName"] = "LDRMaxWaarde";
  field4["fieldPrompt"] = "LDR Max. Waarde";
  field4["fieldValue"] = settings.LDRMaxWaarde;
  field4["fieldType"] = "n";
  field4["fieldMin"] = deviceAttributes.LDRMaxWaardeMin;
  field4["fieldMax"] = deviceAttributes.LDRMaxWaardeMax;
  field4["fieldStep"] = 1;

  //-- uint8_t maxIntensiteitLeds;
  JsonObject field5 = fields.createNestedObject();
  field5["fieldName"] = "maxIntensiteitLeds";
  field5["fieldPrompt"] = "Max. Intensiteit LEDS";
  field5["fieldValue"] = settings.maxIntensiteitLeds;
  field5["fieldType"] = "n";
  field5["fieldMin"] = deviceAttributes.maxIntensiteitLedsMin;
  field5["fieldMax"] = deviceAttributes.maxIntensiteitLedsMax;
  field5["fieldStep"] = 1;

  //-- std::string weerliveAuthToken;
  JsonObject field6 = fields.createNestedObject();
  field6["fieldName"] = "weerliveAuthToken";
  field6["fieldPrompt"] = "weerlive Auth. Tokend";
  field6["fieldValue"] = settings.weerliveAuthToken.c_str();
  field6["fieldType"] = "s";
  field6["fieldLen"] = deviceAttributes.weerliveAuthTokenLen;

  //-- std::string weerlivePlaats;
  JsonObject field7 = fields.createNestedObject();
  field7["fieldName"] = "weerlivePlaats";
  field7["fieldPrompt"] = "Scroll Snelheid";
  field7["fieldValue"] = settings.weerlivePlaats;
  field7["fieldType"] = "s";
  field7["fieldMin"] = deviceAttributes.weerlivePlaatsLen;
  
  //-- std::string skipItems;
  JsonObject field8 = fields.createNestedObject();
  field8["fieldName"] = "skipItems";
  field8["fieldPrompt"] = "Words to skip Items";
  field8["fieldValue"] = settings.skipItems.c_str();
  field8["fieldType"] = "s";
  field8["fieldLen"] = deviceAttributes.skipItemsLen;

  // Serialize to a string and return it
  std::string jsonString;
  serializeJson(doc, jsonString);
  debug->printf("buildDeviceFieldsJson(): JSON string: %s\n", jsonString.c_str());
  // Return the JSON string
  return jsonString;

} // buildDeviceFieldsJson()


void SettingsClass::readSettings() 
{
    File file = LittleFS.open("/settings.ini", "r");

    if (!file) {
        debug->println("Failed to open settings.ini file for reading");
        return;
    }

    String line;
    while (file.available()) 
    {
        line = file.readStringUntil('\n');

        // Read and parse settings from each line
        debug->printf("readSettings(): line: [%s]\n", line.c_str());

        if (line.startsWith("hostname=")) {
            settings.hostname = std::string(line.substring(9).c_str());
        }
        else if (line.startsWith("scrollSnelheid=")) {
            settings.scrollSnelheid = line.substring(15).toInt();
        }
        else if (line.startsWith("LDRMinWaarde=")) {
            settings.LDRMinWaarde = line.substring(13).toInt();
        }
        else if (line.startsWith("LDRMaxWaarde=")) {
            settings.LDRMaxWaarde = line.substring(13).toInt();
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
            settings.weerliveRequestInterval = line.substring(24).toInt();
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
    if (settings.hostname.length() > deviceAttributes.hostnameLen) {
        debug->println("Error: Hostname exceeds maximum length, truncating");
        settings.hostname = settings.hostname.substr(0, deviceAttributes.hostnameLen);
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
    if (settings.weerliveAuthToken.length() > deviceAttributes.weerliveAuthTokenLen) {
        debug->println("Error: WeerliveAuthToken exceeds maximum length, truncating");
        settings.weerliveAuthToken = settings.weerliveAuthToken.substr(0, deviceAttributes.weerliveAuthTokenLen);
    }
    file.printf("weerliveAuthToken=%s\n", settings.weerliveAuthToken.c_str());

    // Validate and write weerlivePlaats
    if (settings.weerlivePlaats.length() > deviceAttributes.weerlivePlaatsLen) {
        debug->println("Error: WeerlivePlaats exceeds maximum length, truncating");
        settings.weerlivePlaats = settings.weerlivePlaats.substr(0, deviceAttributes.weerlivePlaatsLen);
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
    debug->printf("weerliveRequestInterval=[%d]\n", settings.weerliveRequestInterval);
    file.printf("weerliveRequestInterval=%d\n", settings.weerliveRequestInterval);

    // Validate and write skipItems
    if (settings.skipItems.length() > deviceAttributes.skipItemsLen) {
        debug->println("Error: SkipItems exceeds maximum length, truncating");
        settings.skipItems = settings.skipItems.substr(0, deviceAttributes.skipItemsLen);
    }
    file.printf("skipItems=%s\n", settings.skipItems.c_str());

    file.close();
    debug->println("Settings saved successfully");

} // writeSettings()