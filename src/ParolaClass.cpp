#include "ParolaClass.h"
#include <stdarg.h>

ParolaClass::ParolaClass() 
{
  // Initialize default state
  initialized = false;
  spiInitialized = false;
  parola = nullptr;
}

ParolaClass::~ParolaClass()
{
  // Clean up resources
  cleanup();
}

void ParolaClass::cleanup()
{
  if (parola != nullptr)
  {
    delete parola;
    parola = nullptr;
  }
  
  initialized = false;
}

bool ParolaClass::begin(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, const PAROLA &config)
{
  // Clean up any previous initialization
  cleanup();
  
  // Store pin configuration
  this->dataPin = dataPin;
  this->clkPin  = clkPin;
  this->csPin   = csPin;
  this->numZones = config.MY_MAX_ZONES;
  
  debugPrint("ParolaClass::begin() - dataPin[%d], clkPin[%d], csPin[%d], MAX_DEVICES[%d], ZONES[%d]", 
             dataPin, clkPin, csPin, config.MY_MAX_DEVICES, config.MY_MAX_ZONES);
  Serial.printf("ParolaClass::begin() - dataPin[%d], clkPin[%d], csPin[%d], MAX_DEVICES[%d], ZONES[%d]\n", 
             dataPin, clkPin, csPin, config.MY_MAX_DEVICES, config.MY_MAX_ZONES);
   
  // Validate configuration
  if (config.MY_MAX_DEVICES == 0 || config.MY_MAX_DEVICES > 32)
  {
    debugPrint("ParolaClass::begin() - Invalid MAX_DEVICES: [%d]", config.MY_MAX_DEVICES);
    return false;
  }
  
  // Set up CS pin first
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);
  
  // Initialize the SPI bus
  if (!initSPI())
  {
    debugPrint("ParolaClass::begin() - SPI initialization failed");
    return false;
  }
  
  // Add a delay to ensure SPI is ready
  delay(100);
  
  // Determine hardware type
  MD_MAX72XX::moduleType_t hardware;
  switch (config.MY_HARDWARE_TYPE)
  {
    case 0: hardware = MD_MAX72XX::PAROLA_HW; break;
    case 1: hardware = MD_MAX72XX::FC16_HW; break;
    case 2: hardware = MD_MAX72XX::GENERIC_HW; break;
    default: 
      debugPrint("ParolaClass::begin() - Unknown hardware type: [%d], defaulting to FC16_HW", config.MY_HARDWARE_TYPE);
      hardware = MD_MAX72XX::FC16_HW; 
      break;
  }

  try
  {
    // Create and configure the MD_Parola instance
    parola = new MD_Parola(hardware, csPin, config.MY_MAX_DEVICES);
    
    if (parola == nullptr)
    {
      debugPrint("ParolaClass::begin() - Failed to allocate MD_Parola instance");
      return false;
    }
    
    // Initialize the display
    if (!parola->begin(config.MY_MAX_ZONES))
    {
      debugPrint("ParolaClass::begin(MY_MAX_ZONES) - Failed to initialize MD_Parola");
      cleanup();
      return false;
    }
    if (config.MY_MAX_ZONES == 2)
    {
      parola->displaySuspend(true);  // <--- Freeze display updates
      
      parola->setZone(ZONE_LOWER, 0, (config.MY_MAX_DEVICES/2)-1);
      parola->setZone(ZONE_UPPER, (config.MY_MAX_DEVICES/2), config.MY_MAX_DEVICES-1);

      parola->setIntensity(5);       // Felheid 0-15
      parola->displayClear();        // Schoonmaken
      
      // Fonts instellen
      parola->setFont(BigFont);
      //parola->setFont(1, BigFont);
/**
      // Teksten instellen
      parola->displayZoneText(0, "ABCDE", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      parola->displayZoneText(1, "EFGHI", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      delay(200);
      parola->displayReset(0);
      parola->displayReset(1);
      parola->displaySuspend(false); // <--- Resume updates

      while(parola->displayAnimate())
      {
        delay(20);
      }
**/
    }
    else
    {
      parola->setFont(nullptr);  // Standaard font
    }

    // Set the display configuration
    // Set animation speed
    parola->setSpeed((50 + 5) - config.MY_MAX_SPEED);
    
    // Set initial intensity (brightness)
    parola->setIntensity(7); // Medium brightness (0-15)
    parola->displayClear();
    
    initialized = true;
    debugPrint("ParolaClass::begin() - Initialization successful");
    return true;
  }
  catch (...)
  {
    debugPrint("ParolaClass::begin() - Exception during initialization");
    cleanup();
    return false;
  }
} // begin()


void ParolaClass::setScrollSpeed(int16_t speed)
{
  // Update the speed in the display configuration
  displayConfig.speed = (50 + 5) - speed;
  
  // If initialized, update the speed in the parola object
  if (initialized && parola != nullptr)
  {
    //-- High Value is slow scrolling, Low Value is fast scrolling
    parola->setSpeed((displayConfig.speed));
  }
  
  debugPrint("ParolaClass::setScrollSpeed() - Speed -> updated to [%d]", displayConfig.speed);
}


void ParolaClass::setIntensity(int16_t intensity)
{
  parola->setIntensity(intensity);
  debugPrint("ParolaClass::setSIntensity() - updated to [%d]", intensity);
}


bool ParolaClass::initSPI()
{
  if (!spiInitialized) 
  {
    debugPrint("ParolaClass::initSPI() - Initializing SPI");
    
    try
    {
      SPI.setFrequency(1000000);
      // Initialize SPI with the specified pins
      SPI.begin(clkPin, -1, dataPin, csPin);  // SCK, MISO, MOSI, SS
      SPI.setFrequency(2000000);
      SPI.setHwCs(false);

      spiInitialized = true;
      debugPrint("ParolaClass::initSPI() - SPI initialization successful");
      return true;
    }
    catch (...)
    {
      debugPrint("ParolaClass::initSPI() - Exception during SPI initialization");
      spiInitialized = false;
      return false;
    }
  }
  
  return true; // Already initialized
}

void ParolaClass::setRandomEffects(const std::vector<uint8_t> &effects)
{
  if (effects.empty())
  {
    debugPrint("ParolaClass::setRandomEffects() - Warning: Empty effects list");
    return;
  }
  
  effectList = effects;
  debugPrint("ParolaClass::setRandomEffects() - Set [%d] effects", effectList.size());
}

void ParolaClass::setCallback(std::function<void(const std::string&)> callback)
{
  onFinished = callback;
  debugPrint("ParolaClass::setCallback() - Callback function set");
}

void ParolaClass::setDisplayConfig(const DisplayConfig &config)
{
  displayConfig = config;
  debugPrint("ParolaClass::setDisplayConfig() - Display config updated: speed[%d], pause[%d]", 
             config.speed, config.pauseTime);
}

textEffect_t ParolaClass::getRandomEffect()
{
  if (effectList.empty())
  {
    // Default to scroll left if no effects are defined
    debugPrint("ParolaClass::getRandomEffect() - No effects defined, using PA_SCROLL_RIGHT");
    return PA_SCROLL_RIGHT;
  }
  
  // Get a random effect from the list
  size_t index = random(0, effectList.size());
  if (index >= effectList.size())
  {
    index = 0; // Safety check
  }
  
  return static_cast<textEffect_t>(effectList[index]);
}


const char* ParolaClass::setHighBits(const std::string &text)
{
  upperZoneText.clear();
  upperZoneText.reserve(text.length() + 1);
  
  for (char c : text)
  {
    upperZoneText.push_back(c | 0x80);
  }
  
  return upperZoneText.c_str();

} // setHighBits()



bool ParolaClass::sendNextText(const std::string &text)
{
  if (!initialized || parola == nullptr)
  {
    debugPrint("ParolaClass::sendNextText() - Error: Not initialized");
    return false;
  }
  
  if (text.empty())
  {
    debugPrint("ParolaClass::sendNextText() - Warning: Empty text");
  }
  
  currentText = text;
  
  // Get effects for entry and exit
  textEffect_t effectIn = PA_SCROLL_LEFT;
  textEffect_t effectOut = getRandomEffect();
  
  debugPrint("ParolaClass::sendNextText() - Text: [%s], Effects: in=%d, out=%d", 
             currentText.c_str(), effectIn, effectOut);
  
  debugPrint("ParolaClass::sendNextText() - Using %d zone(s)", numZones);
  
  if (numZones <= 1)
  {
    // Single zone configuration - use displayText
    parola->displayText(
      (char *)currentText.c_str(),
      displayConfig.align,
      displayConfig.speed,
      displayConfig.pauseTime,
      effectIn,
      effectOut
    );
  }
  else
  {
    // Multi-zone configuration - use displayZoneText for each zone
    parola->displaySuspend(true);  // Freeze display updates while configuring
    
    // For a 2-zone setup:
    // ZONE_LOWER 0 (bottom) gets the original text
    // ZONE_UPPER 1 (top) gets the text with high bit set for each character
    parola->displayZoneText(ZONE_LOWER, (char *)currentText.c_str(), 
                           displayConfig.align,
                           displayConfig.speed,
                           displayConfig.pauseTime,
                           effectIn, effectOut);
    
    parola->displayZoneText(ZONE_UPPER, setHighBits(currentText), 
                           displayConfig.align,
                           displayConfig.speed,
                           displayConfig.pauseTime,
                           effectIn, effectOut);
    
    // Reset both zones to start the animation
    parola->displayReset(ZONE_LOWER);
    parola->displayReset(ZONE_UPPER);
    
    parola->displaySuspend(false);  // Resume display updates
  }
  
  return true;

} // sendNextText()


bool ParolaClass::animateBlocking(const String &text)
{
  if (!initialized || parola == nullptr)
  {
    debugPrint("ParolaClass::animateBlocking() - Error: Not initialized");
    return false;
  }

  if (text.length() == 0)
  {
    debugPrint("ParolaClass::animateBlocking() - Warning: Empty text");
  }

  // DEBUG: show what we're about to display
  debugPrint("ParolaClass::animateBlocking() - Text: [%s]", text.c_str());
  
  debugPrint("ParolaClass::animateBlocking() - Using %d zone(s)", numZones);
  
  if (numZones <= 1)
  {
    // Single zone configuration - use displayText
    parola->displayText(
      (char *)text.c_str(),
      displayConfig.align,
      5,  // Fixed speed for blocking animation
      displayConfig.pauseTime,
      PA_SCROLL_LEFT,
      PA_NO_EFFECT
    );
    
    // Wait for animation to complete
    while (!parola->displayAnimate())
    {
      delay(1);
    }
  }
  else
  {
    parola->setIntensity(2);  // Set intensity for both zones
    // Multi-zone configuration - use displayZoneText for each zone
    parola->displaySuspend(true);  // Freeze display updates while configuring
  
    // Convert String to std::string for our setHighBits method
    std::string stdText(text.c_str());
    
    // For a 2-zone setup:
    // ZONE_LOWER 0 (bottom) gets the original text
    // ZONE_UPPER 1 (top) gets the text with high bit set for each character
    parola->displayZoneText(ZONE_LOWER, (char *)text.c_str(), 
                           displayConfig.align,
                           10,  // Fixed speed for blocking animation
                           displayConfig.pauseTime,
                           PA_SCROLL_LEFT, PA_NO_EFFECT);
    
    parola->displayZoneText(ZONE_UPPER, setHighBits(stdText), 
                           displayConfig.align,
                           10,  // Fixed speed for blocking animation
                           displayConfig.pauseTime,
                           PA_SCROLL_LEFT, PA_NO_EFFECT);
    
    // Reset both zones to start the animation
    parola->displayReset(ZONE_LOWER);
    parola->displayReset(ZONE_UPPER);
    delay(500);  // Allow time for display to update

    parola->displaySuspend(false);  // Resume display updates
    
    delay(500);  // Allow time for display to update
    // Wait for animation to complete on both zones
    while (!parola->displayAnimate())
    {
      delay(10);
    }
    delay(1000);

  } // numZones == 2

  return true;

} // animateBlocking()



void ParolaClass::loop()
{
  if (!initialized || parola == nullptr)
  {
    return;
  }
  
  // Update the display animation
  if (parola->displayAnimate())
  {
    // Animation has completed, call the callback if set
    if (onFinished)
    {
      debugPrint("ParolaClass::loop() - Animation complete, calling callback");
      onFinished(currentText);
    }
  }
}

void ParolaClass::setDebug(Stream* debugPort)
{
  debug = debugPort;
  if (debug) 
  {
    debug->println("ParolaClass::setDebug() - Debugging enabled");
  }
} //  setDebug()

void ParolaClass::debugPrint(const char* format, ...)
{
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  if (doDebug)
  {
    if (debug)  debug->println(buffer);
    else        Serial.println(buffer);
  }
  
} // debugPrint()