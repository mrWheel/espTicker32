#include "ReadLdrClass.h"

void ReadLdrClass::setup(SettingsClass* settings, Stream* debugStream) {
  this->settings = settings;
  this->debug = debugStream;

#ifdef USE_PAROLA
  if (settings->devLDRpin == 0 || settings->devLDRpin == settings->parolaPinCLK ||
      settings->devLDRpin == settings->parolaPinCS || settings->devLDRpin == settings->parolaPinDIN) {
    if (debug) debug->println("LDR setup: Pin conflict with PAROLA, disabling LDR");
    settings->devLDRpin = -1;
    return;
  }
#endif

#ifdef USE_NEOPIXELS
  if (settings->devLDRpin == 0 || settings->devLDRpin == settings->neopixDataPin) {
    if (debug) debug->println("LDR setup: Pin conflict with NEOPIXELS, disabling LDR");
    settings->devLDRpin = -1;
    return;
  }
#endif

  if (settings->devLDRpin >= 34 && settings->devLDRpin <= 39 &&
      settings->devLDRpin != 37 && settings->devLDRpin != 38) {
    if (debug) debug->printf("LDR setup: Using pin [%d]\n", settings->devLDRpin);
    analogSetAttenuation(ADC_11db);  // Best for full 0–3.3V range
  } else {
    if (debug) debug->println("LDR setup: Invalid or disabled pin");
    settings->devLDRpin = -1;
  }
}

void ReadLdrClass::loop(bool forceRead) {
  static uint32_t lastLDRRead = 0;

  if (!settings || settings->devLDRpin == -1) return;

  if (!forceRead && millis() - lastLDRRead < 10000) return;

  lastLDRRead = millis();

  int rawValue = analogRead(settings->devLDRpin);
  const int ADC_MIN = 0;
  const int ADC_MAX = 4095;

  int mappedValue = map(rawValue, ADC_MIN, ADC_MAX,
                        settings->devLDRMaxWaarde,
                        settings->devLDRMinWaarde);

  mappedValue = constrain(mappedValue, 0, 100);
  settings->devMaxIntensiteitLeds = mappedValue;

  if (debug && settings->doDebug) {
    debug->printf("LDR read: raw=%d, mapped=%d\n", rawValue, mappedValue);
  }
}