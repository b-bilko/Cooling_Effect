# Cooling Effect

An ESP32 sketch that fades a WS2812/NeoPixel strip along FastLED's
`HeatColors_p` palette, from glowing orange-red down to black, like embers
cooling off.

## Hardware

| Part | Notes |
| --- | --- |
| ESP32 dev board | Any classic ESP32 (e.g. ESP32-DevKitC / WROOM-32) |
| WS2812B / NeoPixel strip | 24 LEDs by default (`NUM_LEDS`) |
| Momentary push button | Normally open |
| 5 V supply | Size it for the strip: ~60 mA per LED at full white |

## Wiring

| ESP32 | Connects to |
| --- | --- |
| GPIO 18 | Strip DIN (a 300–500 Ω series resistor is recommended) |
| GPIO 33 | Button, other leg to GND (internal pull-up is used) |
| GND | Strip GND **and** supply GND (common ground) |
| 5V / VIN | Strip 5 V (or power both from the external 5 V supply) |

The ESP32 drives 3.3 V data. Most WS2812B strips accept that, but if the first
LED flickers, add a 74AHCT125 level shifter on the data line.

## Building

### Arduino IDE
1. Add the ESP32 board package
   (`https://espressif.github.io/arduino-esp32/package_esp32_index.json`)
   and install **esp32 by Espressif, 3.3.12**.
2. Install the **FastLED 3.10.5** library.
3. Select board **ESP32 Dev Module**, open `Cooling_Effect.ino`, upload.

### arduino-cli
```sh
arduino-cli core install esp32:esp32@3.3.12 \
  --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install FastLED@3.10.5
arduino-cli compile -b esp32:esp32:esp32 .
arduino-cli upload  -b esp32:esp32:esp32 -p /dev/ttyUSB0 .
```

The repository folder must be named `Cooling_Effect` (Arduino requires the
folder to match the `.ino` file name).

## Continuous integration

`.github/workflows/compile.yml` compiles the sketch on every push and pull
request with the same pinned core and library versions listed above.
