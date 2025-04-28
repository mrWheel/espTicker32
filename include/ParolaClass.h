#pragma once
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include "BigFont.h" // dubbel hoog font
#include <SPI.h>
#include <vector>

#define ZONE_UPPER  1
#define ZONE_LOWER  0

// Configuration structure for Parola display
struct PAROLA
{
    uint8_t MY_HARDWARE_TYPE;  // 0=PAROLA_HW, 1=FC16_HW, 2=GENERIC_HW
    uint8_t MY_MAX_DEVICES;    // Number of connected display modules
    uint8_t MY_MAX_ZONES;      // Number of display zones
    uint8_t MY_MAX_SPEED;      // Animation speed (lower = faster)
};

// Display configuration for text animation
struct DisplayConfig
{
    uint16_t speed;         // Animation speed
    uint16_t pauseTime;     // Pause time in ms
    textPosition_t align;   // Text alignment
};

class ParolaClass
{
  public:
    // Constructor and destructor
    ParolaClass();
    ~ParolaClass();
    
    // Initialization
    bool begin(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, const PAROLA &config);
    
    // Configuration methods
    void setRandomEffects(const std::vector<uint8_t> &effects);
    void setCallback(std::function<void(const std::string&)> callback);
    void setDisplayConfig(const DisplayConfig &config);
    void displayClear() { parola->displayClear(); }
    void setScrollSpeed(int16_t speed);
    void setIntensity(int16_t intensity);
    void setDebug(Stream* debugPort = &Serial);
    
    // Operation methods
    bool sendNextText(const std::string &text);
    // Blocking fixed animation method (right to left)
    bool animateBlocking(const String &text);
    void loop();
    
    // Status methods
    bool isInitialized() const { return initialized; }
    
  private:
    // Hardware configuration
    MD_Parola *parola = nullptr;
    uint8_t csPin = 0;
    uint8_t dataPin = 0;
    uint8_t clkPin = 0;
    uint8_t numZones = 1;  // Default to 1 zone

    bool initialized = false;
    bool spiInitialized = false;
    
    // Display configuration
    std::vector<uint8_t> effectList;
    DisplayConfig displayConfig = {50, 1000, PA_CENTER};
    
    // State tracking
    std::string currentText = "";
    std::function<void(const std::string&)> onFinished = nullptr;
    std::string upperZoneText; // For storing the high-bit version of text
    
    // Debug support
    Stream* debug = nullptr;
    void debugPrint(const char* format, ...);
    
    // Helper methods
    bool initSPI();
    void cleanup();
    textEffect_t getRandomEffect();
    const char* setHighBits(const std::string &text);


  #ifdef PAROLA_DEBUG
    bool doDebug = true;
  #else
    bool doDebug = false;
  #endif


};