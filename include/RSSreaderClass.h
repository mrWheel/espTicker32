#ifndef RSSREADERCLASS_H
#define RSSREADERCLASS_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <vector>
#include <ctime>

class RSSreaderClass {
public:
  RSSreaderClass();

  void setup(const char* url, const char* storageFile, size_t maxFeeds, unsigned long requestIntervalMs, Stream* debugPort);
  void loop(struct tm timeNow); 
  String readRssFeed(size_t feedNr);

private:
  WiFiClientSecure  secureClient;
  String _url;
  String _filePath;
  size_t _maxFeeds = 10;
  unsigned long _interval = 300000; // standaard 5 min
  unsigned long _lastCheck = 0;

  String fetchFeed(const char* url);
  std::vector<String> extractTitles(const String& feed);
  std::vector<String> getStoredLines();
  void saveTitles(const std::vector<String>& titles);
  bool titleExists(const String& title, const std::vector<String>& lines);
  void checkForNewFeedItems();
  void deleteFeedsOlderThan(struct tm timeNow);
  Stream* debug = nullptr; // Optional, default to nullptr
};

#endif
