#ifndef MEDIASTACK_CLASS_H
#define MEDIASTACK_CLASS_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <LittleFS.h>

#define NEWS_SIZE 512

class Mediastack {
public:
    Mediastack(WiFiClient &wmediastackClient);
    void setup(const char *key, uint8_t maxMessages, Stream* debugPort);
    void setInterval(int newInterval);
    bool fetchNews();
    void clearNewsFiles();

    // Previously externs – now public methods you can override
    virtual void updateMessage(const char* id, const char* msg);
    virtual void writeFileById(const char* prefix, int id, const char* content);
    virtual bool hasNoNoWord(const char* text);
    virtual int strIndex(const char* str, const char* substr);

private:
    WiFiClient         &thisClient;
    void replaceUnicode(char* message);

    uint32_t            requestInterval = 0;
    uint32_t            loopTimer = 0;
    uint8_t  maxMessages;
    String              apiUrl;
    static const char  *apiHost;
    String              apiKey;
    Stream* debug = nullptr; // Optional, default to nullptr

};

#endif // MEDIASTACK_CLASS_H