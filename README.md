# Motor control for ESP32

Drive a DC motor driver from an ESP32 over WiFi: speed (PWM), direction and brake, plus an automatic program
that switches between a low and a high speed. The ESP32 serves its own web page, so a phone or PC on the same
network is all you need. Made by [Rune Johansen](https://r-j.no).

*På norsk: [LES MEG.md](LES%20MEG.md).*

## What it does

* **Manual:** a slider sets the speed from 0 to 255 (8-bit PWM at 40 kHz). Buttons toggle direction and brake.
* **Automatic program:** runs at a low speed for a set time, then at a high speed for a set time, and repeats.
  Both speeds (0–255) and both times (50–3000 ms) are set with sliders while it runs.
  Moving the manual slider stops the automatic program.
* The web page talks to the ESP32 over a WebSocket, so the motor reacts straight away.

## Hardware

Made for an **ESP32-S3** and a motor driver with separate inputs for PWM, direction and brake.

| Signal | GPIO | Use |
|---|---|---|
| PWM | 7 | Speed, 40 kHz, 0–255 |
| Direction | 8 | Toggled from the web page, starts LOW |
| Brake | 9 | Toggled from the web page, starts LOW |

On a classic ESP32, GPIO 6–11 are used by the flash memory. Change the pin numbers at the top of the sketch.
Check in the driver's datasheet whether the brake and direction inputs are active high or low.

## Getting started

1. Install the ESP32 board package in the Arduino IDE, and the library **WebSockets** by Markus Sattler.
2. Open `Motor_PWM_med_retning_og_brems/Motor_PWM_med_retning_og_brems.ino`.
3. Enter your own WiFi name and password at the top (`DITT_WIFI_NAVN` and `DITT_WIFI_PASSORD`).
4. Upload, open the serial monitor at 115200 baud and note the IP address.
5. Open that IP address in a browser on the same network.

## Commands (WebSocket, port 81)

| Command | Effect |
|---|---|
| `PWM:<0-255>` | Sets the speed and stops the automatic program |
| `DIR:TOGGLE` | Toggles direction |
| `BRAKE:TOGGLE` | Toggles the brake |
| `AUTO:TOGGLE` | Starts or stops the automatic program |
| `AUTO:LOWSPD:<0-255>` / `AUTO:HGHSPD:<0-255>` | Low and high speed for the automatic program |
| `AUTO:LOWT:<50-3000>` / `AUTO:HGHT:<50-3000>` | Time at low and high speed, in ms |

## Related

[NexusDrive](https://github.com/runej42-stack/nexus-drive) is a phone app for a similar motor control over
Bluetooth instead of WiFi. It uses its own Bluetooth protocol and does not talk to this sketch.
