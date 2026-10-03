#include "FastLED.h"
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "app.h"
#include "portal.h"

// How many leds in your strip?
#define NUM_LEDS 24

// ESP32 GPIOs (see README for wiring). The button must be an RTC-capable
// GPIO (0, 2, 4, 12-15, 25-27, 32-39) so it can wake the chip.
#define DATA_PIN   18
#define BUTTON_PIN 33

// Optional GPIO driving a MOSFET that switches the strip's 5 V supply, so the
// strip draws nothing while asleep. -1 if the strip is always powered.
#define LED_POWER_PIN -1

#define LED_VOLTS 5

// The button must read the same for this long before a change counts
#define DEBOUNCE_MS 30

// Define the array of leds
CRGB leds[NUM_LEDS];

Settings settings;

// millis() when the current cool-down began
uint32_t fadeStartMs;

// millis() of the last moment something kept us awake: the fade running,
// or someone using the portal
uint32_t lastBusyMs;

bool sleepRequested = false;

void setup() {
  // Release pins held through deep sleep
  gpio_hold_dis((gpio_num_t)DATA_PIN);
  rtc_gpio_deinit((gpio_num_t)BUTTON_PIN);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (LED_POWER_PIN >= 0) {
    gpio_hold_dis((gpio_num_t)LED_POWER_PIN);
    pinMode(LED_POWER_PIN, OUTPUT);
    digitalWrite(LED_POWER_PIN, HIGH);
  }

  loadSettings(settings);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  applySettings();

  portalBegin();

  // Power-up and a button wake both start a fresh fade
  restartFade();
}

void loop() {
  portalLoop();

  if (buttonPressed()) {
    restartFade();
  }

  coolDown();
  FastLED.show();

  uint32_t now = millis();
  if (fadeStatus().cooling || portalActive()) {
    lastBusyMs = now;
  }
  if (sleepRequested ||
      now - lastBusyMs >= settings.sleepDelaySeconds * 1000UL) {
    goToSleep();
  }
}

void applySettings() {
  FastLED.setBrightness(settings.brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, settings.maxMilliamps);
}

void restartFade() {
  fadeStartMs = millis();
  lastBusyMs = fadeStartMs;
}

void requestSleep() {
  sleepRequested = true;
}

FadeStatus fadeStatus() {
  uint32_t durationMs = settings.fadeSeconds * 1000UL;
  uint32_t elapsed = millis() - fadeStartMs;

  FadeStatus st;
  st.cooling = elapsed < durationMs;
  if (st.cooling) {
    // Walk the palette index linearly from startHeat down to 0 (black)
    st.heat = settings.startHeat - (uint64_t)settings.startHeat * elapsed / durationMs;
    st.percent = (uint64_t)100 * elapsed / durationMs;
    st.secondsLeft = (durationMs - elapsed + 999) / 1000;
  } else {
    st.heat = 0;
    st.percent = 100;
    st.secondsLeft = 0;
  }

  if (st.cooling || portalActive()) {
    st.sleepInSeconds = -1;
  } else {
    uint32_t sleepAtMs = lastBusyMs + settings.sleepDelaySeconds * 1000UL;
    int32_t remaining = (int32_t)(sleepAtMs - millis());
    st.sleepInSeconds = remaining > 0 ? (remaining + 999) / 1000 : 0;
  }
  return st;
}

// Returns true once per press, after the button has been stably LOW for
// DEBOUNCE_MS.
bool buttonPressed() {
  static bool stableState = HIGH;
  static bool lastReading = HIGH;
  static uint32_t lastChangeMs = 0;

  bool reading = digitalRead(BUTTON_PIN);
  if (reading != lastReading) {
    lastReading = reading;
    lastChangeMs = millis();
  }

  if (reading != stableState && millis() - lastChangeMs >= DEBOUNCE_MS) {
    stableState = reading;
    return stableState == LOW;
  }
  return false;
}

void coolDown() {
  // HeatColors_p is a gradient palette built in to FastLED
  // that fades from black to red, orange, yellow, white
  CRGB color = ColorFromPalette(HeatColors_p, fadeStatus().heat);

  // fill the entire strip with the current color
  fill_solid(leds, NUM_LEDS, color);
}

// Lights off, Wi-Fi off, deep sleep until the button is pressed. Waking
// reboots the sketch from setup().
void goToSleep() {
  // Let a pending HTTP reply (e.g. from "Sleep now") go out
  uint32_t flushUntil = millis() + 200;
  while ((int32_t)(millis() - flushUntil) < 0) {
    portalLoop();
    delay(10);
  }

  FastLED.clear(true);
  portalEnd();

  // Keep the data line low so the strip doesn't latch noise while asleep
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  gpio_hold_en((gpio_num_t)DATA_PIN);

  if (LED_POWER_PIN >= 0) {
    digitalWrite(LED_POWER_PIN, LOW);
    gpio_hold_en((gpio_num_t)LED_POWER_PIN);
  }
  gpio_deep_sleep_hold_en();

  // Don't wake straight back up if the button is still held
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }

  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  rtc_gpio_pullup_en((gpio_num_t)BUTTON_PIN);
  rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);
  esp_deep_sleep_start();
}
