#pragma once

#include <Arduino.h>

// User-adjustable settings, edited from the web portal and kept in flash.
struct Settings {
  uint16_t fadeSeconds;        // length of the cool-down
  uint8_t  startHeat;          // HeatColors_p index the fade starts from
  uint8_t  brightness;         // global FastLED brightness
  uint16_t maxMilliamps;       // strip power budget at 5 V
  uint16_t sleepDelaySeconds;  // stay awake this long after the fade ends
  String   apPassword;         // password for the device's own access point
  String   wifiSsid;           // optional home network to also join
  String   wifiPassword;
};

// Valid ranges, shared by load, save and the portal form
#define FADE_SECONDS_MIN        10
#define FADE_SECONDS_MAX     14400
#define START_HEAT_MIN           1
#define START_HEAT_MAX         255
#define BRIGHTNESS_MIN           1
#define BRIGHTNESS_MAX         255
#define MAX_MILLIAMPS_MIN      100
#define MAX_MILLIAMPS_MAX     5000
#define SLEEP_DELAY_MIN          0
#define SLEEP_DELAY_MAX       3600
#define AP_PASSWORD_MIN_LEN      8
#define WIFI_PASSWORD_MAX_LEN   63
#define WIFI_SSID_MAX_LEN       32

void loadSettings(Settings &s);
void saveSettings(const Settings &s);
void clampSettings(Settings &s);
