# CYD-Buddy – Design

Stand 2026-10-25, freigegeben vom Nutzer.

## Ziel

Auf dem ESP32-2432S028 („Cheap Yellow Display“, Variante mit Micro-USB **und** USB-C) lebt ein Wesen mit großen Augen auf dem Schreibtisch, angelehnt an EMO/Vector: Es schaut sich um, blinzelt, freut sich über Streicheln, langweilt sich, wenn sich keiner kümmert, schläft im Dunkeln und reagiert auf Tageszeit und Wetter. Kein Ton, keine Sprache (Board hat kein Mikrofon).

Nicht-Ziele: Sterben/Krankheit, Ton, Home-Assistant-Anbindung, Cloud-Konto.

## Hardware (CYD2USB)

| Funktion | Pins / Bauteil | Hinweis |
|---|---|---|
| Display 320×240 | SPI (HSPI): SCLK 14, MOSI 13, MISO 12, CS 15, DC 2, RST –, Backlight 21 (PWM) | Treiber bei 2-USB-Variante meist **ST7789**, ggf. Farben invertiert. Per Build-Flag `CYD_PANEL_ST7789` / `CYD_PANEL_ILI9341` und `CYD_INVERT=0/1` umschaltbar; Testbild (Task 1) entscheidet. |
| Touch XPT2046 (resistiv) | eigener SPI (VSPI): CLK 25, MOSI 32, MISO 39, CS 33, IRQ 36 | Rohwerte → Pixel mit Kalibrierkonstanten (Startwerte 200…3700), in Task 1 am Gerät prüfen |
| Lichtsensor (LDR) | GPIO 34 (ADC) | Richtung und Schwelle **am Gerät messen** (Task 1 gibt Werte seriell aus), dann in `config.h` |
| RGB-LED | R 4, G 16, B 17, **aktiv low** | PWM, höchstens ca. 20 % Helligkeit |
| Flash | 4 MB, kein PSRAM | Partition `min_spiffs` (2 × 1,9 MB App für OTA) |

## Technik

- PlatformIO, Plattform **pioarduino** (Arduino-Core 3.x), Bibliotheken: LovyanGFX, WiFiManager (tzapu), ArduinoJson 7.
- Umgebung `cyd2usb` (Gerät) und `native` (Unity-Tests am PC für alles unter `src/logic/`).
- **Regel:** Alles unter `src/logic/` ist reine Logik (nur `<stdint.h>`, `<string.h>`, `<math.h>` usw.), keine Arduino-Includes, Zeit wird als Parameter (`now_ms`) übergeben. Hardware und Zeichnen liegen in `src/hw/`, `src/ui/`, `src/net/`, `src/ota/`.

## Bausteine

### 1. Stimmung (`src/logic/mood.h`)
Drei Werte 0…100 (float): **Zuneigung**, **Unterhaltung**, **Energie**.

Abbau/Aufbau pro Minute, wach: Zuneigung −0,2 · Unterhaltung −0,33 · Energie −0,17. Schlafend: nur Energie +1,0, kein Abbau (sonst wäre er jeden Morgen traurig).

Ereignisse:

| Ereignis | Wirkung |
|---|---|
| Tippen | Unterhaltung +1, kurz *überrascht* (1,5 s) |
| Streicheln | Zuneigung +8 (höchstens alle 2 s), *verliebt* (4 s) |
| Stupsen (≥ 5 Tipps in 3 s) | Zuneigung −3, *genervt* (20 s) |
| Spiel gewonnen | Unterhaltung +25, Zuneigung +5, *aufgeregt* (6 s) |
| Spiel verloren | Unterhaltung +10 |
| Aufwachen aus Dunkelheit am Tag | *begrüßt* (3 s, fröhliches Zwinkern) |

Gefühl (`Emotion`), Vorrang von oben nach unten:
1. `SLEEPING` – **nachts (22–7 Uhr) nach 10 min ohne Berührung** (Berührung weckt, um 7 Uhr wacht er mit Begrüßung auf), oder dunkel seit > 10 s (Lichtsensor, siehe §4: am CYD kaum brauchbar), oder Energie < 10 (Nickerchen bis Energie > 60 oder Berührung)
2. kurzzeitige Gefühle, solange ihre Zeit läuft: `SURPRISED`, `LOVE`, `ANNOYED`, `EXCITED`, `GREETING`
3. `SLEEPY` – Energie < 30 oder Uhrzeit 22–6 Uhr (wenn Zeit bekannt)
4. `SAD` – Zuneigung < 20
5. `BORED` – Unterhaltung < 25
6. `HAPPY` – Zuneigung > 70 und Unterhaltung > 60
7. sonst `CONTENT`

Werte werden auf 0…100 begrenzt. Nach dem Start wird die Zeit, in der das Gerät aus war (aus gespeicherter Epoche, sobald NTP da ist), wie Schlaf nachgerechnet: nur Energie steigt.

### 2. Gesten (`src/logic/gesture.h`)
Eingabe: Folge von (x, y, gedrückt, now_ms). Ausgabe-Ereignisse:
- **Tippen**: Druck < 300 ms, Bewegung < 15 px
- **Streicheln**: durchgehender Druck, Wegsumme > 60 px innerhalb 1,5 s (wiederholt auslösbar alle 1,5 s)
- **Stupsen**: ≥ 5 Tipps in 3 s (meldet zusätzlich zu den Tipps)
- **Lang drücken**: > 1,2 s, Bewegung < 15 px

### 3. Augen
- **Modell (`src/logic/eye_model.h`, testbar):** `EyeShape { w, h, radius, open (0…1), lid_top_tilt (−1…1, negativ = böse nach innen), lid_top_cut (0…1), lid_bottom_cut (0…1, „^^“-Lächeln), gaze_x, gaze_y (px) }` plus Farbe. Tabelle `Emotion → EyeShape+Farbe`. Weiche Übergänge per `eye_tween(cur, target, dt_ms)` (exponentiell, ca. 150 ms Zeitkonstante). Blinzel-Planer (alle 2–6 s zufällig, 80 ms zu, 120 ms auf; mit Seed testbar) und Blick-Planer (alle 1–4 s neues Ziel, x ±30 px, y ±15 px).
- **Zeichnen (`src/ui/eyes.cpp`):** Ein 8-Bit-Sprite (RGB332) 320×200 (64 000 B) für Augen und Overlays, unten ein 40-px-Statusstreifen, der direkt gezeichnet wird (Uhrzeit, Wetter-Symbol, WLAN-Symbol). Augen = abgerundete Rechtecke ohne Pupillen; Lider werden als Polygone in Hintergrundfarbe darübergelegt. Ziel ≥ 25 Bilder/s.
- **Farben:** zufrieden/fröhlich cyan, verliebt pink, genervt rot, traurig blau, gelangweilt grau-violett, müde gedämpftes cyan, aufgeregt gelb.
- **Overlays:** Herzchen (verliebt), Sonnenbrille (sonnig + fröhlich/zufrieden, tagsüber), Schweißtropfen (> 28 °C), Träne (traurig), „zZz“ (schlafend), Zittern (Schnee/< 0 °C), kurzes Aufschrecken (Gewitter, alle paar Minuten).

### 4. Umgebung (`src/hw/`)
- Lichtsensor: Mittelwert über 1 s, Schwelle mit Hysterese, Werte aus `config.h`. **Gemessen 2026-10-25:** bei Raumlicht 0, in Dämmerung kurz ~200; die Beschaltung des CYD ist bekannt schlecht, und das Display leuchtet den Sensor an. Darum nur als Zusatz mit vorsichtiger Schwelle (dunkel > 400, hell < 250); Schlafen läuft hauptsächlich über Uhrzeit + Ruhe (§1). Beim Schlafen Hintergrundlicht 10 %.
- RGB-LED zeigt sanft die Farbe des Gefühls; aus beim Schlafen.

### 5. Netz (`src/net/`)
- **WiFiManager** nicht blockierend: Beim ersten Start bzw. wenn 30 s keine Verbindung: Access-Point **„CYD-Buddy“** mit Zusatzfeld **„Ort“**. Der Buddy lebt währenddessen weiter, der Statusstreifen zeigt „WLAN einrichten: CYD-Buddy“.
- **Zeit:** `configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com")`.
- **Ort → Koordinaten:** einmal nach Änderung über `https://geocoding-api.open-meteo.com/v1/search?name=<Ort>&count=1&language=de` (HTTPS, Zertifikatspaket des Cores), Ergebnis (Name, lat, lon) im NVS.
- **Wetter:** alle 30 min `http://api.open-meteo.com/v1/forecast?latitude=..&longitude=..&current=temperature_2m,weather_code,is_day&timezone=Europe%2FBerlin` (HTTP reicht, keine Geheimnisse, spart Speicher).
- **Wetter → Laune (`src/logic/weather_mood.h`, testbar):** WMO-Code + Temperatur → `{SUNNY, CLOUDY, RAIN, SNOW, STORM, FOG}` + Flags `hot` (> 28 °C), `cold` (< 0 °C oder Schnee). Regen: Augen leicht blau getönt und etwas hängend.
- Netz-Abrufe (Wetter, OTA) laufen in einem eigenen FreeRTOS-Task (`net_task`, Kern 0, niedrige Priorität); die UI liest nur threadsicher kopierte Zustände.
- Ohne WLAN läuft alles außer Uhr/Wetter/OTA.

### 6. Menü und Spiele (`src/ui/`)
- **Lang drücken** öffnet ein Menü mit großen, farbigen **Symbolen** (keine Textkürzel): ▶ Spielen, Uhr, Zahnrad (Einstellungen), ✕ Zurück. Schließt nach 10 s ohne Eingabe.
- **Uhr:** große Uhrzeit, Datum, Wetter-Symbol und Temperatur für 15 s; die Augen schauen kurz von unten herein.
- **Einstellungen:** Helligkeit (3 Stufen), WLAN/Ort neu einrichten (Portal öffnen), Firmware (jetzt nach Update suchen, Stand anzeigen), Info (Version, freier Speicher).
- **Spiel „Fang mich“ (`src/logic/catch_game.h` + UI):** Ein kleines Auge springt herum, 20 s Runde, Treffer zählt; ab 8 Treffern gewonnen; es wird schneller.
- **Spiel „Simon“ (`src/logic/simon.h` + UI):** 4 farbige Felder, Folge wird länger; Länge 6 geschafft = gewonnen, Fehler = verloren. Folge per Seed erzeugt (testbar).

### 7. Speichern (`src/hw/store.*`)
NVS-Namensraum `buddy`: Bedürfniswerte, letzte Epoche, Helligkeit, Ort (Name, lat, lon). Schreiben alle 10 min und bei Änderungen an Einstellungen (Flash schonen).

### 8. Online-Update (OTA)
Portiert aus `HA_BUDDY/HA_Buddy_7in/src/ota/ota_pull.*`, `src/logic/ota_policy.h`, `tools/ota-publish.mjs`, `docs/ota-7in.md`. Die Kopien liegen als Vorlage in `reference/ha_buddy_ota/` im Repo (die Cloud sieht das HA-Buddy-Repo nicht).

Gleich bleibt:
- Manifest abfragen (erstmals nach 90 s, dann alle 10 min, nach Fehler 3 min)
- neue Version an der **ELF-SHA-256** im Image erkennen
- Zustimmung **am Display** (Karte „Update bereit“ mit Ja und Später; Später = 1 h Ruhe; nur bei wachem Display; Tipps in der ersten Sekunde zählen nicht)
- vor dem Schreiben das **Image selbst prüfen** (Magic 0xE9, Chip, App-Kennung, Projektname = der laufenden Firmware, ELF = Manifest)
- **SHA-256 über die ganze Datei**, erst dann `Update.end(true)`, sonst `Update.abort()`
- A/B-Slot, Neustart, Gesundheitsnachweis (`esp_ota_mark_app_valid_cancel_rollback()` nach 20 s laufender Augen-Schleife)

Anders als beim HA-Buddy:
- **Chip-ID 0** (ESP32) statt 9 (ESP32-S3) in `ota_policy.h` und im Skript.
- **Ablage GitHub-Release** im öffentlichen Repo `M4dm0nky/cyd-buddy`: Basis `https://github.com/M4dm0nky/cyd-buddy/releases/latest/download`, Dateien `manifest.json` und `firmware.bin` (keine `.png`-Tarnung, kein Token, denn die `.bin` enthält keine Geheimnisse).
- **HTTPS** mit `WiFiClientSecure` + Zertifikatspaket des Cores (kein `setInsecure`). **Umleitungen selbst verfolgen** (`HTTPC_DISABLE_FOLLOW_REDIRECTS`, `Location` lesen, höchstens 3 Sprünge), erlaubt sind nur `https://` und Hosts `github.com` oder Endung `.githubusercontent.com` (testbare Funktion `ota_redirect_ok(url)` in `ota_policy.h`).
- **Kein PSRAM:** Puffer per `malloc`, ArduinoJson mit Standard-Allocator. Bei Zustand DOWNLOADING gibt die UI das Augen-Sprite frei und zeichnet direkt einen Fortschrittsbalken mit „konzentrierten“ Augen; nach Fehler Sprite neu anlegen.
- **Projektname** im Image: wird nach dem ersten Build mit `node tools/ota-publish.mjs --check` ausgelesen und als Konstante eingetragen (beim HA-Buddy war es `arduino-lib-builder`, hier nicht raten).
- Slot-Grenze im Skript: 1 966 080 B (`min_spiffs`).
- **Veröffentlichen:** `node tools/ota-publish.mjs --release` prüft Image, Veralterung, uncommittete Änderungen und ruft dann `gh release create v<YYYYMMDD-HHMM> firmware.bin manifest.json --latest --repo M4dm0nky/cyd-buddy` auf. Das macht nur der lokale Claude.
- Erster Flash per Kabel (`pio run -e cyd2usb -t upload`), danach ohne Kabel.

## Fehlerverhalten
- Kein WLAN: Buddy lebt, Statusstreifen zeigt WLAN-Symbol rot, Portal offen.
- Wetter/Geocoding fehlgeschlagen: letzter Wert bleibt; nach 3 h ohne Wetter keine Wetter-Overlays.
- OTA-Fehler: Karte „Update fehlgeschlagen“ mit Grund, Antippen schließt; nächster Versuch in 3 min.
- Zu wenig Speicher für das Sprite: seriell melden, ohne Sprite direkt (langsamer) zeichnen.
- Freier Heap (min.) wird seriell alle 60 s ausgegeben.

## Prüfung
- `pio test -e native`: mood, gesture, eye_model, weather_mood, simon, catch_game, ota_policy. Jede Testgruppe einmal mit absichtlich eingebautem Fehler gegenprüfen.
- `pio run -e cyd2usb`: baut, Image < 1 966 080 B.
- Am Gerät (lokal, mit Nutzer): Testbild/Farben/Touch, Augen flüssig, Gesten, Lichtsensor, Portal, Wetter, Uhr, Spiele, OTA Ende zu Ende mit zweitem Release.
