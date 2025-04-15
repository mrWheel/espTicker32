#include "MediastackClass.h"

Mediastack::Mediastack(WiFiClient &thisClient) : thisClient(thisClient), apiUrl("") {}

void Mediastack::setup(const char *key, uint8_t maxMessages, Stream* debugPort)
{
  debug = debugPort;
  char tmp[100];
  apiKey = key;
  if (debug) debug->flush();
  if (debug) debug->printf("\nMediastack::setup() with apiKey[%s]\n", apiKey.c_str());
  snprintf(tmp, sizeof(tmp), "/api/weerlive_api_v2.php?key=%s&locatie=%s", key);
  apiUrl = String(tmp);
  if (debug) debug->printf("Mediastack::setup() apiUrl = [%s]\n", apiUrl.c_str());
  debug->printf("Mediastack::setup() Done! Free Heap: [%d] bytes\n", ESP.getFreeHeap());

} //  setup()

void Mediastack::setInterval(int newInterval)
{
  requestInterval = newInterval;

} //  setInterval()

void Mediastack::replaceUnicode(char* newsMessage) {
    const char* unicodeSequences[] = { "\\u2019", "\\u201e", "\\u201d", "\\u00e9", "\\u2018" };
    const char replacements[] = { '"', '"', '"', 'e', '"' };

    char* pos = newsMessage;
    while (*pos != '\0') {
        bool replaced = false;
        for (size_t i = 0; i < sizeof(unicodeSequences) / sizeof(unicodeSequences[0]); ++i) {
            size_t seqLength = strlen(unicodeSequences[i]);
            if (strncmp(pos, unicodeSequences[i], seqLength) == 0) {
                *pos = replacements[i];
                memmove(pos + 1, pos + seqLength, strlen(pos + seqLength) + 1);
                replaced = true;
                break;
            }
        }

        if (!replaced && strncmp(pos, "\\u", 2) == 0 &&
            isxdigit(pos[2]) && isxdigit(pos[3]) && isxdigit(pos[4]) && isxdigit(pos[5])) {
            *pos = '?';
            memmove(pos + 1, pos + 6, strlen(pos + 6) + 1);
        }

        if (!replaced) ++pos;
    }
}

bool Mediastack::fetchNews() {
    const char* host = "api.mediastack.com";
    const int port = 80;
    WiFiClient client;
    char newsMessage[NEWS_SIZE] = {};
    int statusCode = 0;

    String url = "/v1/news?access_key=";
    url += apiKey;
    url += "&countries=nl";

    Serial.printf("Connecting to %s:%d\n", host, port);
    if (!client.connect(host, port)) {
        Serial.println("Connection failed.");
        for (int i = 0; i <= maxMessages; ++i) {
            writeFileById("NWS", i, (i == 1) ? "There is No News ...." : "");
        }
        return false;
    }

    client.print(String("GET ") + url + " HTTP/1.0\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: ESP-ticker\r\n" +
                 "Connection: close\r\n\r\n");

    client.setTimeout(5000);
    while (client.connected() || client.available()) {
        yield();
        while (client.available()) {
            if (client.find("HTTP/1.1")) {
                statusCode = client.parseInt();
                Serial.printf("HTTP Status: %d\n", statusCode);
                if (statusCode != 200) {
                    for (int i = 0; i <= maxMessages; ++i) {
                        if (i == 1 && statusCode == 429)
                            writeFileById("NWS", i, "Too many requests. Try again later.");
                        else
                            writeFileById("NWS", i, (i == 1) ? "Er is geen nieuws ...." : "");
                    }
                    return false;
                }
            } else {
                Serial.println("Malformed response.");
                for (int i = 0; i <= maxMessages; ++i) {
                    writeFileById("NWS", i, (i == 1) ? "Er is geen nieuws ...." : "");
                }
                return false;
            }

            int msgNr = 0;
            while (client.find("\"title\":\"")) {
                memset(newsMessage, 0, sizeof(newsMessage));
                int msgIdx = 0;
                while (strIndex(newsMessage, "\"description\":") == -1 && msgIdx < NEWS_SIZE - 2) {
                    char c = client.read();
                    if (c >= 32 && c <= 126)
                        newsMessage[msgIdx++] = c;
                }
                newsMessage[msgIdx] = '\0';
                //-- remove the '"description":' part from the end
                if (msgIdx > 30)
                    newsMessage[msgIdx - 16] = 0;

                replaceUnicode(newsMessage);
                if (!hasNoNoWord(newsMessage) && strlen(newsMessage) > 15) {
                    if (msgNr <= maxMessages) {
                        writeFileById("NWS", msgNr, newsMessage);
                    }
                    msgNr++;
                }
            }
        }
    }

    client.stop();
    updateMessage("0", "News brought to you by 'Mediastack.com'");
    return true;
}

void Mediastack::clearNewsFiles() {
    char path[32];
    for (int i = 0; i <= maxMessages; ++i) {
        snprintf(path, sizeof(path), "/newsFiles/NWS-%03d", i);
        Serial.printf("Deleting: %s\n", path);
        LittleFS.remove(path);
    }
}

// ========== Default Implementations (can be overridden) ==========

void Mediastack::updateMessage(const char* id, const char* msg) {
    Serial.printf("[updateMessage] id: %s | msg: %s\n", id, msg);
}

void Mediastack::writeFileById(const char* prefix, int id, const char* content) {
    Serial.printf("[writeFileById] %s-%03d -> %s\n", prefix, id, content);
}

bool Mediastack::hasNoNoWord(const char* text) {
    return false;
}

int Mediastack::strIndex(const char* str, const char* substr) {
    const char* found = strstr(str, substr);
    return found ? (int)(found - str) : -1;
}