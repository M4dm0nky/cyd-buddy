# CYD-Buddy

Ein Wesen mit großen Augen auf dem ESP32-2432S028 (Cheap Yellow Display, Variante mit 2 USB-Buchsen). Es schaut sich um, blinzelt, freut sich über Streicheln, langweilt sich, wenn sich keiner kümmert, schläft im Dunkeln und reagiert auf Tageszeit und Wetter.

## Bedienung

| Geste | Wirkung |
|---|---|
| **Tippen** | kurz überrascht, ein bisschen Unterhaltung |
| **Streicheln** (Finger über das Display ziehen) | verliebt, Herzchen; mehr Zuneigung (höchstens alle 2 s) |
| **Stupsen** (5 Tipps in 3 s) | genervt, 20 s rote, böse Augen |
| **Lang drücken** (> 1,2 s) | Menü |

**Menü** (schließt nach 10 s ohne Eingabe): ▶ Spielen (Fang mich, Simon) · Uhr · Zahnrad (Einstellungen) · ✕ Zurück.

- **Uhr:** große Uhrzeit, Datum, Wetter für 15 s; Antippen schließt.
- **Fang mich:** ein kleines Auge springt herum, 20 s; ab 8 Treffern gewonnen.
- **Simon:** Folge der vier Farbfelder nachtippen; Länge 6 = gewonnen.
- **Einstellungen:** Helligkeit (3 Stufen), WLAN/Ort (öffnet das Portal), Firmware (Stand; Tippen sucht nach einem Update), Info (Version, freier Speicher, Lichtsensor).

**WLAN und Ort:** Beim ersten Start (und wenn 30 s kein WLAN da ist) öffnet der Buddy den Access-Point **„CYD-Buddy“**. Mit dem Handy verbinden, WLAN wählen, im Feld **„Ort“** den Wohnort eintragen (z. B. „Berlin“), speichern. Der Buddy lebt währenddessen weiter; der Statusstreifen zeigt „WLAN einrichten: CYD-Buddy“. Aus dem Ort werden die Koordinaten für das Wetter (Open-Meteo) bestimmt.

**Schlaf:** Ist es länger als 10 s dunkel, schläft er (zZz, Hintergrundlicht 10 %). Wird es tagsüber wieder hell, begrüßt er dich.

**Online-Update:** Liegt eine neue Firmware im GitHub-Release, erscheint die Karte „Update bereit“. *Ja* lädt und startet neu, *Später* fragt in einer Stunde wieder. Details: [`docs/ota.md`](docs/ota.md).

**Testbild:** Beim Einschalten den Bildschirm gedrückt halten (Farbbalken, Touch-Punkte, Rohwerte von Touch und Lichtsensor seriell).

## Entwicklung

- Bauen: `pio run -e cyd2usb`
- Flashen (erstes Mal per Kabel): `pio run -e cyd2usb -t upload`
- Tests (am PC): `pio test -e native` (Logik unter `src/logic/`) und `node --test tools/*.test.mjs`
- Veröffentlichen (nur lokal): `pio run -e cyd2usb && node tools/ota-publish.mjs --release`
- Seriell (115200): alle 60 s `heap=… min=… fps=… emo=…`

Aufbau: reine, am PC getestete Logik in `src/logic/*.h` (keine Arduino-Includes, Zeit als Parameter); Hardware in `src/hw/`, Bildschirme in `src/ui/`, Netz in `src/net/` (eigener Task auf Kern 0), Update in `src/ota/`; `src/app.cpp` verbindet alles.
Pins und Messwerte (Panel, Touch-Kalibrierung, Lichtsensor) stehen in `src/config.h`.

Design und Pläne: `docs/superpowers/`, Fortschritt: `docs/FORTSCHRITT.md`, Online-Update: `docs/ota.md`. Vorlage für das Online-Update: `reference/ha_buddy_ota/`.
