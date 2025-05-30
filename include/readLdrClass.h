#ifndef READ_LDR_CLASS_H
#define READ_LDR_CLASS_H

#include <Arduino.h>
#include "SettingsClass.h"

class ReadLdrClass {
public:
  ReadLdrClass() = default;
  void setup(SettingsClass* settings, Stream* debugStream = nullptr);
  void loop(bool forceRead = false);

private:
  SettingsClass* settings = nullptr;
  Stream* debug = nullptr;
};

#endif // READ_LDR_CLASS_H