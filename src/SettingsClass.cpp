/*
**  SettingsClass.cpp
*/
#include "SettingsClass.h"

SettingsClass::SettingsClass() 
{
  // Initialize settings containers
  initializeSettingsContainers();
}

void SettingsClass::setDebug(Stream* debugPort) 
{
  debug = debugPort;
}

void SettingsClass::initializeSettingsContainers() 
{
  // Device settings
  SettingsContainer deviceContainer("Device Settings", "/settings.ini", "deviceSettings");
  deviceContainer.addField({"devHostname", "devHostname", "s", 32, 0, 0, 0, &devHostname});
  deviceContainer.addField({"devTickerSpeed", "Ticker Scroll Speed (%)", "n", 0, 5, 100, 1, &devTickerSpeed});
  deviceContainer.addField({"devLDRpin", "GPIO Pin LDR", "n", 0, 1, 23, 1, &devLDRpin});
  deviceContainer.addField({"devLDRMinWaarde", "LDR Min. Waarde", "n", 0, 10, 100, 1, &devLDRMinWaarde});
  deviceContainer.addField({"devLDRMaxWaarde", "LDR Max. Waarde", "n", 0, 11, 101, 1, &devLDRMaxWaarde});
  deviceContainer.addField({"devMaxIntensiteitLeds", "Max. Intensiteit LEDS (%)", "n", 0, 1, 100, 1, &devMaxIntensiteitLeds});
  deviceContainer.addField({"devSkipWords", "Words to skip", "s", 250, 0, 0, 0, &devSkipWords});
  deviceContainer.addField({"devResetWiFiPin", "Reset WiFi GPIO pin", "n", 0, 1, 23, 1, &devResetWiFiPin});
  settingsContainers["deviceSettings"] = deviceContainer;
  
  // Weerlive settings
  SettingsContainer weerliveContainer("Weerlive Settings", "/weerlive.ini", "weerliveSettings");
  weerliveContainer.addField({"authToken", "weerlive Auth. Token", "s", 16, 0, 0, 0, &weerliveAuthToken});
  weerliveContainer.addField({"plaats", "Plaats", "s", 32, 0, 0, 0, &weerlivePlaats});
  weerliveContainer.addField({"requestInterval", "Request Interval (minuten)", "n", 0, 10, 120, 1, &weerliveRequestInterval});
  settingsContainers["weerliveSettings"] = weerliveContainer;
  
#ifdef USE_MEDIASTACK
// Mediastack settings
  SettingsContainer mediastackContainer("Mediastack Settings", "/Mediastack.ini", "mediastackSettings");
  mediastackContainer.addField({"authToken", "mediastack Auth. Token", "s", 32, 0, 0, 0, &mediastackAuthToken});
  mediastackContainer.addField({"maxMessages", "Max. Messages to save", "n", 0, 0, 50, 1, &mediastackMaxMessages});
  mediastackContainer.addField({"requestInterval", "Request Interval (minuten)", "n", 0, 60, 240, 1, &mediastackRequestInterval});
  mediastackContainer.addField({"onlyDuringDay", "Update alleen tussen 08:00 en 18:00", "n", 0, 0, 1, 1, &mediastackOnlyDuringDay});
  settingsContainers["mediastackSettings"] = mediastackContainer;
#endif

#ifdef USE_PAROLA
  // Parola settings
  SettingsContainer parolaContainer("Parola Settings", "/parola.ini", "parolaSettings");
  parolaContainer.addField({"parolaHardwareType", "Type (0=PAROLA_HW, 1=FC16_HW, 2=GENERIC_HW)", "n", 0, 0, 2, 1, &parolaHardwareType});
  parolaContainer.addField({"numDevices", "Aantal segmenten", "n", 0, 1, MAX_ZONES, 1, &parolaNumDevices});
  parolaContainer.addField({"parolaNumZones", "Aantal rijen (Zones)", "n", 0, 1, 2, 1, &parolaNumZones});
  parolaContainer.addField({"parolaPinDIN", "DIN/MOSI GPIO pin (0 = default[23])", "n", 0, 0, 23, 1, &parolaPinDIN});
  parolaContainer.addField({"parolaPinCLK", "CLK/SCK GPIO pin (0 = default[18])", "n",  0, 0, 23, 1, &parolaPinCLK});
  parolaContainer.addField({"parolaPinCS", "CS/SS GPIO pin (0 = default [5])", "n",     0, 0, 23, 1, &parolaPinCS});
  settingsContainers["parolaSettings"] = parolaContainer;
#endif

#ifdef USE_NEOPIXELS
  // Neopixel settings
  SettingsContainer neopixelsContainer("Neopixels Settings", "/neopixels.ini", "neopixelsSettings");
  neopixelsContainer.addField({"neopixDataPin", "Neopixels Data GPIO pin", "n", 0, 0, 23, 1, &neopixDataPin});
  neopixelsContainer.addField({"neopixWidth", "Neopixels Width", "n", 0, 1, 128, 1, &neopixWidth});
  neopixelsContainer.addField({"neopixHeight", "Neopixels Height", "n", 0, 1, 16, 1, &neopixHeight});
  neopixelsContainer.addField({"neopixPixPerChar", "Pixels per Charackter", "n", 0, 1, 16, 1, &neopixPixPerChar});
  neopixelsContainer.addField({"neopixCOLOR", "NEOPIXELS COLOR (0=RGB, 1=RBG, 2= GRB(!), 3=GBR, 4=BRG, 5=BGR)", "n", 0, 0, 5, 0, &neopixCOLOR});
  neopixelsContainer.addField({"neopixFREQ", "NEOPIXELS SIGNAL FREQ. (false=800kHz(!), true=400kjHz)", "b", 0, 0, 0, 0, &neopixFREQ});
  neopixelsContainer.addField({"neopixMATRIXTYPEV", "MATRIX TYPE (false=TOP, true=BOTTOM)", "b", 0, 0, 0, 0, &neopixMATRIXTYPEV});
  neopixelsContainer.addField({"neopixMATRIXTYPEH", "MATRIX TYPE (false=LEFT, true=RIGHT)", "b", 0, 0, 0, 0, &neopixMATRIXTYPEH});
  neopixelsContainer.addField({"neopixMATRIXORDER", "MATRIX LAYOUT (false=ROWS, true=COLUMNS)", "b", 0, 0, 0, 0, &neopixMATRIXORDER});
  neopixelsContainer.addField({"neopixMATRIXSEQUENCE", "MATRIX SEQUENCE (false=PROGRESSIVE, true=ZIGZAG)", "b", 0, 0, 0, 0, &neopixMATRIXSEQUENCE});
  settingsContainers["neopixelsSettings"] = neopixelsContainer;
#endif

  // rssfeed settings
  SettingsContainer rssfeedContainer("RSSfeed Settings", "/rssFeeds.ini", "rssfeedSettings");
  rssfeedContainer.addField({"requestInterval", "Request Interval (minuten)", "n", 0, 10, 120, 1, &requestInterval});
  rssfeedContainer.addField({"domain0", "Domain 1", "s", 32, 0, 0, 0, &domain0});
  rssfeedContainer.addField({"path0", "Path 1", "s", 64, 0, 0, 0, &path0});
  rssfeedContainer.addField({"maxFeeds0", "Max. aantal berichten", "n", 0, 1, 254, 1, &maxFeeds0});
  rssfeedContainer.addField({"domain1", "Domain 2", "s", 32, 0, 0, 0, &domain1});
  rssfeedContainer.addField({"path1", "Path 2", "s", 64, 0, 0, 0, &path1});
  rssfeedContainer.addField({"maxFeeds1", "Max. aantal berichten", "n", 0, 1, 254, 1, &maxFeeds1});
  rssfeedContainer.addField({"domain2", "Domain 3", "s", 32, 0, 0, 0, &domain2});
  rssfeedContainer.addField({"path2", "Path 3", "s", 64, 0, 0, 0, &path2});
  rssfeedContainer.addField({"maxFeeds2", "Max. aantal berichten", "n", 0, 1, 254, 1, &maxFeeds2});
  rssfeedContainer.addField({"domain3", "Domain 4", "s", 32, 0, 0, 0, &domain3});
  rssfeedContainer.addField({"path3", "Path 4", "s", 64, 0, 0, 0, &path3});
  rssfeedContainer.addField({"maxFeeds3", "Max. aantal berichten", "n", 0, 1, 254, 1, &maxFeeds3});
  rssfeedContainer.addField({"domain4", "Domain 5", "s", 32, 0, 0, 0, &domain4});
  rssfeedContainer.addField({"path4", "Path 5", "s", 64, 0, 0, 0, &path4});
  rssfeedContainer.addField({"maxFeeds4", "Max. aantal berichten", "n", 0, 1, 254, 1, &maxFeeds4});
  /****** if yoy need them ********
  rssfeedContainer.addField({"domain5", "Domain 6", "s", 32, 0, 0, 0, &domain5});
  rssfeedContainer.addField({"path5", "Path 6", "s", 64, 0, 0, 0, &path5});
  rssfeedContainer.addField({"maxFeeds5", "Max. aantal berichten", "n", 0, 1, 254, 1, &maxFeeds5});
  rssfeedContainer.addField({"domain6", "Domain 7", "s", 32, 0, 0, 0, &domain6});
  rssfeedContainer.addField({"path6", "Path 7", "s", 64, 0, 0, 0, &path6});
  rssfeedContainer.addField({"maxFeeds6", "Max. aantal berichten", "n", 0, 1, 254, 1, &maxFeeds6});
  rssfeedContainer.addField({"domain7", "Domain 8", "s", 32, 0, 0, 0, &domain7});
  rssfeedContainer.addField({"path7", "Path 8", "s", 64, 0, 0, 0, &path7});
  rssfeedContainer.addField({"maxFeeds7", "Max. aantal berichten", "n", 0, 1, 254, 1, &maxFeeds7});
  ************* if you want them *********/
  settingsContainers["rssfeedSettings"] = rssfeedContainer;

} // initializeSettingsContainers()
  
std::string SettingsClass::buildJsonFieldsString(const std::string& settingsType) 
{
  if (settingsContainers.find(settingsType) == settingsContainers.end()) 
  {
    if (debug && doDebug) debug->printf("buildJsonFieldsString(): Unknown settings type: %s\n", settingsType.c_str());
    return "";
  }
  
  const SettingsContainer& container = settingsContainers[settingsType];
  DynamicJsonDocument doc(4096);
  
  // Set the root-level keys
  doc["type"] = "update";
  doc["target"] = container.getTarget();
  doc["settingsName"] = container.getName();
  
  // Add the fields array
  JsonArray fields = doc.createNestedArray("fields");
  
  // Add each field
  for (const auto& field : container.getFields()) 
  {
    JsonObject jsonField = fields.createNestedObject();
    jsonField["fieldName"] = field.fieldName;
    jsonField["fieldPrompt"] = field.fieldPrompt;
    
    if (field.fieldType == "s") 
    {
      // Get the string value and verify it
      std::string value = getStringValue(field.fieldValue);
      
      if (debug && doDebug) 
      {
        debug->printf("buildJsonFieldsString(): Field [%s], value: [%s], length: %d\n", 
                     field.fieldName.c_str(), value.c_str(), value.length());
      }
      
      // String field - use the value directly
      jsonField["fieldValue"] = value;
      jsonField["fieldType"] = "s";
      jsonField["fieldLen"] = field.fieldLen;
    } 
    else if (field.fieldType == "n") 
    {
      // Numeric field
      int value = getNumericValue(field.fieldValue);
      
      if (debug && doDebug) 
      {
        debug->printf("buildJsonFieldsString(): Field [%s], value: %d\n", 
                     field.fieldName.c_str(), value);
      }
      
      jsonField["fieldValue"] = value;
      jsonField["fieldType"] = "n";
      jsonField["fieldMin"] = field.fieldMin;
      jsonField["fieldMax"] = field.fieldMax;
      jsonField["fieldStep"] = field.fieldStep;
    }
    else if (field.fieldType == "b") 
    {
      // Boolean field
      bool value = getBooleanValue(field.fieldValue);
      
      if (debug && doDebug) 
      {
        debug->printf("buildJsonFieldsString(): Field [%s], value: %s\n", 
                     field.fieldName.c_str(), value ? "true" : "false");
      }
      
      jsonField["fieldValue"] = value;
      jsonField["fieldType"] = "b";
    }
  }
  
  // Serialize to a string and return it
  std::string jsonString;
  serializeJson(doc, jsonString);
  if (debug && doDebug) debug->printf("buildJsonFieldsString(): [%s], JSON string: %s\n", settingsType.c_str(), jsonString.c_str());

  return jsonString;

} // buildJsonFieldsString()

void SettingsClass::readSettingFields(const std::string& settingsType) 
{
  if (settingsContainers.find(settingsType) == settingsContainers.end()) 
  {
    if (debug && doDebug) debug->printf("readSettingFields(): Unknown settings type: [%s]\n", settingsType.c_str());
    return;
  }
  
  const SettingsContainer& container = settingsContainers[settingsType];
  File file = LittleFS.open(container.getFile().c_str(), "r");
  
  if (!file) 
  {
    if (debug && doDebug) debug->printf("readSettingFields(): Failed to open [%s] file for reading\n", container.getFile().c_str());
    return;
  }
  
  String line;
  while (file.available()) 
  {
    line = file.readStringUntil('\n');
    
    // Trim any trailing newline or carriage return characters
    line.trim();
    
    if (debug && doDebug) debug->printf("readSettingFields(): line: [%s]\n", line.c_str());
    
    // Parse each line
    for (const auto& field : container.getFields()) 
    {
      String fieldNameStr = String(field.fieldName.c_str());
      String prefix = fieldNameStr + "=";
      
      if (line.startsWith(prefix)) 
      {
        // Get the value part (everything after the '=')
        String value = line.substring(prefix.length());
        
        // Trim any whitespace or control characters
        value.trim();
        
        // Debug the exact string being extracted
        if (debug && doDebug) {
          debug->printf("readSettingFields(): Found field [%s], raw value: [%s], length: %d\n", 
                       field.fieldName.c_str(), value.c_str(), value.length());
        }
        
        if (field.fieldType == "s") 
        {
          // Create a new string from the value to ensure proper handling
          std::string stdValue(value.c_str(), value.length());
          if (debug && doDebug) {
            debug->printf("readSettingFields(): Converting to std::string, length: %d\n", stdValue.length());
          }
          
          // Set the value and verify it was set correctly
          setStringValue(field.fieldValue, stdValue);
          
          // Double-check the stored value
          std::string storedValue = getStringValue(field.fieldValue);
          if (debug && doDebug) {
            debug->printf("readSettingFields(): Verified stored value: [%s], length: %d\n", 
                         storedValue.c_str(), storedValue.length());
          }
        } 
        else if (field.fieldType == "n") 
        {
          int numValue = value.toInt();
          if (debug && doDebug) {
            debug->printf("readSettingFields(): Converting to int: %d\n", numValue);
          }
          setNumericValue(field.fieldValue, numValue);
        }
        else if (field.fieldType == "b") 
        {
          // Convert string to boolean (true/false, 1/0, yes/no, on/off)
          bool boolValue = false;
          value.toLowerCase();
          if (value == "true" || value == "1" || value == "yes" || value == "on") {
            boolValue = true;
          }
          
          if (debug && doDebug) {
            debug->printf("readSettingFields(): Converting to bool: %s\n", boolValue ? "true" : "false");
          }
          setBooleanValue(field.fieldValue, boolValue);
        }
        break;
      }
    }
  }
  
  file.close();
  if (debug && doDebug) debug->printf("readSettingFields(): [%s] read successfully\n", container.getFile().c_str());

} //  readSettingFields()

void SettingsClass::writeSettingFields(const std::string& settingsType) 
{
  if (settingsContainers.find(settingsType) == settingsContainers.end()) 
  {
    if (debug) debug->printf("writeSettingFields(): Unknown settings type: [%s]\n", settingsType.c_str());
    return;
  }
  
  const SettingsContainer& container = settingsContainers[settingsType];
  File file = LittleFS.open(container.getFile().c_str(), "w");
  
  if (!file) 
  {
    if (debug) debug->printf("writeSettingFields(): Failed to open [%s] file for writing\n", container.getFile().c_str());
    return;
  }
  
  // Write each field
  for (const auto& field : container.getFields()) 
  {
    if (field.fieldType == "s") 
    {
      // Get the string value and verify it
      std::string value = getStringValue(field.fieldValue);
      
      // Validate string length
      if (value.length() > field.fieldLen) 
      {
        if (debug && doDebug) debug->printf("writeSettingFields(): Error: [%s] exceeds maximum length, truncating\n", field.fieldName.c_str());
        value = value.substr(0, field.fieldLen);
        setStringValue(field.fieldValue, value);
      }
      
      if (debug && doDebug) {
        debug->printf("writeSettingFields(): Writing field [%s], value: [%s], length: %d\n", 
                     field.fieldName.c_str(), value.c_str(), value.length());
      }
      
      // Use a more explicit approach to write the field
      file.print(field.fieldName.c_str());
      file.print("=");
      file.println(value.c_str());
    } 
    else if (field.fieldType == "n") 
    {
      int value = getNumericValue(field.fieldValue);
      // Validate numeric range
      if (value < field.fieldMin) 
      {
        if (debug && doDebug) debug->printf("writeSettingFields(): Error: [%s] below minimum, setting to minimum\n", field.fieldName.c_str());
        value = field.fieldMin;
        setNumericValue(field.fieldValue, value);
      } 
      else if (value > field.fieldMax) 
      {
        if (debug && doDebug) debug->printf("writeSettingFields(): Error: [%s] above maximum, setting to maximum\n", field.fieldName.c_str());
        value = field.fieldMax;
        setNumericValue(field.fieldValue, value);
      }
      
      if (debug && doDebug) debug->printf("writeSettingFields(): Writing field [%s], value: %d\n", field.fieldName.c_str(), value);
      
      // Use a more explicit approach to write the field
      file.print(field.fieldName.c_str());
      file.print("=");
      file.println(value);
    }
    else if (field.fieldType == "b") 
    {
      bool value = getBooleanValue(field.fieldValue);
      
      if (debug && doDebug) debug->printf("writeSettingFields(): Writing field [%s], value: %s\n", 
                                         field.fieldName.c_str(), value ? "true" : "false");
      
      // Use a more explicit approach to write the field
      file.print(field.fieldName.c_str());
      file.print("=");
      file.println(value ? "true" : "false");
    }
  }
  
  file.close();
  if (debug && doDebug) debug->printf("writeSettingFields(): [%s] saved successfully\n", container.getFile().c_str());
} // writeSettingFields()

void SettingsClass::saveSettings(const std::string& target) 
{
  if (debug && doDebug) debug->printf("saveSettings(): Saving settings for target: [%s]\n", target.c_str());
  writeSettingFields(target);
  //-not yet- spa.setPopupMessage("Settings saved successfully!");
}

const SettingsContainer* SettingsClass::getSettingsContainer(const std::string& settingsType) const 
{
  auto it = settingsContainers.find(settingsType);
  if (it != settingsContainers.end()) 
  {
    return &(it->second);
  }
  return nullptr;
}