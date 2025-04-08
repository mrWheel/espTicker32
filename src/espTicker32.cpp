//#include "espTicker32.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <FS.h>
#include <LittleFS.h>
#include <Networking.h>
#include <ArduinoJson.h>
#include <FSmanager.h>
#include <string>
#include "SPAmanager.h"
#include "SettingsClass.h"
#include "LocalMessagesIO.h"
#include "WeerliveClass.h"

#define CLOCK_UPDATE_INTERVAL  1000
#define LOCAL_MESSAGES_PATH      "/localMessages.txt"
#define LOCAL_MESSAGES_RECORD_SIZE  100

Networking* network = nullptr;
Stream* debug = nullptr;

SettingsClass settings;
DeviceSettings* gDeviceSettings = nullptr;
const DeviceAttributes* gDeviceAttributes = nullptr;


LocalMessagesIO localMessages(LOCAL_MESSAGES_PATH, LOCAL_MESSAGES_RECORD_SIZE);

WiFiClient weerliveClient;
Weerlive weerlive(weerliveClient);

SPAmanager spa(80);
//WebServer server(80);
//-- we need to use the server from the SPAmanager!!
FSmanager fsManager(spa.server);


uint32_t lastCounterUpdate = 0;

const char *hostName = "espTicker32";
uint32_t  counter = 0;

// Global variable to track the current active page
std::string currentActivePage = "";


// Function to build a JSON string with input field data
std::string buildInputFieldsJson()
{
  uint8_t recNr = 0;
  bool first = true;
  std::string localMessage = "";

  std::string jsonString = "[";
  while (true) {
    std::string localMessage = localMessages.read(recNr++);
    if (localMessage.empty()) 
    {
      break;
    }

    //-- Escape quotes inside message
    std::string escaped;
    for (char c : localMessage) 
    {
      if (c == '"') escaped += "\\\"";
      else escaped += c;
    }

    if (!first) 
    {
      jsonString += ",";
    }
    jsonString += "\"" + escaped + "\"";

    first = false;
  }

  jsonString += "]";
  return jsonString;

} // buildInputFieldsJson()


std::string buildDeviceFieldsJson()
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

  // First field
  JsonObject field1 = fields.createNestedObject();
  field1["fieldName"] = "hostname";
  field1["fieldPrompt"] = "hostname";
  field1["fieldValue"] = gDeviceSettings->hostname.c_str();
  field1["fieldType"] = "s";
  field1["fieldLen"] = 25;

  // Second field
  JsonObject field2 = fields.createNestedObject();
  field2["fieldName"] = "scrollSnelheid";
  field2["fieldPrompt"] = "Scroll Snelheid";
  field2["fieldValue"] = gDeviceSettings->scrollSnelheid;
  field2["fieldType"] = "n";
  field2["fieldMin"] = gDeviceAttributes->scrollSnelheidMin;
  field2["fieldMax"] = gDeviceAttributes->scrollSnelheidMax;
  field2["fieldStep"] = 1;

  // Third field
  JsonObject field3 = fields.createNestedObject();
  field3["fieldName"] = "weerliveRequestInterval";
  field3["fieldPrompt"] = "Weerlive Request Interval";
  field3["fieldValue"] = gDeviceSettings->weerliveRequestInterval;
  field3["fieldType"] = "n";
  field3["fieldMin"] = gDeviceAttributes->weerliveRequestIntervalMin;
  field3["fieldMax"] = gDeviceAttributes->weerliveRequestIntervalMax;
  field3["fieldStep"] = 1;

  // Serialize to a string and return it
  std::string jsonString;
  serializeJson(doc, jsonString);
  debug->printf("buildDeviceFieldsJson(): JSON string: %s\n", jsonString.c_str());
  // Return the JSON string
  return jsonString;

} // buildDeviceFieldsJson()

// Function to send the JSON string to the client when localMessages page is activated
void sendInputFieldsToClient()
{
  std::string jsonData = buildInputFieldsJson();
  debug->printf("sendInputFieldsToClient(): Sending JSON data to client: %s\n", jsonData.c_str());
  
  // First, send the HTML content for the input fields
  DynamicJsonDocument doc(1024);
  doc["type"] = "update";
  doc["target"] = "inputTableBody";
  
  // Create the HTML content for the input fields
  std::string htmlContent = "";
  
  // Parse the JSON array
  DynamicJsonDocument inputFields(1024);
  DeserializationError error = deserializeJson(inputFields, jsonData);
  
  if (!error) {
    JsonArray array = inputFields.as<JsonArray>();
    for (size_t i = 0; i < array.size(); i++) {
      String value = array[i].as<String>();
      
      // Create a table row for each input field
      htmlContent += "<tr><td style='padding: 8px;'>";
      htmlContent += "<input type='text' value='";
      htmlContent += value.c_str();
      htmlContent += "' style='width: 100%; max-width: 80ch;' maxlength='80' data-index='";
      htmlContent += String(i).c_str();
      htmlContent += "' oninput='updateInputField(this)'>";
      htmlContent += "</td></tr>";
    }
  }
  
  doc["content"] = htmlContent.c_str();
  
  // Serialize the message
  std::string message;
  serializeJson(doc, message);
  
  // Send the structured message via WebSocket
  spa.ws.broadcastTXT(message.c_str(), message.length());
  
  // Now send the raw JSON data in a format that SPAmanager can understand
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["type"] = "custom";
  jsonDoc["action"] = "inputFieldsData";
  jsonDoc["data"] = jsonData;
  
  std::string jsonMessage;
  serializeJson(jsonDoc, jsonMessage);
  debug->printf("sendInputFieldsToClient(): Sending JSON message to client: %s\n", jsonMessage.c_str());
  // Send the JSON message via WebSocket
  spa.ws.broadcastTXT(jsonMessage.c_str(), jsonMessage.length());
  
} // sendInputFieldsToClient()


// Function to send the JSON string to the client when devSettingsPage is activated
void sendDevFieldsToClient()
{
  std::string jsonData = buildDeviceFieldsJson();
  debug->printf("sendDevFieldsToClient(): Sending JSON data to client: %s\n", jsonData.c_str());
  
  // First, send the HTML content for the device settings fields
  DynamicJsonDocument doc(4096);
  doc["type"] = "update";
  doc["target"] = "devSettingsTableBody";
  
  // Create the HTML content for the device settings fields
  std::string htmlContent = "";
  
  // Parse the JSON
  DynamicJsonDocument devSettings(4096);
  DeserializationError error = deserializeJson(devSettings, jsonData);
  
  if (!error) {
    if (devSettings.containsKey("fields") && devSettings["fields"].is<JsonArray>()) {
      JsonArray fields = devSettings["fields"].as<JsonArray>();
      
      for (size_t i = 0; i < fields.size(); i++) {
        JsonObject field = fields[i];
        
        // Create a table row for each field
        htmlContent += "<tr><td style='padding: 8px;'>";
        htmlContent += field["fieldPrompt"].as<String>().c_str();
        htmlContent += "</td><td style='padding: 8px;'>";
        
        // Create the appropriate input element based on fieldType
        if (field["fieldType"] == "s") {
          // String input
          htmlContent += "<input type='text' value='";
          htmlContent += field["fieldValue"].as<String>().c_str();
          htmlContent += "' style='width: 100%;' maxlength='";
          htmlContent += field["fieldLen"].as<String>().c_str();
          htmlContent += "' data-field-name='";
          htmlContent += field["fieldName"].as<String>().c_str();
          htmlContent += "' data-field-type='s' oninput='updateDevSetting(this)'>";
        } else if (field["fieldType"] == "n") {
          // Numeric input
          htmlContent += "<input type='number' value='";
          htmlContent += field["fieldValue"].as<String>().c_str();
          htmlContent += "' style='width: 100%;' min='";
          htmlContent += field["fieldMin"].as<String>().c_str();
          htmlContent += "' max='";
          htmlContent += field["fieldMax"].as<String>().c_str();
          htmlContent += "' step='";
          htmlContent += field["fieldStep"].as<String>().c_str();
          htmlContent += "' data-field-name='";
          htmlContent += field["fieldName"].as<String>().c_str();
          htmlContent += "' data-field-type='n' oninput='updateDevSetting(this)'>";
        }
        
        htmlContent += "</td></tr>";
      }
    }
  }
  
  doc["content"] = htmlContent.c_str();
  
  // Serialize the message
  std::string message;
  serializeJson(doc, message);
  
  // Send the structured message via WebSocket
  spa.ws.broadcastTXT(message.c_str(), message.length());
  
  // Now send the raw JSON data in a format that SPAmanager can understand
  DynamicJsonDocument jsonDoc(4096);
  jsonDoc["type"] = "custom";
  jsonDoc["action"] = "devSettingsData";
  jsonDoc["data"] = jsonData;
  
  std::string jsonMessage;
  serializeJson(jsonDoc, jsonMessage);
  
  // Send the JSON message via WebSocket
  spa.ws.broadcastTXT(jsonMessage.c_str(), jsonMessage.length());
} // sendDevFieldsToClient()


// Function to process the received input fields from the client
void processInputFields(const std::string& jsonString)
{
  uint8_t recNr = 0; // record number

  debug->println("processInputFields(): Processing input fields from JSON:");
  debug->println(jsonString.c_str());
  
  //-- Use ArduinoJson library to parse the JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) 
  {
    debug->printf("processInputFields(): JSON parsing error: %s\n", error.c_str());
    return;
  }
  
  if (!doc.is<JsonArray>()) 
  {
    debug->println("processInputFields(): JSON is not an array");
    return;
  }
  
  JsonArray array = doc.as<JsonArray>();
  debug->printf("processInputFields(): Processing array with %d elements\n", array.size());
  
  for (size_t i = 0; i < array.size(); i++) {
    // Safely handle each value, even if it's null or empty
    if (array[i].isNull()) {
      debug->println("processInputFields(): Skip (null)\n");
    } else {
      // Use String for Arduino compatibility, which handles empty strings properly
      String value = array[i].as<String>();
      debug->printf("processInputFields(): Input value[%d]: %s\n", i, value.c_str());
      if (localMessages.write(recNr, value.c_str()))
      {
        recNr++;
      }
    }
  }
  
  debug->printf("processInputFields(): [%d] Local Messages written to [%s]\n", recNr, LOCAL_MESSAGES_PATH);
  sendInputFieldsToClient();

} // processInputFields()


// Function to process the received device settings from the client
void processDevSettings(const std::string& jsonString)
{
  debug->println("processDevSettings(): Processing device settings from JSON:");
  debug->println(jsonString.c_str());
  
  // Use ArduinoJson library to parse the JSON
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) 
  {
    debug->printf("processDevSettings(): JSON parsing error: %s\n", error.c_str());
    return;
  }
  
  // Check if the JSON has the expected structure
  if (!doc.containsKey("fields") || !doc["fields"].is<JsonArray>()) 
  {
    debug->println("processDevSettings(): JSON does not contain fields array");
    return;
  }
  
  JsonArray fields = doc["fields"].as<JsonArray>();
  debug->printf("processDevSettings(): Processing array with %d fields\n", fields.size());
  
  // Process each field
  for (JsonObject field : fields) {
    if (!field.containsKey("fieldName") || !field.containsKey("value")) {
      debug->println("processDevSettings(): Field missing required properties");
      continue;
    }
    
    const char* fieldName = field["fieldName"];
    
    // Update the appropriate setting based on the field name
    if (strcmp(fieldName, "hostname") == 0) {
      std::string newHostname = field["value"].as<std::string>();
      debug->printf("processDevSettings(): Setting hostname to [%s]\n", newHostname.c_str());
      gDeviceSettings->hostname = newHostname;
    }
    else if (strcmp(fieldName, "scrollSnelheid") == 0) {
      uint8_t newValue = field["value"].as<uint8_t>();
      debug->printf("processDevSettings(): Setting scrollSnelheid to [%d]\n", newValue);
      gDeviceSettings->scrollSnelheid = newValue;
    }
    else if (strcmp(fieldName, "weerliveRequestInterval") == 0) {
      uint8_t newValue = field["value"].as<uint8_t>();
      debug->printf("processDevSettings(): Setting weerliveRequestInterval to [%d]\n", newValue);
      gDeviceSettings->weerliveRequestInterval = newValue;
    }
    else {
      debug->printf("processDevSettings(): Unknown field: %s\n", fieldName);
    }
  }
  
  // Always write settings
  debug->println("processDevSettings(): Writing settings to storage");
  settings.writeSettings();
  
  // Send a confirmation message to the client
  DynamicJsonDocument confirmDoc(512);
  confirmDoc["type"] = "update";
  confirmDoc["target"] = "message";
  confirmDoc["content"] = "Settings saved successfully!";
  
  std::string confirmMessage;
  serializeJson(confirmDoc, confirmMessage);
  spa.ws.broadcastTXT(confirmMessage.c_str(), confirmMessage.length());
  
  // Refresh the device settings display
  sendDevFieldsToClient();

} // processDevSettings()


// WebSocket event handler to receive messages from the client
void handleLocalWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  // This function will be called when no other event handlers match
  debug->printf("handleLocalWebSocketEvent(): WebSocket event type: %d\n", type);
  
  // Check the WebSocket event type
  if (type == WStype_TEXT) {
    // Handle text data
    debug->println("handleLocalWebSocketEvent(): Received text data that wasn't handled by other event handlers");
    
    // Ensure null termination of the payload
    payload[length] = 0;
    
    // Try to parse the JSON message
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      debug->printf("handleLocalWebSocketEvent(): JSON parsing error: %s\n", error.c_str());
      return;
    }
    
    // Check if this is a requestInputFields message
    if (doc["type"] == "requestInputFields") {
      debug->println("handleLocalWebSocketEvent(): Handling requestInputFields message");
      sendInputFieldsToClient();
      return;
    }
    
    // Check if this is a requestDevSettings message
    if (doc["type"] == "requestDevSettings") {
      debug->println("handleLocalWebSocketEvent(): Handling requestDevSettings message");
      sendDevFieldsToClient();
      return;
    }
    
    // Check if this is a process message
    if (doc["type"] == "process") {
      const char* processType = doc["processType"];
      debug->printf("handleLocalWebSocketEvent(): Processing message type: %s\n", processType);
      
      // Check if this is a saveInputFields message
      if (strcmp(processType, "saveInputFields") == 0) {
        debug->println("handleLocalWebSocketEvent(): Handling saveInputFields message");
        
        // Check if inputValues exists and contains inputFieldsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("inputFieldsData")) {
          // Get the inputFieldsData as a string
          const char* inputFieldsData = doc["inputValues"]["inputFieldsData"];
          debug->println("handleLocalWebSocketEvent(): Received input fields data:");
          debug->println(inputFieldsData);
          
          // Process the input fields data
          processInputFields(inputFieldsData);
        } else {
          debug->println("handleLocalWebSocketEvent(): No inputFieldsData found in the message");
        }
      } 
      // Check if this is a saveDevSettings message
      else if (strcmp(processType, "saveDevSettings") == 0) {
        debug->println("handleLocalWebSocketEvent(): Handling saveDevSettings message");
        
        // Check if inputValues exists and contains devSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("devSettingsData")) {
          // Get the devSettingsData as a string
          const char* devSettingsData = doc["inputValues"]["devSettingsData"];
          debug->println("handleLocalWebSocketEvent(): Received device settings data:");
          debug->println(devSettingsData);
          
          // Process the device settings data
          processDevSettings(devSettingsData);
        } else {
          debug->println("handleLocalWebSocketEvent(): No devSettingsData found in the message");
        }
      }
      else {
        debug->printf("handleLocalWebSocketEvent(): Unknown process type: %s\n", processType);
      }
    } else {
      debug->printf("handleLocalWebSocketEvent(): Unknown message type: %s\n", doc["type"] | "unknown");
    }
  }
  else if (type == WStype_BIN) {
    // Handle binary data
    debug->println("handleLocalWebSocketEvent(): Received binary data that wasn't handled by other event handlers");
    // Process the binary payload...
  }
  else if (type == WStype_ERROR) {
    // Handle error events
    debug->println("handleLocalWebSocketEvent(): WebSocket error occurred");
  }
  else if (type == WStype_FRAGMENT) {
    // Handle fragmented messages
    debug->println("handleLocalWebSocketEvent(): Received message fragment");
  }
  // Add other event types as needed
  
} // handleLocalWebSocketEvent()


void pageIsLoadedCallback()
{
  debug->println("pageIsLoadedCallback(): Page is loaded callback executed");
  
  // Get the current active page
  std::string activePage = spa.getActivePageName();
  debug->printf("pageIsLoadedCallback(): Current active page: %s\n", activePage.c_str());
  
  // Check if the Main page was just activated
  if (activePage == "Main") {
    debug->println("Main page activated");
  }
  // Check if the device settings page was just activated
  else if (activePage == "devSettingsPage") {
    debug->println("Device settings page activated");
    sendDevFieldsToClient();
  }
  
  // Update the current active page
  currentActivePage = activePage;

} // pageIsLoadedCallback()


void localMessagesCallback()
{
  debug->println("localMessagesCallback(): Local Messages menu item clicked");
  
  spa.setErrorMessage("Main Menu \"Local Messagesr\" clicked!", 5);
  spa.activatePage("localMessagesPage");
  
  // Call the JavaScript function to set up event handlers
  // It will request the input fields data itself
  spa.callJsFunction("isEspTicker32Loaded");
    
} // localMessagesCallback()

    
void mainCallbackDevSettings()
{
    spa.setErrorMessage("Main Menu \"Dev Settings\" clicked!", 5);
    spa.activatePage("devSettingsPage");
    
    // Call the JavaScript function to set up event handlers
    spa.callJsFunction("isEspTicker32Loaded");
    
    // Send the device settings to the client
    sendDevFieldsToClient();

  } //  mainCallbackDevSettings()

    
void mainCallbackSettings()
{
    spa.setErrorMessage("Main Menu \"Settings\" clicked!", 5);
    spa.activatePage("settingsPage");

} // mainCallbackSettings()
    



void mainCallbackFSmanager()
{
    spa.setMessage("Main Menu \"FSmanager\" clicked!", 5);
    spa.activatePage("FSmanagerPage");
    spa.callJsFunction("loadFileList");

} // mainCallbackFSmanager()



void processUploadFileCallback()
{
  debug->println("Process processUploadFileCallback(): proceed action received");

} // processUploadFileCallback()


void doJsFunction(std::string functionName)
{
    debug->printf("doJsFunction(): JavaScript function called with: [%s]\n", functionName.c_str());
    
    // Call the JavaScript function with the given name
    if (functionName == "isEspTicker32Loaded") {
        spa.callJsFunction("isEspTicker32Loaded");
    } else if (functionName == "isFSmanagerLoaded") {
        spa.callJsFunction("isFSmanagerLoaded");
    }
    
    // You can add more conditions for other functions as needed

} // doJsFunction()




void processInputCallback(const std::map<std::string, std::string>& inputValues)
{
  debug->println("Process callback: proceed action received");
  debug->printf("Received %d input values\n", inputValues.size());
  
  // Check if this is our custom input fields data
  if (inputValues.count("inputFieldsData") > 0) 
  {
    const std::string& jsonData = inputValues.at("inputFieldsData");
    debug->println("Received input fields data:");
    debug->println(jsonData.c_str());
    
    // Process the input fields data
    processInputFields(jsonData.c_str());
  }
  
}


void handleMenuItem(std::string itemName)
{
    debug->printf("handleMenuItem(): Menu item clicked: %s\n", itemName.c_str());
    
    if (itemName == "FSM-1") {
        spa.setMessage("FS Manager: \"List LittleFS\" clicked!", 5);
        spa.activatePage("FSmanagerPage");
        spa.callJsFunction("loadFileList");
    } else if (itemName == "FSM-EXIT") {
        spa.setMessage("FS manager: \"Exit\" clicked!", 5);
        spa.activatePage("Main");
    } else if (itemName == "LMP-EXIT") {
        spa.setMessage("Local Messages: \"Exit\" clicked!", 5);
        spa.activatePage("Main");
      } else if (itemName == "SET-EXIT") {
        spa.setMessage("Settings: \"Exit\" clicked!", 5);
        spa.activatePage("Main");
    }
} // handleMenuItem()

void setupMainPage()
{
    const char *mainPage = R"HTML(
    <div style="font-size: 48px; text-align: center; font-weight: bold;">esp Ticker32</div>
    )HTML";
    
    spa.addPage("Main", mainPage);
    spa.setPageTitle("Main", "esp Ticker32");

    //-- Add Main menu
    spa.addMenu("Main", "Main Menu");
    spa.addMenuItem("Main", "Main Menu", "LocalMessages", localMessagesCallback);
    spa.addMenuItem("Main", "Main Menu", "Settings", mainCallbackSettings);
    spa.addMenuItem("Main", "Main Menu", "FSmanager", mainCallbackFSmanager);
    spa.addMenuItem("Main", "Main Menu", "isFSmanagerLoaded", doJsFunction, "isFSmanagerLoaded");
    spa.addMenuItem("Main", "Main Menu", "isEspTicker32Loaded", doJsFunction, "isEspTicker32Loaded");
    
} // setupMainPage()

void setupLocalMessagesPage()
{
    const char *localMessagesPage = R"HTML(
    <div style="font-size: 48px; text-align: center; font-weight: bold;">Local Messages</div>
    <div id="dynamicInputContainer">
      <table id="inputTable" style="width: 100%; border-collapse: collapse;">
        <thead>
          <tr>
            <th style="text-align: left; padding: 8px;">Input Fields</th>
          </tr>
        </thead>
        <tbody id="inputTableBody">
          <!-- Input fields will be dynamically added here -->
        </tbody>
      </table>
      <div style="margin-top: 20px;">
        <button id="saveButton" onclick="saveInputFields()">Save</button>
        <button id="addButton" onclick="addInputField()">Add</button>
      </div>
    </div>
    )HTML";
    
    spa.addPage("localMessagesPage", localMessagesPage);
    spa.setPageTitle("localMessagesPage", "Local Messages");
    spa.addMenu("localMessagesPage", "Local Messages");
    spa.addMenuItem("localMessagesPage", "Local Messages", "Exit", handleMenuItem, "LMP-EXIT");
  
} // setupLocalMessagesPage()

void setupFSmanagerPage()
{
  const char *fsManagerPage = R"HTML(
    <div id="fsm_fileList" style="display: block;">
    </div>
    <div id="fsm_spaceInfo" class="FSM_space-info" style="display: block;">
      <!-- Space information will be displayed here -->
    </div>    
  )HTML";
  
  spa.addPage("FSmanagerPage", fsManagerPage);

  const char *popupUploadFile = R"HTML(
    <div id="popUpUploadFile">Upload File</div>
    <div id="fsm_fileUpload">
      <input type="file" id="fsm_fileInput">
      <div id="selectedFileName" style="margin-top: 5px; font-style: italic;"></div>
    </div>
    <div style="margin-top: 10px;">
      <button type="button" onClick="closePopup('popup_FS_Manager_Upload_File')">Cancel</button>
      <button type="button" id="uploadButton" onClick="uploadSelectedFile()" disabled>Upload File</button>
    </div>
  )HTML";
  
  const char *popupNewFolder = R"HTML(
    <div id="popupCreateFolder">Create Folder</div>
    <label for="folderNameInput">Folder Name:</label>
    <input type="text" id="folderNameInput" placeholder="Enter folder name">
    <br>
    <button type="button" onClick="closePopup('popup_FS_Manager_New_Folder')">Cancel</button>
    <button type="button" onClick="createFolderFromInput()">Create Folder</button>
  )HTML";

  spa.setPageTitle("FSmanagerPage", "FileSystem Manager");
  //-- Add FSmanager menu
  spa.addMenu("FSmanagerPage", "FS Manager");
  //spa.addMenuItem("FSmanagerPage", "FS Manager", "List LittleFS", handleMenuItem, "FSM-1");
  spa.addMenuItemPopup("FSmanagerPage", "FS Manager", "Upload File", popupUploadFile);
  spa.addMenuItemPopup("FSmanagerPage", "FS Manager", "Create Folder", popupNewFolder);
  spa.addMenuItem("FSmanagerPage", "FS Manager", "Exit", handleMenuItem, "FSM-EXIT");

}


void setupDevSettingsPage()
{
  const char *settingsPage = R"HTML(
    <div style="font-size: 48px; text-align: center; font-weight: bold;">Device Settings</div>
    <div id="dynamicDevSettingsContainer">
      <table id="devSettingsTable" style="width: 100%; border-collapse: collapse;">
        <thead>
          <tr>
            <th style="text-align: left; padding: 8px;">Setting</th>
            <th style="text-align: left; padding: 8px;">Value</th>
          </tr>
        </thead>
        <tbody id="devSettingsTableBody">
          <!-- Device settings will be dynamically added here -->
        </tbody>
      </table>
      <div style="margin-top: 20px;">
        <button id="saveDevSettingsButton" onclick="saveDevSettings()">Save</button>
      </div>
    </div>
    )HTML";
  
  spa.addPage("devSettingsPage", settingsPage);
  spa.setPageTitle("devSettingsPage", "Device Settings");
  //-- Add Settings menu
  spa.addMenu("devSettingsPage", "Device Settings");
  spa.addMenuItem("devSettingsPage", "Device Settings", "Exit", handleMenuItem, "SET-EXIT");

} // setupDevSettingsPage()



void setupSettingsPage()
{
  const char *settingsPage = R"HTML(
    <div style="font-size: 48px; text-align: center; font-weight: bold;">Settings</div>
    )HTML";
  
  spa.addPage("settingsPage", settingsPage);
  spa.setPageTitle("settingsPage", "Settings");
  //-- Add Settings menu
  spa.addMenu("settingsPage", "Settings");
  spa.addMenuItem("settingsPage", "Settings", "Device Settings", mainCallbackDevSettings);
  spa.addMenuItem("settingsPage", "Settings", "Exit", handleMenuItem, "SET-EXIT");

}


void listFiles(const char * dirname, int numTabs) 
{
  // Ensure that dirname starts with '/'
  String path = String(dirname);
  if (!path.startsWith("/")) 
  {
    path = "/" + path;  // Ensure it starts with '/'
  }
  
  File root = LittleFS.open(path);
  
  if (!root) 
  {
    Serial.print("Failed to open directory: ");
    Serial.println(path);
    return;
  }
  
  if (root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      for (int i = 0; i < numTabs; i++) 
      {
        Serial.print("\t");
      }
      Serial.print(file.name());
      if (file.isDirectory()) 
      {
        Serial.println("/");
        listFiles(file.name(), numTabs + 1);
      } else {
        Serial.print("\t\t\t");
        Serial.println(file.size());
      }
      file = root.openNextFile();
    }
  }

} // listFiles()  


void setup()
{
    Serial.begin(115200);
    delay(3000);

    //-- Connect to WiFi
    network = new Networking();
    
    //-- Parameters: hostname, resetWiFi pin, serial object, baud rate
    debug = network->begin(hostName, 0, Serial, 115200);
    
    debug->println("\nsetup(): WiFi connected");
    debug->print("setup(): IP address: ");
    debug->println(WiFi.localIP());

    if (!LittleFS.begin()) 
    {
      debug->println("setup(): LittleFS Mount Failed");
      return;
    }
    listFiles("/", 0);

    settings.setDebug(debug);
    debug->println("setup(): readSettings()");
    settings.readSettings();
    
    // Store references to settings and attributes globally
    gDeviceSettings = &settings.getSettings();
    gDeviceAttributes = &settings.getDeviceAttributes();
    if (gDeviceSettings->hostname.empty()) 
    {
        debug->println("setup(): No hostname found in settings, using default");
        gDeviceSettings->hostname = std::string(hostName);
        settings.writeSettings();
        // Update the global reference after writing
        gDeviceSettings = &settings.getSettings();
    }

    //-- Define custom NTP servers (optional)
    const char* ntpServers[] = {"time.google.com", "time.cloudflare.com", nullptr};

    //-- Start NTP with Amsterdam timezone and custom servers
    if (network->ntpStart("CET-1CEST,M3.5.0,M10.5.0/3", ntpServers))
    {
        debug->println("setup(): NTP started successfully");
    }
    
    localMessages.setDebug(debug);
    weerlive.setup("bb83641a38", "Baarn", debug);
    weerlive.setInterval(10); // Set interval to 10 minutes

    spa.begin("/SYS", debug);
    // Set the local events handler
    spa.setLocalEventHandler(handleLocalWebSocketEvent);

    debug->printf("setup(): SPAmanager files are located [%s]\n", spa.getSystemFilePath().c_str());
    fsManager.begin();
    fsManager.addSystemFile("/favicon.ico");
    fsManager.addSystemFile(spa.getSystemFilePath() + "/SPAmanager.html", false);
    fsManager.addSystemFile(spa.getSystemFilePath() + "/SPAmanager.css", false);
    fsManager.addSystemFile(spa.getSystemFilePath() + "/SPAmanager.js", false);
    fsManager.addSystemFile(spa.getSystemFilePath() + "/disconnected.html", false);
   
    spa.pageIsLoaded(pageIsLoadedCallback);

    fsManager.setSystemFilePath("/SYS");
    debug->printf("setup(): FSmanager files are located [%s]\n", fsManager.getSystemFilePath().c_str());
    spa.includeJsFile(fsManager.getSystemFilePath() + "/FSmanager.js");
    fsManager.addSystemFile(fsManager.getSystemFilePath() + "/FSmanager.js", false);
    spa.includeCssFile(fsManager.getSystemFilePath() + "/FSmanager.css");
    fsManager.addSystemFile(fsManager.getSystemFilePath() + "/FSmanager.css", false);
    spa.includeJsFile(fsManager.getSystemFilePath() + "/espTicker32.js");
    fsManager.addSystemFile(fsManager.getSystemFilePath() + "/espTicker32.js", false);

    setupMainPage();
    setupLocalMessagesPage();
    setupSettingsPage();
    setupDevSettingsPage();
    setupFSmanagerPage();

    spa.activatePage("Main");

    debug->println("Done with setup() ..\n");

} // setup()

void loop()
{
  network->loop();
  spa.server.handleClient();
  spa.ws.loop();
  char weerliveText[2000] = {};
  std::string weerliveInfo = {};
  if (weerlive.loop(weerliveInfo))
  {
    snprintf(weerliveText, 2000, "%s", weerliveInfo.c_str());
    debug->printf("loop(): weerliveText: [%s]\n", weerliveText);
  
  }
  //-- Print current time every 60 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 60000)
  {
    if (network->ntpIsValid())
         debug->printf("loop(): NTP time: %s\n", network->ntpGetTime());
    else debug->println("loop(): NTP is not valid");
    lastPrint = millis();
  }
  
} // loop()loop()
