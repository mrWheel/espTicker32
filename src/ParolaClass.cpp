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
  this->clkPin = clkPin;
  this->csPin = csPin;
  
  debugPrint("ParolaClass::begin() - dataPin[%d], clkPin[%d], csPin[%d], MAX_DEVICES[%d]", 
             dataPin, clkPin, csPin, config.MY_MAX_DEVICES);
  
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
    
    // Set animation speed
    parola->setSpeed(206 - config.MY_MAX_SPEED);
    
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
  
  debugPrint("ParolaClass::setScrollSpeed() - Speed[%d] -> updated to [%d]", speed, displayConfig.speed);
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
  
  // Get random effects for entry and exit
  //textEffect_t effectIn = getRandomEffect();
  textEffect_t effectIn = PA_SCROLL_LEFT;
  textEffect_t effectOut = getRandomEffect();
  
  debugPrint("ParolaClass::sendNextText() - Text: [%s], Effects: in=%d, out=%d", 
             currentText.c_str(), effectIn, effectOut);
  
  // Display the text with the configured parameters
  parola->displayText(
    (char *)currentText.c_str(),
    displayConfig.align,
    displayConfig.speed,
    displayConfig.pauseTime,
    effectIn,
    effectOut
  );
  
  return true;
}

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

  parola->displayText(
    (char *)text.c_str(),  // <-- use input text directly
    displayConfig.align,
    5,
    displayConfig.pauseTime,
    PA_SCROLL_LEFT,
    PA_NO_EFFECT
  );

  while (!parola->displayAnimate())
  {
    delay(10);
  }

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
}

void ParolaClass::debugPrint(const char* format, ...)
{
  if (debug == nullptr)
  {
    return;
  }
  
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  if (debug)  debug->println(buffer);
  else        Serial.println(buffer);
  
} // debugPrint()