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
  brightness = 30;
  scrollPosition = 0;
  currentMessage = "";
  nextMessage = "";
  currentMessageWidth = 0;

  pass = 0;
  textComplete = false;
  lastUpdateTime = 0;
  matrixInitialized = false;
  configInitialized = false;
  previousText = "";
  readyForNextMessage = false;
  lastStopPosition = 0;
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
    debugPrint("  Pixel Type        : 0x%02x", (uint32_t)pixelType);
  }
  else
  {
    Serial.println("NeopixelsClass: Initializing NeoPixel matrix...");
    Serial.printf("  neopixelPin       : %d\n", neopixelPin);
    Serial.printf("  Width             : %d\n", width);
    Serial.printf("  Height            : %d\n", height);
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
    matrix->setFont();
    matrix->setTextSize(1);
    matrix->setTextWrap(false);
    matrix->setBrightness(brightness);
    matrix->setTextColor(matrix->Color(red, green, blue));

    matrix->setBrightness(40);
    matrix->setTextColor(matrix->Color(0, 255, 0)); // Green
    matrix->setRotation(0);
  
    
    scrollPosition = matrix->width(); // Initialize x position
    if (debug)
    {
      debugPrint("NeopixelsClass: Initial x position set to: %d", scrollPosition);
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
  else
  {
    Serial.printf("NeopixelsClass: Setting up matrix configuration\n");
    Serial.printf("  Matrix Type      : 0x%02x\n", matrixType);
    Serial.printf("  Matrix Layout    : 0x%02x\n", matrixLayout);
    Serial.printf("  Matrix Direction : 0x%02x\n", matrixDirection);
    Serial.printf("  Matrix Sequence  : 0x%02x\n", matrixSequence);
  }
  
  this->matrixType = matrixType;
  this->matrixLayout = matrixLayout;
  this->matrixDirection = matrixDirection;
  this->matrixSequence = matrixSequence;
  
  configInitialized = true;
  
  if (debug) debugPrint("NeopixelsClass: Matrix configuration set");

} // setup()

// Set the RGB color for the text
void NeopixelsClass::setColor(int r, int g, int b)
{
  if (r < 0) r = 0; if (r > 255) r = 255;
  if (g < 0) g = 0; if (g > 255) g = 255;
  if (b < 0) b = 0; if (b > 255) b = 255;
  
  if (doDebug && debug)
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
void NeopixelsClass::setIntensity(int newBrightness)
{
  if (newBrightness < 0)   newBrightness =  10;
  if (newBrightness > 100) newBrightness = 100;

  this->brightness = scaleValue(newBrightness, 0, 100, 0, 100);
  
  if (debug)
  {
    debugPrint("NeopixelsClass: Setting intensity to (%d)[%d]", newBrightness, this->brightness);
  }
  
  
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
} // setIntensity()


// Set the scrolling speed
void NeopixelsClass::setScrollSpeed(int newSpeed)
{
  int tmpScrollDelay = 0;

  tmpScrollDelay = scaleValue(newSpeed, 0, 100, 100, 0);
  if (debug && doDebug)
  {
    debugPrint("NeopixelsClass: Setting scroll speed to (%d)[%d]", newSpeed, tmpScrollDelay);
  }
  else if (debug)
  {
    debugPrint("NeopixelsClass: Setting speed to (%d)[%d]", newSpeed, tmpScrollDelay);
  }
  
  // Higher speed means lower delay
  if (tmpScrollDelay >= 0 && tmpScrollDelay <= 100)
  {
    //this->scrollDelay = (100 - this->speed) * 1.0; // Changed from 2.5 to 1.0
    this->scrollDelay = tmpScrollDelay; 
    
    if (debug)
    {
      debugPrint("NeopixelsClass: Calculated scroll delay: [%dms]", scrollDelay);
    }
  }
  else if (debug)
  {
    debugPrint("NeopixelsClass: Warning - speed out of range (1-100), not changing");
  }

} // setScrollSpeed()


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
  
  // If we're ready for the next message (previous message has completed but is still on screen)
  if (readyForNextMessage && !currentMessage.empty())
  {
    // Remove the previous message from the concatenated text
    // Find the separator and remove everything before it
    size_t separatorPos = this->text.find("*");
    if (separatorPos != std::string::npos)
    {
      // Calculate how much text we're removing
      std::string removedText = this->text.substr(0, separatorPos + 1); // Include separator
      int removedWidth = removedText.length() * pixelPerChar;
      
      // Remove the old message from the concatenated text
      this->text = this->text.substr(separatorPos + 1);
      
      // Adjust scroll position to account for removed text
      scrollPosition += removedWidth;
      
      if (debug)
      {
        debugPrint("NeopixelsClass: Removed text: '%s' (width: %d)", removedText.c_str(), removedWidth);
        debugPrint("NeopixelsClass: Adjusted scroll position to: %d", scrollPosition);
      }
    }
    
    // Now append the new message
    this->text = this->text + "*" + text;
    currentMessage = text;
    currentMessageWidth = text.length() * pixelPerChar;
    
    if (debug)
    {
      debugPrint("NeopixelsClass: Added new text, combined text is: '%s'", this->text.c_str());
      debugPrint("NeopixelsClass: Current scroll position: %d", scrollPosition);
    }
    
    // Reset the flag
    readyForNextMessage = false;
  }
  // If there's already text being displayed and animation is in progress
  else if (!previousText.empty() && !textComplete)
  {
    // Append the new text to the existing text
    this->text = this->text + "*" + text;
    nextMessage = text;
    
    if (debug)
    {
      debugPrint("NeopixelsClass: Appending to existing text, new text is: '%s'", this->text.c_str());
    }
  }
  else
  {
    // First message or previous message completed, set the text directly
    this->text = text;
    currentMessage = text;
    currentMessageWidth = text.length() * pixelPerChar;
    
    try
    {
      this->scrollPosition = matrix->width(); // Reset position only for new text
      if (debug)
      {
        debugPrint("NeopixelsClass: Reset scroll position to %d", scrollPosition);
      }
    }
    catch (...)
    {
      if (debug) debugPrint("NeopixelsClass: Exception while resetting position");
      this->scrollPosition = width; // Use the stored width if matrix access fails
    }
  }
  
  // Store this text as the previous text for next time
  previousText = text;
  
  this->pass = 0;
  this->textComplete = false;
  
  if (debug) debugPrint("NeopixelsClass: Text set successfully");

} // sendNextText()


// Clear the display (ticker)
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
    
    // Reset continuous scrolling state
    readyForNextMessage = false;
    previousText = "";
    textComplete = false;
    currentMessage = "";
    nextMessage = "";
    currentMessageWidth = 0;
    
    // Reset position
    try
    {
      this->scrollPosition = matrix->width();
    }
    catch (...)
    {
      this->scrollPosition = width;
    }
    
    if (debug) debugPrint("NeopixelsClass: Display cleared and scrolling state reset");
  }
  catch (...)
  {
    if (debug) debugPrint("NeopixelsClass: Exception while clearing display");
  }

} // tickerClear()

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

} //  show()

  /**
   * @brief Animate the display by scrolling the text horizontally.
   * 
   * @param triggerCallback If true, the callback function will be called when the animation is complete.
   * @return true if the animation is complete, false otherwise.
   */
bool NeopixelsClass::animateNeopixels(bool triggerCallback)
{
  if (!initialized || matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass::animateNeopixels - matrix is null, returning");
    return true;
  }
 
  textComplete = false;

  // Calculate the text width in pixels
  int textWidth = text.length() * pixelPerChar;
  
  // Calculate the stopping position:
  // We want to stop when the last character is at the right edge of the display
  // This means the position should be: matrixWidth - textWidth
  int matrixWidth = matrix->width();
  int stopPosition = matrixWidth - textWidth;
  
  // If the text is shorter than the display width, don't scroll past the left edge
  if (stopPosition > 0)
  {
    stopPosition = 0;
  }
  
  try
  {
    // Clear the display before drawing new content
    matrix->fillScreen(0);
    
    // Set the cursor position and print the text
    matrix->setCursor(scrollPosition, 0);
    matrix->print(text.c_str());
    
    // Update the display
    matrix->show();
    
    // Move the text position for the next frame
    scrollPosition--;
    
    // Check if we've reached the stopping position
    if (scrollPosition <= stopPosition)
    {
      if (debug) debugPrint("NeopixelsClass::animateNeopixels Reached end of text");
      
      // Text has completed its animation
      textComplete = true;
      
      // Set the flag to indicate we're ready for the next message
      readyForNextMessage = true;
      
      // Store the last stop position
      lastStopPosition = stopPosition;
      
      if (debug) debugPrint("NeopixelsClass::animateNeopixels complete, triggering callback");
      else Serial.println("NeopixelsClass::animateNeopixels complete, triggering callback");
      
      // Only call the callback if triggerCallback is true
      if (textComplete && onFinished && triggerCallback)
      {
        if (debug) debugPrint("NeopixelsClass::animateNeopixels Triggering callback with text: [%s]", text.c_str());
        onFinished(text);
      }
      
      return true;
    }
    
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
  // Animation delay - faster scrolling
  int animationDelay = 1;
  
  if (!initialized || matrix == nullptr)
  {
    if (debug) debugPrint("NeopixelsClass: animateBlocking - not initialized, returning");
    else   Serial.println("NeopixelsClass: animateBlocking - not initialized, returning");
    return;
  }
  
  if (debug) debugPrint("NeopixelsClass: Starting animationBlocking for text: %s", text.c_str());
  else    Serial.printf("NeopixelsClass[S]: Starting animationBlocking for text: %s\n", text.c_str()); 
  
  int matrixWidth = matrix->width();
  
  // If we're ready for the next message (previous message has completed but is still on screen)
  if (readyForNextMessage && !this->text.empty())
  {
    // Remove the previous message from the concatenated text
    size_t separatorPos = this->text.find(" ");
    if (separatorPos != std::string::npos)
    {
      // Calculate how much text we're removing
      std::string removedText = this->text.substr(0, separatorPos + 1); // Include separator
      int removedWidth = removedText.length() * pixelPerChar;
      
      // Remove the old message from the concatenated text
      this->text = this->text.substr(separatorPos + 1);
      
      // Adjust scroll position to account for removed text
      scrollPosition += removedWidth;
      
      if (debug)
      {
        debugPrint("NeopixelsClass: Removed text: '%s' (width: %d)", removedText.c_str(), removedWidth);
        debugPrint("NeopixelsClass: Adjusted scroll position to: %d", scrollPosition);
      }
    }
    
    // Now append the new message
    this->text = this->text + " " + text.c_str();
    
    if (debug)
    {
      debugPrint("NeopixelsClass: Adding new text at right edge, combined text is: '%s'", this->text.c_str());
      debugPrint("NeopixelsClass: Keeping scroll position at %d", this->scrollPosition);
    }
    
    // Reset the flag
    readyForNextMessage = false;
  }
  // If there's already text being displayed and animation is in progress
  else if (!this->text.empty() && this->scrollPosition <= 0) 
  {
    this->text = this->text + " " + text.c_str();
    if (debug)
    {
      debugPrint("NeopixelsClass: Appending to existing text, new text is: '%s'", this->text.c_str());
    }
  } 
  else 
  {
    this->text = text.c_str();
  }
  
  // Set the starting position
  int textPosition;
  if (this->scrollPosition <= 0) {
    // Continue from current position if text is already scrolling
    textPosition = this->scrollPosition;
  } else {
    // Start from right edge for new text
    textPosition = matrixWidth;
  }
  
  // Calculate the width of the text in pixels
  int textWidth = this->text.length() * pixelPerChar;
  
  // Calculate the stopping position:
  // We want to stop when the last character is at the right edge of the display
  // This means the position should be: matrixWidth - textWidth
  int stopPosition = matrixWidth - textWidth;
  
  // If the text is shorter than the display width, don't scroll past the left edge
  if (stopPosition > 0) {
    stopPosition = 0;
  }
  
  if (debug && doDebug) {
    debugPrint("NeopixelsClass: Animation parameters:");
    debugPrint("  Text: '%s'", this->text.c_str());
    debugPrint("  Text width: %d", textWidth);
    debugPrint("  Matrix width: %d", matrixWidth);
    debugPrint("  Starting position: %d", textPosition);
    debugPrint("  Stop position: %d", stopPosition);
  }
  
  // Now animate until we reach the stopping position
  int animationStep = 0;
  while (textPosition > stopPosition && animationStep < 1000)
  {
    // Clear the display and set the cursor position
    matrix->fillScreen(0);
    matrix->setCursor(textPosition, 0);
    
    // Print the text
    matrix->print(this->text.c_str());
    
    // Update the display
    matrix->show();
    
    // Move the text position for the next frame
    textPosition--;
    this->scrollPosition = textPosition;
    
    delay(animationDelay);
    animationStep++;
    yield();
  }
  
  // Set the flag to indicate we're ready for the next message
  readyForNextMessage = true;
  
  // Store the last stop position
  lastStopPosition = stopPosition;
  
  // Store this text as the previous text for next time
  previousText = text.c_str();
  
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
} // loop()

int16_t NeopixelsClass::scaleValue(int16_t input
                                     , int16_t minInValue, int16_t maxInValue
                                     , int16_t minOutValue, int16_t maxOutValue) 
{
  // Prevent division by zero
  if (maxInValue == minInValue) return minOutValue;

  return (int16_t)(((int32_t)(input - minInValue) * (maxOutValue - minOutValue)) / 
                   (maxInValue - minInValue) + minOutValue);

} // scaleValue()

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

