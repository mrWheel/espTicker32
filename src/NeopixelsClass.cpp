#include "NeopixelsClass.h"

// Constructor implementation
NeopixelsClass::NeopixelsClass()
{
  // Initialize member variables
  debug = nullptr;
  matrix = nullptr;
  neopixelPin = 5; // Default neopixelPin
  width = 32; // Default width
  height = 8; // Default height
  pixelType = NEO_GRB + NEO_KHZ800;  // Default to most common NeoPixel configuration
  red = 255;
  green = 0;
  blue = 0;
  text = "Easy Tech";
  pixelPerChar = 6;
  scrollDelay = 2;
  brightness = 50;
  x = 0;
  pass = 0;
  textComplete = false;
  lastUpdateTime = 0;
}

// Destructor implementation
NeopixelsClass::~NeopixelsClass()
{
  if (debug) debugPrint("NeopixelsClass: Destructor called");
  
  if (matrix != nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: Deleting matrix object");
    delete matrix;
    matrix = nullptr;
  }
}

// Set the pixel type flags
void NeopixelsClass::setPixelType(neoPixelType pixelType)
{
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting pixel type to 0x%02d\n", (uint32_t)pixelType, HEX);
  }
  
  this->pixelType = pixelType;
}

// Set the matrix dimensions
void NeopixelsClass::setMatrixSize(int width, int height)
{
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting matrix size to %d x %d\n", width, height);
  }
  
  this->width = width;
  this->height = height;
}

// Set the number of pixels per character
void NeopixelsClass::setPixelsPerChar(int pixels)
{
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting pixels per character to %d\n", pixels);
  }
  
  this->pixelPerChar = pixels;
}

// Initialize the NeoPixel matrix with previously set parameters
void NeopixelsClass::begin(uint8_t pin)
{
  initialized = true;

  this->neopixelPin = pin;
  
  pinMode(this->neopixelPin, OUTPUT);
  digitalWrite(this->neopixelPin, LOW);

  if (debug)
  {
    debugPrint("NeopixelsClass: Initializing NeoPixel matrix...");
    debugPrint("  neopixelPin       : %d", neopixelPin);
    debugPrint("  Width             : %d", width);
    debugPrint("  Height            : %d", height);
    debugPrint("  Matrix Type       : 0x%02x", matrixType);
    debugPrint("  Matrix Layout     : 0x%02x", matrixLayout);
    debugPrint("  Matrix Direction  : 0x%02x", matrixDirection);
    debugPrint("  Matrix Sequence   : 0x%02x", matrixSequence);
    debugPrint("  Pixel Type        : 0x%02x", (uint32_t)pixelType);
  }
  
  if (matrix != nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: Deleting existing matrix object");
    delete matrix;
    matrix = nullptr;
  }
  
  if (debug) debugPrint("NeopixelsClass: Creating new matrix object");
  
  // Combine matrix configuration parameters
  uint8_t matrixConfig = matrixType + matrixDirection + matrixLayout + matrixSequence;
  
  if (debug)
  {
    debugPrint("NeopixelsClass: Combined matrix config: 0x%02x", matrixConfig);
  }
  
  // Create the matrix object with the specified parameters
  matrix = new Adafruit_NeoMatrix(
    width, height, neopixelPin,
    matrixConfig,
    pixelType);
  
  if (matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: ERROR - Failed to create matrix object!");
    return;
  }
  
  if (debug) debugPrint("NeopixelsClass: Calling matrix->begin()");
  matrix->begin();
  
  if (debug) debugPrint("NeopixelsClass: Setting matrix parameters");
  matrix->setTextWrap(false);
  matrix->setBrightness(brightness);
  matrix->setTextColor(matrix->Color(red, green, blue));
  
  x = matrix->width(); // Initialize x position
  if (debug)
  {
    debugPrint("NeopixelsClass: Initial x position set to: %d", x);
    debugPrint("NeopixelsClass: Initialization complete");
  }
  
  // Show an initial blank display
  matrix->fillScreen(0);
  matrix->show();
}

// Set the matrix configuration parameters
void NeopixelsClass::setup(uint8_t matrixType, uint8_t matrixLayout, uint8_t matrixDirection, uint8_t matrixSequence)
{
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting up matrix configuration");
    debugPrint("  Matrix Type      : 0x%02x", matrixType);
    debugPrint("  Matrix Layout    : 0x%02x", matrixLayout);
    debugPrint("  Matrix Direction : 0x%02x", matrixDirection);
    debugPrint("  Matrix Sequence  : 0x%02x", matrixSequence);
  }
  
  this->matrixType = matrixType;
  this->matrixLayout = matrixLayout;
  this->matrixDirection = matrixDirection;
  this->matrixSequence = matrixSequence;
  
  if (debug) debugPrint("NeopixelsClass: Matrix configuration set");
}

// Set the RGB color for the text
void NeopixelsClass::setColor(int r, int g, int b)
{
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting color");
    debugPrint("  Red  : %3d", r);
    debugPrint("  Green: %3d", g);
    debugPrint("  Blue : %3d", b);
  }
  
  this->red = r;
  this->green = g;
  this->blue = b;
  
  if (matrix != nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: Applying color to matrix");
    matrix->setTextColor(matrix->Color(red, green, blue));
  }
  else if (debug)
  {
    debugPrint("NeopixelsClass: Warning - matrix is null, color will be applied when matrix is initialized");
  }
}

// Set the brightness level
void NeopixelsClass::setIntensity(int brightness)
{
  if (brightness < 0) brightness = 0;
  if (brightness > 255) brightness = 255;
  
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting intensity to %d", brightness);
  }
  
  this->brightness = brightness;
  
  if (matrix != nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: Applying brightness to matrix");
    matrix->setBrightness(brightness);
    matrix->show(); // Update the display with new brightness
  }
  else if (debug)
  {
    debugPrint("NeopixelsClass: Warning - matrix is null, brightness will be applied when matrix is initialized");
  }
}

// Set the scrolling speed
void NeopixelsClass::setScrollSpeed(int speed)
{
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting speed to %d", speed);
  }
  
  // Convert speed (0-100) to delay value
  // Higher speed means lower delay
  if (speed > 0 && speed <= 100)
  {
    this->scrollDelay = (100 - speed) * 2.5;
    
    if (debug)
    {
      debugPrint("NeopixelsClass: Calculated scroll delay: %d", scrollDelay);
    }
  }
  else if (debug)
  {
    debugPrint("NeopixelsClass: Warning - speed out of range (1-100), not changing");
  }
}

// Set the text to be displayed
void NeopixelsClass::sendNextText(const std::string& text)
{
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting text to display: '");
    debugPrint(text.c_str());
    debugPrint("'");
  }
  
  this->text = text;
  
  if (matrix != nullptr)
  {
    this->x = matrix->width(); // Reset position
    if (debug)
    {
      debugPrint("NeopixelsClass: Reset x position to %d", x);
    }
  }
  else if (debug)
  {
    debugPrint("NeopixelsClass: Warning - matrix is null, x position will be reset when matrix is initialized");
  }
  
  this->pass = 0;
  this->textComplete = false;
  
  Serial.printf("NeopixelsClass: Text send: [%s]\n", text.c_str());
  if (debug) debugPrint("NeopixelsClass: Text set successfully");
}

// Clear the display
void NeopixelsClass::tickerClear()
{
  if (matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: tickerClear - matrix is null, returning");
    return;
  }
  
  matrix->fillScreen(0);
  matrix->show();
  
  if (debug) debugPrint("NeopixelsClass: Display cleared");
}

// Update the display
void NeopixelsClass::show()
{
  if (matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: show - matrix is null, returning");
    return;
  }
  
  matrix->show();
}

// Perform one step of the animation, return true when complete
bool NeopixelsClass::animateNeopixels(bool triggerCallback)
{
  if (matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: animateNeopixels - matrix is null, returning");
    return true;
  }
  
  // Calculate the maximum displacement based on text length and pixel width
  int maxDisplacement = text.length() * pixelPerChar + matrix->width();
  
  if (debug && x % 10 == 0)  // Only log every 10 steps to avoid flooding
  {
    debugPrint("NeopixelsClass: Animating - x=%d, pass=%d, maxDisplacement=%d", x, pass, maxDisplacement);
  }
  
  // Clear the display and set the cursor position
  matrix->fillScreen(0);
  matrix->setCursor(x, 0);
  
  // Print the text
  matrix->print(text.c_str());
  
  // Move the text position for the next frame
  if (--x < -maxDisplacement)
  {
    if (debug) debugPrint("NeopixelsClass: Reached end of text, resetting position");
    
    x = matrix->width();
    
    if (++pass >= 1)
    {
      if (debug) debugPrint("NeopixelsClass: Animation complete");
      
      pass = 0;
      matrix->setTextColor(matrix->Color(red, green, blue));
      textComplete = true;
      
      // Only call the callback if triggerCallback is true
      if (textComplete && onFinished && triggerCallback)
      {
        onFinished(text);
      }
      return true;
    }
  }
  
  // Update the display
  matrix->show();
  
  return textComplete;

}

// Block until the entire text is displayed
void NeopixelsClass::animateBlocking(const String &text)
{
  if (debug) debugPrint("NeopixelsClass: Starting blocking animation for text: %s", text.c_str());
  Serial.printf("NeopixelsClass: Starting blocking animation for text: %s\n", text.c_str()); 
  
  // Set the text to be displayed
  this->text = text.c_str();
  
  // Reset animation state
  if (matrix != nullptr) {
    this->x = matrix->width();
  } else {
    this->x = width; // Use the stored width if matrix is null
  }
  this->pass = 0;
  this->textComplete = false;
  
  int animationStep = 0;
  
  // Now animate until complete - pass false to prevent callback triggering
  while (!animateNeopixels(false))
  {
    if (animationStep % 20 == 0)
    {
      if (debug) debugPrint("NeopixelsClass: Blocking animation step %d", animationStep);
    }
    
    delay(5);
    animationStep++;
  }
  
  if (debug) debugPrint("NeopixelsClass: Blocking animation complete");

}

void NeopixelsClass::setRandomEffects(const std::vector<uint8_t> &effects)
{
  // This can be a no-op or log that it was called
  if (debug) debugPrint("NeopixelsClass: setRandomEffects called (no effect)");
}

void NeopixelsClass::setCallback(std::function<void(const std::string&)> callback)
{
  if (debug) debugPrint("NeopixelsClass: Setting callback function");
  this->onFinished = callback;
}

void NeopixelsClass::setDisplayConfig(const DisplayConfig &config)
{
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting display config");
    debugPrint("  Speed       : %d", config.speed);
    debugPrint("  Pause Time  : %d", config.pauseTime);
  }
  
  // Map the DisplayConfig parameters to NeopixelsClass parameters
  this->setScrollSpeed(config.speed);

}

// Main loop function that calls animateNeopixels with timing control
void NeopixelsClass::loop()
{
  if (matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: loop - matrix is null, returning");
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Only update the animation if enough time has passed
  if (currentTime - lastUpdateTime >= scrollDelay)
  {
    animateNeopixels();
    lastUpdateTime = currentTime;
  }
}

void NeopixelsClass::setDebug(Stream* debugPort)
{
  debug = debugPort;
  if (debug) 
  {
    debugPrint("ParolaClass::setDebug() - Debugging enabled");
  }
}

void NeopixelsClass::debugPrint(const char* format, ...)
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