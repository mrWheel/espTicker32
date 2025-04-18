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
#include "MediastackClass.h"
#include "RSSreaderClass.h"
//#include <MD_Parola.h>
//#include <MD_MAX72xx.h>
//#include <SPI.h>
#include "ParolaManager.h"

ParolaManager display(5);  // 5 = your CS pin

#define CLOCK_UPDATE_INTERVAL  1000
#define LOCAL_MESSAGES_PATH      "/localMessages.txt"
#define LOCAL_MESSAGES_RECORD_SIZE  150

Networking* network = nullptr;
Stream* debug = nullptr;

SettingsClass settings;

LocalMessagesIO localMessages(LOCAL_MESSAGES_PATH, LOCAL_MESSAGES_RECORD_SIZE);

WiFiClient wifiClient;
Weerlive weerlive(wifiClient);
Mediastack mediastack(wifiClient);
RSSreaderClass rssReader;

SPAmanager spa(80);
//WebServer server(80);
//-- we need to use the server from the SPAmanager!!
FSmanager fsManager(spa.server);


uint32_t lastCounterUpdate = 0;

const char *hostName = "espTicker32";
uint32_t  counter = 0;
uint16_t nr = 1;  //-- name it like "parolaMessageNr"

std::string actMessage = "";
char weerliveText[2000] = {};
char localMessage[LOCAL_MESSAGES_RECORD_SIZE +2] = {};

// Global variable to track the current active page
std::string currentActivePage = "";


String getWeerliveMessage()
{
    return weerliveText;

} // getWeerliveMessage()

String getMediastackMessage()
{
  static uint8_t msgNr = 0;
  char mediastackMessage[2000] = {};

  snprintf(mediastackMessage, sizeof(mediastackMessage), "%s", mediastack.readFileById("NWS", msgNr).c_str());
  if (strlen(mediastackMessage) == 0) 
  {
    msgNr = 0;
    snprintf(mediastackMessage, sizeof(mediastackMessage), "%s", mediastack.readFileById("NWS", msgNr).c_str());
  } 
  debug->printf("getMediastackMessage(): msgNr = [%d] mediastackMessage = [%s]\n", msgNr, mediastackMessage);
  msgNr++;
  return mediastackMessage;

} // getMediastackMessage()

String getRSSfeedMessage()
{
  uint8_t feedIndex = 0;
  size_t itemIndex = 0;
  char rssFeedMessage[1000] = {};

  // Get the next feed item indices
  if (rssReader.getNextFeedItem(feedIndex, itemIndex))
  {
    snprintf(rssFeedMessage, sizeof(rssFeedMessage), "[%d][%d] %s"
                                            , feedIndex, itemIndex
                                            , rssReader.readRssFeed(feedIndex, itemIndex).c_str());
    
    // If we get an empty message, try again with the next indices
    if (strlen(rssFeedMessage) == 0 && rssReader.getNextFeedItem(feedIndex, itemIndex))
    {
      snprintf(rssFeedMessage, sizeof(rssFeedMessage), "[%d][%d] %s"
                                                  , feedIndex, itemIndex
                                                  , rssReader.readRssFeed(feedIndex, itemIndex).c_str());
    }
  }
  
  debug->printf("getRSSfeedMessage(): feedNr[%d], msgNr[%d] rssFeedMessage[%s]\n" 
                                  , feedIndex, itemIndex, rssFeedMessage);
  
  return rssFeedMessage;

} // getRSSfeedMessage()


String getLocalMessage()
{
  static uint8_t msgNr = 0;

  snprintf(localMessage, sizeof(localMessage), "%s", localMessages.read(msgNr++).c_str());
  if (strlen(localMessage) == 0) 
  {
    msgNr = 0;
    snprintf(localMessage, sizeof(localMessage), "%s", localMessages.read(msgNr++).c_str());
  } 
  debug->printf("getLocalMessage(): msgNr = [%d] localMessage = [%s]\n", msgNr, localMessage);
  return localMessage;

} // getLocalMessage()

std::string nextMessage()
{
    std::string newMessage = "-";
    std::string tmpMessage = "";
    newMessage = getLocalMessage().c_str();


    if (strcmp(newMessage.c_str(), "<weerlive>") == 0) 
    {
        debug->println("nextMessage(): weerlive message");
        newMessage = getWeerliveMessage().c_str();
    }
    else if (strcmp(newMessage.c_str(), "<mediastack>") == 0) 
    {
        debug->println("nextMessage(): mediastack message");
        newMessage = getMediastackMessage().c_str();
    }
    else if (strcmp(newMessage.c_str(), "<rssfeed>") == 0) 
    {
        debug->println("nextMessage(): rssfeed message");
        newMessage = getRSSfeedMessage().c_str();
    }
    else if (strcmp(newMessage.c_str(), "<date>") == 0) 
    {
        debug->println("nextMessage(): The Date");
        newMessage = network->ntpGetDate();
    }
    else if (strcmp(newMessage.c_str(), "<time>") == 0) 
    {
        debug->println("nextMessage(): The Time");
        newMessage = network->ntpGetTime();
    }
    else if (strcmp(newMessage.c_str(), "<datetime>") == 0) 
    {
        debug->println("nextMessage(): The Date & Time");
        newMessage = network->ntpGetDateTime();
    }
    else if (strcmp(newMessage.c_str(), "<spaces>") == 0) 
    {
        debug->println("nextMessage(): spaces");
        newMessage = "                                                                                     ";
    }
    debug->printf("nextMessage(): Sending text: [** %s]\n", newMessage.c_str()); 
    display.sendNextText(("* "+newMessage+"*  ").c_str());
    spa.callJsFunction("queueMessageToMonitor", ("* "+newMessage+" *").c_str());

    return newMessage;

} // nextMessage()


// Function to build a JSON string with input field data
std::string buildLocalMessagesJson()
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

} // buildLocalMessagesJson()



// Function to send the JSON string to the client when localMessages page is activated
void sendLocalMessagesToClient()
{
  std::string jsonData = buildLocalMessagesJson();
  debug->printf("sendLocalMessagesToClient(): Sending JSON data to client: %s\n", jsonData.c_str());
  
  // First, send the HTML content for the input fields
  DynamicJsonDocument doc(1024);
  doc["type"] = "update";
  doc["target"] = "inputTableBody";
  
  // Create the HTML content for the input fields
  std::string htmlContent = "";
  
  // Parse the JSON array
  DynamicJsonDocument LocalMessages(1024);
  DeserializationError error = deserializeJson(LocalMessages, jsonData);
  
  if (!error) {
    JsonArray array = LocalMessages.as<JsonArray>();
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
  jsonDoc["action"] = "LocalMessagesData";
  jsonDoc["data"] = jsonData;
  
  std::string jsonMessage;
  serializeJson(jsonDoc, jsonMessage);
  debug->printf("sendLocalMessagesToClient(): Sending JSON message to client: %s\n", jsonMessage.c_str());
  // Send the JSON message via WebSocket
  spa.ws.broadcastTXT(jsonMessage.c_str(), jsonMessage.length());
  
} // sendLocalMessagesToClient()


// Function to process the received input fields from the client
void processLocalMessages(const std::string& jsonString)
{
  uint8_t recNr = 0; // record number

  debug->println("processLocalMessages(): Processing input fields from JSON:");
  debug->println(jsonString.c_str());
  
  //-- Use ArduinoJson library to parse the JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) 
  {
    debug->printf("processLocalMessages(): JSON parsing error: %s\n", error.c_str());
    return;
  }
  
  if (!doc.is<JsonArray>()) 
  {
    debug->println("processLocalMessages(): JSON is not an array");
    return;
  }
  
  JsonArray array = doc.as<JsonArray>();
  debug->printf("processLocalMessages(): Processing array with %d elements\n", array.size());
  
  for (size_t i = 0; i < array.size(); i++) {
    // Safely handle each value, even if it's null or empty
    if (array[i].isNull()) {
      debug->println("processLocalMessages(): Skip (null)\n");
    } else {
      // Use String for Arduino compatibility, which handles empty strings properly
      String value = array[i].as<String>();
      debug->printf("processLocalMessages(): Input value[%d]: %s\n", i, value.c_str());
      if (localMessages.write(recNr, value.c_str()))
      {
        recNr++;
      }
    }
  }
  
  debug->printf("processLocalMessages(): [%d] Local Messages written to [%s]\n", recNr, LOCAL_MESSAGES_PATH);
  sendLocalMessagesToClient();

} // processLocalMessages()



// Generic function to send settings fields to the client
void sendSettingFieldToClient(const std::string& settingsType)
{
  std::string jsonData = settings.buildJsonFieldsString(settingsType);
  debug->printf("sendSettingFieldToClient(): Sending JSON data for %s to client: %s\n", settingsType.c_str(), jsonData.c_str());
  
  // First, send the HTML content for the settings fields
  DynamicJsonDocument doc(4096);
  doc["type"] = "update";
  doc["target"] = "settingsTableBody";
  
  // Create the HTML content for the settings fields
  std::string htmlContent = "";
  
  // Parse the JSON
  DynamicJsonDocument settingsDoc(4096);
  DeserializationError error = deserializeJson(settingsDoc, jsonData);
  
  if (!error) {
    if (settingsDoc.containsKey("fields") && settingsDoc["fields"].is<JsonArray>()) {
      JsonArray fields = settingsDoc["fields"].as<JsonArray>();
      
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
          htmlContent += "' data-field-type='s' oninput='update";
          
          // Determine the appropriate update function based on settings type
          if (settingsType == "deviceSettings") {
            htmlContent += "DevSetting";
          } else if (settingsType == "parolaSettings") {
            htmlContent += "ParolaSettings";
          } else if (settingsType == "weerliveSettings") {
            htmlContent += "WeerliveSettings";
          } else if (settingsType == "mediastackSettings") {
            htmlContent += "MediastackSettings";
          } else {
            // Generic fallback
            htmlContent += "Setting";
          }
          
          htmlContent += "(this)'>";
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
          htmlContent += "' data-field-type='n' oninput='update";
          
          // Determine the appropriate update function based on settings type
          if (settingsType == "deviceSettings") {
            htmlContent += "DevSetting";
          } else if (settingsType == "parolaSettings") {
            htmlContent += "ParolaSettings";
          } else if (settingsType == "weerliveSettings") {
            htmlContent += "WeerliveSettings";
          } else if (settingsType == "mediastackSettings") {
            htmlContent += "MediastackSettings";
          } else {
            // Generic fallback
            htmlContent += "Setting";
          }
          
          htmlContent += "(this)'>";
        }
        
        htmlContent += "</td></tr>";
      }
    }
    
    // Update the settings name in the page if available
    if (settingsDoc.containsKey("settingsName")) {
      DynamicJsonDocument nameDoc(512);
      nameDoc["type"] = "update";
      nameDoc["target"] = "settingsName";
      nameDoc["content"] = settingsDoc["settingsName"].as<String>().c_str();
      
      std::string nameMessage;
      serializeJson(nameDoc, nameMessage);
      
      // Send the name update message via WebSocket
      spa.ws.broadcastTXT(nameMessage.c_str(), nameMessage.length());
    }
  }
  
  // Set the content in the document
  doc["content"] = htmlContent.c_str();
  
  // Serialize the message
  std::string message;
  serializeJson(doc, message);
  
  // Send the structured message via WebSocket
  spa.ws.broadcastTXT(message.c_str(), message.length());
  
  // Now send the raw JSON data in a format that SPAmanager can understand
  DynamicJsonDocument jsonDoc(4096);
  jsonDoc["type"] = "custom";
  
  // Use the exact same action name format as the original functions
  if (settingsType == "deviceSettings") {
    jsonDoc["action"] = "deviceSettingsData";
  } else if (settingsType == "parolaSettings") {
    jsonDoc["action"] = "parolaSettingsData";
  } else if (settingsType == "weerliveSettings") {
    jsonDoc["action"] = "weerliveSettingsData";
  } else if (settingsType == "mediastackSettings") {
    jsonDoc["action"] = "mediastackSettingsData";
  } else {
    // Generic fallback
    jsonDoc["action"] = settingsType + "Data";
  }
  
  jsonDoc["data"] = jsonData;
  
  std::string jsonMessage;
  serializeJson(jsonDoc, jsonMessage);
  
  // Send the JSON message via WebSocket
  spa.ws.broadcastTXT(jsonMessage.c_str(), jsonMessage.length());
  
  // Call any specific JavaScript initialization functions if needed
  if (settingsType == "deviceSettings") {
    // If there's a specific initialization function for device settings
    // spa.callJsFunction("initializeDevSettings", jsonData.c_str());
  }

} // sendSettingFieldToClient()


// Function to send the JSON string to the client when deviceSettingsPage is activated
void sendDeviceFieldsToClient()
{
  sendSettingFieldToClient("deviceSettings");
}

// Function to send the JSON string to the client when weerliveSettingsPage is activated
void sendWeerliveFieldsToClient()
{
  sendSettingFieldToClient("weerliveSettings");
}

// Function to send the JSON string to the client when mediastackSettingsPage is activated
void sendMediastackFieldsToClient()
{
  sendSettingFieldToClient("mediastackSettings");
}

// Function to send the JSON string to the client when parolaSettingsPage is activated
void sendParolaFieldsToClient()
{
  sendSettingFieldToClient("parolaSettings");
}

//=============================================================================


// Generic function to process settings from the client
void processSettings(const std::string& jsonString, const std::string& settingsType)
{
  debug->printf("processSettings(): Processing %s settings from JSON:\n", settingsType.c_str());
  debug->println(jsonString.c_str());
  
  // Get the settings container for this type
  const SettingsContainer* container = settings.getSettingsContainer(settingsType);
  if (!container) {
    debug->printf("processSettings(): Unknown settings type: %s\n", settingsType.c_str());
    return;
  }
  
  // Use ArduinoJson library to parse the JSON
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) 
  {
    debug->printf("processSettings(): JSON parsing error: %s\n", error.c_str());
    return;
  }
  
  // Check if the JSON has the expected structure
  if (!doc.containsKey("fields") || !doc["fields"].is<JsonArray>()) 
  {
    debug->println("processSettings(): JSON does not contain fields array");
    return;
  }
  
  JsonArray fields = doc["fields"].as<JsonArray>();
  debug->printf("processSettings(): Processing array with %d fields\n", fields.size());
  
  // Get the field descriptors for this settings type
  const std::vector<FieldDescriptor>& fieldDescriptors = container->getFields();
  
  // Process each field from the JSON
  for (JsonObject field : fields) 
  {
    if (!field.containsKey("fieldName") || !field.containsKey("value")) 
    {
      debug->println("processSettings(): Field missing required properties");
      continue;
    }
    
    const char* fieldName = field["fieldName"];
    
    // Find the matching field descriptor
    auto it = std::find_if(fieldDescriptors.begin(), fieldDescriptors.end(),
                          [fieldName](const FieldDescriptor& fd) {
                            return fd.fieldName == fieldName;
                          });
    
    if (it != fieldDescriptors.end()) 
    {
      // Found the field descriptor, update the value
      const FieldDescriptor& descriptor = *it;
      
      if (descriptor.fieldType == "s") 
      {
        // String field
        std::string newValue = field["value"].as<std::string>();
        debug->printf("processSettings(): Setting %s to [%s]\n", fieldName, newValue.c_str());
        
        // Update the value using the field pointer
        std::string* valuePtr = static_cast<std::string*>(descriptor.fieldValue);
        *valuePtr = newValue;
      } 
      else if (descriptor.fieldType == "n") 
      {
        // Numeric field
        uint8_t newValue = field["value"].as<uint8_t>();
        debug->printf("processSettings(): Setting %s to [%d]\n", fieldName, newValue);
        
        // Update the value using the field pointer
        uint8_t* valuePtr = static_cast<uint8_t*>(descriptor.fieldValue);
        *valuePtr = newValue;
      }
    } 
    else 
    {
      debug->printf("processSettings(): Unknown field: %s\n", fieldName);
    }
  }
  
  // Save settings using the generic method
  debug->printf("processSettings(): Saving settings for type: %s\n", settingsType.c_str());
  settings.writeSettingFields(settingsType);
  
  // Send a confirmation message to the client
  DynamicJsonDocument confirmDoc(512);
  confirmDoc["type"] = "update";
  confirmDoc["target"] = "message";
  confirmDoc["content"] = "Settings saved successfully!";
  
  std::string confirmMessage;
  serializeJson(confirmDoc, confirmMessage);
  spa.ws.broadcastTXT(confirmMessage.c_str(), confirmMessage.length());
  
  // Refresh the settings display
  sendSettingFieldToClient(settingsType);

} // processSettings()


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
    
    // Check if this is a jsFunctionResult message
    if (doc["type"] == "jsFunctionResult") 
    {
      debug->println("handleLocalWebSocketEvent(): Handling jsFunctionResult message");
      
      // Extract the function name and result
      const char* functionName = doc["function"];
      const char* result = doc["result"];
      
      // Safely print the function name and result with null checks
      if (functionName && result) 
      {
        debug->printf("handleLocalWebSocketEvent(): JavaScript function [%s] returned result: %s\n", 
                      functionName, result);
        
        // Handle specific function results if needed
        if (strcmp(functionName, "queueMessageToMonitor") == 0) 
        {
          debug->printf("handleLocalWebSocketEvent(): queueMessageToMonitor result: %s\n", result);
          // Add any specific handling for queueMessageToMonitor results here
        }
      } else {
        debug->println("handleLocalWebSocketEvent(): Received jsFunctionResult with null function name or result");
        if (functionName) {
          debug->printf("handleLocalWebSocketEvent(): JavaScript function [%s] returned null result\n", functionName);
        } else if (result) {
          debug->printf("handleLocalWebSocketEvent(): Unknown JavaScript function returned result: %s\n", result);
        }
      }
      
      return;
    }
    
    // Check if this is a requestLocalMessages message
    if (doc["type"] == "requestLocalMessages") {
      debug->println("handleLocalWebSocketEvent(): Handling requestLocalMessages message");
      sendLocalMessagesToClient();
      return;
    }
    
    // Check if this is a requestDeviceSettings message
    if (doc["type"] == "requestDeviceSettings") {
      debug->println("handleLocalWebSocketEvent(): Handling requestDeviceSettings message");
      sendDeviceFieldsToClient();
      return;
    }
    
    // Check if this is a requestParolaSettings message
    if (doc["type"] == "requestParolaSettings") {
      debug->println("handleLocalWebSocketEvent(): Handling requestParolaSettings message");
      sendParolaFieldsToClient();
      return;
    }
    
    // Check if this is a requestWeerliveSettings message
    if (doc["type"] == "requestWeerliveSettings") {
      debug->println("handleLocalWebSocketEvent(): Handling requestWeerliveSettings message");
      sendWeerliveFieldsToClient();
      return;
    }
    // Check if this is a requestMediastackSettings message
    if (doc["type"] == "requestMediastackSettings") {
      debug->println("handleLocalWebSocketEvent(): Handling requestMediastackSettings message");
      sendMediastackFieldsToClient();
      return;
    }    
    // Check if this is a process message
    if (doc["type"] == "process") 
    {
      const char* processType = doc["processType"];
      debug->printf("handleLocalWebSocketEvent(): Processing message type: %s\n", processType);
      
      // Check if this is a saveLocalMessages message
      if (strcmp(processType, "saveLocalMessages") == 0) 
      {
        debug->println("handleLocalWebSocketEvent(): Handling saveLocalMessages message");
        
        // Check if inputValues exists and contains LocalMessagesData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("LocalMessagesData")) 
        {
          // Get the LocalMessagesData as a string
          const char* LocalMessagesData = doc["inputValues"]["LocalMessagesData"];
          debug->println("handleLocalWebSocketEvent(): Received input fields data:");
          debug->println(LocalMessagesData);
          
          // Process the input fields data
          processLocalMessages(LocalMessagesData);
        } else {
          debug->println("handleLocalWebSocketEvent(): No LocalMessagesData found in the message");
        }
      } 
      // Handle settings processing generically
      else if (strcmp(processType, "saveDeviceSettings") == 0) 
      {
        debug->println("handleLocalWebSocketEvent(): Handling saveDeviceSettings message");
        
        // Check if inputValues exists and contains deviceSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("deviceSettingsData")) 
        {
          // Get the deviceSettingsData as a string
          const char* deviceSettingsData = doc["inputValues"]["deviceSettingsData"];
          debug->println("handleLocalWebSocketEvent(): Received device settings data:");
          debug->println(deviceSettingsData);
          
          // Process the device settings data using the generic function
          processSettings(deviceSettingsData, "deviceSettings");
        } else {
          debug->println("handleLocalWebSocketEvent(): No deviceSettingsData found in the message");
        }
      }
      else if (strcmp(processType, "saveParolaSettings") == 0) 
      {
        debug->println("handleLocalWebSocketEvent(): Handling saveParolaSettings message");
        
        // Check if inputValues exists and contains parolaSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("parolaSettingsData")) 
        {
          // Get the parolaSettingsData as a string
          const char* parolaSettingsData = doc["inputValues"]["parolaSettingsData"];
          debug->println("handleLocalWebSocketEvent(): Received parola settings data:");
          debug->println(parolaSettingsData);
          
          // Process the parola settings data using the generic function
          processSettings(parolaSettingsData, "parolaSettings");
        } else {
          debug->println("handleLocalWebSocketEvent(): No parolaSettingsData found in the message");
        }
      }
      else if (strcmp(processType, "saveWeerliveSettings") == 0) 
      {
        debug->println("handleLocalWebSocketEvent(): Handling saveWeerliveSettings message");
        
        // Check if inputValues exists and contains weerliveSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("weerliveSettingsData")) 
        {
          // Get the weerliveSettingsData as a string
          const char* weerliveSettingsData = doc["inputValues"]["weerliveSettingsData"];
          debug->println("handleLocalWebSocketEvent(): Received weerlive settings data:");
          debug->println(weerliveSettingsData);
          
          // Process the weerlive settings data using the generic function
          processSettings(weerliveSettingsData, "weerliveSettings");
        } else {
          debug->println("handleLocalWebSocketEvent(): No weerliveSettingsData found in the message");
        }
      }
      else if (strcmp(processType, "saveMediastackSettings") == 0) 
      {
        debug->println("handleLocalWebSocketEvent(): Handling saveMediastackSettings message");
        
        // Check if inputValues exists and contains mediastackSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("mediastackSettingsData")) 
        {
          // Get the mediastackSettingsData as a string
          const char* mediastackSettingsData = doc["inputValues"]["mediastackSettingsData"];
          debug->println("handleLocalWebSocketEvent(): Received mediastack settings data:");
          debug->println(mediastackSettingsData);
          
          // Process the mediastack settings data using the generic function
          processSettings(mediastackSettingsData, "mediastackSettings");
        } else {
          debug->println("handleLocalWebSocketEvent(): No mediastackSettingsData found in the message");
        }
      } else {
        debug->printf("handleLocalWebSocketEvent(): Unknown process type: %s\n", processType);
      }
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
  // Get the current active page
  std::string activePage = spa.getActivePageName();
  debug->printf("pageIsLoadedCallback(): Current active page: %s\n", activePage.c_str());
  
  // Update the current active page
  currentActivePage = activePage;

} // pageIsLoadedCallback()


void localMessagesCallback()
{
  debug->println("localMessagesCallback(): Local Messages menu item clicked");
  
  spa.setErrorMessage("Main Menu \"Local Messages\" clicked!", 5);
  spa.activatePage("localMessagesPage");
  
  // Call the JavaScript function to set up event handlers
  // It will request the input fields data itself
  spa.callJsFunction("isEspTicker32Loaded");
    
} // localMessagesCallback()

    
void mainCallbackDeviceSettings()
{
  spa.setErrorMessage("Main Menu \"Dev Settings\" clicked!", 5);
  spa.activatePage("deviceSettingsPage");
  
  // Call the JavaScript function to set up event handlers
  spa.callJsFunction("isEspTicker32Loaded");
  
  // Send the device settings to the client
  sendDeviceFieldsToClient();

} //  mainCallbackDeviceSettings()


void mainCallbackParolaSettings()
{
  spa.setErrorMessage("Main Menu \"Parola Settings\" clicked!", 5);
  spa.activatePage("parolaSettingsPage");
  
  // Call the JavaScript function to set up event handlers
  spa.callJsFunction("isEspTicker32Loaded");
  
  // Send the weerlive settings to the client
  sendParolaFieldsToClient();

} // mainCallbackParolaSettings()


void mainCallbackWeerliveSettings()
{
  spa.setErrorMessage("Main Menu \"Weerlive Settings\" clicked!", 5);
  spa.activatePage("weerliveSettingsPage");
  
  // Call the JavaScript function to set up event handlers
  spa.callJsFunction("isEspTicker32Loaded");
  
  // Send the weerlive settings to the client
  sendWeerliveFieldsToClient();

} // mainCallbackWeerliveSettings()


void mainCallbackMediastackSettings()
{
  spa.setMessage("Main Menu \"Mediastack Settings\" clicked!", 5);
  spa.activatePage("mediastackSettingsPage");
  
  // Call the JavaScript function to set up event handlers
  spa.callJsFunction("isEspTicker32Loaded");
  
  // Send the mediastack settings to the client
  sendMediastackFieldsToClient();

} // mainCallbackMediastackSettings()

    
void mainCallbackSettings()
{
  debug->println("\nmainCallbackSettings(): Settings menu item clicked");
  spa.setErrorMessage("Main Menu \"Settings\" clicked!", 5);
  spa.activatePage("mainSettingsPage");

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
  if (inputValues.count("LocalMessagesData") > 0) 
  {
    const std::string& jsonData = inputValues.at("LocalMessagesData");
    debug->println("Received input fields data:");
    debug->println(jsonData.c_str());
    
    // Process the input fields data
    processLocalMessages(jsonData.c_str());
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
    } else if (itemName == "SET-UP") {
        spa.setMessage("Settings: \"Exit\" clicked!", 5);
        //spa.disableID("settings", "settingsTable");
        spa.activatePage("mainSettingsPage");
    } else if (itemName == "SET-EXIT") {
      spa.setMessage("Main Settings: \"Exit\" clicked!", 5);
      //spa.disableID("settings", "settingsTable");
      spa.activatePage("Main");
    }

} // handleMenuItem()

void setupMainPage()
{
    const char *mainPage = R"HTML(
    <div style="font-size: 48px; text-align: left; font-weight: bold;">Ticker Monitor</div>
    <pre id="scrollingMonitor" style="
    width: 80ch;
    height: 18em;
    overflow: hidden;
    font-family: 'Courier New', Courier, monospace;
    font-size: 1.2em;
    line-height: 1.3em;
    white-space: pre-wrap;
    border: 1px solid #ccc;
    padding: 0.5em;
    margin: 1em 0;
">
</pre>
)HTML";
    debug->println("\nsetupMainPage(): Setting up Main page");
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
    <div style="font-size: 48px; text-align: center; font-weight: bold;">Messages</div>
    <div id="dynamicInputContainer">
      <table id="inputTable" style="width: 100%; border-collapse: collapse;">
        <thead>
          <tr>
            <th style="text-align: left; padding: 8px;">(Local) Messages</th>
          </tr>
        </thead>
        <tbody id="inputTableBody">
          <!-- Input fields will be dynamically added here -->
        </tbody>
      </table>
      <div style="margin-top: 20px;">
        <button id="saveButton" onclick="saveLocalMessages()">Save</button>
        <!--<button id="addButton" onclick="addInputField()">Add</button>-->
      </div>
    </div>
    )HTML";

    const char *popupHelpLocalMessages = R"HTML(
    <style>
        li {
          display: grid;
          grid-template-columns: 130px 1fr; /* pas 150px aan indien nodig */
          gap: 1em;
          margin-bottom: 0.3em;
        }
        li::marker {
          content: none; /* verbergt het standaard bolletje */
        }
      </style>

      <div id="popupHelpLocalMessages">Help Sleutelwoorden</div>
      <div>
        <ul>
          <li><b>&lt;time&gt;</b> - Laat de tijd zien</li>
          <li><b>&lt;date&gt;</b> - Laat de datum zien</li>
          <li><b>&lt;datetime&gt;</b> - Laat de datum en tijd zien</li>
          <li><b>&lt;space&gt;</b> - Maakt de Ticker leeg</li>
          <li><b>&lt;weerlive&gt;</b> - Laat de weergegevens van weerlive zien</li>
          <li><b>&lt;mediastack&gt;</b> - Laat het volgende item van mediastack zien</li>
          <li><b>&lt;rssfeed&gt;</b> - Laat het volgende item van een rssfeed zien</li>
        </ul>
      </div>
      Let op! Deze sleutelwoorden moeten als enige in een regel staan!
      <br><button type="button" onClick="closePopup('popup_Help')">Close</button></br>
    )HTML";
        
    spa.addPage("localMessagesPage", localMessagesPage);
    spa.setPageTitle("localMessagesPage", "Local Messages");
    spa.addMenu("localMessagesPage", "Local Messages");
    spa.addMenuItemPopup("localMessagesPage", "Local Messages", "Help", popupHelpLocalMessages);
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


void setupSettingsPage()
{
  const char *settingsPage = R"HTML(
    <div id="settingsName" style="font-size: 48px; text-align: center; font-weight: bold;">Settings</div>
    <div id="dynamicSettingsContainer">
      <table id="settingsTable" style="width: 100%; border-collapse: collapse;">
        <thead>
          <tr>
            <th style="text-align: right; padding: 8px;">Setting</th>
            <th style="text-align: left; padding: 8px;">Value</th>
          </tr>
        </thead>
        <tbody id="settingsTableBody">
          <!-- Settings will be dynamically added here -->
        </tbody>
      </table>
      <div style="margin-top: 20px;">
        <button id="saveSettingsButton" onclick="saveSettings()">Save</button>
      </div>
    <div style="font-size: 12px; text-align: center;">
    <p>De meeste settings worden pas toegepast bij een <b>reset</b> van de espTicker32
    </div>
    )HTML";
  
  debug->println("\nsetupSettingsPage(): Adding settings page");
  spa.addPage("deviceSettingsPage", settingsPage);
  spa.setPageTitle("deviceSettingsPage", "Device Settings");
  spa.addMenu("deviceSettingsPage", "Device Settings");
  spa.addMenuItem("deviceSettingsPage", "Device Settings", "Exit", handleMenuItem, "SET-UP");
  
  spa.addPage("parolaSettingsPage", settingsPage);
  spa.setPageTitle("parolaSettingsPage", "Parola Settings");
  spa.addMenu("parolaSettingsPage", "Parola Settings");
  spa.addMenuItem("parolaSettingsPage", "Parola Settings", "Exit", handleMenuItem, "SET-UP");

  spa.addPage("weerliveSettingsPage", settingsPage);
  spa.setPageTitle("weerliveSettingsPage", "Weerlive Settings");
  spa.addMenu("weerliveSettingsPage", "Weerlive Settings");
  spa.addMenuItem("weerliveSettingsPage", "Weerlive Settings", "Exit", handleMenuItem, "SET-UP");

  spa.addPage("mediastackSettingsPage", settingsPage);
  spa.setPageTitle("mediastackSettingsPage", "Mediastack Settings");
  spa.addMenu("mediastackSettingsPage", "Mediastack Settings");
  spa.addMenuItem("mediastackSettingsPage", "Mediastack Settings", "Exit", handleMenuItem, "SET-UP");

  
} // setupSettingsPage()



void setupMainSettingsPage()
{
  const char *settingsPage = R"HTML(
    <div style="font-size: 48px; text-align: center; font-weight: bold;">Settings</div>
    <br>You can modify system settings here that influence the operation of the device.
    <ul>
    <li>Device settings</li>
    <li>Parola settings</li>
    <li>Weerlive settings</li>
    <li>Mediastack settings</li>
    </ul> 
    )HTML";
  
  debug->println("\nsetupMainSettingsPage(): Adding main settings page");
  spa.addPage("mainSettingsPage", settingsPage);
  spa.setPageTitle("mainSettingsPage", "Settings");
  //-- Add Settings menu
  spa.addMenu("mainSettingsPage", "Settings");
  spa.addMenuItem("mainSettingsPage", "Settings", "Device Settings", mainCallbackDeviceSettings);
  spa.addMenuItem("mainSettingsPage", "Settings", "Parola Settings", mainCallbackParolaSettings);
  spa.addMenuItem("mainSettingsPage", "Settings", "Weerlive Settings", mainCallbackWeerliveSettings);
  spa.addMenuItem("mainSettingsPage", "Settings", "Mediastack Settings", mainCallbackMediastackSettings);
  spa.addMenuItem("mainSettingsPage", "Settings", "Exit", handleMenuItem, "SET-EXIT");

} //  setupMainSettingsPage()


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


void setupParola()
{
    PAROLA config = {
        .HARDWARE_TYPE = 1, // FC16_HW
        .MAX_DEVICES = 32,
        .MAX_ZONES = 2,
        .MAX_SPEED = 2
    };

    display.begin(config);

    display.setRandomEffects({
        PA_SCROLL_LEFT,
        PA_SCROLL_RIGHT,
        PA_FADE,
        PA_WIPE
    });

    display.setCallback([](const String& finishedText) 
    {
      debug->print("[FINISHED] ");
      debug->println(finishedText);
      actMessage = nextMessage();
      //spa.callJsFunction("queueMessageToMonitor", actMessage.c_str());
    });

} // setupParola()


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
    //-test- listFiles("/", 0);

    settings.setDebug(debug);
    debug->println("setup(): readSettingFields(deviceSettings)");
    settings.readSettingFields("deviceSettings");
    debug->println("setup(): readSettingFields(parolaSettings)");
    settings.readSettingFields("parolaSettings");
    debug->println("setup(): readSettingFields(weerliveSettings)");
    settings.readSettingFields("weerliveSettings");
    debug->println("setup(): readSettingFields(mediastackSettings)");
    settings.readSettingFields("mediastackSettings");

    if (settings.hostname.empty()) 
    {
        debug->println("setup(): No hostname found in settings, using default");
        settings.hostname = std::string(hostName);
        settings.writeSettingFields("deviceSettings");
    }

    //-- Define custom NTP servers (optional)
    const char* ntpServers[] = {"time.google.com", "time.cloudflare.com", nullptr};

    //-- Start NTP with Amsterdam timezone and custom servers
    if (network->ntpStart("CET-1CEST,M3.5.0,M10.5.0/3", ntpServers))
    {
        debug->println("setup(): NTP started successfully");
    }
    
    localMessages.setDebug(debug);

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
    setupMainSettingsPage();
    setupSettingsPage();
    setupFSmanagerPage();

    debug->printf("setup(): weerliveAuthToken: [%s], weerlivePlaats: [%s], requestInterval: [%d minuten]\n",
                  settings.weerliveAuthToken.c_str(),
                  settings.weerlivePlaats.c_str(),
                  settings.weerliveRequestInterval);
    weerlive.setup(settings.weerliveAuthToken.c_str(), settings.weerlivePlaats.c_str(), debug);
    weerlive.setInterval(settings.weerliveRequestInterval); 

    debug->printf("setup(): mediastackAuthToken: [%s], requestInterval: [%d minuten], maxMessages: [%d], onlyDuringDay: [%s]\n",
                  settings.mediastackAuthToken.c_str(),
                  settings.mediastackRequestInterval,
                  settings.mediastackMaxMessages,
                  settings.mediastackOnlyDuringDay ? "true" : "false");
    mediastack.setup(settings.mediastackAuthToken.c_str(), settings.mediastackRequestInterval,
                     settings.mediastackMaxMessages,
                     settings.mediastackOnlyDuringDay,
                     debug);

    rssReader.setDebug(debug);
    
    rssReader.addRSSfeed("https://feeds.nos.nl/nosnieuwsopmerkelijk", 10);
    rssReader.addRSSfeed("https://feeds.nos.nl/nosnieuwsalgemeen", 5);
    rssReader.addRSSfeed("https://www.nrc.nl/rss/", 10);
    rssReader.addRSSfeed("https://feeds.nos.nl/nosnieuwsbuitenland", 5);
    rssReader.addRSSfeed("https://www.volkskrant.nl/voorpagina/rss.xml", 10);
    
    rssReader.setRequestInterval(11 * 60 * 1000); // 11 minutes

    spa.activatePage("Main");

    setupParola();
    actMessage = nextMessage();

    debug->println("Done with setup() ..\n");
} // setup()

void loop()
{
  network->loop();
  spa.server.handleClient();
  spa.ws.loop();
  std::string weerliveInfo = {};
  if (weerlive.loop(weerliveInfo))
  {
    snprintf(weerliveText, 2000, "%s", weerliveInfo.c_str());
    debug->printf("weerlive::loop(): weerliveText: [%s]\n", weerliveText);
  
  }
  if (mediastack.loop(network->ntpGetTmStruct()))
  {
    debug->println("mediastack::loop(): mediastack updated");
  }

  rssReader.loop(network->ntpGetTmStruct());

  //-- Print current time every 60 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 60000)
  {
    if (network->ntpIsValid())
         debug->printf("loop(): NTP time: %s\n", network->ntpGetTime());
    else debug->println("loop(): NTP is not valid");
    lastPrint = millis();
  }
  
  static uint32_t lastDisplay = 0;
  if (millis() - lastDisplay >= 2500)
  {
    display.loop();
    lastDisplay = millis();
  }

} // loop()