#include "RSSreaderClass.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <LittleFS.h>

#define RSS_BASE_FOLDER "/RSSfeeds"

RSSreaderClass::RSSreaderClass() 
{
  _activeFeedCount = 0;
  // Initialize feed update tracking
  for (int i = 0; i < 10; i++) {
    _lastFeedUpdate[i] = 0;
  }

} // RSSreaderClass()

bool RSSreaderClass::addRSSfeed(const char* url, const char* path, size_t maxFeeds) 
{
  if (_activeFeedCount >= 10)
  {
    if (debug) debug->println("RSSreaderClass::addRSSfeed(): Maximum number of feeds reached");
    return false;
  }
  
  uint8_t feedIndex = _activeFeedCount;
  _urls[feedIndex] = url;
  _paths[feedIndex] = path;
  _filePaths[feedIndex] = "/RSSfeed" + String(feedIndex) + ".txt";
  _maxFeedsPerFile[feedIndex] = maxFeeds;
  _totalMaxFeeds += maxFeeds; // Update total max feeds
  _lastCheck = 0;

  createRSSfeedMap();

  if (debug && doDebug) debug->printf("RSSreaderClass::addRSSfeed(): URL:[%s], Path:[%s], File:[%s], Max Feeds:[%d], Index: [%d]\n" 
                                                                  , _urls[feedIndex].c_str()
                                                                  , _paths[feedIndex].c_str()
                                                                  , _filePaths[feedIndex].c_str()
                                                                  , _maxFeedsPerFile[feedIndex], feedIndex);

  LittleFS.begin();
  if (!LittleFS.exists(RSS_BASE_FOLDER + _filePaths[feedIndex])) 
  {
    if (debug) debug->printf("RSSreaderClass::addRSSfeed(): Bestand [%s] bestaat niet, aanmaken...\n", (RSS_BASE_FOLDER + _filePaths[feedIndex]).c_str());
    File file = LittleFS.open(RSS_BASE_FOLDER + _filePaths[feedIndex], "w");
    if (file) 
    {
      file.close();
      if (debug && doDebug) debug->printf("RSSreaderClass::addRSSfeed(): Bestand [%s]aangemaakt.\n", (RSS_BASE_FOLDER + _filePaths[feedIndex]).c_str());
    } 
    else 
    {
      if (debug) debug->printf("RSSreaderClass::addRSSfeed(): Kan bestand [%s] niet aanmaken.\n", (RSS_BASE_FOLDER + _filePaths[feedIndex]).c_str());
      return false;
    }
  } 
  else 
  {
    if (debug && doDebug) debug->println("RSSreaderClass::addRSSfeed(): Bestand bestaat al.");
  }
  
  _activeFeedCount++;  // Increment FIRST
  checkFeed(feedIndex);  // Then check the feed we just added
  return true;

} // addRSSfeed()


void RSSreaderClass::loop(struct tm timeNow) 
{
  // Main interval check - start the feed checking process
  if (!_checkingFeeds && (millis() - _lastCheck >= _interval)) 
  {
    // Start the feed checking process
    _checkingFeeds = true;
    _feedCheckState = 0;
    _lastFeedCheck = 0; // Force immediate check of first feed
    
    if (debug && doDebug) debug->println("RSSreaderClass::loop(): Starting feed check cycle");
  }
  
  // If we're in the process of checking feeds, check one feed at a time
  if (_checkingFeeds && (millis() - _lastFeedCheck >= _feedCheckInterval)) 
  {
    if (_feedCheckState < _activeFeedCount) 
    {
      // Check the current feed
      if (debug && doDebug) debug->printf("RSSreaderClass::loop(): Checking feed[%d] of [%d]\n", 
                              _feedCheckState + 1, _activeFeedCount);
      
      checkFeed(_feedCheckState);
      _lastFeedCheck = millis();
      _feedCheckState++;
    } 
    else 
    {
      // All feeds checked, now it's safe to delete old items
      deleteFeedsOlderThan(timeNow);
      
      if (debug && doDebug) debug->println("RSSreaderClass::loop(): Feed check cycle complete");
      _checkingFeeds = false;
      _lastCheck = millis(); // Reset the main interval timer
    }
  }

} // loop()


String RSSreaderClass::fetchFeed(const char* host, const char* path) 
{
  if (debug) debug->printf("RSSreaderClass::fetchFeed(): URL[%s], PATH[%s]\n", host, path);

  WiFiClientSecure client;
  client.setInsecure();  //-- we hebben geen certificaat
  client.setTimeout(5000); // 5s timeout

  if (!client.connect(host, 443)) 
  {
    if (debug && doDebug) debug->println("RSSreaderClass::fetchFeed(): Verbinding mislukt!");
    return "Fout: Kon geen TLS verbinding maken";
  }

  client.println("GET /" + String(path) + " HTTP/1.1");
  client.println("Host: " + String(host));
  client.println("User-Agent: ESP32RSSReader/1.0");
  client.println("Connection: close");
  client.println();

  if (debug && doDebug) debug->println("RSSreaderClass::fetchFeed(): Verzoek verzonden, wacht op antwoord...");

  // Antwoord inlezen (met totale timeout)
  String response;
  unsigned long startTime = millis();
  const unsigned long maxDuration = 8000;

  while ((client.connected() || client.available()) && millis() - startTime < maxDuration) 
  {
    if (client.available()) 
    {
      String line = client.readStringUntil('\n');
      response += line + "\n";
    } else {
      delay(10);
    }
  }

  client.stop();
  if (debug && doDebug) debug->println("RSSreaderClass::fetchFeed(): Verbinding gesloten");

  // Strip headers
  int bodyIndex = response.indexOf("\r\n\r\n");
  if (bodyIndex == -1) 
  {
    if (debug && doDebug) debug->println("RSSreaderClass::fetchFeed(): Kon body niet vinden in antwoord");
    return "Fout: HTTP headers niet correct";
  }

  String xmlPayload = response.substring(bodyIndex + 4);
  if (debug && xmlPayload.length() == 0) 
  {
    debug->printf("RSSreaderClass::fetchFeed(): Payload length: %d\n", xmlPayload.length());
    //debug->println(xmlPayload.substring(0, 300));
  }
  if (debug && doDebug) 
  {
    debug->printf("RSSreaderClass::fetchFeed(): Payload length: %d\n", xmlPayload.length());
    //debug->println(xmlPayload.substring(0, 300));
  }

  return xmlPayload;

} // fetchFeed()


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
    if (title.startsWith("<![CDATA[") && title.endsWith("]]>")) 
    {
      title = title.substring(9, title.length() - 3);
    }
    
    pos = end + 8;
    if (title.length() > 0) titles.push_back(title);
  }

  if (!titles.empty()) titles.erase(titles.begin()); // verwijder kanaaltitel
  return titles;

} // extractTitles()

std::vector<RSSreaderClass::FeedItem> RSSreaderClass::extractFeedItems(const String& feed)
{
  std::vector<FeedItem> items;
  int itemStart = 0;
  
  // Skip the channel title by finding the first item
  itemStart = feed.indexOf("<item>", itemStart);
  
  while (itemStart != -1) {
    int itemEnd = feed.indexOf("</item>", itemStart);
    if (itemEnd == -1) break;
    
    String itemContent = feed.substring(itemStart, itemEnd + 7);
    
    // Extract title
    String title = "";
    int titleStart = itemContent.indexOf("<title>");
    if (titleStart != -1) {
      titleStart += 7; // Length of "<title>"
      int titleEnd = itemContent.indexOf("</title>", titleStart);
      if (titleEnd != -1) {
        title = itemContent.substring(titleStart, titleEnd);
        title.trim();
        
        // Remove CDATA tags if present
        if (title.startsWith("<![CDATA[") && title.endsWith("]]>")) {
          title = title.substring(9, title.length() - 3);
        }
      }
    }
    
    // Extract publication date
    time_t pubDate = 0;
    int dateStart = itemContent.indexOf("<pubDate>");
    if (dateStart != -1) {
      dateStart += 9; // Length of "<pubDate>"
      int dateEnd = itemContent.indexOf("</pubDate>", dateStart);
      if (dateEnd != -1) {
        String dateStr = itemContent.substring(dateStart, dateEnd);
        dateStr.trim();
        pubDate = parseRSSDate(dateStr);
      }
    }
    
    // If no pubDate found or parsing failed, use current time
    if (pubDate == 0) {
      pubDate = time(nullptr);
      if (debug && doDebug) debug->printf("RSSreaderClass::extractFeedItems(): No valid pubDate found for item, using current time\n");
    }
    
    // Add item if title is not empty
    if (title.length() > 0) {
      FeedItem item;
      item.title = title;
      item.pubDate = pubDate;
      items.push_back(item);
    }
    
    // Find next item
    itemStart = feed.indexOf("<item>", itemEnd);
  }
  
  return items;

} // extractFeedItems()


time_t RSSreaderClass::parseRSSDate(const String& dateStr)
{
  // RFC 822/2822 format: "Wed, 02 Oct 2002 13:00:00 GMT"
  // We'll use a simple parsing approach
  
  if (debug && doDebug) debug->printf("RSSreaderClass::parseRSSDate(): Parsing date: %s\n", dateStr.c_str());
  
  // Extract components
  int dayPos = dateStr.indexOf(", ");
  if (dayPos == -1) return 0;
  
  dayPos += 2; // Skip ", "
  int dayEnd = dateStr.indexOf(" ", dayPos);
  if (dayEnd == -1) return 0;
  
  int monthPos = dayEnd + 1;
  int monthEnd = dateStr.indexOf(" ", monthPos);
  if (monthEnd == -1) return 0;
  
  int yearPos = monthEnd + 1;
  int yearEnd = dateStr.indexOf(" ", yearPos);
  if (yearEnd == -1) return 0;
  
  int timePos = yearEnd + 1;
  
  // Parse day
  int day = dateStr.substring(dayPos, dayEnd).toInt();
  
  // Parse month
  String monthStr = dateStr.substring(monthPos, monthEnd);
  int month = 0;
  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  for (int i = 0; i < 12; i++) {
    if (monthStr.equalsIgnoreCase(months[i])) {
      month = i + 1;
      break;
    }
  }
  if (month == 0) return 0;
  
  // Parse year
  int year = dateStr.substring(yearPos, yearEnd).toInt();
  
  // Parse time
  String timeStr = dateStr.substring(timePos);
  int hour = 0, minute = 0, second = 0;
  
  int colonPos = timeStr.indexOf(":");
  if (colonPos != -1) {
    hour = timeStr.substring(0, colonPos).toInt();
    int colonPos2 = timeStr.indexOf(":", colonPos + 1);
    if (colonPos2 != -1) {
      minute = timeStr.substring(colonPos + 1, colonPos2).toInt();
      int spacePos = timeStr.indexOf(" ", colonPos2);
      if (spacePos != -1) {
        second = timeStr.substring(colonPos2 + 1, spacePos).toInt();
      } else {
        second = timeStr.substring(colonPos2 + 1).toInt();
      }
    }
  }
  
  // Create tm structure and convert to time_t
  struct tm timeinfo;
  timeinfo.tm_year = year - 1900;
  timeinfo.tm_mon = month - 1;
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;
  timeinfo.tm_isdst = -1;
  
  time_t timestamp = mktime(&timeinfo);
  
  if (debug && doDebug) debug->printf("RSSreaderClass::parseRSSDate(): Parsed date components: %d-%02d-%02d %02d:%02d:%02d, timestamp: %ld\n", 
                          year, month, day, hour, minute, second, timestamp);
  
  return timestamp;

} // parseRSSDate()

std::vector<String> RSSreaderClass::getStoredLines(uint8_t feedIndex) 
{
  std::vector<String> lines;
  if (feedIndex >= _activeFeedCount) return lines;
  
  LittleFS.begin();
  File file = LittleFS.open(RSS_BASE_FOLDER + _filePaths[feedIndex], "r");
  if (!file) return lines;

  while (file.available()) 
  {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) lines.push_back(line);
  }

  file.close();
  return lines;

} // getStoredLines()

void RSSreaderClass::saveTitles(const std::vector<String>& titles, uint8_t feedIndex) 
{
  if (feedIndex >= _activeFeedCount) return;
  
  LittleFS.begin();
  File file = LittleFS.open(RSS_BASE_FOLDER + _filePaths[feedIndex], "a");
  if (!file) 
  {
    if (debug && doDebug) debug->println("RSSreaderClass::saveTitles(): Kan bestand niet openen voor schrijven");
    return;
  }

  for (const String& title : titles) 
  {
    time_t now = time(nullptr);
    if (debug && doDebug) debug->printf("RSSreaderClass::saveTitles(): Titel: %s\n", title.c_str());
    file.print(now);
    file.print("|");
    file.println(title);
  }

  file.close();

} // saveTitles()

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

} // titleExists()

void RSSreaderClass::checkFeed(uint8_t feedIndex)
{
  if (feedIndex >= _activeFeedCount)
  {
    if (debug && doDebug) debug->printf("RSSreaderClass::checkFeed(): Invalid feed index: %d\n", feedIndex);
    return;
  }
  
  if (debug && doDebug) debug->printf("RSSreaderClass::checkFeed(): Checking feed %d: %s\n", 
                          feedIndex, _urls[feedIndex].c_str());
                          
  String feed = fetchFeed(_urls[feedIndex].c_str(), _paths[feedIndex].c_str());

  if (feed.startsWith("Fout:")) 
  {
    if (debug) debug->printf("RSSreaderClass::checkFeed(): Fout bij ophalen RSS-feed: [%s]\n", feed);
    return;
  }

  std::vector<FeedItem> feedItems = extractFeedItems(feed);
  std::vector<String> titlesToSave;
  
  // Filter items that have sufficient words
  for (const auto& item : feedItems) 
  {
    if (hasSufficientWords(item.title)) 
    {
      if (debug && doDebug) debug->printf("RSSreaderClass::checkFeed(): Titel gevonden: %s (timestamp: %ld)\n", 
                              item.title.c_str(), item.pubDate);
      titlesToSave.push_back(String(item.pubDate) + "|" + item.title);
    }
  }

  // Limit to _maxFeedsPerFile titles
  while (titlesToSave.size() > _maxFeedsPerFile[feedIndex]) 
  {
    titlesToSave.erase(titlesToSave.begin());
  }

  // Always open the file in "w" mode to empty it first
  LittleFS.begin();
  File file = LittleFS.open(RSS_BASE_FOLDER + _filePaths[feedIndex], "w"); 
  if (file) 
  {
    for (const auto& line : titlesToSave) 
    {
      file.println(line);
    }
    file.close();
    _lastFeedUpdate[feedIndex] = millis(); // Update the last feed update time
    if (debug) debug->printf("RSSreaderClass::checkFeed(): Feed[%d] now has [%d] items\n", feedIndex, titlesToSave.size());
  } 
  else 
  {
    if (debug) debug->println("RSSreaderClass::checkFeed(): Kan niet naar bestand schrijven.");
  }
  
} // checkFeed()


void RSSreaderClass::checkForNewFeedItems() 
{
  if (debug && doDebug) debug->println("RSSreaderClass::checkForNewFeedItems(): Checken op nieuwe RSS-items...");
  
  for (uint8_t feedIndex = 0; feedIndex < _activeFeedCount; feedIndex++)
  {
    if (debug && doDebug) debug->printf("RSSreaderClass::checkForNewFeedItems(): Checking feed %d: %s\n", 
                            feedIndex, _urls[feedIndex].c_str());
                            
    String feed = fetchFeed(_urls[feedIndex].c_str(), _paths[feedIndex].c_str());

    if (feed.startsWith("Fout:")) 
    {
      if (debug) debug->printf("RSSreaderClass::checkForNewFeedItems(): Fout bij ophalen RSS-feed: [%s]\n", feed);
      continue;
    }

    std::vector<String> existing = getStoredLines(feedIndex);
    std::vector<String> newTitles = extractTitles(feed);
    std::vector<String> updated = existing;
    bool changed = false;

    for (const auto& title : newTitles) 
    {
      if (!titleExists(title, existing) && hasSufficientWords(title)) 
      {
        if (debug && doDebug) debug->printf("RSSreaderClass::checkForNewFeedItems(): Nieuwe titel gevonden: %s\n", title.c_str());
        time_t now = time(nullptr);
        updated.push_back(String(now) + "|" + title);
        changed = true;
      }
    }

    // FIFO: maximaal _maxFeedsPerFile titels
    while (updated.size() > _maxFeedsPerFile[feedIndex]) 
    {
      updated.erase(updated.begin());
    }

    if (changed) 
    {
      LittleFS.begin();
      File file = LittleFS.open(RSS_BASE_FOLDER + _filePaths[feedIndex], "w"); 
      if (file) 
      {
        for (const auto& line : updated) 
        {
          file.println(line);
        }
        file.close();
        if (debug && doDebug) debug->println("RSSreaderClass::checkForNewFeedItems(): Nieuwe feeds toegevoegd.");
        if (debug) debug->printf("RSSreaderClass::checkForNewFeedItems(): Feed[%d] now has [%d] items\n", feedIndex, updated.size());
      } 
      else 
      {
        if (debug) debug->println("RSSreaderClass::checkForNewFeedItems(): Kan niet naar bestand schrijven.");
      }
    } 
    else 
    {
      if (debug && doDebug) debug->println("RSSreaderClass::checkForNewFeedItems(): Geen nieuwe titels.");
    }
  }

} // checkForNewFeedItems()

void RSSreaderClass::deleteFeedsOlderThan(struct tm timeNow) 
{
  if (debug && doDebug) debug->println("RSSreaderClass::deleteFeedsOlderThan(): Verwijderen van oude feeds...");

  // Calculate timestamp for 48 hours ago
  time_t now = time(nullptr);
  time_t fortyEightHoursAgo = now - (48 * 60 * 60); // 48 hours in seconds

  if (debug && doDebug) debug->printf("RSSreaderClass::deleteFeedsOlderThan(): Current time: %ld, 48 hours ago: %ld\n", 
                          now, fortyEightHoursAgo);

  for (uint8_t feedIndex = 0; feedIndex < _activeFeedCount; feedIndex++)
  {
    // Skip feeds that haven't been updated recently
    if (_lastFeedUpdate[feedIndex] == 0 || millis() - _lastFeedUpdate[feedIndex] > _interval * 2) {
      if (debug) debug->printf("RSSreaderClass::deleteFeedsOlderThan(): Skipping feed[%d] as it hasn't been updated recently\n", 
                              feedIndex);
      continue;
    }
    
    std::vector<String> lines = getStoredLines(feedIndex);
    std::vector<String> filtered;

    for (const auto& line : lines) 
    {
      int sep = line.indexOf('|');
      if (sep != -1) 
      {
        time_t ts = atol(line.substring(0, sep).c_str());
        if (ts >= fortyEightHoursAgo) 
        {
          filtered.push_back(line);
        }
        else if (debug && doDebug)
        {
          debug->printf("RSSreaderClass::deleteFeedsOlderThan(): Removing old item: %s (timestamp: %ld)\n", 
                        line.c_str(), ts);
        }
      }
    }

    // Safeguard: Don't delete all items if it would leave the feed empty
    if (filtered.empty() && !lines.empty()) {
      if (debug) debug->printf("RSSreaderClass::deleteFeedsOlderThan(): Keeping some items in feed[%d] to prevent empty feed\n", 
                              feedIndex);
      // Keep the newest items (up to half of max)
      size_t itemsToKeep = min(lines.size(), _maxFeedsPerFile[feedIndex] / 2);
      for (size_t i = lines.size() - itemsToKeep; i < lines.size(); i++) {
        filtered.push_back(lines[i]);
      }
    }

    // Only write if we have changes
    if (filtered.size() != lines.size()) 
    {
      LittleFS.begin();
      File file = LittleFS.open(RSS_BASE_FOLDER + _filePaths[feedIndex], "w");
      if (file) 
      {
        for (const auto& line : filtered) 
        {
          file.println(line);
        }
        file.close();
      }

      if (debug && doDebug) debug->printf("RSSreaderClass::deleteFeedsOlderThan(): Feed[%d], Feeds behouden[%d] / verwijderd[%d]\n", 
                              feedIndex, filtered.size(), lines.size() - filtered.size());
    }
  }

} // deleteFeedsOlderThan()


bool RSSreaderClass::getNextFeedItem(uint8_t& feedIndex, size_t& itemIndex)
{
  if (_activeFeedCount == 0)
  {
    if (debug) debug->println("RSSreaderClass::getNextFeedItem(): No active feeds");
    return false;
  }
  
  // Return current feed and item indices
  feedIndex = _currentFeedIndex;
  itemIndex = _currentItemIndices[_currentFeedIndex];
  
  // Increment the item index for this feed
  _currentItemIndices[_currentFeedIndex]++;
  
  // Check if we've reached the end of the current feed
  std::vector<String> lines = getStoredLines(_currentFeedIndex);
  if (_currentItemIndices[_currentFeedIndex] >= lines.size())
  {
    // Reset to beginning of this feed
    _currentItemIndices[_currentFeedIndex] = 0;
  }
  
  // Increment read counter for current feed
  _feedReadCounts[_currentFeedIndex]++;
  
  // Determine which feed to read from next using deficit round-robin
  if (_activeFeedCount > 1)
  {
    // Calculate ideal read ratios for each feed
    float totalItems = 0;
    for (uint8_t i = 0; i < _activeFeedCount; i++)
    {
      totalItems += _maxFeedsPerFile[i];
    }
    
    // Find feed with largest deficit between ideal and actual reads
    float maxDeficit = -1.0;
    uint8_t nextFeed = (_currentFeedIndex + 1) % _activeFeedCount; // Default to next feed
    
    uint32_t totalReads = 0;
    for (uint8_t i = 0; i < _activeFeedCount; i++)
    {
      totalReads += _feedReadCounts[i];
    }
    
    if (totalReads > 0) // Avoid division by zero
    {
      for (uint8_t i = 0; i < _activeFeedCount; i++)
      {
        float idealRatio = _maxFeedsPerFile[i] / totalItems;
        float actualRatio = (float)_feedReadCounts[i] / (float)totalReads;
        float deficit = idealRatio - actualRatio;
        
        if (deficit > maxDeficit)
        {
          maxDeficit = deficit;
          nextFeed = i;
        }
      }
    }
    
    _currentFeedIndex = nextFeed;
  }
  else
  {
    // Only one feed, stay on it
    _currentFeedIndex = 0;
  }
  
  // Reset counters if we've read a lot of items to prevent overflow
  if (_feedReadCounts[0] > 1000) // Arbitrary large number
  {
    for (uint8_t i = 0; i < _activeFeedCount; i++)
    {
      _feedReadCounts[i] = _feedReadCounts[i] / 2; // Reduce all proportionally
    }
  }
  
  return true;

} // getNextFeedItem()


bool RSSreaderClass::hasSufficientWords(const String& title) 
{
  int wordCount = 0;
  bool inWord = false;
  
  // Count words by tracking transitions between word and non-word characters
  for (size_t i = 0; i < title.length(); i++) 
  {
    char c = title[i];
    
    // Consider a character as part of a word if it's alphanumeric or certain special characters
    bool isWordChar = isAlphaNumeric(c) || c == '-' || c == '\'';
    
    // If we're not in a word and we find a word character, we're starting a new word
    if (!inWord && isWordChar) 
    {
      wordCount++;
      inWord = true;
    }
    // If we're in a word and we find a non-word character, we're ending the current word
    else if (inWord && !isWordChar) 
    {
      inWord = false;
    }
    
    // Explicitly treat certain punctuation as word separators
    if (c == ':' || c == ',' || c == ';' || c == '.' || c == '!' || c == '?') 
    {
      inWord = false;
    }
  }
  
  if (debug && doDebug) debug->printf("RSSreaderClass::hasSufficientWords(): Title [%s] has %d words\n", 
                          title.c_str(), wordCount);
  
  return wordCount > 3; // Return true if more than 3 words

} // hasSufficientWords()


void RSSreaderClass::checkFeedHealth() 
{
  unsigned long currentTime = millis();
  
  // Only check once per hour
  if (currentTime - _lastHealthCheck < _healthCheckInterval) {
    return;
  }
  
  _lastHealthCheck = currentTime;
  
  if (debug) debug->println("RSSreaderClass::checkFeedHealth(): Checking feed health...");
  
  for (uint8_t feedIndex = 0; feedIndex < _activeFeedCount; feedIndex++) 
  {
    std::vector<String> lines = getStoredLines(feedIndex);
    
    // If feed is empty or has very few items, force an update
    if (lines.size() < _maxFeedsPerFile[feedIndex] / 4) {
      if (debug) debug->printf("RSSreaderClass::checkFeedHealth(): Feed[%d] has only %d items, forcing update\n", 
                              feedIndex, lines.size());
      checkFeed(feedIndex);
    }
  }
} // checkFeedHealth()


void RSSreaderClass::createRSSfeedMap()
{
  if (!LittleFS.exists(RSS_BASE_FOLDER)) 
  {
    if (debug) debug->printf("RSSreaderClass::createRSSfeedMap(): Creating [%s] directory\n", RSS_BASE_FOLDER);
    if (!LittleFS.mkdir(RSS_BASE_FOLDER)) 
    {
      if (debug) debug->printf("RSSreaderClass::createRSSfeedMap(): Failed to create [%s] directory\n", RSS_BASE_FOLDER);
    }
  }

} // createRSSfeedMap()


String RSSreaderClass::readRssFeed(uint8_t feedIndex, size_t itemIndex) 
{
  if (feedIndex >= _activeFeedCount)
  {
    if (debug) debug->printf("RSSreaderClass::readRssFeed(): Invalid feed index[%d]\n", feedIndex);
    return "";
  }
  
  if (debug && doDebug) debug->printf("RSSreaderClass::readRssFeed(): FeedIndex[%d], ItemIndex[%d]\n", feedIndex, itemIndex);
  std::vector<String> lines = getStoredLines(feedIndex);
  
  if (itemIndex >= lines.size()) 
  {
    if (debug) debug->printf("RSSreaderClass::readRssFeed(): Geen RSS-feed gevonden voor nummer [%d] in feed[%d]\n", 
                            itemIndex, feedIndex);
    return "";
  }
  
  int sep = lines[itemIndex].indexOf('|');
  if (sep != -1) 
  {
    if (debug && doDebug) debug->printf("RSSreaderClass::readRssFeed(): Feed[%d], Item[%d] - [%s]\n", 
                            feedIndex, itemIndex, lines[itemIndex].c_str());
    return lines[itemIndex].substring(sep + 1);
  }

  return "";

} // RSSreaderClass::readRssFeed()