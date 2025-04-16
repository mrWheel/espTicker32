#include "RSSreaderClass.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <LittleFS.h>

RSSreaderClass::RSSreaderClass() {}

void RSSreaderClass::setup(const char* url, const char* storageFile, size_t maxFeeds, unsigned long requestIntervalMs, Stream* debugPort) {
  _url = url;
  _filePath = storageFile;
  _maxFeeds = maxFeeds;
  _interval = requestIntervalMs;
  _lastCheck = 0;
  debug = debugPort;
  debug->printf("RSSreaderClass::setup(): URL: [%s], File: [%s], Max Feeds: [%d], Interval: [%lu]\n", _url.c_str(), _filePath.c_str(), _maxFeeds, _interval);

  LittleFS.begin();
  if (!LittleFS.exists(_filePath)) 
  {
    debug->printf("RSSreaderClass::setup(): Bestand [%s] bestaat niet, aanmaken...\n", _filePath.c_str());
    File file = LittleFS.open(_filePath, "w");
    if (file) 
    {
      file.close();
      debug->println("RSSreaderClass::setup(): Bestand aangemaakt.");
    } else {
      debug->println("RSSreaderClass::setup(): Kan bestand niet aanmaken.");
    }
  } else {
    debug->println("RSSreaderClass::setup(): Bestand bestaat al.");
  } 
  checkForNewFeedItems();

}

void RSSreaderClass::loop(struct tm timeNow) 
{
  if (millis() - _lastCheck >= _interval) 
  {
    deleteFeedsOlderThan(timeNow);
    checkForNewFeedItems();
    _lastCheck = millis();
  }
}

String RSSreaderClass::fetchFeed(const char* url) 
{
  debug->printf("RSSreaderClass::fetchFeed(): URL: [%s]\n", url);

  secureClient.setInsecure();  // Accepteer alle certificaten (NIET voor productie)

  HTTPClient http;
  
  http.begin(secureClient, url);
  int httpCode = http.GET();
  String payload;

  if (httpCode == HTTP_CODE_OK) 
  {
    payload = http.getString();
  } else {
    payload = "Fout: " + http.errorToString(httpCode);
  }

  http.end();
  debug->printf("RSSreaderClass::fetchFeed(): HTTP Code: [%d], Payload Length: [%d]\n", httpCode, payload.length());
  debug->printf("RSSreaderClass::fetchFeed(): Payload: [%s]\n", payload.c_str());
  return payload;
}

std::vector<String> RSSreaderClass::extractTitles(const String& feed) 
{
  std::vector<String> titles;
  int pos = 0;
  while ((pos = feed.indexOf("<title>", pos)) != -1) 
  {
    int start = pos + 7;
    int end = feed.indexOf("</title>", start);
    if (end == -1) break;
    String title = feed.substring(start, end);
    title.trim();
    
    // Add code to remove CDATA tags
    if (title.startsWith("<![CDATA[") && title.endsWith("]]>")) {
      title = title.substring(9, title.length() - 3);
    }
    
    pos = end + 8;
    if (title.length() > 0) titles.push_back(title);
  }

  if (!titles.empty()) titles.erase(titles.begin()); // verwijder kanaaltitel
  return titles;
} // extractTitles()


std::vector<String> RSSreaderClass::getStoredLines() 
{
  std::vector<String> lines;
  LittleFS.begin();
  File file = LittleFS.open(_filePath, "r");
  if (!file) return lines;

  while (file.available()) 
  {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) lines.push_back(line);
  }

  file.close();
  return lines;
}

void RSSreaderClass::saveTitles(const std::vector<String>& titles) 
{
  LittleFS.begin();
  File file = LittleFS.open(_filePath, "a"); // or "a"? overschrijf bestand 
  if (!file) {
    debug->println("RSSreaderClass::saveTitles(): Kan bestand niet openen voor schrijven");
    return;
  }

  for (const String& title : titles) 
  {
    time_t now = time(nullptr);
    debug->printf("RSSreaderClass::saveTitles(): Titel: %s\n", title.c_str());
    file.print(now);
    file.print("|");
    file.println(title);
  }

  file.close();
}

bool RSSreaderClass::titleExists(const String& title, const std::vector<String>& lines) 
{
  for (const auto& line : lines) 
  {
    int sep = line.indexOf('|');
    if (sep != -1) 
    {
      String existingTitle = line.substring(sep + 1);
      if (existingTitle == title) return true;
    }
  }
  return false;
}

void RSSreaderClass::checkForNewFeedItems() 
{
  debug->println("RSSreaderClass::checkForNewFeedItems(): Checken op nieuwe RSS-items...");
  String feed = fetchFeed(_url.c_str());


  if (feed.startsWith("Fout:")) 
  {
    debug->println(feed);
    return;
  }

  std::vector<String> existing = getStoredLines();
  std::vector<String> newTitles = extractTitles(feed);
  std::vector<String> updated = existing;
  bool changed = false;

  for (const auto& title : newTitles) 
  {
    if (!titleExists(title, existing)) 
    {
      debug->printf("RSSreaderClass::checkForNewFeedItems(): Nieuwe titel gevonden: %s\n", title.c_str());
      time_t now = time(nullptr);
      updated.push_back(String(now) + "|" + title);
      changed = true;
    }
  }

  // FIFO: maximaal _maxFeeds titels
  while (updated.size() > _maxFeeds) 
  {
    updated.erase(updated.begin());
  }

  if (changed) 
  {
    LittleFS.begin();
    File file = LittleFS.open(_filePath, "w"); 
    if (file) {
      for (const auto& line : updated) 
      {
        file.println(line);
      }
      file.close();
      debug->println("RSSreaderClass::checkForNewFeedItems(): Nieuwe feeds toegevoegd.");
    } else {
      debug->println("RSSreaderClass::checkForNewFeedItems(): Kan niet naar bestand schrijven.");
    }
  } else {
    debug->println("RSSreaderClass::checkForNewFeedItems(): Geen nieuwe titels.");
  }
}

void RSSreaderClass::deleteFeedsOlderThan(struct tm timeNow) 
{
  debug->println("RSSreaderClass::deleteFeedsOlderThan(): Verwijderen van oude feeds...");

  timeNow.tm_hour = 0;
  timeNow.tm_min = 0;
  timeNow.tm_sec = 0;
  time_t startOfToday = mktime(&timeNow);

  std::vector<String> lines = getStoredLines();
  std::vector<String> filtered;

  for (const auto& line : lines) 
  {
    int sep = line.indexOf('|');
    if (sep != -1) 
    {
      time_t ts = atol(line.substring(0, sep).c_str());
      if (ts >= startOfToday) 
      {
        filtered.push_back(line);
      }
    }
  }

  LittleFS.begin();
  File file = LittleFS.open(_filePath, "w"); // append mode");
  if (file) 
  {
    for (const auto& line : filtered) 
    {
      file.println(line);
    }
    file.close();
  }

  debug->printf("RSSreaderClass::deleteFeedsOlderThan(): Feeds behouden: %d / verwijderd: %d\n", filtered.size(), lines.size() - filtered.size());
}

String RSSreaderClass::readRssFeed(size_t feedNr) 
{
  debug->printf("RSSreaderClass::readRssFeed(): FeedNr: %d\n", feedNr);
  std::vector<String> lines = getStoredLines();
  if (feedNr >= lines.size()) 
  {
    debug->printf("RSSreaderClass::readRssFeed(): Geen RSS-feed gevonden voor nummer %d\n", feedNr);
    return "";
  }
  int sep = lines[feedNr].indexOf('|');
  if (sep != -1) 
  {
    debug->printf("RSSreaderClass::readRssFeed(): Feed %d: %s\n", feedNr, lines[feedNr].c_str());
    return lines[feedNr].substring(sep + 1);
  }

  return "";
}
