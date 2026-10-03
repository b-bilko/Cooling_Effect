#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 24

// ESP32 GPIOs (see README for wiring)
#define DATA_PIN   18
#define BUTTON_PIN 33

// Power budget for the strip. FastLED dims the whole frame to stay under it.
#define LED_VOLTS       5
#define MAX_MILLIAMPS 500

// Total cool-down length, in minutes
#define COOL_DOWN_MINUTES 3

// Palette index to start from. In HeatColors_p, 0 is black and 255 is white;
// 90 is a deep orange-red.
#define START_HEAT 90

// The button must read the same for this long before a change counts
#define DEBOUNCE_MS 30

// Define the array of leds
CRGB leds[NUM_LEDS];

// millis() when the current cool-down began
uint32_t fadeStartMs;

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, MAX_MILLIAMPS);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  restartFade();
}

void loop() {
  if (buttonPressed()) {
    restartFade();
  }

  coolDown();

  FastLED.show();
}

void restartFade() {
  fadeStartMs = millis();
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
  static const uint32_t durationMs = COOL_DOWN_MINUTES * 60000UL;

  // Walk the palette index linearly from START_HEAT down to 0 (black)
  uint32_t elapsed = millis() - fadeStartMs;
  uint8_t heatIndex = 0;
  if (elapsed < durationMs) {
    heatIndex = START_HEAT - (uint64_t)START_HEAT * elapsed / durationMs;
  }

  // HeatColors_p is a gradient palette built in to FastLED
  // that fades from black to red, orange, yellow, white
  CRGB color = ColorFromPalette(HeatColors_p, heatIndex);

  // fill the entire strip with the current color
  fill_solid(leds, NUM_LEDS, color);
}
