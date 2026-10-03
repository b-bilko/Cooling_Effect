# Cooling Effect

An ESP32 sketch that fades a WS2812/NeoPixel strip along FastLED's
`HeatColors_p` palette, from glowing orange-red down to black, like embers
cooling off. It's configured from a phone through a built-in web portal and
powers itself down when the fade is over.

## How it behaves

1. **Power-up or button press**: the fade starts from the top (3 minutes by
   default). Pressing the button during a fade restarts it.
2. **Fade ends**: the strip goes black. After the *stay awake* delay
   (60 s by default) the ESP32 turns the LEDs and Wi-Fi off and enters deep
   sleep.
3. **Unless the portal is in use**: while a phone or laptop is joined to the
   device's Wi-Fi, or the portal page is open (it polls every 2 s), the device
   stays awake. The delay starts counting again once everyone disconnects.
4. **Asleep**: press the button to wake. Waking starts a fresh fade.

## Web portal

The device always runs its own access point:

| | |
| --- | --- |
| Network | `CoolingEffect-XXXX` (unique per board) |
| Password | `coolingeffect` (change it in the portal) |
| Address | <http://192.168.4.1> (most phones pop it up automatically) |

You can also have it join your home network from the portal's Wi-Fi section.
It then also answers at <http://cooling-effect.local> or its IP on that
network. The access point stays up as a fallback.

From the portal you can:

- see the current color, progress and time until sleep
- restart the fade, or put the device to sleep immediately
- set the fade length, starting color, brightness, power limit and the stay
  awake delay (applied immediately, saved to flash)
- set the home network and the access point password (saved, then the device
  reboots)

Anyone who can join the access point can change settings, so change the
default password.

## Hardware

| Part | Notes |
| --- | --- |
| ESP32 dev board | Any classic ESP32 (e.g. ESP32-DevKitC / WROOM-32) |
| WS2812B / NeoPixel strip | 24 LEDs by default (`NUM_LEDS`) |
| Momentary push button | Normally open |
| 5 V supply | Size it for the strip: ~60 mA per LED at full white |
| Logic-level MOSFET (optional) | Cuts strip power during sleep, see below |

## Wiring

| ESP32 | Connects to |
| --- | --- |
| GPIO 18 | Strip DIN (a 300–500 Ω series resistor is recommended) |
| GPIO 33 | Button, other leg to GND (internal pull-up is used) |
| GND | Strip GND **and** supply GND (common ground) |
| 5V / VIN | Strip 5 V (or power both from the external 5 V supply) |

The ESP32 drives 3.3 V data. Most WS2812B strips accept that, but if the first
LED flickers, add a 74AHCT125 level shifter on the data line.

The button has to be on an RTC-capable GPIO (0, 2, 4, 12–15, 25–27, 32–39)
to wake the chip from deep sleep.

### Cutting strip power while asleep

Even when dark, each WS2812B draws about 1 mA, so 24 LEDs use ~24 mA while the
ESP32 itself sleeps at a few µA. For battery use, switch the strip's ground
(or 5 V) through a MOSFET and set `LED_POWER_PIN` in `Cooling_Effect.ino` to
the GPIO driving its gate. It's driven HIGH while awake and held LOW in sleep.

## Code layout

| File | Purpose |
| --- | --- |
| `Cooling_Effect.ino` | Fade, button, sleep/wake |
| `settings.h/.cpp` | Settings, defaults, valid ranges, flash storage |
| `portal.h/.cpp` | Access point, captive DNS, mDNS, HTTP API |
| `portal_page.h` | The portal's HTML/CSS/JS |
| `app.h` | Hooks the sketch exposes to the portal |

HTTP API (form-encoded POST bodies):

| Endpoint | |
| --- | --- |
| `GET /api/state` | Progress, settings and network info as JSON |
| `POST /api/settings` | `fadeSeconds`, `startHeat`, `brightness`, `maxMilliamps`, `sleepDelaySeconds` |
| `POST /api/restart` | Restart the fade |
| `POST /api/sleep` | Sleep now |
| `POST /api/wifi` | `wifiSsid`, `wifiPassword`, `apPassword`, then reboot |

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
folder to match the `.ino` file name). The build uses about 85% of the
default 1.2 MB app partition.

## Continuous integration

`.github/workflows/compile.yml` compiles the sketch on every push and pull
request with the same pinned core and library versions listed above.
