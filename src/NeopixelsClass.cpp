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
  scrollDelay = 1;
  brightness = 50;
  x = 0;
  pass = 0;
  textComplete = false;
  lastUpdateTime = 0;
  matrixInitialized = false;
  configInitialized = false;
}

// Destructor implementation
NeopixelsClass::~NeopixelsClass()
{
  if (debug) debugPrint("NeopixelsClass: Destructor called");
  cleanup();
}

// Cleanup resources
void NeopixelsClass::cleanup()
{
  if (matrix != nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: Deleting matrix object");
    delete matrix;
    matrix = nullptr;
  }
  
  matrixInitialized = false;
  configInitialized = false;
  initialized = false;
}

// Reset the class
void NeopixelsClass::reset()
{
  if (debug) debugPrint("NeopixelsClass: Resetting");
  
  // Clean up resources
  cleanup();
  
  // Reinitialize with default values
  if (neopixelPin > 0)
  {
    begin(neopixelPin);
  }
}

// Set the pixel type flags
void NeopixelsClass::setPixelType(neoPixelType pixelType)
{
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting pixel type to 0x%02x", (uint32_t)pixelType);
  }
  
  this->pixelType = pixelType;
}

// Set the matrix dimensions
void NeopixelsClass::setMatrixSize(int width, int height)
{
  if (width <= 0 || height <= 0)
  {
    if (debug) debugPrint("NeopixelsClass: Invalid matrix dimensions: %dx%d", width, height);
    return;
  }
  
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting matrix size to %d x %d", width, height);
  }
  
  this->width = width;
  this->height = height;
}

// Set the number of pixels per character
void NeopixelsClass::setPixelsPerChar(int pixels)
{
  if (pixels <= 0)
  {
    if (debug) debugPrint("NeopixelsClass: Invalid pixels per character: %d", pixels);
    return;
  }
  
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting pixels per character to %d", pixels);
  }
  
  this->pixelPerChar = pixels;
}

// Initialize the NeoPixel matrix with previously set parameters
void NeopixelsClass::begin(uint8_t pin)
{
  // Clean up any previous initialization
  cleanup();
  
  this->neopixelPin = pin;
  
  // Validate parameters
  if (width <= 0 || height <= 0)
  {
    if (debug) debugPrint("NeopixelsClass: Invalid matrix dimensions: %dx%d", width, height);
    return;
  }
  
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
  else
  {
    Serial.println("NeopixelsClass: Initializing NeoPixel matrix...");
    Serial.printf("  neopixelPin       : %d\n", neopixelPin);
    Serial.printf("  Width             : %d\n", width);
    Serial.printf("  Height            : %d\n", height);
    Serial.printf("  Matrix Type       : 0x%02x\n", matrixType);
    Serial.printf("  Matrix Layout     : 0x%02x\n", matrixLayout);
    Serial.printf("  Matrix Direction  : 0x%02x\n", matrixDirection);
    Serial.printf("  Matrix Sequence   : 0x%02x\n", matrixSequence);
    Serial.printf("  Pixel Type        : 0x%02x\n", (uint32_t)pixelType);
  }
  
  // Add a delay to ensure pin is ready
  delay(100);
  
  try
  {
    // Combine matrix configuration parameters
    uint8_t matrixConfig = matrixType + matrixDirection + matrixLayout + matrixSequence;
    
    if (debug)  debugPrint("NeopixelsClass: Combined matrix config: 0x%02x", matrixConfig);
    else     Serial.printf("Combined matrix config: 0x%02x\n", matrixConfig);
    
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
    
    // Add a delay to ensure memory allocation is complete
    delay(50);
    
    if (debug) debugPrint("NeopixelsClass: Calling matrix->begin()");
    matrix->begin();
    
    // Add a delay after begin
    delay(50);
    
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
    
    matrixInitialized = true;
    configInitialized = true;
    initialized = true;
  }
  catch (...)
  {
    if (debug) debugPrint("NeopixelsClass: Exception during initialization");
    cleanup();
  }
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
  
  configInitialized = true;
  
  if (debug) debugPrint("NeopixelsClass: Matrix configuration set");
}

// Set the RGB color for the text
void NeopixelsClass::setColor(int r, int g, int b)
{
  if (r < 0) r = 0; if (r > 255) r = 255;
  if (g < 0) g = 0; if (g > 255) g = 255;
  if (b < 0) b = 0; if (b > 255) b = 255;
  
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
  
  if (initialized && matrix != nullptr)
  {
    try
    {
      if (debug) debugPrint("NeopixelsClass: Applying color to matrix");
      matrix->setTextColor(matrix->Color(red, green, blue));
    }
    catch (...)
    {
      if (debug) debugPrint("NeopixelsClass: Exception while setting color");
    }
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
  
  if (initialized && matrix != nullptr)
  {
    try
    {
      if (debug) debugPrint("NeopixelsClass: Applying brightness to matrix");
      matrix->setBrightness(brightness);
      matrix->show(); // Update the display with new brightness
    }
    catch (...)
    {
      if (debug) debugPrint("NeopixelsClass: Exception while setting intensity");
    }
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
  if (!initialized || matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: sendNextText - not initialized, returning");
    return;
  }
  
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting text to display: '%s'", text.c_str());
  }
  
  this->text = text;
  
  try
  {
    this->x = matrix->width(); // Reset position
    if (debug)
    {
      debugPrint("NeopixelsClass: Reset x position to %d", x);
    }
  }
  catch (...)
  {
    if (debug) debugPrint("NeopixelsClass: Exception while resetting position");
    this->x = width; // Use the stored width if matrix access fails
  }
  
  this->pass = 0;
  this->textComplete = false;
  
  Serial.printf("NeopixelsClass: Text send: [%s]\n", text.c_str());
  if (debug) debugPrint("NeopixelsClass: Text set successfully");
}

// Clear the display
void NeopixelsClass::tickerClear()
{
  if (!initialized || matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: tickerClear - matrix is null, returning");
    return;
  }
  
  try
  {
    matrix->fillScreen(0);
    matrix->show();
    
    if (debug) debugPrint("NeopixelsClass: Display cleared");
  }
  catch (...)
  {
    if (debug) debugPrint("NeopixelsClass: Exception while clearing display");
  }
}

// Update the display
void NeopixelsClass::show()
{
  if (!initialized || matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: show - matrix is null, returning");
    return;
  }
  
  try
  {
    matrix->show();
  }
  catch (...)
  {
    if (debug) debugPrint("NeopixelsClass: Exception while showing display");
  }
}

// Perform one step of the animation, return true when complete
bool NeopixelsClass::animateNeopixels(bool triggerCallback)
{
  if (!initialized || matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass::animateNeopixels - matrix is null, returning");
    return true;
  }
 
  textComplete = false;

  // Calculate the maximum displacement based on text length and pixel width
  int maxDisplacement = text.length() * pixelPerChar + matrix->width();
  
  if (debug && x % 10 == 0)  // Only log every 10 steps to avoid flooding
  {
    debugPrint("NeopixelsClass::animateNeopixels - x=%d, pass=%d, maxDisplacement=%d", x, pass, maxDisplacement);
  //  Serial.printf("NeopixelsClass: Animating - x=%d, pass=%d, maxDisplacement=%d\n", x, pass, maxDisplacement);
  }
  
  try
  {
    // Clear the display and set the cursor position
    matrix->fillScreen(0);
    matrix->setCursor(x, 0);
    
    // Print the text
    matrix->print(text.c_str());
    
    // Move the text position for the next frame
    if (--x < -maxDisplacement)
    {
      if (debug) debugPrint("NeopixelsClass::animateNeopixels Reached end of text, resetting position");
      
      x = matrix->width();
      
      if (++pass >= 1)
      {
        if (debug) debugPrint("NeopixelsClass::animateNeopixels complete");
        
        pass = 0;
        matrix->setTextColor(matrix->Color(red, green, blue));
        textComplete = true;
       
        if (debug)  debugPrint("NeopixelsClass::animateNeopixels complete, triggering callback");
        else    Serial.println("NeopixelsClass::animateNeopixels complete, triggering callback");
        // Only call the callback if triggerCallback is true
        if (textComplete && onFinished && triggerCallback)
        {
          Serial.printf("NeopixelsClass::animateNeopixels Triggering callback with text: [%s]\n", text.c_str());
          onFinished(text);
        }
        
        // Clear the display after animation completes
        matrix->fillScreen(0);
        matrix->show();
        
        return true;
      }
    }
    
    // Update the display
    matrix->show();
    
    // Yield to prevent watchdog timer issues
    yield();
  }
  catch (...)
  {
    if (debug) debugPrint("NeopixelsClass::animateNeopixels Exception during animation");
    return true; // Return true to prevent infinite loop on error
  }
  
  return textComplete;
  
} // animateNeopixels()


// Block until the entire text is displayed
void NeopixelsClass::animateBlocking(const String &text)
{
  if (!initialized || matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: animateBlocking - not initialized, returning");
    else   Serial.println("NeopixelsClass: animateBlocking - not initialized, returning");
    return;
  }
  
  if (debug) debugPrint("NeopixelsClass: Starting animationBlocking for text: %s", text.c_str());
  else    Serial.printf("NeopixelsClass[S]: Starting animationBlocking for text: %s\n", text.c_str()); 
  
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
  
  // Clear the display first
  matrix->fillScreen(0);
  matrix->show();
  
  // Add a small delay to ensure display is cleared
  delay(50);
  
  int animationStep = 0;
  int maxDisplacement = text.length() * pixelPerChar + matrix->width();
  
  // Now animate until complete - pass false to prevent callback triggering
  while (x > -maxDisplacement && animationStep < 1000) // Add safety limit
  {
    if (animationStep % 20 == 0)
    {
      if (debug) debugPrint("NeopixelsClass: animationBlocking step %d", animationStep);
      else    Serial.printf("NeopixelsClass[S]: animationBlocking step %d\n", animationStep);
    }
    
    // Clear the display and set the cursor position
    matrix->fillScreen(0);
    matrix->setCursor(x, 0);
    
    // Print the text
    matrix->print(text.c_str());
    
    // Update the display
    matrix->show();
    
    // Move the text position for the next frame
    x--;
    
    //-??- delay(scrollDelay > 0 ? scrollDelay : 2);
    animationStep++;
    
    // Yield to prevent watchdog timer issues
    yield();
  }
  
  // Ensure we show the final frame
  matrix->fillScreen(0);
  matrix->show();
  
  if (debug) debugPrint("NeopixelsClass: animationBlocking complete");
  else   Serial.println("NeopixelsClass[S]: animationBlocking complete");

} // animateBlocking()

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
  if (!initialized || matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: loop - matrix is null, returning");
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Only update the animation if enough time has passed
  if (currentTime - lastUpdateTime >= scrollDelay)
  {
    try
    {
      animateNeopixels();
      lastUpdateTime = currentTime;
    }
    catch (...)
    {
      if (debug) debugPrint("NeopixelsClass: Exception in loop");
    }
  }
}

void NeopixelsClass::setDebug(Stream* debugPort)
{
  // Print to Serial first to ensure we see this message
  Serial.println("NeopixelsClass[S]::setDebug() - Debugging enabled");
  
  // Then set the debug pointer
  debug = debugPort;
  
  // Now try to use the debug pointer
  if (debug) 
  {
    try
    {
      debugPrint("NeopixelsClass::setDebug() - Debugging enabled");
    }
    catch (...)
    {
      // If debug print fails, fall back to Serial
      Serial.println("NeopixelsClass[S]::setDebug() - Exception in debug print");
    }
  }
} // setDebug()

void NeopixelsClass::debugPrint(const char* format, ...)
{
  // Safety check for null format
  if (format == nullptr) return;
  
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  // Always print to Serial for critical messages
  Serial.println(buffer);
  
  // Only print to debug stream if enabled
  if (doDebug && debug)
  {
    try
    {
      debug->println(buffer);
    }
    catch (...)
    {
      // If debug print fails, fall back to Serial
      Serial.println("NeopixelsClass[S]: Exception in debug->println");
    }
  }
} // debugPrint()


void NeopixelsClass::testLayout()
{
  matrix->setBrightness(40);
  matrix->setTextWrap(false);
  matrix->setTextSize(1); // Use 5x7 font
  matrix->setTextColor(matrix->Color(0, 255, 0)); // Green
  matrix->setRotation(0);

  for (uint16_t i = 0; i < matrix->width() * matrix->height(); i++) {
    matrix->fillScreen(0);

    // Draw red pixel at current index
    uint8_t x = i % matrix->width();
    uint8_t y = i / matrix->width();
    matrix->drawPixel(x, y, matrix->Color(255, 0, 0));

    // Print pixel index in top-left
    matrix->setCursor(0, 0);
    matrix->print(i);

    matrix->show();
    delay(50);
  }

}