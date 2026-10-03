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

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, MAX_MILLIAMPS);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // Hold the effect while the button is pressed
  while (digitalRead(BUTTON_PIN) == LOW) { delay(10); }

  coolDown();

  FastLED.show();
}

void coolDown() {
  // One palette step every stepMs walks START_HEAT down to 0 in
  // COOL_DOWN_MINUTES (3 min / 90 steps = 2 s per step).
  static const uint32_t stepMs = (COOL_DOWN_MINUTES * 60000UL) / START_HEAT;

  // current gradient palette color index
  static uint8_t heatIndex = START_HEAT;

  // HeatColors_p is a gradient palette built in to FastLED
  // that fades from black to red, orange, yellow, white
  CRGB color = ColorFromPalette(HeatColors_p, heatIndex);

  // fill the entire strip with the current color
  fill_solid(leds, NUM_LEDS, color);

  // slowly cool down, stopping at 0 (black)
  EVERY_N_MILLISECONDS(stepMs) {
    if (heatIndex > 0) {
      heatIndex--;
    }
  }
}
