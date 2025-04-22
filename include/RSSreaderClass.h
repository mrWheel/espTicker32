#ifndef RSSREADERCLASS_H
#define RSSREADERCLASS_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <vector>
#include <ctime>

class RSSreaderClass {
public:
  RSSreaderClass();

  void loop(struct tm timeNow); 
  void setDebug(Stream* debugPort) { debug = debugPort; }
  void setRequestInterval(uint32_t interval) { _interval = interval * 60 * 1000; }
  bool addRSSfeed(const char* url, const char* path, size_t maxFeeds);
  bool getNextFeedItem(uint8_t& feedIndex, size_t& itemIndex);
  String readRssFeed(uint8_t feedIndex, size_t itemIndex);
  void checkFeedHealth();
  void addToSkipWords(std::string noNoWord);
  void addWordStringToSkipWords(std::string wordList);

private:
  struct FeedItem {
    String title;
    time_t pubDate;
  };

  WiFiClientSecure secureClient;
  String _urls[10];
  String _paths[10];
  String _filePaths[10];
  uint8_t _activeFeedCount = 0;
  size_t _maxFeedsPerFile[10] = {0};
  uint32_t _interval = 12000000; // standaard 20 min
  uint32_t _lastCheck = 0;
  void createRSSfeedMap();
  uint8_t _feedCheckState = 0;         // Current feed being checked
  uint32_t _lastFeedCheck = 0;     // Time of last individual feed check
  uint32_t _feedCheckInterval = 25000; // Time between checking individual feeds (25 seconds)
  bool _checkingFeeds = false;          // Flag to indicate we're in the middle of checking feeds
  uint8_t _currentFeedIndex = 0;
  uint8_t _currentItemIndex = 0;
  size_t _currentItemIndices[10] = {0}; // Track current item index for each feed
  uint8_t _feedReadCounts[10] = {0}; // Track how many items we've read from each feed
  uint16_t _totalMaxFeeds = 0;       // Sum of all maxFeeds values
  uint32_t _lastHealthCheck = 0;
  const uint32_t _healthCheckInterval = 60 * 60 * 1000; // one hour
  uint32_t _lastFeedUpdate[10] = {0}; // Track when each feed was last updated
  std::vector<std::string> _skipWords;
  bool hasSufficientWords(const String& title);
  bool hasNoSkipWords(const String& title);

  String fetchFeed(const char* host, const char* path);
  std::vector<String> extractTitles(const String& feed);
  std::vector<String> getStoredLines(uint8_t feedIndex);
  void saveTitles(const std::vector<String>& titles, uint8_t feedIndex);
  bool titleExists(const String& title, const std::vector<String>& lines);
  ;void checkForNewFeedItems();
  void checkFeed(uint8_t feedIndex);
  std::vector<FeedItem> extractFeedItems(const String& feed);
  time_t parseRSSDate(const String& dateStr);

  Stream* debug = nullptr; // Optional, default to nullptr

  #ifdef RSSREADER_DEBUG
    bool doDebug = true;
  #else
    bool doDebug = false;
  #endif

};

#endif