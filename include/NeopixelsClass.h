#ifndef NEOPIXELS_CLASS_H
#define NEOPIXELS_CLASS_H

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <string>
#include <vector>

//-- needed to have a true dropin replacement for the ParolaClass
struct DisplayConfig
{
    uint16_t speed;         // Animation speed
    uint16_t pauseTime;     // Pause time in ms
    uint8_t  align;         // Text alignment
};

class NeopixelsClass 
{
private:
  // Private member variables
  Adafruit_NeoMatrix* matrix;
  std::vector<uint8_t> columnData;       // All pixel columns of the full message
  int scrollOffset = 0;                  // How far left we’ve scrolled
  bool scrollingActive = false;          // Is scrolling currently active
  int neopixelPin;
  int width;
  int height;
  uint8_t matrixType;
  uint8_t matrixLayout;
  uint8_t matrixDirection;
  uint8_t matrixSequence;
  neoPixelType pixelType;
  int red, green, blue;
  std::string text;
  std::string currentMessage;     // Current message being displayed
  std::string nextMessage;        // Next message to be displayed
  std::string combinedMessage;    // Combination of previous and newmessage
  std::string scrollBuffer;
  int currentMessageWidth;        // Width of current message in pixels
  int messageStartOffset = 0;
  int messageEndOffset = 0;
  int pixelPerChar;
  int scrollDelay;
  int brightness;
  int scrollPosition;
  int pass;
  bool textComplete;
  bool scrollStarted = false;
  unsigned long lastUpdateTime;
  bool initialized = false;
  bool matrixInitialized = false;
  bool configInitialized = false;
  std::string previousText;  // Store the previous text for continuous scrolling
  bool readyForNextMessage = false;  // Flag to indicate we're ready for the next message
  int lastStopPosition = 0;  // Store the last stop position for continuous scrolling

  std::function<void(const std::string&)> onFinished = nullptr;
  
  // Debug support
  Stream* debug = nullptr;
  void debugPrint(const char* format, ...);
  
  // Private helper methods
  int16_t scaleValue(int16_t input, int16_t minInValue, int16_t maxInValue, int16_t minOutValue, int16_t maxOutValue);
  void cleanup();
  std::string makeCombinedMessage(const std::string& oldText, const std::string& newText);
  void debugPrintBuffer();
  
public:
  // Constructor and destructor
  NeopixelsClass();
  ~NeopixelsClass();
  
  // Configuration methods
  void begin(uint8_t pin);
  void setup(uint8_t matrixType, uint8_t matrixLayout, uint8_t matrixDirection, uint8_t matrixSequence);
  void setPixelType(neoPixelType pixelType);
  void setMatrixSize(int width, int height);
  void setPixelsPerChar(int pixels);
  void setDebug(Stream* debugPort = &Serial);
  //void initializeDisplay();
  
  // Display control methods
  void setColor(int r, int g, int b);
  void setIntensity(int newBrightness);
  void setScrollSpeed(int newSpeed);
  void sendNextText(const std::string& text);
  bool animateNeopixels(bool triggerCallback = true);  
  void animateBlocking(const String &text);
  void loop();
  void tickerClear();
  void show();
  void setRandomEffects(const std::vector<uint8_t> &effects);
  void setCallback(std::function<void(const std::string&)> callback);
  void setDisplayConfig(const DisplayConfig &config);
  bool isInitialized() const { return initialized; }
  void reset();

#ifdef NEOPIXELS_DEBUG
    bool doDebug = true;
#else
    bool doDebug = false;
#endif

};

#endif // NEOPIXELS_CLASS_H
