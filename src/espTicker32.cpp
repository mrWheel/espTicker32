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

#define CLOCK_UPDATE_INTERVAL  1000
#define LOCAL_MESSAGES_PATH      "/localMessages.txt"
#define LOCAL_MESSAGES_RECORD_SIZE  100

Networking* network = nullptr;
Stream* debug = nullptr;

SPAmanager spa(80);
//WebServer server(80);
//-- we need to use the server from the SPAmanager!!
FSmanager fsManager(spa.server);

uint32_t lastCounterUpdate = 0;

const char *hostName = "espTicker32";
uint32_t  counter = 0;

// Global variable to track the current active page
std::string currentActivePage = "";


std::string readLocalMessages(uint8_t recNr, const char* path, size_t recordSize) 
{
  if (!LittleFS.begin()) 
  {
    debug->println("readLocalMessages(): Failed to mount LittleFS");
    return "";
  }

  File file = LittleFS.open(path, "r");
  if (!file) 
  {
    debug->printf("readLocalMessages(): Failed to open file: %s\n", path);
    return "";
  }

  size_t offset = recNr * recordSize;
  if (!file.seek(offset)) 
  {
    debug->printf("readLocalMessages(): Failed to seek to record %d (offset %d)\n", recNr, offset);
    file.close();
    return "";
  }

  char buffer[recordSize + 1]; // +1 for null terminator
  size_t bytesRead = file.readBytes(buffer, recordSize);
  buffer[bytesRead] = '\0'; // Null-terminate

  file.close();

  if (bytesRead == 0) 
  {
    debug->printf("readLocalMessages(): No data read for record %d\n", recNr);
    return "";
  }

  // Replace newline with null terminator if present
  size_t len = strlen(buffer);
  if (len > 0 && buffer[len - 1] == '\n') {
    buffer[len - 1] = '\0'; // Remove newline character
  }

  return std::string(buffer);

} // readLocalMessages()

bool writeLocalMessages(uint8_t recNr, const char* path, size_t recordSize, const char* data) 
{
  if (!LittleFS.begin()) 
  {
    debug->println("Failed to mount LittleFS");
    return false;
  }

  // Trim leading and trailing spaces
  const char* start = data;
  while (*start == ' ') start++; // Move past leading spaces

  const char* end = data + strlen(data);
  while (end > start && *(end - 1) == ' ') end--; // Move before trailing spaces

  size_t trimmedLen = end - start;

  // Check if after trimming we still have something left
  if (trimmedLen == 0) 
  {
    debug->println("Skipped writing empty or space-only localMessage after trimming.");
    return false;
  }

  File file = LittleFS.open(path, "r+"); // Open for reading and writing
  if (!file) 
  {
    debug->printf("Failed to open file: %s\n", path);
    return false;
  }

  size_t offset = recNr * recordSize;
  if (!file.seek(offset)) 
  {
    debug->printf("Failed to seek to record %d (offset %d)\n", recNr, offset);
    file.close();
    return false;
  }

  // Prepare a fixed-size buffer
  char buffer[recordSize];
  memset(buffer, 0, recordSize); // Fill with zeros

  // Limit trimmed text to fit inside the record (leave room for '\n')
  if (trimmedLen > recordSize - 1)
  {
    trimmedLen = recordSize - 1;
  }

  memcpy(buffer, start, trimmedLen);
  buffer[trimmedLen] = '\n'; // Always end with a newline

  size_t bytesWritten = file.write((uint8_t*)buffer, recordSize);
  file.flush(); // Ensure data is written to flash

  file.close();

  if (bytesWritten != recordSize) 
  {
    debug->printf("Incomplete write: expected %d bytes, wrote %d bytes\n", recordSize, bytesWritten);
    return false;
  }

  debug->printf("Successfully wrote record %d: '%.*s'\n", recNr, (int)trimmedLen, start);
  return true;
} // writeLocalMessages()

// Function to build a JSON string with input field data
std::string buildInputFieldsJson()
{
  uint8_t recNr = 0;
  bool first = true;
  std::string localMessage = "";

  std::string jsonString = "[";
  while (true) {
    std::string localMessage = readLocalMessages(recNr++, LOCAL_MESSAGES_PATH, LOCAL_MESSAGES_RECORD_SIZE);
    if (localMessage.empty()) {
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


// Function to send the JSON string to the client when Main page is activated
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
  
  // Send the JSON message via WebSocket
  spa.ws.broadcastTXT(jsonMessage.c_str(), jsonMessage.length());
  
} // sendInputFieldsToClient()


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
      if (writeLocalMessages(recNr, LOCAL_MESSAGES_PATH, LOCAL_MESSAGES_RECORD_SIZE, value.c_str()))
      {
        recNr++;
      }
    }
  }
  
  debug->printf("processInputFields(): [%d] Local Messages processed successfully\n", recNr);

} // processInputFields()


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
    DynamicJsonDocument doc(1024);
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
      } else {
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
  //if (activePage == "Main" && currentActivePage != "Main") {
  if (activePage == "Main") {
    debug->println("Main page activated");
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

    
void mainCallback1()
{
    spa.setErrorMessage("Main Menu \"Counter\" clicked!", 5);
    spa.activatePage("CounterPage");
}
    
void mainCallback2()
{
    spa.setErrorMessage("Main Menu \"Input\" clicked!", 5);
    spa.activatePage("InputPage");
}


void exitCounterCallback()
{
    spa.setMessage("Counter: \"Exit\" clicked!", 10);
    spa.activatePage("Main");
}



void mainCallback3()
{
    spa.setMessage("Main Menu \"FSmanager\" clicked!", 5);
    spa.activatePage("FSmanagerPage");
    spa.callJsFunction("loadFileList");
}



void processUploadFileCallback()
{
  debug->println("Process processUploadFileCallback(): proceed action received");
}


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
}




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

/*****
void handleMenuItem(std::string itemName)
{
    debug->printf("handleMenuItem(): Menu item clicked: %s\n", itemName.c_str());
    
    if (itemName == "FSM-1") {
        spa.setMessage("FS Manager: \"List LittleFS\" clicked!", 5);
        spa.activatePage("FSmanagerPage");
        spa.callJsFunction("loadFileList");
    } else if (itemName == "FSM-4") {
        spa.setMessage("FS Manager: \"Exit\" clicked!", 5);
        spa.activatePage("Main");
        spa.callJsFunction("isEspTicker32Loaded");
        // Don't call sendInputFieldsToClient() here - it will be called by isEspTicker32Loaded
    }
} // handleMenuItem()
*****/

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
    spa.addMenuItem("Main", "Main Menu", "Settings", mainCallback1);
    spa.addMenuItem("Main", "Main Menu", "FSmanager", mainCallback3);
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
    
    debug->println("\nWiFi connected");
    debug->print("IP address: ");
    debug->println(WiFi.localIP());
    
    spa.begin("/SYS", debug);
    // Set the local events handler
    spa.setLocalEventHandler(handleLocalWebSocketEvent);

    debug->printf("SPAmanager files are located [%s]\n", spa.getSystemFilePath().c_str());
    fsManager.begin();
    fsManager.addSystemFile("/favicon.ico");
    fsManager.addSystemFile(spa.getSystemFilePath() + "/SPAmanager.html", false);
    fsManager.addSystemFile(spa.getSystemFilePath() + "/SPAmanager.css", false);
    fsManager.addSystemFile(spa.getSystemFilePath() + "/SPAmanager.js", false);
    fsManager.addSystemFile(spa.getSystemFilePath() + "/disconnected.html", false);
   
    spa.pageIsLoaded(pageIsLoadedCallback);

    fsManager.setSystemFilePath("/SYS");
    debug->printf("FSmanager files are located [%s]\n", fsManager.getSystemFilePath().c_str());
    spa.includeJsFile(fsManager.getSystemFilePath() + "/FSmanager.js");
    fsManager.addSystemFile(fsManager.getSystemFilePath() + "/FSmanager.js", false);
    spa.includeCssFile(fsManager.getSystemFilePath() + "/FSmanager.css");
    fsManager.addSystemFile(fsManager.getSystemFilePath() + "/FSmanager.css", false);
    spa.includeJsFile(fsManager.getSystemFilePath() + "/espTicker32.js");
    fsManager.addSystemFile(fsManager.getSystemFilePath() + "/espTicker32.js", false);

    setupMainPage();
    setupLocalMessagesPage();
    setupFSmanagerPage();

    spa.activatePage("Main");

    if (!LittleFS.begin()) 
    {
      debug->println("setup(): LittleFS Mount Failed");
      return;
    }
    if (!LittleFS.exists(LOCAL_MESSAGES_PATH)) 
    {
      debug->println("setup(): LittleFS file does not exist");
      File file = LittleFS.open(LOCAL_MESSAGES_PATH, "w");
      if (!file) 
      {
        debug->println("setup(): Failed to create file");
      } 
      else 
      {
        debug->println("setup(): File created successfully");
        file.close();
      }
    }
    listFiles("/", 0);

    debug->println("Done with setup() ..\n");

} // setup()

void loop()
{
  network->loop();
  spa.server.handleClient();
  spa.ws.loop();

} // loop()loop()
