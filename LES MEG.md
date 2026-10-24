# Motorstyring for ESP32

Styr en motordriver fra en ESP32 over WiFi: fart (PWM), retning og brems, og et automatikkprogram
som veksler mellom lav og høy fart. ESP32-en har sin egen nettside, så du trenger bare en telefon eller PC
på samme nett. Laget av [Rune Johansen](https://r-j.no).

*In English: [README.md](README.md).*

## Hva det gjør

* **Manuelt:** en glidebryter stiller farten fra 0 til 255 (8-bits PWM på 40 kHz). Knapper bytter retning og brems.
* **Automatikk:** kjører med lav fart en viss tid, så med høy fart en viss tid, og gjentar.
  Begge fartene (0–255) og begge tidene (50–3000 ms) stilles med glidebrytere mens programmet går.
  Rører du den manuelle glidebryteren, stopper automatikken.
* Nettsiden snakker med ESP32-en over WebSocket, så motoren reagerer med en gang.

## Maskinvare

Laget for **ESP32-S3** og en motordriver med egne innganger for PWM, retning og brems.

| Signal | GPIO | Bruk |
|---|---|---|
| PWM | 7 | Fart, 40 kHz, 0–255 |
| Retning | 8 | Byttes fra nettsiden, starter LAV |
| Brems | 9 | Byttes fra nettsiden, starter LAV |

På en vanlig ESP32 brukes GPIO 6–11 av flash-minnet. Bytt pinnenumrene øverst i programmet.
Sjekk i databladet til driveren om brems- og retningsinngangen er aktiv høy eller lav.

## Kom i gang

1. Installer ESP32-kortpakken i Arduino IDE, og biblioteket **WebSockets** av Markus Sattler.
2. Åpne `Motor_PWM_med_retning_og_brems/Motor_PWM_med_retning_og_brems.ino`.
3. Skriv inn ditt eget WiFi-navn og passord øverst (`DITT_WIFI_NAVN` og `DITT_WIFI_PASSORD`).
4. Last opp, åpne seriemonitoren på 115200 baud og noter IP-adressen.
5. Åpne IP-adressen i en nettleser på samme nett.

## Kommandoer (WebSocket, port 81)

| Kommando | Virkning |
|---|---|
| `PWM:<0-255>` | Setter farten og stopper automatikken |
| `DIR:TOGGLE` | Bytter retning |
| `BRAKE:TOGGLE` | Bytter brems |
| `AUTO:TOGGLE` | Starter eller stopper automatikken |
| `AUTO:LOWSPD:<0-255>` / `AUTO:HGHSPD:<0-255>` | Lav og høy fart i automatikken |
| `AUTO:LOWT:<50-3000>` / `AUTO:HGHT:<50-3000>` | Tid på lav og høy fart, i ms |

## Beslektet

[NexusDrive](https://github.com/runej42-stack/nexus-drive) er en telefonapp for lignende motorstyring over
Bluetooth i stedet for WiFi. Den bruker sitt eget Bluetooth-oppsett og snakker ikke med dette programmet.
