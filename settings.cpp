#include "settings.h"

#include <Preferences.h>

// Defaults used until something is saved from the portal
#define DEFAULT_FADE_SECONDS        180
#define DEFAULT_START_HEAT           90  // deep orange-red in HeatColors_p
#define DEFAULT_BRIGHTNESS          255
#define DEFAULT_MAX_MILLIAMPS       500
#define DEFAULT_SLEEP_DELAY_SECONDS  60
#define DEFAULT_AP_PASSWORD "coolingeffect"

static const char *NAMESPACE = "cooling";

void clampSettings(Settings &s) {
  s.fadeSeconds       = constrain(s.fadeSeconds, FADE_SECONDS_MIN, FADE_SECONDS_MAX);
  s.startHeat         = constrain(s.startHeat, START_HEAT_MIN, START_HEAT_MAX);
  s.brightness        = constrain(s.brightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  s.maxMilliamps      = constrain(s.maxMilliamps, MAX_MILLIAMPS_MIN, MAX_MILLIAMPS_MAX);
  s.sleepDelaySeconds = constrain(s.sleepDelaySeconds, SLEEP_DELAY_MIN, SLEEP_DELAY_MAX);

  if (s.apPassword.length() < AP_PASSWORD_MIN_LEN ||
      s.apPassword.length() > WIFI_PASSWORD_MAX_LEN) {
    s.apPassword = DEFAULT_AP_PASSWORD;
  }
  if (s.wifiSsid.length() > WIFI_SSID_MAX_LEN) {
    s.wifiSsid = "";
  }
  if (s.wifiPassword.length() > WIFI_PASSWORD_MAX_LEN) {
    s.wifiPassword = "";
  }
}

void loadSettings(Settings &s) {
  Preferences prefs;
  prefs.begin(NAMESPACE, true);
  s.fadeSeconds       = prefs.getUShort("fadeSec", DEFAULT_FADE_SECONDS);
  s.startHeat         = prefs.getUChar("startHeat", DEFAULT_START_HEAT);
  s.brightness        = prefs.getUChar("bright", DEFAULT_BRIGHTNESS);
  s.maxMilliamps      = prefs.getUShort("maxMa", DEFAULT_MAX_MILLIAMPS);
  s.sleepDelaySeconds = prefs.getUShort("sleepDelay", DEFAULT_SLEEP_DELAY_SECONDS);
  s.apPassword        = prefs.getString("apPass", DEFAULT_AP_PASSWORD);
  s.wifiSsid          = prefs.getString("wifiSsid", "");
  s.wifiPassword      = prefs.getString("wifiPass", "");
  prefs.end();

  clampSettings(s);
}

void saveSettings(const Settings &s) {
  Preferences prefs;
  prefs.begin(NAMESPACE, false);
  prefs.putUShort("fadeSec", s.fadeSeconds);
  prefs.putUChar("startHeat", s.startHeat);
  prefs.putUChar("bright", s.brightness);
  prefs.putUShort("maxMa", s.maxMilliamps);
  prefs.putUShort("sleepDelay", s.sleepDelaySeconds);
  prefs.putString("apPass", s.apPassword);
  prefs.putString("wifiSsid", s.wifiSsid);
  prefs.putString("wifiPass", s.wifiPassword);
  prefs.end();
}
