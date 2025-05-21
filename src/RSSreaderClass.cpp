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
  // Initialize skipWords container
  _skipWords = { "Voetbal", "Voetballer", "Voetballers", "Voetbalster", "Voetbalsters", "KNVB" };  
  //-- be aware: no debug set yes, so no print messages
  readSkipWordsFromFile();

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

  createRSSfeedFolder();

  if (debug && doDebug) debug->printf("RSSreaderClass::addRSSfeed(): URL:[%s], Path:[%s], File:[%s], Max Feeds:[%d], Index: [%d]\n" 
                                                                  , _urls[feedIndex].c_str()
                                                                  , _paths[feedIndex].c_str()
                                                                  , _filePaths[feedIndex].c_str()
                                                                  , _maxFeedsPerFile[feedIndex], feedIndex);

  // Always open the file in "w" mode to empty it
  LittleFS.begin();
  File file = LittleFS.open(RSS_BASE_FOLDER + _filePaths[feedIndex], "w");
  if (file) 
  {
    file.close();
    if (debug && doDebug) debug->printf("RSSreaderClass::addRSSfeed(): Bestand [%s] aangemaakt/geleegd.\n", (RSS_BASE_FOLDER + _filePaths[feedIndex]).c_str());
  } 
  else 
  {
    if (debug) debug->printf("RSSreaderClass::addRSSfeed(): Kan bestand [%s] niet aanmaken/legen.\n", (RSS_BASE_FOLDER + _filePaths[feedIndex]).c_str());
    return false;
  }
  
  _activeFeedCount++;  // Increment counter
  
  // Check this feed immediately
  if (feedIndex == 0)
  {
    if (debug && doDebug) debug->printf("RSSreaderClass::addRSSfeed(): Checking feed[%d] immediately\n", feedIndex);
    checkFeed(feedIndex);
    _feedCheckState = 1;  // Start with feed 1 in the next loop cycle  
  }

  return true;

} // addRSSfeed()


void RSSreaderClass::loop(struct tm timeNow) 
{
  // Check if this is the first time loop() is called or if the interval has passed
  if (!_checkingFeeds && (_lastCheck == 0 || (millis() - _lastCheck >= _interval))) 
  {
    // Start the feed checking process
    _checkingFeeds = true;
    
    // If feed 0 has already been checked, start with feed 1
    if (_lastFeedUpdate[0] > 0) {
      _feedCheckState = 1;
    } else {
      _feedCheckState = 0;
    }
    
    _lastFeedCheck = 0; // Force immediate check of first feed
    
    if (debug && doDebug) debug->println("RSSreaderClass::loop(): Starting feed check cycle");
  }
  
  // Check feed health periodically
  if (millis() - _lastHealthCheck >= _healthCheckInterval) 
  {
    checkAllFeedsHealth();
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
      // All feeds checked
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
  uint32_t startTime = millis();
  const uint32_t maxDuration = 8000;

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
    if (debug) debug->printf("RSSreaderClass::checkFeed(): Invalid feed index: %d\n", feedIndex);
    return;
  }
  
  if (debug && doDebug) debug->printf("RSSreaderClass::checkFeed(): Checking feed [%d] [%s]\n", 
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
    if (hasSufficientWords(item.title) && hasNoSkipWords(item.title)) 
    {
      if (debug && doDebug) debug->printf("RSSreaderClass::checkFeed(): Titel gevonden: [%s] (timestamp: [%ld])\n", 
                              item.title.c_str(), item.pubDate);
      titlesToSave.push_back(String(item.pubDate) + "|" + item.title);
    }
  }

  // Limit to _maxFeedsPerFile titles
  while (titlesToSave.size() > _maxFeedsPerFile[feedIndex]) 
  {
    titlesToSave.erase(titlesToSave.begin());
  }

  _actFeedsPerFile[feedIndex] = 0; // Reset the count for this feed

  // Always open the file in "w" mode to empty it first
  LittleFS.begin();
  File file = LittleFS.open(RSS_BASE_FOLDER + _filePaths[feedIndex], "w"); 
  if (file) 
  {    
    for (const auto& line : titlesToSave) 
    {
//      simplifyCharacters(line);
//      file.println(line);
//      if (debug && doDebug) debug->printf("[%s]\n", line.c_str());
      String simplifiedLine = simplifyCharacters(line);
      file.println(simplifiedLine);
      if (debug && doDebug) debug->printf("[%s]\n", simplifiedLine.c_str());
      _actFeedsPerFile[feedIndex]++;
    }
    file.close();
    
    // Reset the read count for this feed when we update its contents
    // This ensures the balancing mechanism in getNextFeedItem() will work correctly
    // with the new feed content
    _feedReadCounts[feedIndex] = 0;
    
    _lastFeedUpdate[feedIndex] = millis(); // Update the last feed update time
    if (debug) debug->printf("RSSreaderClass::checkFeed(): Feed[%d] now has [%d] items\n", feedIndex, titlesToSave.size());
  } 
  else 
  {
    if (debug) debug->println("RSSreaderClass::checkFeed(): Kan niet naar bestand schrijven.");
  }

} // checkFeed()


/*
** 1. First, it tries to find a feed with items by checking each feed starting from `_currentFeedIndex`.
** 2. Once it finds a feed with items, it sets the `feedIndex` and `itemIndex` parameters to the 
**    current feed and item.
** 3. It increments the item index for the current feed and checks if it has reached the end of the feed.
** 4. It increments a read counter for the current feed.
** 5. It tracks consecutive reads from the same feed and forces rotation after 3 consecutive reads.
** 6. It determines which feed to read from next using a deficit round-robin approach.
*/
bool RSSreaderClass::getNextFeedItem(uint8_t& feedIndex, size_t& itemIndex)
{
  if (_activeFeedCount == 0)
  {
    if (debug) debug->println("RSSreaderClass::getNextFeedItem(): No active feeds");
    return false;
  }
  
  // Try to find a feed with items
  uint8_t attemptCount = 0;
  bool foundFeedWithItems = false;
  
  while (attemptCount < _activeFeedCount && !foundFeedWithItems)
  {
    // Check if current feed has items
    std::vector<String> lines = getStoredLines(_currentFeedIndex);
    
    if (lines.size() > 0)
    {
      foundFeedWithItems = true;
    }
    else
    {
      // Move to next feed
      _currentFeedIndex = (_currentFeedIndex + 1) % _activeFeedCount;
      attemptCount++;
    }
  }
  
  if (!foundFeedWithItems)
  {
    if (debug) debug->println("RSSreaderClass::getNextFeedItem(): No feeds have items");
    return false;
  }
  
  // Now we have a feed with items
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
  
  // Force rotation after reading a certain number of consecutive items from the same feed
  static uint8_t consecutiveReadsFromSameFeed = 0;
  static uint8_t lastFeedRead = _currentFeedIndex;
  
  if (lastFeedRead == _currentFeedIndex)
  {
    consecutiveReadsFromSameFeed++;
  }
  else
  {
    consecutiveReadsFromSameFeed = 1;
    lastFeedRead = _currentFeedIndex;
  }
  
  // Force rotation after 3 consecutive reads from the same feed
  bool forceRotation = (consecutiveReadsFromSameFeed >= 3);
  
  // Determine which feed to read from next using modified deficit round-robin
  if (_activeFeedCount > 1)
  {
    // Get actual item counts for all feeds
    std::vector<size_t> itemCounts(_activeFeedCount, 0);
    float totalItems = 0;
    
    for (uint8_t i = 0; i < _activeFeedCount; i++)
    {
      std::vector<String> feedLines = getStoredLines(i);
      itemCounts[i] = feedLines.size();
      totalItems += feedLines.size();
    }
    
    uint8_t nextFeed = _currentFeedIndex;
    
    // Reset read counts more frequently - when any feed reaches 100 reads
    // This helps prevent the deficit calculations from getting "stuck"
    bool shouldResetCounts = false;
    for (uint8_t i = 0; i < _activeFeedCount; i++)
    {
      if (_feedReadCounts[i] >= 100)
      {
        shouldResetCounts = true;
        break;
      }
    }
    
    if (shouldResetCounts)
    {
      if (debug && doDebug) debug->println("RSSreaderClass::getNextFeedItem(): Resetting feed read counts");
      for (uint8_t i = 0; i < _activeFeedCount; i++)
      {
        // Reduce all proportionally but maintain their ratios
        // This is better than setting to 0 as it preserves recent history
        _feedReadCounts[i] = _feedReadCounts[i] / 4;
      }
    }
    
    if (forceRotation)
    {
      // Force rotation to next feed with items
      uint8_t attempts = 0;
      nextFeed = (_currentFeedIndex + 1) % _activeFeedCount;
      
      while (attempts < _activeFeedCount)
      {
        if (itemCounts[nextFeed] > 0)
        {
          break; // Found a feed with items
        }
        nextFeed = (nextFeed + 1) % _activeFeedCount;
        attempts++;
      }
      
      if (debug && doDebug) debug->printf("RSSreaderClass::getNextFeedItem(): Forcing rotation to feed %d after %d consecutive reads\n", 
                            nextFeed, consecutiveReadsFromSameFeed);
      
      consecutiveReadsFromSameFeed = 0;
    }
    else
    {
      // Calculate total reads
      uint32_t totalReads = 0;
      for (uint8_t i = 0; i < _activeFeedCount; i++)
      {
        totalReads += _feedReadCounts[i];
      }
      
      if (totalReads > 0) // Avoid division by zero
      {
        // Modified deficit calculation with decay factor
        float maxScore = -1000.0;
        
        for (uint8_t i = 0; i < _activeFeedCount; i++)
        {
          // Only consider feeds that have items
          if (itemCounts[i] > 0)
          {
            // Calculate ideal ratio based on actual item counts
            float idealRatio = itemCounts[i] / totalItems;
            
            // Calculate actual ratio of reads
            float actualRatio = (float)_feedReadCounts[i] / (float)totalReads;
            
            // Basic deficit calculation
            float deficit = idealRatio - actualRatio;
            
            // Add a small random factor (0.01 to 0.05) to prevent feeds from being "stuck"
            float randomFactor = (float)(random(1, 5)) / 100.0;
            
            // Add a "recovery factor" that gives feeds with negative deficit a chance to be selected
            // The more negative the deficit, the stronger the recovery factor
            float recoveryFactor = 0;
            if (deficit < 0)
            {
              // This will be stronger for more negative deficits
              recoveryFactor = -deficit * 0.2;
            }
            
            // Final score combines deficit, recovery factor, and random factor
            float score = deficit + recoveryFactor + randomFactor;
            
            if (debug && doDebug) debug->printf("RSSreaderClass::getNextFeedItem(): Feed[%d] score: %f (deficit: %f, recovery: %f, random: %f, items: %d)\n", 
                                  i, score, deficit, recoveryFactor, randomFactor, itemCounts[i]);
            
            if (score > maxScore)
            {
              maxScore = score;
              nextFeed = i;
            }
          }
        }
        
        if (debug && doDebug) debug->printf("RSSreaderClass::getNextFeedItem(): Selected next feed: %d (max score: %f)\n", 
                              nextFeed, maxScore);
      }
      else
      {
        // If no reads yet, just go to the next feed with items
        nextFeed = (_currentFeedIndex + 1) % _activeFeedCount;
        uint8_t attempts = 0;
        
        while (attempts < _activeFeedCount)
        {
          if (itemCounts[nextFeed] > 0)
          {
            break;
          }
          nextFeed = (nextFeed + 1) % _activeFeedCount;
          attempts++;
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
  
  if (debug) debug->printf("RSSreaderClass::getNextFeedItem(): return Feed[%d] Item[%d] (feed has [%d] items)\n", 
                          feedIndex, itemIndex, lines.size());
  
  return true;

} // getNextFeedItem()


String RSSreaderClass::checkFeedHealth(uint8_t feedNr)
{
  if (debug && doDebug) debug->printf("RSSreaderClass::checkFeedHealth(%d): Checking feed health...\n", feedNr);
  
  // Check if the feed file exists
  LittleFS.begin();
  String filePath = RSS_BASE_FOLDER + _filePaths[feedNr];
  bool fileExists = LittleFS.exists(filePath);
  
  if (debug && doDebug) debug->printf("RSSreaderClass::checkFeedHealth(): Feed[%d] file [%s] exists: %s\n", 
                          feedNr, filePath.c_str(), fileExists ? "Yes" : "No");
  
  // Get the stored lines and count them
  std::vector<String> lines = getStoredLines(feedNr);
  
  if (debug && doDebug) debug->printf("RSSreaderClass::checkFeedHealth(): Feed[%d] has %d items, max allowed: [%d]\n", 
                          feedNr, lines.size(), _maxFeedsPerFile[feedNr]);
  
  // Check read counts
  if (debug && doDebug) debug->printf("RSSreaderClass::checkFeedHealth(): Feed[%d] read count: [%d]\n", 
                          feedNr, _feedReadCounts[feedNr]);
  
  // Check current item index
  if (debug && doDebug) debug->printf("RSSreaderClass::checkFeedHealth(): Feed[%d] current item index: [%d]\n", 
                          feedNr, _currentItemIndices[feedNr]);
  
  // Print the first few items if any exist
  if (!lines.empty())
  {
    if (debug && doDebug) debug->printf("RSSreaderClass::checkFeedHealth(): Feed[%d] first few items:\n", feedNr);
    for (size_t j = 0; j < std::min(lines.size(), (size_t)3); j++)
    {
      if (debug && doDebug) debug->printf("  [%d]: %s\n", j, lines[j].c_str());
    }
  }
  
  //-- Print statistics
  char _msg[100];
  snprintf(_msg, sizeof(_msg), "feed[%d] has[%d] items, max Items [%d]", feedNr, lines.size(), _maxFeedsPerFile[feedNr]);
  if (debug) debug->printf("RSSreaderClass::checkFeedHealth(): %s\n", _msg);
  return String(_msg);

} // checkFeedHealth()


void RSSreaderClass::checkAllFeedsHealth()
{
  if (debug && doDebug) debug->println("RSSreaderClass::checkFeedHealth(): Checking feed health...");
  
  for (uint8_t i = 0; i < _activeFeedCount; i++)
  {
    checkFeedHealth(i);
  }
  
  _lastHealthCheck = millis();

} // checkAllFeedHealth()

String RSSreaderClass::simplifyCharacters(const String& input)
{
  if (debug && doDebug) debug->printf("RSSreaderClass::simplifyCharacters(): Processing [%s]\n", input.c_str());
  
  String result = input;
  
  // Handle HTML entities
  struct HtmlEntity {
    const char* entity;
    const char* replacement;
  };
  
  const HtmlEntity htmlEntities[] = {
    {"&amp;", "&"},
    {"&lt;", "<"},
    {"&gt;", ">"},
    {"&quot;", "\""},
    {"&apos;", "'"},
    {"&nbsp;", " "},
    {"&#39;", "'"},
    {"&#34;", "\""},
    {"&ndash;", "-"},
    {"&mdash;", "-"},
    {"&lsquo;", "'"},
    {"&rsquo;", "'"},
    {"&ldquo;", "\""},
    {"&rdquo;", "\""},
    {"&bull;", "*"},
    {"&hellip;", "..."},
    {"&trade;", "TM"},
    {"&copy;", "(c)"},
    {"&reg;", "(r)"},
    {"&euro;", "EUR"},
    {"&pound;", "GBP"},
    {"&yen;", "JPY"},
    {"&cent;", "c"},
    {"&sect;", "S"},
    {"&para;", "P"},
    {"&deg;", "deg"},
    {"&plusmn;", "+/-"},
    {"&times;", "x"},
    {"&divide;", "/"},
    {"&frac14;", "1/4"},
    {"&frac12;", "1/2"},
    {"&frac34;", "3/4"}
  };
  
  // Replace HTML entities
  for (const auto& entity : htmlEntities) {
    int pos = 0;
    while ((pos = result.indexOf(entity.entity, pos)) != -1) {
      result = result.substring(0, pos) + entity.replacement + result.substring(pos + strlen(entity.entity));
      pos += strlen(entity.replacement);
    }
  }
  
  // Handle numeric HTML entities (&#xxxx;)
  int pos = 0;
  while ((pos = result.indexOf("&#", pos)) != -1) {
    int end = result.indexOf(";", pos);
    if (end == -1) break;
    
    String numericEntity = result.substring(pos, end + 1);
    String numStr = result.substring(pos + 2, end);
    
    char replacement = ' ';
    if (numStr.length() > 0) {
      if (numStr[0] == 'x' || numStr[0] == 'X') {
        // Hexadecimal entity (&#xXXXX;)
        long num = strtol(numStr.substring(1).c_str(), NULL, 16);
        replacement = (char)num;
      } else {
        // Decimal entity (&#XXXX;)
        long num = numStr.toInt();
        replacement = (char)num;
      }
    }
    
    result = result.substring(0, pos) + replacement + result.substring(end + 1);
    pos += 1; // Move past the replacement character
  }
  
  // Replace accented characters
  struct CharReplacement {
    const char* original;
    const char* replacement;
  };
  
  const CharReplacement charReplacements[] = {
    // Accented A
    {"à", "a"}, {"á", "a"}, {"â", "a"}, {"ã", "a"}, {"ä", "a"}, {"å", "a"}, {"ā", "a"}, {"ă", "a"}, {"ą", "a"},
    // Accented C
    {"ç", "c"}, {"ć", "c"}, {"č", "c"}, {"ĉ", "c"}, {"ċ", "c"},
    // Accented D
    {"ð", "d"}, {"ď", "d"}, {"đ", "d"},
    // Accented E
    {"è", "e"}, {"é", "e"}, {"ê", "e"}, {"ë", "e"}, {"ē", "e"}, {"ĕ", "e"}, {"ė", "e"}, {"ę", "e"}, {"ě", "e"},
    // Accented G
    {"ğ", "g"}, {"ģ", "g"}, {"ǧ", "g"}, {"ġ", "g"},
    // Accented H
    {"ĥ", "h"}, {"ħ", "h"},
    // Accented I
    {"ì", "i"}, {"í", "i"}, {"î", "i"}, {"ï", "i"}, {"ĩ", "i"}, {"ī", "i"}, {"ĭ", "i"}, {"į", "i"}, {"ı", "i"},
    // Accented J
    {"ĵ", "j"},
    // Accented K
    {"ķ", "k"}, {"ĸ", "k"},
    // Accented L
    {"ĺ", "l"}, {"ļ", "l"}, {"ľ", "l"}, {"ŀ", "l"}, {"ł", "l"},
    // Accented N
    {"ñ", "n"}, {"ń", "n"}, {"ņ", "n"}, {"ň", "n"}, {"ŉ", "n"}, {"ŋ", "n"},
    // Accented O
    {"ò", "o"}, {"ó", "o"}, {"ô", "o"}, {"õ", "o"}, {"ö", "o"}, {"ø", "o"}, {"ō", "o"}, {"ŏ", "o"}, {"ő", "o"},
    // Accented R
    {"ŕ", "r"}, {"ŗ", "r"}, {"ř", "r"},
    // Accented S
    {"ś", "s"}, {"ŝ", "s"}, {"ş", "s"}, {"š", "s"}, {"ſ", "s"},
    // Accented T
    {"ţ", "t"}, {"ť", "t"}, {"ŧ", "t"},
    // Accented U
    {"ù", "u"}, {"ú", "u"}, {"û", "u"}, {"ü", "u"}, {"ũ", "u"}, {"ū", "u"}, {"ŭ", "u"}, {"ů", "u"}, {"ű", "u"}, {"ų", "u"},
    // Accented W
    {"ŵ", "w"},
    // Accented Y
    {"ý", "y"}, {"ÿ", "y"}, {"ŷ", "y"},
    // Accented Z
    {"ź", "z"}, {"ż", "z"}, {"ž", "z"},
    // Special characters
    {"æ", "ae"}, {"œ", "oe"}, {"ß", "ss"}, {"þ", "th"},
    // Currency symbols
    {"€", "EUR"}, {"£", "GBP"}, {"¥", "JPY"}, {"¢", "c"},
    // Punctuation and symbols
    {"\xAB", "\""}, {"\xBB", "\""}, {"\x84", "\""}, {"\x93", "\""}, {"\x94", "\""}, {"\x91", "'"}, {"\x92", "'"}, {"\x82", ","},
    {"\x85", "..."}, {"\x96", "-"}, {"\x97", "-"}, {"\x95", "*"}, {"\xA9", "(c)"}, {"\xAE", "(r)"}, {"\x99", ""},
    {"\xA7", "S"}, {"\xB6", "P"}, {"\x86", "+"}, {"\x87", "++"}, {"\x89", "%"}, {"\x8B", "<"}, {"\x9B", ">"},
    {"\xB5", "u"}, {"\xBF", "?"}, {"\xA1", "!"}, {"\xB1", "+/-"}, {"\xD7", "x"}, {"\xF7", "/"},
    {"\xBC", "1/4"}, {"\xBD", "1/2"}, {"\xBE", "3/4"},{"\xB0", "*"}
  };
  
  // Replace special characters
  for (const auto& replacement : charReplacements) {
    int pos = 0;
    while ((pos = result.indexOf(replacement.original, pos)) != -1) {
      result = result.substring(0, pos) + replacement.replacement + result.substring(pos + strlen(replacement.original));
      pos += strlen(replacement.replacement);
    }
  }
  
  // Handle uppercase accented characters by converting to lowercase first
  String upperResult = result;
  upperResult.toUpperCase();
  
  if (upperResult != result) {
    // There are uppercase characters, apply the same replacements to the uppercase version
    String lowerResult = result;
    lowerResult.toLowerCase();
    
    for (int i = 0; i < result.length(); i++) {
      if (result[i] != lowerResult[i]) {
        // This is an uppercase character
        char lowerChar = lowerResult[i];
        char upperChar = result[i];
        
        // Check if the lowercase version was replaced
        for (int j = 0; j < i; j++) {
          if (lowerResult[j] == lowerChar && result[j] != upperChar) {
            // The lowercase version was replaced, apply the same replacement to uppercase
            result[i] = toupper(result[j]);
            break;
          }
        }
      }
    }
  }
  //-- last resort --
  for (int i = 0; i < result.length(); i++) {
    if (result[i] < 32 || result[i] > 126) {
      // Replace non-printable or non-ASCII characters with a space
      result[i] = ' ';
    }
  }
  
  // Recursively replace double spaces with single spaces
  bool hasDoubleSpace = true;
  while (hasDoubleSpace) {
    int pos = result.indexOf("  ");
    if (pos != -1) {
      // Replace double space with single space
      result = result.substring(0, pos) + " " + result.substring(pos + 2);
    } else {
      // No more double spaces found
      hasDoubleSpace = false;
    }
  }
  
  if (debug && doDebug) debug->printf("RSSreaderClass::simplifyCharacters(): Result [%s]\n", result.c_str());
  
  return result;

} // simplifyCharacters()


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

void RSSreaderClass::addToSkipWords(std::string skipWord)
{
  _skipWords.push_back(skipWord);
  if (debug && doDebug) debug->printf("RSSreaderClass::addToSkipWords(): Added word: [%s]\n", skipWord.c_str());

} // addToSkipWords()


void RSSreaderClass::addWordStringToSkipWords(std::string wordList)
{
  if (debug && doDebug) debug->printf("RSSreaderClass::addWordStringToSkipWords(): wordList: [%s]\n", wordList.c_str());
  // Current position in the string
  size_t pos = 0;
  size_t commaPos = 0;

  // Iterate through the string
  while (pos < wordList.length()) 
  {
    // Find the next comma
    commaPos = wordList.find(',', pos);
    
    // If no more commas, use the rest of the string
    if (commaPos == std::string::npos) 
    {
      commaPos = wordList.length();
    }
    
    // Extract the word
    std::string word = wordList.substr(pos, commaPos - pos);
    
    // Trim whitespace from the word
    size_t start = word.find_first_not_of(" \t\r\n");
    size_t end = word.find_last_not_of(" \t\r\n");
    
    if (start != std::string::npos && end != std::string::npos) 
    {
      word = word.substr(start, end - start + 1);
      
      // Add the word to the skipWords list
      addToSkipWords(word);
      
      if (debug && doDebug) debug->printf("RSSreaderClass::addWordStringToSkipWords(): Added word: %s\n", word.c_str());
    }
    
    // Move to the next word (skip the comma)
    pos = commaPos + 1;
  }

} // addWordStringToSkipWords()


void RSSreaderClass::readSkipWordsFromFile()
{
  if (debug) debug->println("RSSreaderClass::readSkipWordsFromFile(): Reading skip words from file");
  else       Serial.println("RSSreaderClass::readSkipWordsFromFile(): Reading skip words from file");
  
  LittleFS.begin();
  
  // Check if the file exists
  if (LittleFS.exists("/skipWords.txt"))
  {
    File file = LittleFS.open("/skipWords.txt", "r");
    if (file)
    {
      if (debug) debug->println("RSSreaderClass::readSkipWordsFromFile(): File opened successfully");
      else       Serial.println("RSSreaderClass::readSkipWordsFromFile(): File opened successfully");
      
      // Read the file line by line
      while (file.available())
      {
        String line = file.readStringUntil('\n');
        line.trim();
        
        if (line.length() > 0)
        {
          // Convert String to std::string and add to skip words
          std::string stdLine = line.c_str();
          addWordStringToSkipWords(stdLine);
          
          if (doDebug)
          {
            if (debug) debug->printf("RSSreaderClass::readSkipWordsFromFile(): Added line: [%s]\n", line.c_str());
            else       Serial.printf("RSSreaderClass::readSkipWordsFromFile(): Added line: [%s]\n", line.c_str());
          }
        }
      }
      
      file.close();
    }
    else
    {
      if (debug) debug->println("RSSreaderClass::readSkipWordsFromFile(): Failed to open file");
      else       Serial.println("RSSreaderClass::readSkipWordsFromFile(): Failed to open file");
    }
  }

} // readSkipWordsFromFile()


bool RSSreaderClass::hasNoSkipWords(const String& title)
{
  // Create a lowercase copy of the title
  String lowerTitle = title;
  lowerTitle.toLowerCase();
  
  // Check each skipWord
  for (const auto& word : _skipWords)
  {
    // Convert the skipWord to lowercase
    String lowerWord = String(word.c_str());
    lowerWord.toLowerCase();
    
    // For single-word skipWords, check with word boundaries
    if (lowerWord.indexOf(' ') == -1) {
      // Define word boundary characters
      const char* boundaries = " .,;:!?()-[]{}\"'";
      
      // Check if the word appears as a complete word in the title
      int pos = 0;
      while ((pos = lowerTitle.indexOf(lowerWord, pos)) != -1) {
        bool isWordStart = (pos == 0 || strchr(boundaries, lowerTitle[pos-1]));
        bool isWordEnd = (pos + lowerWord.length() == lowerTitle.length() || 
                          strchr(boundaries, lowerTitle[pos + lowerWord.length()]));
        
        if (isWordStart && isWordEnd) {
          if (debug) debug->printf("RSSreaderClass::hasNoSkipWords(): Found single-word skipWord [%s] in title [%s]\n", 
                                  word.c_str(), title.c_str());
          return false; // Found a skipWord, so return false
        }
        pos += lowerWord.length();
      }
    }
    // For multi-word skipWords, just check if they appear in the title
    else {
      if (lowerTitle.indexOf(lowerWord) != -1) {
        if (debug) debug->printf("RSSreaderClass::hasNoSkipWords(): Found multi-word skipWord [%s] in title [%s]\n", 
                                word.c_str(), title.c_str());
        return false; // Found a skipWord, so return false
      }
    }
  }
  
  return true; // No skipWords found, so return true

} // hasNoSkipWords()



void RSSreaderClass::createRSSfeedFolder()
{
  if (!LittleFS.exists(RSS_BASE_FOLDER)) 
  {
    if (debug) debug->printf("RSSreaderClass::createRSSfeedFolder(): Creating [%s] directory\n", RSS_BASE_FOLDER);
    if (!LittleFS.mkdir(RSS_BASE_FOLDER)) 
    {
      if (debug) debug->printf("RSSreaderClass::createRSSfeedFolder(): Failed to create [%s] directory\n", RSS_BASE_FOLDER);
    }
  }

} // createRSSfeedFolder()


String RSSreaderClass::readRSSfeed(uint8_t feedIndex, size_t itemIndex) 
{
  if (feedIndex >= _activeFeedCount)
  {
    if (debug) debug->printf("RSSreaderClass::readRSSfeed(): Invalid feed index[%d]\n", feedIndex);
    return "";
  }
  
  if (debug && doDebug) debug->printf("RSSreaderClass::readRSSfeed(): FeedIndex[%d], ItemIndex[%d]\n", feedIndex, itemIndex);
  std::vector<String> lines = getStoredLines(feedIndex);
  
  if (itemIndex >= lines.size()) 
  {
    if (debug) debug->printf("RSSreaderClass::readRSSfeed(): Geen RSS-feed gevonden voor nummer [%d] in feed[%d]\n", 
                            itemIndex, feedIndex);
    return "";
  }
  
  int sep = lines[itemIndex].indexOf('|');
  if (sep != -1) 
  {
    if (debug && doDebug) debug->printf("RSSreaderClass::readRSSfeed(): Feed[%d], Item[%d] - [%s]\n", 
                            feedIndex, itemIndex, lines[itemIndex].c_str());
    return lines[itemIndex].substring(sep + 1);
  }

  return "";

} // RSSreaderClass::readRSSfeed()