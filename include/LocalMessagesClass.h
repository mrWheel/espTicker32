#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <string>

class LocalMessagesClass {
private:
  std::string path;
  size_t recordSize;
  Stream* debug = nullptr; // Optional, default to nullptr

public:
  struct MessageItem {
    char key = 'A';  // Default key is 'A'
    std::string content;
  };
  bool begin();
  LocalMessagesClass(const std::string& path, size_t recordSize);
  void setDebug(Stream* debugPort);
  MessageItem read(uint8_t recNr);
  bool write(uint8_t recNr, const MessageItem& item);
  bool write(uint8_t recNr, const char* data); // Overload for backward compatibility
};
