#pragma once

// Hooks the sketch exposes to the web portal.

#include "settings.h"

extern Settings settings;

struct FadeStatus {
  uint8_t  heat;            // current HeatColors_p index
  uint8_t  percent;         // 0..100 progress through the fade
  bool     cooling;         // false once the strip has reached black
  uint32_t secondsLeft;     // until the fade finishes
  int32_t  sleepInSeconds;  // until deep sleep; -1 while held awake
};

FadeStatus fadeStatus();
void restartFade();
void applySettings();   // push LED-related settings to FastLED
void requestSleep();    // enter deep sleep on the next loop
