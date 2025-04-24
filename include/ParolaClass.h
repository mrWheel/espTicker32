#pragma once
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <vector>

// Configuration structure for Parola display
struct PAROLA
{
    uint8_t HARDWARE_TYPE;  // 0=PAROLA_HW, 1=FC16_HW, 2=GENERIC_HW
    uint8_t MAX_DEVICES;    // Number of connected display modules
    uint8_t MAX_ZONES;      // Number of display zones
    uint8_t MAX_SPEED;      // Animation speed (lower = faster)
};

// Display configuration for text animation
struct DisplayConfig
{
    uint16_t speed;         // Animation speed (default: 50)
    uint16_t pauseTime;     // Pause time in ms (default: 1000)
    textPosition_t align;   // Text alignment (default: PA_CENTER)
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
    void setScrollSpeed(uint16_t speed);
    void setDebug(Stream* debugPort = &Serial);
    
    // Operation methods
    bool sendNextText(const std::string &text);
    void loop();
    
    // Status methods
    bool isInitialized() const { return initialized; }
    
  private:
    // Hardware configuration
    MD_Parola *parola = nullptr;
    uint8_t csPin = 0;
    uint8_t dataPin = 0;
    uint8_t clkPin = 0;
    bool initialized = false;
    bool spiInitialized = false;
    
    // Display configuration
    std::vector<uint8_t> effectList;
    DisplayConfig displayConfig = {50, 1000, PA_CENTER};
    
    // State tracking
    std::string currentText = "";
    std::function<void(const std::string&)> onFinished = nullptr;
    
    // Debug support
    Stream* debug = nullptr;
    void debugPrint(const char* format, ...);
    
    // Helper methods
    bool initSPI();
    void cleanup();
    textEffect_t getRandomEffect();
};