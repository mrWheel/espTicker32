#include "LocalMessagesClass.h"

LocalMessagesClass::LocalMessagesClass(const std::string& path, size_t recordSize)
    : path(path), recordSize(recordSize) {}


bool LocalMessagesClass::begin() 
{
    // Initialize LittleFS if not already initialized
    if (!LittleFS.begin()) 
    {
        if (debug) debug->println("LocalMessagesClass::begin(): Failed to initialize LittleFS");
        else       Serial.println("LocalMessagesClass::begin(): Failed to initialize LittleFS");
        return false;
    }
    if (debug) debug->println("LocalMessagesClass::begin(): LittleFS allready initialized");
    else       Serial.println("LocalMessagesClass::begin(): LittleFS allready initialized");
    
    // Check if the file exists, if not create a default message
    if (!LittleFS.exists(path.c_str())) 
    {
      if (debug) debug->printf("LocalMessagesClass::begin(): File does not exist, creating default message at [%s]\n", path.c_str());
      else       Serial.printf("LocalMessagesClass::begin(): File does not exist, creating default message at [%s]\n", path.c_str());
      // Create the file and write a default message
      File file = LittleFS.open(path.c_str(), "w");
      if (!file) 
      {
          if (debug) debug->printf("LocalMessagesClass::begin(): Failed to create file: [%s]\n", path.c_str());
          else       Serial.printf("LocalMessagesClass::begin(): Failed to create file: [%s]\n", path.c_str());
          return false;
      }
      file.close();
      MessageItem defaultItem;
      defaultItem.key = 'A';
      defaultItem.content = "[A] There are no messages";
      write(0, defaultItem);
      defaultItem.key = 'D';
      defaultItem.content = "[D] This is the second messages";
      write(1, defaultItem);
      defaultItem.key = 'A';
      defaultItem.content = "<rssfeed>";
      write(2, defaultItem);
    }
    else 
    {
      if (debug) debug->printf("LocalMessagesClass::begin(): File already exists: [%s]\n", path.c_str());
      else       Serial.printf("LocalMessagesClass::begin(): File already exists: [%s]\n", path.c_str());
    }
    File file = LittleFS.open(path.c_str(), "r");
    if (!file) 
    {
        if (debug) debug->printf("LocalMessagesClass::begin(): Failed to open file: [%s]\n", path.c_str());
        else       Serial.printf("LocalMessagesClass::begin(): Failed to open file: [%s]\n", path.c_str());
        return false;
    }
    MessageItem tmp = read(0);
    if (file.size() == 0) 
    {
        if (debug) debug->printf("LocalMessagesClass::begin(): File is empty: [%s]\n", path.c_str());
        else       Serial.printf("LocalMessagesClass::begin(): File is empty: [%s]\n", path.c_str());
        file.close();
        return false;
    } 
    else 
    {
        MessageItem tmp = read(0);
        if (debug) debug->printf("LocalMessagesClass::begin(): record[0] key[%c], content[%s]\n", tmp.key, tmp.content.c_str());
        else       Serial.printf("LocalMessagesClass::begin(): record[0] key[%c], content[%s]\n", tmp.key, tmp.content.c_str());
        tmp = read(1);
        if (debug) debug->printf("LocalMessagesClass::begin(): record[1] key[%c], content[%s]\n", tmp.key, tmp.content.c_str());
        else       Serial.printf("LocalMessagesClass::begin(): record[1] key[%c], content[%s]\n", tmp.key, tmp.content.c_str());
        file.close();
    }    
    return true;
}


void LocalMessagesClass::setDebug(Stream* debugPort) {
    debug = debugPort;
}


/****
std::string LocalMessagesClass::read(uint8_t recNr) {
    File file = LittleFS.open(path.c_str(), "r");
    if (!file) {
        if (debug) debug->printf("LocalMessagesClass::read(): Failed to open file: %s\n", path.c_str());
        return "";
    }

    size_t offset = recNr * recordSize;
    if (!file.seek(offset)) {
        if (debug) debug->printf("LocalMessagesClass::read(): Failed to seek to record %d (offset %d)\n", recNr, offset);
        file.close();
        return "";
    }

    char buffer[recordSize + 1];
    size_t bytesRead = file.readBytes(buffer, recordSize);
    buffer[bytesRead] = '\0';

    file.close();

    if (bytesRead == 0) {
        if (debug) debug->printf("LocalMessagesClass::read(): No data read for record %d\n", recNr);
        return "";
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    return std::string(buffer);
}
****/

LocalMessagesClass::MessageItem LocalMessagesClass::read(uint8_t recNr) {
  MessageItem item;
  bool fileExists = false;
  bool needDefaultMessage = false;
  
  // Check if file exists
  if (LittleFS.exists(path.c_str())) {
    fileExists = true;
  } else {
    if (debug) debug->printf("LocalMessagesClass::read(): File does not exist: %s\n", path.c_str());
    needDefaultMessage = true;
  }
  
  // If file exists, try to read from it
  if (fileExists) {
    File file = LittleFS.open(path.c_str(), "r");
    if (!file) {
      if (debug) debug->printf("LocalMessagesClass::read(): Failed to open file: %s\n", path.c_str());
      needDefaultMessage = true;
    } else {
      // Check if file is empty (size == 0)
      if (file.size() == 0) {
        if (debug) debug->printf("LocalMessagesClass::read(): File is empty: %s\n", path.c_str());
        file.close();
        needDefaultMessage = true;
      } else {
        // File exists and has content, proceed with normal read
        size_t offset = recNr * recordSize;
        if (!file.seek(offset)) {
          if (debug) debug->printf("LocalMessagesClass::read(): Failed to seek to record %d (offset %d)\n", recNr, offset);
          file.close();
          return item; // Return default item
        }

        char buffer[recordSize + 1];
        size_t bytesRead = file.readBytes(buffer, recordSize);
        buffer[bytesRead] = '\0';

        file.close();

        if (bytesRead == 0) {
          if (debug) debug->printf("LocalMessagesClass::read(): No data read for record %d\n", recNr);
          // If we're trying to read the first record and got nothing, create default message
          if (recNr == 0) {
            needDefaultMessage = true;
          } else {
            return item; // Return default item for non-first records
          }
        } else {
          // Extract key (first character)
          char keyChar = buffer[0];
          // Validate key is A-J
          if (keyChar >= 'A' && keyChar <= 'J') {
            item.key = keyChar;
          }
          
          // Extract content (rest of buffer)
          size_t contentLen = bytesRead - 1;
          if (contentLen > 0) {
            // Copy content starting from position 1
            char contentBuffer[recordSize];
            memcpy(contentBuffer, buffer + 1, contentLen);
            contentBuffer[contentLen] = '\0';
            
            // Remove trailing newline if present
            if (contentLen > 0 && contentBuffer[contentLen - 1] == '\n') {
              contentBuffer[contentLen - 1] = '\0';
            }
            
            item.content = std::string(contentBuffer);
          }
          
          // If we successfully read content, return it
          if (!item.content.empty()) {
            return item;
          } else if (recNr == 0) {
            // If first record has no content, create default message
            needDefaultMessage = true;
          }
        }
      }
    }
  }
  
  // If we need to create a default message
  if (needDefaultMessage) {
    if (debug) debug->println("LocalMessagesClass::read(): Creating default message");
    
    // Create default message
    item.key = 'A';
    item.content = "There are no messages";
    
    // Write default message to file
    if (write(0, item)) {
      if (debug) debug->println("LocalMessagesClass::read(): Default message written successfully");
    } else {
      if (debug) debug->println("LocalMessagesClass::read(): Failed to write default message");
    }
  }
  
  return item;
}


// New write function for MessageItem
bool LocalMessagesClass::write(uint8_t recNr, const MessageItem& item) {
  // Validate key
  char key = item.key;
  if (key < 'A' || key > 'J') {
    key = 'A'; // Default to 'A' if invalid
  }
  
  //if (recNr == 0) {
  //  if (debug) debug->println("LocalMessagesClass::write(): record number is zero, erase file");
  //  LittleFS.remove(path.c_str());
  //}

  const char* data = item.content.c_str();
  const char* start = data;
  while (*start == ' ') start++;

  const char* end = data + strlen(data);
  while (end > start && *(end - 1) == ' ') end--;

  size_t trimmedLen = end - start;

  if (trimmedLen == 0) {
    if (debug) debug->println("LocalMessagesClass::write(): Skipped writing empty or space-only localMessage after trimming.");
    return false;
  }

  File file = LittleFS.open(path.c_str(), "a");
  if (!file) {
    if (debug) debug->printf("LocalMessagesClass::write(): Failed to open file: %s\n", path.c_str());
    return false;
  }

  size_t offset = recNr * recordSize;
  if (!file.seek(offset)) {
    if (debug) debug->printf("LocalMessagesClass::write(): Failed to seek to record %d (offset %d)\n", recNr, offset);
    file.close();
    return false;
  }

  char buffer[recordSize];
  memset(buffer, 0, recordSize);

  // First byte is the key
  buffer[0] = key;
  
  // Remaining bytes are for content
  if (trimmedLen > recordSize - 2) { // -2 for key and newline
    trimmedLen = recordSize - 2;
  }

  memcpy(buffer + 1, start, trimmedLen);
  buffer[1 + trimmedLen] = '\n';
  buffer[1 + trimmedLen] = '\0';

  size_t bytesWritten = file.write((uint8_t*)buffer, recordSize);
  file.flush();
  file.close();

  if (bytesWritten != recordSize) {
    if (debug) debug->printf("LocalMessagesClass::write(): Incomplete write: expected [%d] bytes, wrote [%d] bytes\n", recordSize, bytesWritten);
    return false;
  }

  if (debug) debug->printf("LocalMessagesClass::Successfully wrote record [%d] with key [%c]: [%.*s]\n", recNr, key, (int)trimmedLen, start);
  return true;
}

// Backward compatibility write function
bool LocalMessagesClass::write(uint8_t recNr, const char* data) {
  MessageItem item;
  item.key = 'A'; // Default key
  item.content = data;
  return write(recNr, item);
}
