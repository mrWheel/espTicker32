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
    uint8_t align;          // Text alignment
};

class NeopixelsClass 
{
private:
  // Private member variables
  Adafruit_NeoMatrix* matrix;
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
  int pixelPerChar;
  int scrollDelay;
  int brightness;
  int x;
  int pass;
  bool textComplete;
  unsigned long lastUpdateTime;
  bool initialized = false;
  std::function<void(const std::string&)> onFinished = nullptr;

  
public:
  // Constructor and destructor
  NeopixelsClass();
  ~NeopixelsClass();
  
  Stream* debug; // Debug output stream

  // Configuration methods
  void begin(uint8_t pin);
  void setup(uint8_t matrixType, uint8_t matrixLayout, uint8_t matrixDirection, uint8_t matrixSequence);
  void setPixelType(neoPixelType pixelType);
  void setMatrixSize(int width, int height);
  void setPixelsPerChar(int pixels);
  void setDebug(Stream* debugOutput = &Serial) { debug = debugOutput; }
  
  // Display control methods
  void setColor(int r, int g, int b);
  void setIntensity(int brightness);
  void setScrollSpeed(int speed);
  void sendNextText(const std::string& text);
  bool animateNeopixels();
  void animateBlocking(const String &text);
  void loop();
  void tickerClear();
  void show();
  void setRandomEffects(const std::vector<uint8_t> &effects);
  void setCallback(std::function<void(const std::string&)> callback);
  void setDisplayConfig(const DisplayConfig &config);
  bool isInitialized() const { return initialized; }

};

#endif // NEOPIXELS_CLASS_H