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

ParolaSettings& SettingsClass::getParolaSettings()
{
  return parolaSettings;
}

const ParolaAttributes& SettingsClass::getParolaAttributes()
{
  return parolaAttributes;
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


std::string SettingsClass::buildParolaFieldsJson()
{
  // Estimate the size (you can also use the ArduinoJson Assistant online)
  //-- to big for the stack: StaticJsonDocument<4096> doc;
  //-- move to the heap
  DynamicJsonDocument doc(4096);

  // Set the root-level keys
  doc["type"] = "update";
  doc["target"] = "parolaSettings";
  doc["settingsName"] = "Parola Settings";

  JsonArray fields = doc.createNestedArray("fields");

  //-- std::string parola hardwareType;
  JsonObject field1 = fields.createNestedObject();
  field1["fieldName"] = "hardwareType";
  field1["fieldPrompt"] = "Type (1=PAROLA_HW, 2=FC16_HW, 3=GENERIC_HW)";
  field1["fieldValue"] = parolaSettings.hardwareType;
  field1["fieldType"] = "n";
  field1["fieldMin"] = parolaAttributes.hardwareTypeMin;
  field1["fieldMax"] = parolaAttributes.hardwareTypeMax;
  field1["fieldStep"] = 1;

  //-- std::string parola numDevices;
  JsonObject field2 = fields.createNestedObject();
  field2["fieldName"] = "numDevices";
  field2["fieldPrompt"] = "Aantal segmenten";
  field2["fieldValue"] = parolaSettings.numDevices;
  field2["fieldType"] = "n";
  field2["fieldMin"] = parolaAttributes.numDevicesMin;
  field2["fieldMax"] = parolaAttributes.numDevicesMax;
  field2["fieldStep"] = 1;

  //-- std::string parola numZones;
  JsonObject field3 = fields.createNestedObject();
  field3["fieldName"] = "numZones";
  field3["fieldPrompt"] = "Aantal rijen (Zones)";
  field3["fieldValue"] = parolaSettings.numZones;
  field3["fieldType"] = "n";
  field3["fieldMin"] = parolaAttributes.numZonesMin;
  field3["fieldMax"] = parolaAttributes.numZonesMax;
  field3["fieldStep"] = 1;

  //-- std::string parola scroll speed;
  JsonObject field4 = fields.createNestedObject();
  field4["fieldName"] = "speed";
  field4["fieldPrompt"] = "Scroll Speed";
  field4["fieldValue"] = parolaSettings.speed;
  field4["fieldType"] = "n";
  field4["fieldMin"] = parolaAttributes.speedMin;
  field4["fieldMax"] = parolaAttributes.speedMax;
  field4["fieldStep"] = 1;    

  // Serialize to a string and return it
  std::string jsonString;
  serializeJson(doc, jsonString);
  debug->printf("buildParolaFieldsJson(): JSON string: %s\n", jsonString.c_str());
  // Return the JSON string
  return jsonString;

} // buildParolaFieldsJson()


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
  field3["fieldName"] = "requestInterval";
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
    debug->printf("writeDeviceSettings(): hostname=%s\n", deviceSettings.hostname.c_str());
    file.printf("hostname=%s\n", deviceSettings.hostname.c_str());

    // Validate and write scrollSnelheid
    if (deviceSettings.scrollSnelheid > 255) {
        debug->println("Error: Scroll Snelheid is above maximum (255), setting to 255");
        deviceSettings.scrollSnelheid = 255;
    }
    debug->printf("writeDeviceSettings(): scrollSnelheid=%d\n", deviceSettings.scrollSnelheid);
    file.printf("scrollSnelheid=%d\n", deviceSettings.scrollSnelheid);

    // Validate and write LDRMinWaarde
    if (deviceSettings.LDRMinWaarde < deviceAttributes.LDRMinWaardeMin) {
        debug->println("Error: LDRMinWaarde below minimum, setting to minimum");
        deviceSettings.LDRMinWaarde = deviceAttributes.LDRMinWaardeMin;
    } else if (deviceSettings.LDRMinWaarde > deviceAttributes.LDRMinWaardeMax) {
        debug->println("Error: LDRMinWaarde above maximum, setting to maximum");
        deviceSettings.LDRMinWaarde = deviceAttributes.LDRMinWaardeMax;
    }
    debug->printf("writeDeviceSettings(): LDRMinWaarde=%d\n", deviceSettings.LDRMinWaarde);
    file.printf("LDRMinWaarde=%d\n", deviceSettings.LDRMinWaarde);

    // Validate and write LDRMaxWaarde
    if (deviceSettings.LDRMaxWaarde < deviceAttributes.LDRMaxWaardeMin) {
        debug->println("Error: LDRMaxWaarde below minimum, setting to minimum");
        deviceSettings.LDRMaxWaarde = deviceAttributes.LDRMaxWaardeMin;
    } else if (deviceSettings.LDRMaxWaarde > deviceAttributes.LDRMaxWaardeMax) {
        debug->println("Error: LDRMaxWaarde above maximum, setting to maximum");
        deviceSettings.LDRMaxWaarde = deviceAttributes.LDRMaxWaardeMax;
    }
    debug->printf("writeDeviceSettings(): LDRMaxWaarde=%d\n", deviceSettings.LDRMaxWaarde);
    file.printf("LDRMaxWaarde=%d\n", deviceSettings.LDRMaxWaarde);

    // Validate and write maxIntensiteitLeds
    if (deviceSettings.maxIntensiteitLeds < deviceAttributes.maxIntensiteitLedsMin) {
        debug->println("Error: maxIntensiteitLeds below minimum, setting to minimum");
        deviceSettings.maxIntensiteitLeds = deviceAttributes.maxIntensiteitLedsMin;
    } else if (deviceSettings.maxIntensiteitLeds > deviceAttributes.maxIntensiteitLedsMax) {
        debug->println("Error: maxIntensiteitLeds above maximum, setting to maximum");
        deviceSettings.maxIntensiteitLeds = deviceAttributes.maxIntensiteitLedsMax;
    }
    debug->printf("writeDeviceSettings(): maxIntensiteitLeds=%d\n", deviceSettings.maxIntensiteitLeds);
    file.printf("maxIntensiteitLeds=%d\n", deviceSettings.maxIntensiteitLeds);

    // Validate and write skipItems
    if (deviceSettings.skipItems.length() > deviceAttributes.skipItemsLen) {
        debug->println("Error: SkipItems exceeds maximum length, truncating");
        deviceSettings.skipItems = deviceSettings.skipItems.substr(0, deviceAttributes.skipItemsLen);
    }
    debug->printf("writeDeviceSettings(): skipItems=%s\n", deviceSettings.skipItems.c_str());
    file.printf("skipItems=%s\n", deviceSettings.skipItems.c_str());

    file.close();
    debug->println("writeDeviceSettings(): Settings saved successfully");

} // writeDeviceSettings()


void SettingsClass::readParolaSettings() 
{
    File file = LittleFS.open("/parola.ini", "r");

    if (!file) {
        debug->println("readParolaSettings(): Failed to open parola.ini file for reading");
        return;
    }

    String line;
    while (file.available()) 
    {
        line = file.readStringUntil('\n');

        // Read and parse settings from each line
        debug->printf("readParolaSettings(): line: [%s]\n", line.c_str());

        if (line.startsWith("hardwareType=")) {
          parolaSettings.hardwareType = line.substring(13).toInt();
        }
        else if (line.startsWith("numDevices=")) {
          parolaSettings.numDevices = line.substring(11).toInt();
        }
        else if (line.startsWith("numZones=")) {
          parolaSettings.numZones = line.substring(9).toInt();
        }
        else if (line.startsWith("speed=")) {
          parolaSettings.speed = line.substring(6).toInt();
        }
    }

    file.close();
    debug->println("readParolaSettings(): read successfully");

} // readParolaSettings()


void SettingsClass::writeParolaSettings() 
{
    File file = LittleFS.open("/parola.ini", "w");

    if (!file) {
        debug->println("writeParolaSettings(): Failed to open parola.ini file for writing");
        return;
    }

    // Validate and write hardwareType
    if (parolaSettings.hardwareType < parolaAttributes.hardwareTypeMin) {
      debug->println("Error: hardwareType is below minimum, setting to minumum");
      parolaSettings.hardwareType = parolaAttributes.hardwareTypeMin;
    } else if (parolaSettings.hardwareType > parolaAttributes.hardwareTypeMax) {
        debug->println("Error: hardwareType is above maximum, setting to maximum");
        parolaSettings.hardwareType = parolaAttributes.hardwareTypeMax;
    }
    debug->printf("writeParolaSettings(): hardwareType=%d\n", parolaSettings.hardwareType);
    file.printf("hardwareType=%d\n", parolaSettings.hardwareType);

    // Validate and write numDevices
    if (parolaSettings.numDevices < parolaAttributes.numDevicesMin) {
        debug->println("Error: numDevices below minimum, setting to minimum");
        parolaSettings.numDevices = parolaAttributes.numDevicesMin;
    } else if (parolaSettings.numDevices > parolaAttributes.numDevicesMax) {
        debug->println("Error: numDevices above maximum, setting to maximum");
        parolaSettings.numDevices = parolaAttributes.numDevicesMax;
    }
    debug->printf("writeParolaSettings(): numDevices=%d\n", parolaSettings.numDevices);
    file.printf("numDevices=%d\n", parolaSettings.numDevices);

    // Validate and write numZones
    if (parolaSettings.numZones < parolaAttributes.numZonesMin) {
      debug->println("Error: numZones below minimum, setting to minimum");
      parolaSettings.numZones = parolaAttributes.numZonesMin;
    } else if (parolaSettings.numZones > parolaAttributes.numZonesMax) {
        debug->println("Error: numZones above maximum, setting to maximum");
        parolaSettings.numZones = parolaAttributes.numZonesMax;
    }
    debug->printf("writeParolaSettings(): numZones=%d\n", parolaSettings.numZones);
    file.printf("numZones=%d\n", parolaSettings.numZones);

    // Validate and write speed
    if (parolaSettings.speed < parolaAttributes.speedMin) {
      debug->println("Error: speed below minimum, setting to minimum");
      parolaSettings.speed = parolaAttributes.speedMin;
    } else if (parolaSettings.speed > parolaAttributes.speedMax) {
        debug->println("Error: speed above maximum, setting to maximum");
        parolaSettings.speed = parolaAttributes.speedMax;
    }
    debug->printf("writeParolaSettings(): speed=%d\n", parolaSettings.speed);
    file.printf("speed=%d\n", parolaSettings.speed);

    file.close();
    debug->println("writeParolaSettings(): Settings saved successfully");

} // writeParolaSettings()


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
    debug->printf("writeWeerliveSettings(): authToken=%s\n", weerliveSettings.authToken.c_str());
    file.printf("authToken=%s\n", weerliveSettings.authToken.c_str());

    // Validate and write plaats
    if (weerliveSettings.plaats.length() > weerliveAttributes.plaatsLen) {
        debug->println("Error: plaats exceeds maximum length, truncating");
        weerliveSettings.plaats = weerliveSettings.plaats.substr(0, weerliveAttributes.plaatsLen);
    }
    debug->printf("writeWeerliveSetting(): plaats=%s\n", weerliveSettings.plaats.c_str());
    file.printf("plaats=%s\n", weerliveSettings.plaats.c_str());

    // Validate and write requestInterval
    if (weerliveSettings.requestInterval < weerliveAttributes.requestIntervalMin) {
        debug->println("Error: requestInterval below minimum, setting to minimum");
        weerliveSettings.requestInterval = weerliveAttributes.requestIntervalMin;
    } else if (weerliveSettings.requestInterval > weerliveAttributes.requestIntervalMax) {
        debug->println("Error: requestInterval above maximum, setting to maximum");
        weerliveSettings.requestInterval = weerliveAttributes.requestIntervalMax;
    }
    debug->printf("writeWeerliveSetting(): requestInterval=%d\n", weerliveSettings.requestInterval);
    file.printf("requestInterval=%d\n", weerliveSettings.requestInterval);

    file.close();
    debug->println("weerliveSettings saved successfully");

} // writeWeerliveSettings()


void SettingsClass::saveSettings(const std::string& target)
{
  debug->printf("saveSettings(): Saving settings for target: %s\n", target.c_str());
  
  if (target == "deviceSettings") {
    writeDeviceSettings();
  } 
  else if (target == "parolaSettings") {
    writeParolaSettings();
  }
  else if (target == "weerliveSettings") {
    writeWeerliveSettings();
  }
  else {
    debug->printf("saveSettings(): Unknown target: %s\n", target.c_str());
  }
} // saveSettings()
