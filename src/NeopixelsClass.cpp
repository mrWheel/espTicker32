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
  scrollDelay = 50;
  brightness = 50;
  x = 0;
  pass = 0;
  textComplete = false;
  lastUpdateTime = 0;
}

// Destructor implementation
NeopixelsClass::~NeopixelsClass()
{
  if (debug) debug->println("NeopixelsClass: Destructor called");
  
  if (matrix != nullptr)
  {
    if (debug) debug->println("NeopixelsClass: Deleting matrix object");
    delete matrix;
    matrix = nullptr;
  }
}

// Set the pixel type flags
void NeopixelsClass::setPixelType(neoPixelType pixelType)
{
  if (debug)
  {
    debug->print("NeopixelsClass: Setting pixel type to 0x");
    debug->println((uint32_t)pixelType, HEX);
  }
  
  this->pixelType = pixelType;
}

// Set the matrix dimensions
void NeopixelsClass::setMatrixSize(int width, int height)
{
  if (debug)
  {
    debug->print("NeopixelsClass: Setting matrix size to ");
    debug->print(width);
    debug->print("x");
    debug->println(height);
  }
  
  this->width = width;
  this->height = height;
}

// Set the number of pixels per character
void NeopixelsClass::setPixelsPerChar(int pixels)
{
  if (debug)
  {
    debug->print("NeopixelsClass: Setting pixels per character to ");
    debug->println(pixels);
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
    debug->println("NeopixelsClass: Initializing NeoPixel matrix...");
    debug->print("  neopixelPin: "); debug->println(neopixelPin);
    debug->print("  Width: "); debug->println(width);
    debug->print("  Height: "); debug->println(height);
    debug->print("  Matrix Type: 0x"); debug->println(matrixType, HEX);
    debug->print("  Matrix Layout: 0x"); debug->println(matrixLayout, HEX);
    debug->print("  Matrix Direction: 0x"); debug->println(matrixDirection, HEX);
    debug->print("  Matrix Sequence: 0x"); debug->println(matrixSequence, HEX);
    debug->print("  Pixel Type: 0x"); debug->println((uint32_t)pixelType, HEX);
  }
  
  if (matrix != nullptr)
  {
    if (debug) debug->println("NeopixelsClass: Deleting existing matrix object");
    delete matrix;
    matrix = nullptr;
  }
  
  if (debug) debug->println("NeopixelsClass: Creating new matrix object");
  
  // Combine matrix configuration parameters
  uint8_t matrixConfig = matrixType + matrixDirection + matrixLayout + matrixSequence;
  
  if (debug)
  {
    debug->print("NeopixelsClass: Combined matrix config: 0x");
    debug->println(matrixConfig, HEX);
  }
  
  // Create the matrix object with the specified parameters
  matrix = new Adafruit_NeoMatrix(
    width, height, neopixelPin,
    matrixConfig,
    pixelType);
  
  if (matrix == nullptr)
  {
    if (debug) debug->println("NeopixelsClass: ERROR - Failed to create matrix object!");
    return;
  }
  
  if (debug) debug->println("NeopixelsClass: Calling matrix->begin()");
  matrix->begin();
  
  if (debug) debug->println("NeopixelsClass: Setting matrix parameters");
  matrix->setTextWrap(false);
  matrix->setBrightness(brightness);
  matrix->setTextColor(matrix->Color(red, green, blue));
  
  x = matrix->width(); // Initialize x position
  if (debug)
  {
    debug->print("NeopixelsClass: Initial x position set to: ");
    debug->println(x);
    debug->println("NeopixelsClass: Initialization complete");
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
    debug->println("NeopixelsClass: Setting up matrix configuration");
    debug->print("  Matrix Type: 0x"); debug->println(matrixType, HEX);
    debug->print("  Matrix Layout: 0x"); debug->println(matrixLayout, HEX);
    debug->print("  Matrix Direction: 0x"); debug->println(matrixDirection, HEX);
    debug->print("  Matrix Sequence: 0x"); debug->println(matrixSequence, HEX);
  }
  
  this->matrixType = matrixType;
  this->matrixLayout = matrixLayout;
  this->matrixDirection = matrixDirection;
  this->matrixSequence = matrixSequence;
  
  if (debug) debug->println("NeopixelsClass: Matrix configuration set");
}

// Set the RGB color for the text
void NeopixelsClass::setColor(int r, int g, int b)
{
  if (debug)
  {
    debug->println("NeopixelsClass: Setting color");
    debug->print("  Red: "); debug->println(r);
    debug->print("  Green: "); debug->println(g);
    debug->print("  Blue: "); debug->println(b);
  }
  
  this->red = r;
  this->green = g;
  this->blue = b;
  
  if (matrix != nullptr)
  {
    if (debug) debug->println("NeopixelsClass: Applying color to matrix");
    matrix->setTextColor(matrix->Color(red, green, blue));
  }
  else if (debug)
  {
    debug->println("NeopixelsClass: Warning - matrix is null, color will be applied when matrix is initialized");
  }
}

// Set the brightness level
void NeopixelsClass::setIntensity(int brightness)
{
  if (brightness < 0) brightness = 0;
  if (brightness > 255) brightness = 255;
  
  if (debug)
  {
    debug->print("NeopixelsClass: Setting intensity to ");
    debug->println(brightness);
  }
  
  this->brightness = brightness;
  
  if (matrix != nullptr)
  {
    if (debug) debug->println("NeopixelsClass: Applying brightness to matrix");
    matrix->setBrightness(brightness);
    matrix->show(); // Update the display with new brightness
  }
  else if (debug)
  {
    debug->println("NeopixelsClass: Warning - matrix is null, brightness will be applied when matrix is initialized");
  }
}

// Set the scrolling speed
void NeopixelsClass::setScrollSpeed(int speed)
{
  if (debug)
  {
    debug->print("NeopixelsClass: Setting speed to ");
    debug->println(speed);
  }
  
  // Convert speed (0-100) to delay value
  // Higher speed means lower delay
  if (speed > 0 && speed <= 100)
  {
    this->scrollDelay = (100 - speed) * 2.5;
    
    if (debug)
    {
      debug->print("NeopixelsClass: Calculated scroll delay: ");
      debug->println(scrollDelay);
    }
  }
  else if (debug)
  {
    debug->println("NeopixelsClass: Warning - speed out of range (1-100), not changing");
  }
}

// Set the text to be displayed
void NeopixelsClass::sendNextText(const std::string& text)
{
  if (debug)
  {
    debug->print("NeopixelsClass: Setting text to display: '");
    debug->print(text.c_str());
    debug->println("'");
  }
  
  this->text = text;
  
  if (matrix != nullptr)
  {
    this->x = matrix->width(); // Reset position
    if (debug)
    {
      debug->print("NeopixelsClass: Reset x position to ");
      debug->println(x);
    }
  }
  else if (debug)
  {
    debug->println("NeopixelsClass: Warning - matrix is null, x position will be reset when matrix is initialized");
  }
  
  this->pass = 0;
  this->textComplete = false;
  
  if (debug) debug->println("NeopixelsClass: Text set successfully");
}

// Clear the display
void NeopixelsClass::tickerClear()
{
  if (matrix == nullptr)
  {
    if (debug) debug->println("NeopixelsClass: tickerClear - matrix is null, returning");
    return;
  }
  
  matrix->fillScreen(0);
  matrix->show();
  
  if (debug) debug->println("NeopixelsClass: Display cleared");
}

// Update the display
void NeopixelsClass::show()
{
  if (matrix == nullptr)
  {
    if (debug) debug->println("NeopixelsClass: show - matrix is null, returning");
    return;
  }
  
  matrix->show();
}

// Perform one step of the animation, return true when complete
bool NeopixelsClass::animateNeopixels()
{
  if (matrix == nullptr)
  {
    if (debug) debug->println("NeopixelsClass: animateNeopixels - matrix is null, returning");
    return true;
  }
  
  // Calculate the maximum displacement based on text length and pixel width
  int maxDisplacement = text.length() * pixelPerChar + matrix->width();
  
  if (debug && x % 10 == 0)  // Only log every 10 steps to avoid flooding
  {
    debug->print("NeopixelsClass: Animating - x=");
    debug->print(x);
    debug->print(", pass=");
    debug->print(pass);
    debug->print(", maxDisplacement=");
    debug->println(maxDisplacement);
  }
  
  // Clear the display and set the cursor position
  matrix->fillScreen(0);
  matrix->setCursor(x, 0);
  
  // Print the text
  matrix->print(text.c_str());
  
  // Move the text position for the next frame
  if (--x < -maxDisplacement)
  {
    if (debug) debug->println("NeopixelsClass: Reached end of text, resetting position");
    
    x = matrix->width();
    
    if (++pass >= 1)
    {
      if (debug) debug->println("NeopixelsClass: Animation complete");
      
      pass = 0;
      matrix->setTextColor(matrix->Color(red, green, blue));
      textComplete = true;
      if (textComplete && onFinished)
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
  if (debug) debug->println("NeopixelsClass: Starting blocking animation");
  
  textComplete = false;
  int animationStep = 0;
  
  while (!animateNeopixels())
  {
    if (debug && animationStep % 20 == 0)  // Log every 20 steps
    {
      debug->print("NeopixelsClass: Blocking animation step ");
      debug->println(animationStep);
    }
    
    delay(scrollDelay);
    animationStep++;
  }
  
  if (debug) debug->println("NeopixelsClass: Blocking animation complete");
}

void NeopixelsClass::setRandomEffects(const std::vector<uint8_t> &effects)
{
  // This can be a no-op or log that it was called
  if (debug) debug->println("NeopixelsClass: setRandomEffects called (no effect)");
}

void NeopixelsClass::setCallback(std::function<void(const std::string&)> callback)
{
  if (debug) debug->println("NeopixelsClass: Setting callback function");
  this->onFinished = callback;
}

void NeopixelsClass::setDisplayConfig(const DisplayConfig &config)
{
  if (debug)
  {
    debug->println("NeopixelsClass: Setting display config");
    debug->print("  Speed: "); debug->println(config.speed);
    debug->print("  Pause Time: "); debug->println(config.pauseTime);
  }
  
  // Map the DisplayConfig parameters to NeopixelsClass parameters
  this->setScrollSpeed(config.speed);

}

// Main loop function that calls animateNeopixels with timing control
void NeopixelsClass::loop()
{
  if (matrix == nullptr)
  {
    if (debug) debug->println("NeopixelsClass: loop - matrix is null, returning");
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