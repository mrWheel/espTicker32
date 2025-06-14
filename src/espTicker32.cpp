/*
**  espTicker32.cpp
*/
const char* PROG_VERSION = "v1.3.2";

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
#include "readLdrClass.h"

#include "SettingsClass.h"
#include "LocalMessagesClass.h"
#include "WeerliveClass.h"
#include "RSSreaderClass.h"
#ifdef USE_PAROLA
  //#include <MD_Parola.h>
  //#include <MD_MAX72xx.h>
  //#include <SPI.h>
  #include "ParolaClass.h"

  ParolaClass ticker; //-- constructor for the ParolaClass
#endif
#ifdef USE_NEOPIXELS
  #include "NeopixelsClass.h"

  NeopixelsClass ticker; //-- constructor for the NeopixelsClass
#endif

#define CLOCK_UPDATE_INTERVAL  1000
#define LOCAL_MESSAGES_PATH      "/localMessages.dat"
#define LOCAL_MESSAGES_RECORD_SIZE  150

#ifdef ESPTICKER32_DEBUG
  bool doDebug = true;
#else
  bool doDebug = false;
#endif

Networking* network = nullptr;
Stream* debug = nullptr;

SettingsClass settings;

LocalMessagesClass localMessages(LOCAL_MESSAGES_PATH, LOCAL_MESSAGES_RECORD_SIZE);

WiFiClient wifiClient;
Weerlive weerlive(wifiClient);
RSSreaderClass rssReader;

SPAmanager spa(80);
//WebServer server(80);
//-- we need to use the server from the SPAmanager!!
FSmanager fsManager(spa.server);

ReadLdrClass readLdr;

uint32_t systemUpTime = 0;
uint32_t lastCounterUpdate = 0;

const char *hostName = "espTicker32";
uint32_t  counter = 0;
//uint16_t nr = 1;  //-- name it like "parolaMessageNr"

std::string actMessage = "";
std::string gTickerMessage;
std::string gMonitorMessage;
char weerliveText[2000] = {};
char localMessage[LOCAL_MESSAGES_RECORD_SIZE +2] = {};

// Global variable to track the current active page
std::string currentActivePage = "";



/**
 * @brief Get the current message from the Weerlive data.
 *
 * This is a convenience function to get the current Weerlive message.
 * It returns the current value of `weerliveText`.
 *
 * @return The current message from Weerlive
 */
String getWeerliveMessage()
{
    return weerliveText;

} // getWeerliveMessage()


/**
 * @brief Get the current message from the RSS feeds.
 *
 * This function retrieves the next feed item from the RSS feeds and returns its content.
 * If the retrieved message is empty, it will try again with the next feed item indices.
 *
 * The returned string will contain the feed index, item index, and the RSS feed content.
 *
 * @return The current message from the RSS feeds
 */
String getRSSfeedMessage()
{
  uint8_t feedIndex = 0;
  size_t itemIndex = 0;
  char rssFeedMessage[1000] = {};

  // Get the next feed item indices
  if (rssReader.getNextFeedItem(feedIndex, itemIndex))
  {
    // Read the feed content
#if defined(NEOPIXELS_DEBUG) || defined(PAROLA_DEBUG)
    snprintf(rssFeedMessage, sizeof(rssFeedMessage), "[%d][%d/%d] %s"
                                          , feedIndex, itemIndex
                                          , rssReader.getActiveFeedCount(feedIndex)
                                          , rssReader.readRSSfeed(feedIndex, itemIndex).c_str());
#else
    snprintf(rssFeedMessage, sizeof(rssFeedMessage), "%s"
                                                , rssReader.readRSSfeed(feedIndex, itemIndex).c_str());
#endif
    // If we get an empty message, try again with the next indices
    if (strlen(rssFeedMessage) == 0 && rssReader.getNextFeedItem(feedIndex, itemIndex))
    {
      #if defined(NEOPIXELS_DEBUG) || defined(PAROLA_DEBUG)
      snprintf(rssFeedMessage, sizeof(rssFeedMessage), "[%d][%d/%d] %s"
                                            , feedIndex, itemIndex
                                            , rssReader.getActiveFeedCount(feedIndex)
                                            , rssReader.readRSSfeed(feedIndex, itemIndex).c_str());
#else
      snprintf(rssFeedMessage, sizeof(rssFeedMessage), "%s"
                                                  , rssReader.readRSSfeed(feedIndex, itemIndex).c_str());
#endif
    }
  }
  
  if (debug && doDebug) debug->printf("getRSSfeedMessage(): feedNr[%d], msgNr[%d] rssFeedMessage[%s]\n" 
                                  , feedIndex, itemIndex, rssFeedMessage);
  
  return rssFeedMessage;

} // getRSSfeedMessage()


/**
 * @brief Get a local message from the flash memory.
 * 
 * Loops through the flash memory records, checks if the key is enabled and if the message is valid.
 * If a valid message is found, it is returned. If no valid message is found, a default message is returned.
 * 
 * @return The local message from the flash memory.
 */
String getLocalMessage()
{
  static uint8_t msgNr = 0;
  bool foundValidMessage = false;
  uint8_t startMsgNr = msgNr; // Remember where we started
  bool completedFullLoop = false;
  
  //-- Check if ANY of the devShowLocalX settings are enabled
  bool anyKeyEnabled = settings.devShowLocalA || 
                       settings.devShowLocalB || 
                       settings.devShowLocalC || 
                       settings.devShowLocalD || 
                       settings.devShowLocalE || 
                       settings.devShowLocalF || 
                       settings.devShowLocalG || 
                       settings.devShowLocalH || 
                       settings.devShowLocalI || 
                       settings.devShowLocalJ;
  
  if (debug) 
  {
    debug->printf("A=%d B=%d C=%d D=%d E=%d F=%d G=%d H=%d I=%d J=%d\n",
                                  settings.devShowLocalA,
                                    settings.devShowLocalB,
                                      settings.devShowLocalC,
                                        settings.devShowLocalD,
                                          settings.devShowLocalE,
                                            settings.devShowLocalF,
                                              settings.devShowLocalG,
                                                settings.devShowLocalH,
                                                  settings.devShowLocalI,
                                                    settings.devShowLocalJ);
    debug->printf("anyKeyEnabled = [%s]\n", anyKeyEnabled ? "true" : "false");
  }
  //-- Loop until we find a valid message or have checked all messages
  do {
    //-- Read the message with the new structure
    LocalMessagesClass::MessageItem item = localMessages.read(msgNr++);
    
    if (debug && doDebug) debug->printf("getLocalMessage(): Read record [%d], key=[%c], [%s]\n", 
                                      msgNr-1, item.key, item.content.c_str());
    
    //-- If content is empty, we've reached the end of messages
    if (item.content.empty()) {
      if (debug && doDebug) debug->printf("getLocalMessage(): End of messages at record [%d]\n", msgNr-1);
      msgNr = 0; // Start over from 0
      
      //-- Mark that we've completed a full loop through all records
      if (startMsgNr == 0) {
        completedFullLoop = true;
      }
      
      //-- If we started in the middle and have now wrapped around to the beginning,
      //-- we need to continue checking from 0 up to our starting point
      continue;
    }
    
    if (item.key == 'A' && settings.devShowLocalA)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (item.key == 'B' && settings.devShowLocalB)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (item.key == 'C' && settings.devShowLocalC)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (item.key == 'D' && settings.devShowLocalD)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (item.key == 'E' && settings.devShowLocalE)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (item.key == 'F' && settings.devShowLocalF)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (item.key == 'G' && settings.devShowLocalG)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (item.key == 'H' && settings.devShowLocalH)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (item.key == 'I' && settings.devShowLocalI)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (item.key == 'J' && settings.devShowLocalJ)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): Found valid message with enabled key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    if (!anyKeyEnabled)
    {
      if (debug && doDebug) debug->printf("getLocalMessage(): anyKeyEnabled message key [%c]\n", item.key);
      snprintf(localMessage, sizeof(localMessage), "%s", item.content.c_str());
      foundValidMessage = true;
      break;
    }
    
    //-- If we've checked all records and looped back to or past our starting point,
    //-- or if we've completed a full loop, then break
    if ((msgNr > startMsgNr && completedFullLoop) || msgNr == startMsgNr) 
    {
      if (debug && doDebug) debug->println("getLocalMessage(): Checked all records, no valid message found");
      break;
    }
    
  } while (!foundValidMessage);
  
  //-- If no valid message was found, set a default message
  if (!foundValidMessage) 
  {
    snprintf(localMessage, sizeof(localMessage), "No message found!");
  }
  
  if (debug && doDebug) debug->printf("getLocalMessage(): msgNr = [%d] localMessage = [%s]\n", msgNr, localMessage);
  return localMessage;
} // getLocalMessage()


/**
 * @brief Gets the next message from the local message array, or a special message type.
 *
 * This function will get the next message from the local message array and return it.
 * It will also handle special message types, such as "<weerlive>", "<rssfeed>", "<date>", "<time>", "<datetime>", "<spaces>", "<feedInfo>", "<feedInfoReset>", "<clear>", and "<pixeltest>".
 * These special message types will return a different message than what is stored in the local message array.
 * 
 * @return The next message from the local message array, or a special message type.
 */
std::string nextMessage()
{
    std::string tmpMessage = "";
    std::string newMessage = getLocalMessage().c_str();
    static uint8_t feedNr = 0;

    ticker.setColor(200, 200, 200); // white

    if (strcasecmp(newMessage.c_str(), "<weerlive>") == 0) 
    {
        if (debug && doDebug) debug->println("nextMessage(): weerlive message");
        //newMessage = getWeerliveMessage().c_str();
        newMessage = rssReader.simplifyCharacters(getWeerliveMessage()).c_str();
        ticker.setColor(0, 0, 255); // Blue
    }
    else if (strcasecmp(newMessage.c_str(), "<rssfeed>") == 0) 
    {
        if (debug && doDebug) if (debug && doDebug) debug->println("nextMessage(): rssfeed message");
        newMessage = getRSSfeedMessage().c_str();
        ticker.setColor(0, 255, 0); // Green
    }
    else if (strcasecmp(newMessage.c_str(), "<date>") == 0) 
    {
        if (debug && doDebug) if (debug && doDebug) debug->println("nextMessage(): The Date");
        newMessage = network->ntpGetDateDMY();
        ticker.setColor(150, 0 , 150); // purple
    }
    else if (strcasecmp(newMessage.c_str(), "<time>") == 0) 
    {
        if (debug && doDebug) debug->println("nextMessage(): The Time");
        newMessage = network->ntpGetTime();
        ticker.setColor(200, 0 , 100); // 
    }
    else if (strcasecmp(newMessage.c_str(), "<datetime>") == 0) 
    {
        if (debug && doDebug) debug->println("nextMessage(): The Date & Time");
        newMessage = network->ntpGetDateTimeDMY();
        ticker.setColor(235, 0 , 235); // Magenta
    }
    else if (strcasecmp(newMessage.c_str(), "<spaces>") == 0) 
    {
        if (debug) debug->println("nextMessage(): <spaces>");
        newMessage = "                                                                                     ";
    }
    else if (strcasecmp(newMessage.c_str(), "<feedInfo>") == 0) 
    {
        if (debug) debug->println("nextMessage(): <feedInfo>");
        newMessage = rssReader.checkFeedHealth(feedNr).c_str();
        feedNr++;
        if (feedNr >= rssReader.getActiveFeedCount()) 
        {
            feedNr = 0;
        }
        ticker.setColor(255, 255 , 0); // Yellow
    }
    else if (strcasecmp(newMessage.c_str(), "<feedInfoReset>") == 0) 
    {
        if (debug) debug->println("nextMessage(): <feedInfoReset>");
        newMessage = "feedInfo reset";
        ticker.setColor(255, 255 , 0); // Yellow
        feedNr = 0;
    }
    else if (strcasecmp(newMessage.c_str(), "<clear>") == 0) 
    {
        if (debug) debug->println("nextMessage(): <clear>");
        ticker.tickerClear();
        tmpMessage = {0};
#ifdef USE_PAROLA
        for(int i=0; i<MAX_ZONES; i++)
        {
            tmpMessage += " ";
        }
#endif  // USE_PAROLA
        newMessage = tmpMessage;
    }
    else if (strcasecmp(newMessage.c_str(), "<pixeltest>") == 0) 
    {
        if (debug) debug->println("nextMessage(): <pixelTest>");
        tmpMessage = {0};
#ifdef USE_PAROLA
        for (int i=0; i<MAX_ZONES; i++)
        {
            tmpMessage += '\x7F';
        }
#endif
        newMessage = tmpMessage;  
    }
    if (doDebug)
    {
      if (debug) debug->printf("nextMessage(): Sending text: [%s]\n", newMessage.c_str()); 
      else       Serial.printf("nextMessage(): Sending text: [%s]\n", newMessage.c_str()); 
    }
    //gMonitorMessage = std::string("* ") + newMessage + std::string(" * ");
    gMonitorMessage = newMessage;
    spa.callJsFunction("queueMessageToMonitor", gMonitorMessage.c_str());
    gTickerMessage = std::string("* ") + newMessage + std::string(" *");
    ticker.sendNextText(gTickerMessage.c_str());

    return newMessage;

} // nextMessage()



/**
 * @brief Builds a JSON string with the local messages, including key and content.
 * 
 * This function will read all local messages from the LocalMessagesClass and build a JSON string with the format:
 * [
 *   {"key":"<key>","content":"<escaped content>"},
 *   {"key":"<key>","content":"<escaped content>"},
 *   ...
 * ]
 * 
 * The function will escape any quotes inside the content of the messages.
 * 
 * @return The JSON string with all local messages.
 */
std::string buildLocalMessagesJson()
{
  uint8_t recNr = 0;
  bool first = true;
  std::string jsonString = "[";
  
  while (true) {
    // Read the message with the new structure
    LocalMessagesClass::MessageItem item = localMessages.read(recNr++);
    
    // Check if we've reached the end of messages
    if (item.content.empty()) 
    {
      break;
    }

    // Escape quotes inside message
    std::string escaped;
    for (char c : item.content) 
    {
      if (c == '"') escaped += "\\\"";
      else escaped += c;
    }

    if (!first) 
    {
      jsonString += ",";
    }
    // Include both key and content in the JSON
    jsonString += "{\"key\":\"" + std::string(1, item.key) + "\",\"content\":\"" + escaped + "\"}";

    first = false;
  }

  jsonString += "]";
  return jsonString;
} // buildLocalMessagesJson()




/**
 * @brief Sends the local messages to the client via WebSocket.
 * 
 * This function builds a JSON string with all local messages, including key and content.
 * The function will escape any quotes inside the content of the messages.
 * 
 * The function will send the JSON string to the client via WebSocket, using the
 * "update" message type, targeting the "inputTableBody" element, and including the
 * raw JSON data in the "content" field.
 * 
 * Additionally, the function will send the raw JSON data in a format that SPAmanager
 * can understand, using the "custom" message type, with the action set to "LocalMessagesData"
 * and the data set to the JSON string.
 */
void sendLocalMessagesToClient()
{
  std::string jsonData = buildLocalMessagesJson();
  if (debug && doDebug) debug->printf("sendLocalMessagesToClient(): Sending JSON data to client: %s\n", jsonData.c_str());
  
  // First, send the HTML content for the input fields
  DynamicJsonDocument doc(4096);
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
  DynamicJsonDocument jsonDoc(4096);
  jsonDoc["type"] = "custom";
  jsonDoc["action"] = "LocalMessagesData";
  jsonDoc["data"] = jsonData;
  
  std::string jsonMessage;
  serializeJson(jsonDoc, jsonMessage);
  if (debug && doDebug) debug->printf("sendLocalMessagesToClient(): Sending JSON message to client: %s\n", jsonMessage.c_str());
  // Send the JSON message via WebSocket
  spa.ws.broadcastTXT(jsonMessage.c_str(), jsonMessage.length());
  
} // sendLocalMessagesToClient()



/**
 * @brief Process the input fields from a JSON string.
 * 
 * This function takes a JSON string, parses it, and writes the input fields to the LocalMessagesClass.
 * 
 * The JSON string should be an array of objects, each with a key and a content.
 * The key is a single character string, and the content is a string.
 * 
 * The function will write each input field to the LocalMessagesClass, with the key as the first character
 * and the content as the rest of the string.
 * 
 * The function will also send the input fields back to the client via WebSocket, using the "update" message type,
 * targeting the "inputTableBody" element, and including the raw JSON data in the "content" field.
 * 
 * Additionally, the function will send the raw JSON data in a format that SPAmanager can understand,
 * using the "custom" message type, with the action set to "LocalMessagesData" and the data set to the JSON string.
 * 
 * @param jsonString The JSON string to process.
 */
void processLocalMessages(const std::string& jsonString)
{
  uint8_t recNr = 0; // record number

  if (debug && doDebug) debug->println("processLocalMessages(): Processing input fields from JSON:");
  if (debug && doDebug) debug->println(jsonString.c_str());
  
  //-- Use ArduinoJson library to parse the JSON
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) 
  {
    if (debug && doDebug) debug->printf("processLocalMessages(): JSON parsing error: %s\n", error.c_str());
    return;
  }
  
  if (!doc.is<JsonArray>()) 
  {
    if (debug && doDebug) debug->println("processLocalMessages(): JSON is not an array");
    return;
  }
  
  JsonArray array = doc.as<JsonArray>();
  if (debug && doDebug) debug->printf("processLocalMessages(): Processing array with %d elements\n", array.size());
  
  for (size_t i = 0; i < array.size(); i++) {
    // Safely handle each value, even if it's null or empty
    if (array[i].isNull()) {
      if (debug && doDebug) debug->println("processLocalMessages(): Skip (null)\n");
    } else {
      // Create a MessageItem
      LocalMessagesClass::MessageItem item;
      
      // Check if the array element is an object with key and content
      if (array[i].is<JsonObject>() && array[i].containsKey("key") && array[i].containsKey("content")) {
        // Get key and content from the object
        item.key = array[i]["key"].as<std::string>()[0]; // Get first character of key string
        item.content = array[i]["content"].as<std::string>();
      } else {
        // Backward compatibility: treat as string content with default key 'A'
        item.key = 'A';
        item.content = array[i].as<std::string>();
      }
      
      if (debug && doDebug) debug->printf("processLocalMessages(): Input value[%d]: key=[%c], content=[%s]\n", 
                                         i, item.key, item.content.c_str());
      
      // Write the item
      if (localMessages.write(recNr, item))
      {
        recNr++;
      }
    }
  }
  
  if (debug && doDebug) debug->printf("processLocalMessages(): [%d] Local Messages written to [%s]\n", recNr, LOCAL_MESSAGES_PATH);
  sendLocalMessagesToClient();

} // processLocalMessages()




/**
 * @brief Send the settings fields to the client.
 * 
 * This function takes a settings type string, builds a JSON string for the settings fields,
 * and sends the JSON data to the client via WebSocket, using the "custom" message type,
 * with the action set to the settings type with "Data" appended to the end.
 * 
 * The function will also send the HTML content for the settings fields to the client via WebSocket,
 * using the "update" message type, targeting the "settingsTableBody" element, and including the raw JSON data in the "content" field.
 * 
 * Additionally, the function will send the raw JSON data in a format that SPAmanager can understand,
 * using the "custom" message type, with the action set to the settings type with "Data" appended to the end.
 * 
 * @param settingsType The settings type string.
 */
void sendSettingFieldToClient(const std::string& settingsType)
{
  std::string jsonData = settings.buildJsonFieldsString(settingsType);
  if (debug && doDebug) debug->printf("sendSettingFieldToClient(): Sending JSON data for %s to client: %s\n", settingsType.c_str(), jsonData.c_str());
  Serial.printf("sendSettingFieldToClient(): Sending JSON data for %s to client: %s\n", settingsType.c_str(), jsonData.c_str());
  
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
          } else if (settingsType == "neopixelsSettings") {
            htmlContent += "NeopixelsSettings";
          } else if (settingsType == "weerliveSettings") {
            htmlContent += "WeerliveSettings";
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
          } else if (settingsType == "neopixelsSettings") {
            htmlContent += "NeopixelsSettings";
          } else if (settingsType == "weerliveSettings") {
            htmlContent += "WeerliveSettings";
          } else if (settingsType == "rssfeedSettings") {
            htmlContent += "RSSfeedSettings";
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
  } else if (settingsType == "neopixelsSettings") {
    jsonDoc["action"] = "neopixelsSettingsData";
  } else if (settingsType == "weerliveSettings") {
    jsonDoc["action"] = "weerliveSettingsData";
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



// Send the device settings fields to the client.
//
// This function is a thin wrapper around sendSettingFieldToClient(), which
// does the actual work of sending the settings fields to the client.
//
// The device settings fields are loaded from the settings file and sent to
// the client in a structured format. The client can then use this data to
// populate the settings page.
void sendDeviceFieldsToClient()
{
  sendSettingFieldToClient("deviceSettings");
}


// Send the Weerlive settings fields to the client.
//
// This function is a thin wrapper around sendSettingFieldToClient(), which
// does the actual work of sending the settings fields to the client.
//
// The Weerlive settings fields are loaded from the settings file and sent to
// the client in a structured format. The client can then use this data to
// populate the Weerlive settings page.

void sendWeerliveFieldsToClient()
{
  sendSettingFieldToClient("weerliveSettings");
}




// Send the RSS feed settings fields to the client.
//
// This function is a thin wrapper around sendSettingFieldToClient(), which
// does the actual work of sending the settings fields to the client.
//
// The RSS feed settings fields are loaded from the settings file and sent to
// the client in a structured format. The client can then use this data to
// populate the RSS feed settings page.
void sendRssfeedFieldsToClient()
{
  sendSettingFieldToClient("rssfeedSettings");
}

#ifdef USE_PAROLA
// Function to send the JSON string to the client when parolaSettingsPage is activated
  void sendParolaFieldsToClient()
  {
    sendSettingFieldToClient("parolaSettings");
  }
#endif

#ifdef USE_NEOPIXELS
// Function to send the JSON string to the client when neopixelsSettingsPage is activated
  void sendNeopixelsFieldsToClient()
  {
    sendSettingFieldToClient("neopixelsSettings");
  }
#endif

//=============================================================================



/**
 * Process a JSON string containing updated settings values for the specified
 * settings type (e.g. "deviceSettings", "parolaSettings", etc.).
 *
 * This function parses the JSON string and updates the corresponding settings
 * values using the pointers stored in the SettingsContainer. It also saves
 * the settings to the settings file using the generic method.
 *
 * @param jsonString the JSON string containing the updated settings values
 * @param settingsType the type of settings (e.g. "deviceSettings", "parolaSettings", etc.)
 */
void processSettings(const std::string& jsonString, const std::string& settingsType)
{
  if (debug && doDebug) debug->printf("processSettings(): Processing %s settings from JSON:\n", settingsType.c_str());
  if (debug && doDebug) debug->println(jsonString.c_str());
  
  // Get the settings container for this type
  const SettingsContainer* container = settings.getSettingsContainer(settingsType);
  if (!container) {
    if (debug && doDebug) debug->printf("processSettings(): Unknown settings type: %s\n", settingsType.c_str());
    return;
  }
  
  // Use ArduinoJson library to parse the JSON
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) 
  {
    if (debug && doDebug) debug->printf("processSettings(): JSON parsing error: %s\n", error.c_str());
    return;
  }
  
  // Check if the JSON has the expected structure
  if (!doc.containsKey("fields") || !doc["fields"].is<JsonArray>()) 
  {
    if (debug && doDebug) debug->println("processSettings(): JSON does not contain fields array");
    return;
  }
  
  JsonArray fields = doc["fields"].as<JsonArray>();
  if (debug && doDebug) debug->printf("processSettings(): Processing array with %d fields\n", fields.size());
  
  // Get the field descriptors for this settings type
  const std::vector<FieldDescriptor>& fieldDescriptors = container->getFields();
  
  // Process each field from the JSON
  for (JsonObject field : fields) 
  {
    if (!field.containsKey("fieldName") || !field.containsKey("value")) 
    {
      if (debug && doDebug) debug->println("processSettings(): Field missing required properties");
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
        if (debug && doDebug) debug->printf("processSettings(): Setting %s to [%s]\n", fieldName, newValue.c_str());
        
        // Update the value using the field pointer
        std::string* valuePtr = static_cast<std::string*>(descriptor.fieldValue);
        *valuePtr = newValue;
      } 
      else if (descriptor.fieldType == "n") 
      {
        // Numeric field
        int16_t newValue = field["value"].as<int16_t>();
        if (debug && doDebug) debug->printf("processSettings(): Setting %s to [%d]\n", fieldName, newValue);
        
        // Update the value using the field pointer
        int16_t* valuePtr = static_cast<int16_t*>(descriptor.fieldValue);
        *valuePtr = newValue;
      }
      else if (descriptor.fieldType == "b") 
      {
        // Boolean field - convert "on"/"off" string to boolean
        std::string strValue = field["value"].as<std::string>();
        bool newValue = false;
        
        // Convert string to boolean (on/off, true/false, yes/no, 1/0)
        if (strValue == "on" || strValue == "true" || strValue == "yes" || strValue == "1") {
          newValue = true;
        }
        
        if (debug && doDebug) debug->printf("processSettings(): Setting %s to [%s]\n", 
                                           fieldName, newValue ? "true" : "false");
        
        // Update the value using the field pointer
        bool* valuePtr = static_cast<bool*>(descriptor.fieldValue);
        *valuePtr = newValue;
      }
    } 
    else 
    {
      if (debug && doDebug) debug->printf("processSettings(): Unknown field: %s\n", fieldName);
    }
  }
  
  // Save settings using the generic method
  if (debug && doDebug) debug->printf("processSettings(): Saving settings for type: %s\n", settingsType.c_str());
  //-not yet- spa.setPopupMessage("Saving settings ...", 1);
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



/**
 * @brief Handle WebSocket events when no other event handlers match
 * 
 * This function is called when a WebSocket event is received and no other event
 * handlers match the event type. It handles a variety of events, including
 * jsFunctionResult, requestLocalMessages, requestDeviceSettings, requestParolaSettings,
 * requestNeopixelsSettings, requestWeerliveSettings, requestRssfeedSettings, process,
 * saveLocalMessages, saveDeviceSettings, saveParolaSettings, saveNeopixelsSettings,
 * saveWeerliveSettings, and saveRssfeedSettings messages.
 * 
 * @param num The WebSocket client number
 * @param type The WebSocket event type
 * @param payload The WebSocket event payload
 * @param length The length of the WebSocket event payload
 */
void handleLocalWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  // This function will be called when no other event handlers match
  if (debug) if (debug && doDebug) debug->printf("handleLocalWebSocketEvent(): WebSocket event type: %d\n", type);
  
  // Check the WebSocket event type
  if (type == WStype_TEXT) {
    // Handle text data
    if (debug && doDebug)if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received text data that wasn't handled by other event handlers");
    
    // Ensure null termination of the payload
    payload[length] = 0;
    
    // Try to parse the JSON message
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      if (debug && doDebug) debug->printf("handleLocalWebSocketEvent(): JSON parsing error: %s\n", error.c_str());
      if (debug) debug->printf("handleLocalWebSocketEvent(): Payload: %s\n", payload);
      return;
    }
    
    // Check if this is a jsFunctionResult message
    if (doc["type"] == "jsFunctionResult") 
    {
      if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling jsFunctionResult message");
      
      // Extract the function name and result
      const char* functionName = doc["function"];
      const char* result = doc["result"];
      
      // Safely print the function name and result with null checks
      if (functionName && result) 
      {
        if (debug && doDebug) debug->printf("handleLocalWebSocketEvent(): JavaScript function [%s] returned result: %s\n", 
                      functionName, result);
        
        // Handle specific function results if needed
        if (strcmp(functionName, "queueMessageToMonitor") == 0) 
        {
          if (debug && doDebug) debug->printf("handleLocalWebSocketEvent(): queueMessageToMonitor result: %s\n", result);
          // Add any specific handling for queueMessageToMonitor results here
        }
      } else {
        if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received jsFunctionResult with null function name or result");
        if (functionName) {
          if (debug && doDebug) debug->printf("handleLocalWebSocketEvent(): JavaScript function [%s] returned null result\n", functionName);
        } else if (result) {
          if (debug && doDebug) debug->printf("handleLocalWebSocketEvent(): Unknown JavaScript function returned result: %s\n", result);
        }
      }
      
      return;
    }
    
    // Check if this is a requestLocalMessages message
    if (doc["type"] == "requestLocalMessages") {
      if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling requestLocalMessages message");
      sendLocalMessagesToClient();
      return;
    }
    
    // Check if this is a requestDeviceSettings message
    if (doc["type"] == "requestDeviceSettings") {
      if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling requestDeviceSettings message");
      sendDeviceFieldsToClient();
      return;
    }
#ifdef USE_PAROLA
    // Check if this is a requestParolaSettings message
    if (doc["type"] == "requestParolaSettings") {
      if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling requestParolaSettings message");
      sendParolaFieldsToClient();
      return;
    }
#endif
#ifdef USE_NEOPIXELS
    // Check if this is a requestNeopixelsSettings message
    if (doc["type"] == "requestNeopixelsSettings") {
      if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling requestNeopixelsSettings message");
      sendNeopixelsFieldsToClient();
      return;
    }
#endif
    // Check if this is a requestWeerliveSettings message
    if (doc["type"] == "requestWeerliveSettings") {
      if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling requestWeerliveSettings message");
      sendWeerliveFieldsToClient();
      return;
    }
    // Check if this is a requestRssfeedSettings message
    if (doc["type"] == "requestRssfeedSettings") {
      if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling requestRssfeedSettings message");
      sendRssfeedFieldsToClient();
      return;
    }    
    // Check if this is a process message
    if (doc["type"] == "process") 
    {
      const char* processType = doc["processType"];
      if (debug && doDebug) debug->printf("handleLocalWebSocketEvent(): Processing message type: %s\n", processType);
      
      // Check if this is a saveLocalMessages message
      if (strcmp(processType, "saveLocalMessages") == 0) 
      {
        if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling saveLocalMessages message");
      //spa.setPopupMessage("Saving Local Messages ...", 1);
        
        // Check if inputValues exists and contains LocalMessagesData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("LocalMessagesData")) 
        {

          // Get the LocalMessagesData as a string
          const char* LocalMessagesData = doc["inputValues"]["LocalMessagesData"];
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received input fields data:");
          if (debug && doDebug) debug->println(LocalMessagesData);
          
          // Process the input fields data
          processLocalMessages(LocalMessagesData);
        } else {
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): No LocalMessagesData found in the message");
        }
      } // saveLocalMessages 
      // Handle settings processing generically
      else if (strcmp(processType, "saveDeviceSettings") == 0) 
      {
        if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling saveDeviceSettings message");
        
        // Check if inputValues exists and contains deviceSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("deviceSettingsData")) 
        {
          // Get the deviceSettingsData as a string
          const char* deviceSettingsData = doc["inputValues"]["deviceSettingsData"];
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received device settings data:");
          if (debug && doDebug) debug->println(deviceSettingsData);
          
          // Process the device settings data using the generic function
          processSettings(deviceSettingsData, "deviceSettings");
        } else {
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): No deviceSettingsData found in the message");
        }
      } // saveDeviceSettings
#ifdef USE_PAROLA
      else if (strcmp(processType, "saveParolaSettings") == 0) 
      {
        if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling saveParolaSettings message");
        
        // Check if inputValues exists and contains parolaSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("parolaSettingsData")) 
        {
          // Get the parolaSettingsData as a string
          const char* parolaSettingsData = doc["inputValues"]["parolaSettingsData"];
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received parola settings data:");
          if (debug && doDebug) debug->println(parolaSettingsData);
          
          // Process the parola settings data using the generic function
          processSettings(parolaSettingsData, "parolaSettings");
        } else {
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): No parolaSettingsData found in the message");
        }
      } // saveParolaSettings
#endif
#ifdef USE_NEOPIXELS
      else if (strcmp(processType, "saveNeopixelsSettings") == 0) 
      {
        if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling saveNeopixelsSettings message");
        
        // Check if inputValues exists and contains neopixelsSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("neopixelsSettingsData")) 
        {
          // Get the neopixelsSettingsData as a string
          const char* neopixelsSettingsData = doc["inputValues"]["neopixelsSettingsData"];
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received neopixels settings data:");
          if (debug && doDebug) debug->println(neopixelsSettingsData);
          
          // Process the neopixels settings data using the generic function
          processSettings(neopixelsSettingsData, "neopixelsSettings");
        } else {
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): No neopixelsSettingsData found in the message");
        }
      } // saveNeopixelsSettings
#endif
      else if (strcmp(processType, "saveWeerliveSettings") == 0) 
      {
        if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling saveWeerliveSettings message");
        
        // Check if inputValues exists and contains weerliveSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("weerliveSettingsData")) 
        {
          // Get the weerliveSettingsData as a string
          const char* weerliveSettingsData = doc["inputValues"]["weerliveSettingsData"];
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received weerlive settings data:");
          if (debug && doDebug) debug->println(weerliveSettingsData);
          
          // Process the weerlive settings data using the generic function
          processSettings(weerliveSettingsData, "weerliveSettings");
        } else {
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): No weerliveSettingsData found in the message");
        }
      } // saveWeerliveSettings
      else if (strcmp(processType, "saveRssfeedSettings") == 0) 
      {
        if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Handling saveRssfeedSettings message");
        
        // Check if inputValues exists and contains rssfeedSettingsData
        if (doc.containsKey("inputValues") && doc["inputValues"].containsKey("rssfeedSettingsData")) 
        {
          // Get the rssfeedSettingsData as a string
          const char* rssfeedSettingsData = doc["inputValues"]["rssfeedSettingsData"];
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received rssfeed settings data:");
          if (debug && doDebug) debug->println(rssfeedSettingsData);
          
          // Process the rssfeed settings data using the generic function
          processSettings(rssfeedSettingsData, "rssfeedSettings");
        } else {
          if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): No rssfeedSettingsData found in the message");
        }
      } else {
        if (debug) debug->printf("handleLocalWebSocketEvent(): Unknown process type: %s\n", processType);
      } // saveRssfeedSettings

    } // processType
  }
  else if (type == WStype_BIN) {
    // Handle binary data
    if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received binary data that wasn't handled by other event handlers");
    // Process the binary payload...
  }
  else if (type == WStype_ERROR) {
    // Handle error events
    if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): WebSocket error occurred");
  }
  else if (type == WStype_FRAGMENT) {
    // Handle fragmented messages
    if (debug && doDebug) debug->println("handleLocalWebSocketEvent(): Received message fragment");
  }
  // Add other event types as needed
  
} // handleLocalWebSocketEvent()

/**
 * @brief Callback function for the pageIsLoaded event in the SPAmanager
 * 
 * This function is called when the pageIsLoaded event is fired by the SPAmanager.
 * It is used to update the currentActivePage variable with the name of the currently
 * active page.
 */
void pageIsLoadedCallback()
{
  // Get the current active page
  std::string activePage = spa.getActivePageName();
  if (debug) debug->printf("pageIsLoadedCallback(): Current active page: %s\n", activePage.c_str());
  
  // Update the current active page
  currentActivePage = activePage;

} // pageIsLoadedCallback()


/**
 * @brief Callback function for handling actions related to the Local Messages menu.
 *
 * This function is triggered when a menu item associated with Local Messages is clicked.
 * Depending on the provided item name, it activates the corresponding page and sets a message.
 *
 * @param itemName The name of the menu item clicked. Expected values are:
 *                 - "LM-START": Activates the "localMessagesPage" and calls a JavaScript function
 *                               to set up event handlers.
 *                 - "LM-EXIT": Activates the "mainPage" and sets an exit message.
 */

void localMessagesCallback(std::string itemName)
{
  if (debug && doDebug) debug->println("localMessagesCallback(): Local Messages menu item clicked");
  
  if (itemName == "LM-START") {
    spa.setMessage("Main Menu [Local Messages] clicked!", 0);
    spa.activatePage("localMessagesPage");
    // Call the JavaScript function to set up event handlers
    // It will request the input fields data itself
    spa.callJsFunction("isEspTicker32Loaded");
  } else if (itemName == "LM-EXIT") {
    spa.setMessage("localMessages Menu [Exit] clicked!", 0);
    spa.activatePage("mainPage");
  }
    
} // localMessagesCallback()

    
/**
 * @brief Callback function for the Device Settings menu item in the main menu.
 *
 * This function is called when the Device Settings menu item is clicked in the main menu.
 * It activates the "deviceSettingsPage" page, calls the JavaScript function "isEspTicker32Loaded"
 * to set up event handlers, and sends the device settings to the client using the
 * sendDeviceFieldsToClient() function.
 */
void mainCallbackDeviceSettings()
{
  spa.setMessage("Main Menu [Dev Settings] clicked!", 0);
  spa.activatePage("deviceSettingsPage");
  
  // Call the JavaScript function to set up event handlers
  spa.callJsFunction("isEspTicker32Loaded");
  
  // Send the device settings to the client
  sendDeviceFieldsToClient();

} //  mainCallbackDeviceSettings()

#ifdef USE_PAROLA
void mainCallbackParolaSettings()
{
  spa.setMessage("Main Menu Parola Settings] clicked!", 0);
  spa.activatePage("parolaSettingsPage");
  
  // Call the JavaScript function to set up event handlers
  spa.callJsFunction("isEspTicker32Loaded");
  
  // Send the weerlive settings to the client
  sendParolaFieldsToClient();

} // mainCallbackParolaSettings()
#endif

#ifdef USE_NEOPIXELS
/**
 * @brief Callback function for the Neopixels Settings menu item in the main menu.
 *
 * This function is called when the Neopixels Settings menu item is clicked in the main menu.
 * It activates the "neopixelsSettingsPage" page, calls the JavaScript function "isEspTicker32Loaded"
 * to set up event handlers, and sends the neopixels settings to the client using the
 * sendNeopixelsFieldsToClient() function.
 */

void mainCallbackNeopixelsSettings()
{
  spa.setMessage("Main Menu Neopixels Settings] clicked!", 0);
  spa.activatePage("neopixelsSettingsPage");
  
  // Call the JavaScript function to set up event handlers
  spa.callJsFunction("isEspTicker32Loaded");
  
  // Send the weerlive settings to the client
  sendNeopixelsFieldsToClient();

} // mainCallbackNeopixelsSettings()
#endif

/**
 * @brief Callback function for the Weerlive Settings menu item in the main menu.
 *
 * This function is called when the Weerlive Settings menu item is clicked in the main menu.
 * It activates the "weerliveSettingsPage" page, calls the JavaScript function "isEspTicker32Loaded"
 * to set up event handlers, and sends the weerlive settings to the client using the
 * sendWeerliveFieldsToClient() function.
 */

void mainCallbackWeerliveSettings()
{
  spa.setMessage("Main Menu [Weerlive Settings] clicked!", 0);
  spa.activatePage("weerliveSettingsPage");
  
  // Call the JavaScript function to set up event handlers
  spa.callJsFunction("isEspTicker32Loaded");
  
  // Send the weerlive settings to the client
  sendWeerliveFieldsToClient();

} // mainCallbackWeerliveSettings()



/**
 * @brief Callback function for the RSSfeed Settings menu item in the main menu.
 *
 * This function is called when the RSSfeed Settings menu item is clicked in the main menu.
 * It activates the "rssfeedSettingsPage" page, calls the JavaScript function "isEspTicker32Loaded"
 * to set up event handlers, and sends the RSS feed settings to the client using the
 * sendRssfeedFieldsToClient() function.
 */

void mainCallbackRssfeedSettings()
{
  spa.setMessage("Main Menu [RSSfeed Settings] clicked!", 5);
  spa.activatePage("rssfeedSettingsPage");
  
  // Call the JavaScript function to set up event handlers
  spa.callJsFunction("isEspTicker32Loaded");
  
  // Send the RssFeed settings to the client
  sendRssfeedFieldsToClient();

} // mainCallbackRssfeedettings()

    
/**
 * @brief Callback function for the Settings menu item in the main menu.
 *
 * This function is called when the Settings menu item is clicked in the main menu.
 * It sets a message in the status bar and activates the "mainSettingsPage" page.
 */
void mainCallbackSettings()
{
  if (debug && doDebug) debug->println("\nmainCallbackSettings(): Settings menu item clicked");
  spa.setMessage("Main Menu [Settings] clicked!", 0);
  spa.activatePage("mainSettingsPage");

} // mainCallbackSettings()    


/**
 * @brief Callback function for the "FSmanager" menu item in the main menu.
 *
 * This function is called when the "FSmanager" menu item is clicked in the main menu.
 * It sets a message in the status bar, activates the "FSmanagerPage" page, and calls the
 * JavaScript function "loadFileList" to load the list of files in LittleFS.
 */
void mainCallbackFSmanager()
{
    spa.setMessage("Main Menu \"FSmanager\" clicked!", 5);
    spa.activatePage("FSmanagerPage");
    spa.callJsFunction("loadFileList");

} // mainCallbackFSmanager()


/**
 * @brief Callback function for the "Process" button in the file upload form.
 *
 * This function is called when the "Process" button is clicked in the file upload form.
 * It currently does nothing but logs a message to the console.
 */
void processUploadFileCallback()
{
  if (debug && doDebug) debug->println("Process processUploadFileCallback(): proceed action received");

} // processUploadFileCallback()


/**
 * @brief Call a JavaScript function with the given name.
 *
 * This function is called with the name of a JavaScript function as a parameter.
 * It logs a message to the console with the name of the function, and then calls
 * the JavaScript function using the @ref SPAmanager::callJsFunction() method.
 *
 * @param[in] functionName The name of the JavaScript function to call.
 */
void doJsFunction(std::string functionName)
{
    if (debug && doDebug) debug->printf("doJsFunction(): JavaScript function called with: [%s]\n", functionName.c_str());
    
    // Call the JavaScript function with the given name
    if (functionName == "isEspTicker32Loaded") {
        spa.callJsFunction("isEspTicker32Loaded");
    } else if (functionName == "isFSmanagerLoaded") {
        spa.callJsFunction("isFSmanagerLoaded");
    }
    
    // You can add more conditions for other functions as needed

} // doJsFunction()


/**
 * @brief Callback function for the input fields.
 *
 * This function is called when the "Process" button is clicked in the input fields form.
 * It processes the input fields data and updates the local messages.
 *
 * @param[in] inputValues A map of input values, with the key being the field name and the value being the field value.
 */
void processInputCallback(const std::map<std::string, std::string>& inputValues)
{
  if (debug && doDebug) debug->println("Process callback: proceed action received");
  if (debug && doDebug) debug->printf("Received %d input values\n", inputValues.size());
  
  // Check if this is our custom input fields data
  if (inputValues.count("LocalMessagesData") > 0) 
  {
    const std::string& jsonData = inputValues.at("LocalMessagesData");
    if (debug && doDebug) debug->println("Received input fields data:");
    if (debug && doDebug) debug->println(jsonData.c_str());
    
    // Process the input fields data
    processLocalMessages(jsonData.c_str());
  }
  
}


/**
 * @brief Handle a menu item click event.
 *
 * This function is called when a menu item is clicked.
 * It handles the menu item click event by activating the corresponding page
 * and calling the corresponding JavaScript function.
 * 
 * @param[in] itemName The menu item name that was clicked.
 */
void handleMenuItem(std::string itemName)
{
    if (debug && doDebug) debug->printf("handleMenuItem(): Menu item clicked: %s\n", itemName.c_str());
    
    if (itemName == "FSM-1") {
        spa.setMessage("FS Manager: [List LittleFS] clicked!", 5);
        spa.activatePage("FSmanagerPage");
        spa.callJsFunction("loadFileList");
    } else if (itemName == "FSM-EXIT") {
        spa.setMessage("FS manager: [Exit] clicked!", 5);
        spa.activatePage("Main");
    } else if (itemName == "LM-EXIT") {
        spa.setMessage("Local Messages: [Exit] clicked!", 5);
        spa.activatePage("Main");
    } else if (itemName == "SET-UP") {
        spa.setMessage("Settings: [Exit] clicked!", 5);
        spa.activatePage("mainSettingsPage");
    } else if (itemName == "SET-RESTART") {
      spa.setMessage("Main Settings: [Restart] clicked!", 5);
      spa.activatePage("Main");
      ESP.restart();
      delay(1000);
    } else if (itemName == "SET-EXIT") {
      spa.setMessage("Main Settings: [Exit] clicked!", 5);
      spa.activatePage("Main");
    }

} // handleMenuItem()

/**
 * @brief Set up the main page.
 *
 * This function sets up the main page, which shows the ticker monitor.
 * It adds a page with the name "Main" and sets the page title.
 * It also adds two menus: "File" and "Edit".
 * Under the "File" menu, it adds two menu items: "FSmanager", which
 * calls the callback function mainCallbackFSmanager(), and "Restart
 * espTicker32", which calls the callback function handleMenuItem() with
 * the argument "SET-RESTART".
 * Under the "Edit" menu, it adds two menu items: "LocalMessages", which
 * calls the callback function localMessagesCallback() with the argument
 * "LM-START", and "Settings", which calls the callback function
 * mainCallbackSettings().
 */
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
    if (debug && doDebug) debug->println("\nsetupMainPage(): Setting up Main page");
    spa.addPage("Main", mainPage);
    char pageTitle[32];
    snprintf(pageTitle, sizeof(pageTitle), "espTicker32  -  %s", PROG_VERSION);
    spa.setPageTitle("Main", pageTitle);

    //-- Add File menu
    spa.addMenu("Main", "File");
    spa.addMenuItem("Main", "File", "FSmanager", mainCallbackFSmanager);
    spa.addMenuItem("Main", "File", "Restart espTicker32", handleMenuItem, "SET-RESTART");

    //-- Add Edit menu
    spa.addMenu("Main", "Edit");
    spa.addMenuItem("Main", "Edit", "LocalMessages", localMessagesCallback, "LM-START");
    spa.addMenuItem("Main", "Edit", "Settings", mainCallbackSettings);
    
} // setupMainPage()

/**
 * @brief Set up the local messages page.
 *
 * This function sets up the local messages page, which shows the local messages.
 * It adds a page with the name "localMessagesPage" and sets the page title.
 * It also adds two menus: "File" and "Local Messages".
 * Under the "File" menu, it adds two menu items: "FSmanager", which
 * calls the callback function mainCallbackFSmanager(), and "Restart
 * espTicker32", which calls the callback function handleMenuItem() with
 * the argument "SET-RESTART".
 * Under the "Local Messages" menu, it adds one menu item: "Help", which
 * calls the callback function popupHelpLocalMessages(), and one menu item: "Exit",
 * which calls the callback function handleMenuItem() with the argument "LM-EXIT".
 */
void setupLocalMessagesPage()
{
    const char *localMessagesPage = R"HTML(
    <div style="height: 80px; font-size: 48px; text-align: center; font-weight: bold;">
      Messages
    </div>

    <div id="dynamicInputContainer" style="
      max-height: calc(100vh - 110px - 100px); 
      border: 1px solid #ccc; 
      display: flex;
      flex-direction: column;
    ">

      <!-- Table Header (static) -->
      <table style="width: 100%; border-collapse: collapse;">
        <thead>
          <tr>
            <th style="text-align: left; padding: 8px;">(Local) Messages</th>
          </tr>
        </thead>
      </table>

      <!-- Scrollable Table Body with persistent scrollbar -->
      <div style="flex: 1; overflow-y: scroll;">
        <table id="inputTable" style="width: 100%; border-collapse: collapse;">
          <tbody id="inputTableBody">
            <!-- Rows dynamically added here -->
          </tbody>
        </table>
      </div>

      <!-- Buttons / controls -->
      <div style="margin-top: 20px; padding: 8px;">
        <button id="saveButton" onclick="saveLocalMessages()">Save</button>
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
        <li><b>&lt;rssfeed&gt;</b> - Laat het volgende item van een rssfeed zien</li>
        <li><b>&lt;feedinfo&gt;</b> - Laat gegevens over een feed zien (één voor één)</li>
        <li><b>&lt;feedinforeset&gt;</b> - Reset feedinfo naar feed 0</li>
      </ul>
    </div>
    Let op! Deze sleutelwoorden moeten als enige in een regel staan!
    <br><button type="button" onClick="closePopup('popup_Help')">Close</button></br>
  )HTML";
    spa.addPage("localMessagesPage", localMessagesPage);
    spa.setPageTitle("localMessagesPage", "Local Messages");
    //-- Add Main menu
    spa.addMenu("localMessagesPage", "File");
    spa.addMenuItem("localMessagesPage", "File", "FSmanager", mainCallbackFSmanager);
    spa.addMenuItem("localMessagesPage", "File", "Restart espTicker32", handleMenuItem, "SET-RESTART");

    //-- Add LocalMessages menu
    spa.addMenu("localMessagesPage", "Local Messages");
    spa.addMenuItemPopup("localMessagesPage", "Local Messages", "Help", popupHelpLocalMessages);
    spa.addMenuItem("localMessagesPage", "Local Messages", "Exit", handleMenuItem, "LM-EXIT");
  
} // setupLocalMessagesPage()


/**
 * @brief Set up the filesystem manager page.
 *
 * This function sets up the FSmanagerPage, which provides an interface for managing files 
 * and folders in the file system. It adds the page to the SPA manager and sets the page title.
 * It also adds a "File" menu with a "Restart espTicker32" option and a "FS Manager" menu 
 * with options to upload files, create folders, and exit the page. Additionally, popups for 
 * uploading files and creating folders are defined and linked to the menu items.
 */

void setupFSmanagerPage()
{
    if (debug && doDebug) debug->printf("setupFSmanagerPage(): Available heap memory: %u bytes\n", ESP.getFreeHeap());
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
    //-- Add File menu
    spa.addMenu("FSmanagerPage", "File");
    spa.addMenuItem("FSmanagerPage", "File", "Restart espTicker32", handleMenuItem, "SET-RESTART");

    //-- Add FSmanager menu
    spa.addMenu("FSmanagerPage", "FS Manager");
    //spa.addMenuItem("FSmanagerPage", "FS Manager", "List LittleFS", handleMenuItem, "FSM-1");
    spa.addMenuItemPopup("FSmanagerPage", "FS Manager", "Upload File", popupUploadFile);
    spa.addMenuItemPopup("FSmanagerPage", "FS Manager", "Create Folder", popupNewFolder);
    spa.addMenuItem("FSmanagerPage", "FS Manager", "Exit", handleMenuItem, "FSM-EXIT");
  
} // setupFSmanagerPage()


/**
 * @brief Shows an error message explaining that the FSmanager is disabled due to memory constraints.
 *
 * This function is called when the FSmanager is disabled because there is not enough memory.
 * It shows an error message to the user explaining the situation and asking them to restart the device.
 */
void mainCallbackDisabledMessage()
{
  spa.setErrorMessage("FSmanager is disabled due to memory constraints. Please restart the device.", 0);
} // mainCallbackDisabledMessage()


/**
 * @brief Sets up the settings page for the device.
 *
 * This function sets up the device settings page, which provides an interface for changing settings
 * related to the device. It adds the page to the SPA manager and sets the page title.
 * It also adds a "File" menu with options to open the FSmanager and restart the device.
 * Additionally, it adds a "Device Settings" menu with an option to exit the page.
 */
void setupMySettingsPage()
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
  
  if (debug && doDebug) debug->println("\nsetupMySettingsPage(): Adding settings page");
  spa.addPage("deviceSettingsPage", settingsPage);
  spa.setPageTitle("deviceSettingsPage", "Device Settings");
  //-- Add File menu
  spa.addMenu("deviceSettingsPage", "File");
  spa.addMenuItem("deviceSettingsPage", "File", "FSmanager", mainCallbackFSmanager);
  spa.addMenuItem("deviceSettingsPage", "File", "Restart espTicker32", handleMenuItem, "SET-RESTART");
  //-- Add Device Settings menu
  spa.addMenu("deviceSettingsPage", "Device Settings");
  spa.addMenuItem("deviceSettingsPage", "Device Settings", "Exit", handleMenuItem, "SET-UP");
#ifdef USE_PAROLA
  spa.addPage("parolaSettingsPage", settingsPage);
  spa.setPageTitle("parolaSettingsPage", "Parola Settings");
  //-- Add File menu
  spa.addMenu("deviceSettingsPage", "File");
  spa.addMenuItem("deviceSettingsPage", "File", "FSmanager", mainCallbackFSmanager);
  spa.addMenuItem("deviceSettingsPage", "File", "Restart espTicker32", handleMenuItem, "SET-RESTART");
  //-- Add Parola Settings menu
  spa.addMenu("parolaSettingsPage", "Parola Settings");
  spa.addMenuItem("parolaSettingsPage", "Parola Settings", "Exit", handleMenuItem, "SET-UP");
#endif
#ifdef USE_NEOPIXELS
  spa.addPage("neopixelsSettingsPage", settingsPage);
  spa.setPageTitle("neopixelsSettingsPage", "Neopixels Settings");
  //-- Add File menu
  spa.addMenu("neopixelsSettingsPage", "File");
  spa.addMenuItem("neopixelsSettingsPage", "File", "FSmanager", mainCallbackFSmanager);
  spa.addMenuItem("neopixelsSettingsPage", "File", "Restart espTicker32", handleMenuItem, "SET-RESTART");
  //-- Add Neopixels Settings menu
  spa.addMenu("neopixelsSettingsPage", "Neopixels Settings");
  spa.addMenuItem("neopixelsSettingsPage", "Neopixels Settings", "Exit", handleMenuItem, "SET-UP");
#endif
  spa.addPage("weerliveSettingsPage", settingsPage);
  spa.setPageTitle("weerliveSettingsPage", "Weerlive Settings");
  //-- Add File menu
  spa.addMenu("weerliveSettingsPage", "File");
  spa.addMenuItem("weerliveSettingsPage", "File", "FSmanager", mainCallbackFSmanager);
  spa.addMenuItem("weerliveSettingsPage", "File", "Restart espTicker32", handleMenuItem, "SET-RESTART");
  //-- Add Weerlive Settings menu
  spa.addMenu("weerliveSettingsPage", "Weerlive Settings");
  spa.addMenuItem("weerliveSettingsPage", "Weerlive Settings", "Exit", handleMenuItem, "SET-UP");

  spa.addPage("rssfeedSettingsPage", settingsPage);
  spa.setPageTitle("rssfeedSettingsPage", "RSSfeed Settings");
  //-- Add File menu
  spa.addMenu("rssfeedSettingsPage", "File");
  spa.addMenuItem("rssfeedSettingsPage", "File", "FSmanager", mainCallbackFSmanager);
  spa.addMenuItem("rssfeedSettingsPage", "File", "Restart espTicker32", handleMenuItem, "SET-RESTART");
  //-- Add RSSfeed Settings menu
  spa.addMenu("rssfeedSettingsPage", "RSSfeed Settings");
  spa.addMenuItem("rssfeedSettingsPage", "RSSfeed Settings", "Exit", handleMenuItem, "SET-UP");
  
} // setupMySettingsPage()



/**
 * @brief Setup the main settings page.
 *
 * This function creates a page with all the various settings pages.
 * The page is added to the SPA manager and the title is set.
 * The page is filled with a list of settings that can be modified.
 * The page also has a menu with the following options:
 *   - File menu:
 *     - FSmanager
 *     - Restart espTicker32
 *   - Settings menu:
 *     - Device settings
 *     - Parola settings (if USE_PAROLA is defined)
 *     - Neopixels settings (if USE_NEOPIXELS is defined)
 *     - Weerlive settings
 *     - RSS feed settings
 *     - Exit
 */
void setupMainSettingsPage()
{
#ifdef USE_PAROLA
  const char *settingsPage = R"HTML(
  <div style="font-size: 48px; text-align: center; font-weight: bold;">Settings</div>
  <br>You can modify system settings here that influence the operation of the device.
  <ul>
  <li>Device settings</li>
  <li>Parola settings</li>
  <li>Weerlive settings</li>
  <li>RSS feed settings</li>
  </ul> 
  )HTML";
#endif
#ifdef USE_NEOPIXELS
  const char *settingsPage = R"HTML(
  <div style="font-size: 48px; text-align: center; font-weight: bold;">Settings</div>
  <br>You can modify system settings here that influence the operation of the device.
  <ul>
  <li>Device settings</li>
  <li>Neopixels settings</li>
  <li>Weerlive settings</li>
  <li>RSS feed settings</li>
  </ul> 
  )HTML";
#endif

  if (debug && doDebug) debug->println("\nsetupMainSettingsPage(): Adding main settings page");
  spa.addPage("mainSettingsPage", settingsPage);
  spa.setPageTitle("mainSettingsPage", "Settings");
  //-- Add File menu
  spa.addMenu("mainSettingsPage", "File");
  spa.addMenuItem("mainSettingsPage", "File", "FSmanager", mainCallbackFSmanager);
  spa.addMenuItem("mainSettingsPage", "File", "Restart espTicker32", handleMenuItem, "SET-RESTART");

  //-- Add Settings menu
  spa.addMenu("mainSettingsPage", "Settings");
  spa.addMenuItem("mainSettingsPage", "Settings", "Device Settings", mainCallbackDeviceSettings);
#ifdef USE_PAROLA
  spa.addMenuItem("mainSettingsPage", "Settings", "Parola Settings", mainCallbackParolaSettings);
#endif
#ifdef USE_NEOPIXELS
  spa.addMenuItem("mainSettingsPage", "Settings", "Neopixels Settings", mainCallbackNeopixelsSettings);
#endif
  spa.addMenuItem("mainSettingsPage", "Settings", "Weerlive Settings", mainCallbackWeerliveSettings);
  spa.addMenuItem("mainSettingsPage", "Settings", "RSS feed Settings", mainCallbackRssfeedSettings);
  spa.addMenuItem("mainSettingsPage", "Settings", "Exit", handleMenuItem, "SET-EXIT");

} //  setupMainSettingsPage()


/**
 * @brief List all files in a directory tree.
 * 
 * This function is used by the FSmanager to list the contents of the LittleFS
 * file system.  It lists the name of each file and its size, indented by
 * numTabs tabs.  If the file is a directory, listFiles is called recursively
 * to list the contents of the directory.
 * 
 * @param dirname The name of the directory to list.  If dirname does not start
 * with '/', it is treated as a relative path and the leading '/' is added.
 * @param numTabs The number of tabs to indent the output with.
 */
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

#ifdef USE_PAROLA
  /**
   * @brief Sets up the Parola display according to settings.
   *
   * This function sets up the Parola display according to the settings in the
   * settings struct. It checks for invalid pin settings and sets default values
   * if necessary. It also sets up the ticker object with the correct pin
   * settings and callbacks. The ticker object is then started.
   */
void setupParolaDisplay()
{

  if (settings.parolaPinDIN == (settings.parolaPinCLK || settings.parolaPinCS) )
  {
    settings.parolaPinDIN = 23;
    settings.parolaPinCLK = 18;
    settings.parolaPinCS  =  5;
  } 
  if (settings.parolaPinCLK == (settings.parolaPinDIN || settings.parolaPinCS) )
  {
    settings.parolaPinDIN = 23;
    settings.parolaPinCLK = 18;
    settings.parolaPinCS  =  5;
  } 
  if (settings.parolaPinCS == (settings.parolaPinDIN || settings.parolaPinCLK) )
  {
    settings.parolaPinDIN = 23;
    settings.parolaPinCLK = 18;
    settings.parolaPinCS  =  5;
  } 

  if (settings.parolaPinDIN == 0) settings.parolaPinDIN = 23;
  if (settings.parolaPinCLK == 0) settings.parolaPinCLK = 18;
  if (settings.parolaPinCS  == 0) settings.parolaPinCS  = 5;

  PAROLA config = {
      .MY_HARDWARE_TYPE = (uint8_t)settings.parolaHardwareType, // FC16_HW
      .MY_MAX_DEVICES   = (uint8_t)settings.parolaNumDevices,
      .MY_MAX_ZONES     = (uint8_t)settings.parolaNumZones,
      .MY_MAX_SPEED     = 200 // Default speed
  };

  // Use default pins if not specified in settings (0 value)
  uint8_t dinPin = (settings.parolaPinDIN > 0) ? settings.parolaPinDIN : 23;  // Default DIN pin is 23
  uint8_t clkPin = (settings.parolaPinCLK > 0) ? settings.parolaPinCLK : 18;  // Default CLK pin is 18
  uint8_t csPin = (settings.parolaPinCS > 0) ? settings.parolaPinCS : 5;      // Default CS pin is 5

  Serial.printf("MY_HARDWARE_TYPE: [%d]\n", config.MY_HARDWARE_TYPE);
  Serial.printf("  MY_MAX_DEVICES: [%d]\n", config.MY_MAX_DEVICES);
  Serial.printf("    MY_MAX_ZONES: [%d]\n", config.MY_MAX_ZONES);
  Serial.printf("    MY_MAX_SPEED: [%d]\n", config.MY_MAX_SPEED);
  Serial.printf("      MY_DIN_PIN: [%d]\n", dinPin);
  Serial.printf("      MY_CLK_PIN: [%d]\n", clkPin);  
  Serial.printf("       MY_CS_PIN: [%d]\n", csPin);

  config.MY_MAX_SPEED = settings.devTickerSpeed;

  if (debug) debug->printf("setupParolaDisplay(): Parola settings: DIN[%d], CLK[%d], CS[%d], MAX_DEVICES [%d]\n", 
                                            dinPin,
                                            clkPin,
                                            csPin,
                                            config.MY_MAX_DEVICES);
  else
      Serial.printf("setupParolaDisplay(): Parola settings: DIN[%d], CLK[%d], CS[%d], MAX_DEVICES [%d]\n", 
                                            dinPin,
                                            clkPin,
                                            csPin,
                                            config.MY_MAX_DEVICES);
  ticker.begin(dinPin, clkPin, csPin, config);

  ticker.setScrollSpeed(settings.devTickerSpeed);
  ticker.setIntensity(settings.devMaxIntensiteitLeds);

  // Rest of the function remains unchanged
  ticker.setRandomEffects({
      PA_NO_EFFECT,
      PA_NO_EFFECT,
      PA_NO_EFFECT,
      PA_NO_EFFECT,
      PA_NO_EFFECT,
      PA_NO_EFFECT,
      PA_NO_EFFECT,
      PA_NO_EFFECT,
      PA_SCROLL_UP,
      PA_FADE,
      PA_DISSOLVE,
      PA_RANDOM,
      PA_WIPE
  });

  ticker.setCallback([](const std::string& finishedText) 
  {
    if (debug && doDebug) debug->print("[FINISHED] ");
    if (debug && doDebug) debug->println(finishedText.c_str());
    ticker.setScrollSpeed(settings.devTickerSpeed);
    readLdr.loop(true); // Force read LDR value to update intensity
    ticker.setIntensity(settings.devMaxIntensiteitLeds);
    debug->printf("callback finished: scrollSpeed[%d], ledIntensity[%d]\n", settings.devTickerSpeed, settings.devMaxIntensiteitLeds);
    actMessage = nextMessage();
  });

  delay(1000);

} // setupParolaDisplay()
#endif // USE_PAROLA

#ifdef USE_NEOPIXELS
/**
 * @brief Setup the Neopixels display
 *
 * This function sets up the Neopixels display for the device. It configures the display
 * with the correct pin, matrix size, pixel type, and other display properties. It also
 * sets up the callback function for when the display is finished scrolling the current
 * message.
 */
void setupNeopixelsDisplay()
{
  int pin    =  5; // Pin connected to the NeoPixel strip
  int width  = 32; // Width of the matrix
  int height =  8; // Height of the matrix
  int pixelPerChar = 6; // Number of pixels per character
  
  // Use the proper matrix configuration values from Adafruit_NeoMatrix.h
  //-- select BOTTOM or TOP
  //uint8_t matrixType      = NEO_MATRIX_BOTTOM; 
  //-- select RIGHT or LEFT
  //uint8_t matrixDirection = NEO_MATRIX_RIGHT; 
  //-- select ROWS or COLUMNS
  //uint8_t matrixLayout    = NEO_MATRIX_COLUMNS;
  // select ZIGZAG or PROGRESSIVE
  //uint8_t matrixSequence  = NEO_MATRIX_ZIGZAG;
  
  uint8_t matrixType      = 0;
  uint8_t matrixDirection = 0;
  uint8_t matrixLayout    = 0;
  uint8_t matrixSequence  = 0;
  uint8_t neopixCOLOR     = 0;
  uint8_t neopixFREQ      = 0;

  if (settings.neopixMATRIXTYPEV) 
        matrixType += NEO_MATRIX_BOTTOM;
  else  matrixType += NEO_MATRIX_TOP;
  if (settings.neopixMATRIXTYPEH) 
        matrixDirection += NEO_MATRIX_RIGHT;
  else  matrixDirection += NEO_MATRIX_LEFT;
  if (settings.neopixMATRIXORDER) 
        matrixLayout += NEO_MATRIX_COLUMNS;
  else  matrixLayout += NEO_MATRIX_ROWS;
  if (settings.neopixMATRIXSEQUENCE) 
        matrixSequence += NEO_MATRIX_ZIGZAG;
  else  matrixSequence += NEO_MATRIX_PROGRESSIVE;

  switch(settings.neopixCOLOR) 
  {
    case 0:   neopixCOLOR = NEO_RGB; break;
    case 1:   neopixCOLOR = NEO_RBG; break;
    case 2:   neopixCOLOR = NEO_GRB; break;
    case 3:   neopixCOLOR = NEO_GBR; break;
    case 4:   neopixCOLOR = NEO_BRG; break;
    case 5:   neopixCOLOR = NEO_BGR; break;
    default:  neopixCOLOR = NEO_GRB; break;
  }
  if (settings.neopixFREQ) 
        neopixFREQ = NEO_KHZ400;
  else  neopixFREQ = NEO_KHZ800;

  // Configure matrix parameters
  ticker.setup(matrixType, matrixLayout, matrixDirection, matrixSequence);
  
  // Set matrix size
  ticker.setMatrixSize(settings.neopixWidth, settings.neopixHeight);
  
  // Set pixel type - try different combinations if one doesn't work
  ticker.setPixelType(neopixCOLOR + neopixFREQ);  // Most common for WS2812 LEDs // NEO_GRB + NEO_KHZ800);  // Most common for WS2812 LEDs

  // Initialize the matrix with the correct pin
  ticker.begin(settings.neopixDataPin);
  
  // Set pixels per character
  ticker.setPixelsPerChar(settings.neopixPixPerChar);
  
  
  // Set display properties
  ticker.setColor(255, 0, 0); // Red text
  ticker.setIntensity(30);    // Medium brightness at startup
  ticker.setScrollSpeed(100);   // Medium-high speed

  //ticker.setScrollSpeed(settings.devTickerSpeed);
  //ticker.setIntensity(settings.devMaxIntensiteitLeds);

  ticker.setCallback([](const std::string& finishedText) 
  {
    if (debug && doDebug) debug->print("[FINISHED] ");
    if (debug && doDebug) debug->println(finishedText.c_str());
    ticker.setScrollSpeed(settings.devTickerSpeed);
    readLdr.loop(true); // Force read LDR value to update intensity
    ticker.setIntensity(settings.devMaxIntensiteitLeds);
    debug->printf("callback finished: scrollSpeed[%d], ledIntensity[%d]\n", settings.devTickerSpeed, settings.devMaxIntensiteitLeds);
    actMessage = nextMessage(); 
  });

  delay(1000);

} // setupNeopixelsDisplay()
#endif // USE_NEOPIXELS

/**
 * @brief Callback function for when the WiFi portal is triggered
 *
 * This function is called when the WiFi portal is triggered. It displays a message
 * on the ticker display instructing the user to connect to the espTicker32 portal
 * and go to 192.168.4.1.
 */
void callbackWiFiPortal()
{
    if (debug) debug->println("callbackWiFiPortal(): WiFi portal callback triggered");
    else       Serial.println("callbackWiFiPortal(): WiFi portal callback triggered");

    ticker.setIntensity(20);
    ticker.animateBlocking(" triggered WiFi portal");
    ticker.animateBlocking(" .. connect to espTicker32 portal");
    ticker.animateBlocking(" .. go to 192.168.4.1 ");

} // callbackWiFiPortal()


void setup()
{
    systemUpTime = millis();
    Serial.begin(115200);
    while (!Serial) { delay(100); } // Wait for Serial to be ready
    Serial.println("\nLets get started!!\n");

    if (!LittleFS.begin()) 
    {
      if (debug) debug->println("espTicker32: setup(): LittleFS Mount Failed");
      else       Serial.println("espTicker32: setup(): LittleFS Mount Failed");
      return;
    }
    //-test- LittleFS.remove(LOCAL_MESSAGES_PATH);
    //-test- listFiles("/", 0);
    if (debug) debug->println("espTicker32: setup(): readSettingFields(deviceSettings)");
    else Serial.println("espTicker32: setup(): readSettingFields(deviceSettings)");
    settings.readSettingFields("deviceSettings");

    if (debug) debug->println("espTicker32: setup(): readSettingFields(weerliveSettings)");
    else       Serial.println("espTicker32: setup(): readSettingFields(weerliveSettings)");
    settings.readSettingFields("weerliveSettings");

    if (debug) debug->println("espTicker32: setup(): readSettingFields(rssfeedSettings)");
    else       Serial.println("espTicker32: setup(): readSettingFields(rssfeedSettings)");
    settings.readSettingFields("rssfeedSettings");

#ifdef USE_PAROLA
      if (debug) debug->println("espTicker32: setup(): readSettingFields(parolaSettings)");
      else       Serial.println("espTicker32: setup(): readSettingFields(parolaSettings)");
      settings.readSettingFields("parolaSettings");
      setupParolaDisplay();
#endif // USE_PAROLA

#ifdef USE_NEOPIXELS
      if (debug) debug->println("espTicker32: setup(): readSettingFields(neopixelsSettings)");
      else       Serial.println("espTicker32: setup(): readSettingFields(neopixelsSettings)");
      settings.readSettingFields("neopixelsSettings");
      setupNeopixelsDisplay();
#endif // USE_NEOPIXELS
    
    delay(2000);
    Serial.printf("espTicker32: setup()[S]: Start espTicker32 [%s] ...\n", PROG_VERSION);
    ticker.animateBlocking(" Start espTicker32 ["+String(PROG_VERSION)+"]");
    delay(500);

    //-- Connect to WiFi
    Serial.println("network = new Networking();");
    network = new Networking();
    
    ticker.animateBlocking(" Start WiFi ");
    //-- Parameters: devHostname, resetWiFi pin, serial object, baud rate, wifiCallback
    pinMode(settings.devResetWiFiPin, INPUT_PULLUP);
    Serial.printf("devResetWiFiPin is [%s]\n", settings.devResetWiFiPin == 0 ? "LOW" : "HIGH");
    if (settings.devResetWiFiPin == 0) 
    {
        ticker.animateBlocking(" reset WiFi requested! ");
    }
    Serial.printf("debug = network.begin(%s, %d, Serial, 115200, callbackWiFiPortal)\n",  hostName, settings.devResetWiFiPin);
    debug = network->begin(hostName, settings.devResetWiFiPin, Serial, 115200, callbackWiFiPortal);

    ticker.animateBlocking(" Done!");
    Serial.println(" ... Done!");

    Serial.println("settings.setDebug(debug);");
    settings.setDebug(debug);
    Serial.println("ticker.setDebug(debug);");
    ticker.setDebug(debug);
    Serial.println("setDebug() done ..");
    
    if (debug) 
    {
      debug->println("\nespTicker32: setup(): WiFi connected");
      debug->print("espTicker32: setup(): IP address: ");
      debug->println(WiFi.localIP());
    }
    else
    {
      Serial.println("\nespTicker32: setup(): WiFi connected");
      Serial.print("espTicker32: setup(): IP address: ");
      Serial.println(WiFi.localIP());
    }
    ticker.animateBlocking(" IP:" + String(WiFi.localIP().toString().c_str()) + " ");

    if (debug) debug->printf("espTicker32 Version: %s\n", PROG_VERSION);

    ticker.setDebug(debug); 

    if (settings.devHostname.empty()) 
    {
        if (debug) debug->println("espTicker32: setup(): No devHostname found in settings, using default");
        settings.devHostname = std::string(hostName);
        settings.writeSettingFields("deviceSettings");
    }

    //-- Define custom NTP servers (optional)
    //const char* ntpServers[] = {"time.google.com", "time.cloudflare.com", nullptr};

    //-- Start NTP with Amsterdam timezone and custom servers
    //if (network->ntpStart("CET-1CEST,M3.5.0,M10.5.0/3", ntpServers))
    if (network->ntpStart("CET-1CEST,M3.5.0,M10.5.0/3"))
    {
        if (debug && doDebug) debug->println("espTicker32: setup(): NTP started successfully");
    }
    else
    {
        if (debug) debug->println("espTicker32: setup(): NTP failed to start");
    } 
    
    //settings.devShowLocalA = true;
    localMessages.setDebug(debug);
    localMessages.begin();

    spa.begin("/SYS", debug);
    // Set the local events handler
    spa.setLocalEventHandler(handleLocalWebSocketEvent);

    if (debug) debug->printf("espTicker32: setup(): SPAmanager files are located [%s]\n", spa.getSystemFilePath().c_str());
    fsManager.begin();
    fsManager.addSystemFile("/favicon.ico");
    fsManager.addSystemFile(spa.getSystemFilePath() + "/SPAmanager.html", false);
    fsManager.addSystemFile(spa.getSystemFilePath() + "/SPAmanager.css", false);
    fsManager.addSystemFile(spa.getSystemFilePath() + "/SPAmanager.js", false);
    fsManager.addSystemFile(spa.getSystemFilePath() + "/disconnected.html", false);
   
    spa.pageIsLoaded(pageIsLoadedCallback);

    fsManager.setSystemFilePath("/SYS");
    if (debug) debug->printf("espTicker32: setup(): FSmanager files are located [%s]\n", fsManager.getSystemFilePath().c_str());
    spa.includeJsFile(fsManager.getSystemFilePath() + "/FSmanager.js");
    fsManager.addSystemFile(fsManager.getSystemFilePath() + "/FSmanager.js", false);
    spa.includeCssFile(fsManager.getSystemFilePath() + "/FSmanager.css");
    fsManager.addSystemFile(fsManager.getSystemFilePath() + "/FSmanager.css", false);
    spa.includeJsFile(fsManager.getSystemFilePath() + "/espTicker32.js");
    fsManager.addSystemFile(fsManager.getSystemFilePath() + "/espTicker32.js", false);

    setupMainPage();
    setupFSmanagerPage();
    setupMainSettingsPage();
    setupMySettingsPage();
    setupLocalMessagesPage();

    if (debug && doDebug) debug->printf("espTicker32: setup(): weerliveAuthToken: [%s], weerlivePlaats: [%s], requestInterval: [%d minuten]\n",
                  settings.weerliveAuthToken.c_str(),
                  settings.weerlivePlaats.c_str(),
                  settings.weerliveRequestInterval);
    weerlive.setup(settings.weerliveAuthToken.c_str(), settings.weerlivePlaats.c_str(), debug);
    weerlive.setInterval(settings.weerliveRequestInterval); 

    rssReader.setDebug(debug);
    rssReader.addWordStringToSkipWords(settings.devSkipWords.c_str());
    
    if (!settings.domain0.empty() && !settings.path0.empty() && settings.maxFeeds0 > 0) 
              rssReader.addRSSfeed(settings.domain0.c_str(), settings.path0.c_str(), settings.maxFeeds0);
    if (!settings.domain1.empty() && !settings.path1.empty() && settings.maxFeeds1 > 0)
              rssReader.addRSSfeed(settings.domain1.c_str(), settings.path1.c_str(), settings.maxFeeds1);
    if (!settings.domain2.empty() && !settings.path2.empty() && settings.maxFeeds2 > 0)
              rssReader.addRSSfeed(settings.domain2.c_str(), settings.path2.c_str(), settings.maxFeeds2);
    if (!settings.domain3.empty() && !settings.path3.empty() && settings.maxFeeds3 > 0)
              rssReader.addRSSfeed(settings.domain3.c_str(), settings.path3.c_str(), settings.maxFeeds3);
    if (!settings.domain4.empty() && !settings.path4.empty() && settings.maxFeeds4 > 0)
              rssReader.addRSSfeed(settings.domain4.c_str(), settings.path4.c_str(), settings.maxFeeds4);
    /****** if you need them ******** 
    if (!settings.domain5.empty() && !settings.path5.empty() && settings.maxFeeds5 > 0) 
              rssReader.addRSSfeed(settings.domain5.c_str(), settings.path5.c_str(), settings.maxFeeds5);
    if (!settings.domain6.empty() && !settings.path6.empty() && settings.maxFeeds6 > 0) 
              rssReader.addRSSfeed(settings.domain6.c_str(), settings.path6.c_str(), settings.maxFeeds6);
    if (!settings.domain7.empty() && !settings.path7.empty() && settings.maxFeeds7 > 0) 
              rssReader.addRSSfeed(settings.domain7.c_str(), settings.path7.c_str(), settings.maxFeeds7);
    ******* if you want them *********/
   
    rssReader.setRequestInterval(settings.requestInterval); // in minutes
    rssReader.checkFeedHealth();

    spa.activatePage("Main");

    ticker.setScrollSpeed(settings.devTickerSpeed);
    ticker.tickerClear();
    
    //LDRsetup();
    readLdr.setup(&settings, &Serial);

    actMessage = nextMessage();

    if (debug) debug->println("\nespTicker32: Done with setup() ..\n");
    else       Serial.println("\nespTicker32: Done with setup() ..\n");

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
    if (debug && doDebug) debug->printf("espTicker32: weerlive::loop(): weerliveText: [%s]\n", weerliveText);
  
  }

  rssReader.loop(network->ntpGetTmStruct());

  //-- Print current time every 60 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 60000)
  {
    if (network->ntpIsValid())
         if (debug && doDebug) debug->printf("espTicker32: loop(): NTP time: %s\n", network->ntpGetTime());
    else if (debug) debug->println("espTicker32: loop(): NTP is not valid");
    lastPrint = millis();
  }
  ticker.loop();
  readLdr.loop();

} // loop()