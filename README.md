# MadBuddy

Ein Wesen mit großen Augen auf dem ESP32-2432S028 (Cheap Yellow Display, Variante mit 2 USB-Buchsen). Es schaut sich um, blinzelt, freut sich über Streicheln, langweilt sich, wenn sich keiner kümmert, schläft im Dunkeln und reagiert auf Tageszeit und Wetter.

## Bedienung

| Geste | Wirkung |
|---|---|
| **Tippen** | kurz überrascht, ein bisschen Unterhaltung |
| **Streicheln** (Finger über das Display ziehen) | verliebt, Herzchen; mehr Zuneigung (höchstens alle 2 s) |
| **Stupsen** (5 Tipps in 3 s) | genervt, 20 s rote, böse Augen |
| **High-Five** (Tipp in eine obere Ecke) | Hand in der Ecke, LED blitzt weiß, er freut sich |
| **Finger auflegen / ziehen** | die Augen folgen dem Finger (3 s) |
| **Lang drücken** (> 1,2 s) | Menü |

**Menü** (2×3 Kacheln, schließt nach 10 s ohne Eingabe): ▶ Spielen (Fang mich, Simon) · Apfel (Füttern) · Sanduhr (Timer) · Uhr · Zahnrad (Einstellungen) · ✕ Zurück.

- **Uhr:** große Uhrzeit, Datum, Wetter für 15 s; Antippen schließt.
- **Fang mich:** ein kleines Auge springt herum, 20 s; ab 8 Treffern gewonnen.
- **Simon:** Folge der vier Farbfelder nachtippen; Länge 6 = gewonnen.
- **Füttern:** Apfel, Pizza, Kuchen, Kaffee; oben der Sättigungsbalken. Die Speise fliegt zum Mund, er kaut und sagt, wie es war. Er hat ein (zufälliges) Lieblingsessen; zu viel macht Bauchweh.
- **Timer (Pomodoro):** 15, 25 oder 45 min Arbeit, dann 5 min Pause. Während der Arbeit schaut er konzentriert und ist still, ein Ring am Rand zeigt den Fortschritt, die Restzeit steht im Statusstreifen. Pause: LED blinkt 5× grün, Ende: 2× blau.
- **Einstellungen:** Helligkeit (3 Stufen), WLAN/Ort (öffnet das Portal), Firmware (Stand; Tippen sucht nach einem Update), Info (Version, freier Speicher, Lichtsensor, Charakter-Balken).

**Sprechblasen:** Ab und zu sagt er etwas (höchstens alle 90 s): Gedanken, wie es ihm geht, zum Wetter, beim Streicheln oder Stupsen. Morgens begrüßt er dich mit dem Tageswetter, um 22 Uhr gähnt er „Gute Nacht“, um 20 Uhr zeigt er den **Tagesrückblick** (gestreichelt, gestupst, Spiele, gegessen, Pomodoros; Tippen schließt).

**Hunger, Krankheit, Charakter:** Wird er nicht gefüttert, wird er hungrig. Ist er 2 Stunden lang sehr hungrig und ungeliebt, wird er krank (blass, „Hatschi“). Heilen: satt füttern, dann 3× streicheln. Sterben kann er nicht. Wie du mit ihm umgehst, prägt seinen Charakter: viel Streicheln macht ihn verschmust, Stupsen frech, Spielen verspielt.

**WLAN und Ort:** Beim ersten Start (und wenn 30 s kein WLAN da ist) öffnet der Buddy den Access-Point **„MadBuddy“**. Mit dem Handy verbinden, WLAN wählen, im Feld **„Ort“** den Wohnort eintragen (z. B. „Berlin“), speichern. Der Buddy lebt währenddessen weiter; der Statusstreifen zeigt „WLAN einrichten: MadBuddy“. Aus dem Ort werden die Koordinaten für das Wetter (Open-Meteo) bestimmt.

**Schlaf:** Nachts (22–7 Uhr) schläft er nach 10 min ohne Berührung ein (zZz, Hintergrundlicht 10 %) und begrüßt dich morgens. Ist seine Energie fast leer, macht er ein Nickerchen; eine Berührung weckt ihn.

**Online-Update:** Liegt eine neue Firmware im GitHub-Release, erscheint die Karte „Update bereit“. *Ja* lädt und startet neu, *Später* fragt in einer Stunde wieder. Details: [`docs/ota.md`](docs/ota.md).

**Touch einrichten:** Beim ersten Start (oder Finger beim Einschalten auf dem Display) die vier Pfeile in den Ecken antippen; die Werte bleiben gespeichert.

**Claude-Modus:** Mit Claude-Code-Hooks auf dem PC zeigt der Buddy, was Claude gerade tut – „Okay!“ beim Auftrag, nachdenklich (orange), Pupillen huschen beim Lesen, Tipp-Punkte beim Schreiben, drehender Ring bei Befehlen, „Mist!“ bei Fehlern, **Blick + gelbe LED „Hey, du!“, wenn Claude dich braucht**, High-Five „Fertig!“ am Ende. Oranger Funke im Statusstreifen = Sitzung aktiv (daneben die Zahl der Helfer). 15 min ohne Ereignis = wieder Haustier. Der Schlüssel steht unter Einstellungen → Info. Einrichtung: [`tools/claude-hooks/README.md`](tools/claude-hooks/README.md).

## Entwicklung

- Bauen: `pio run -e cyd2usb`
- Flashen (erstes Mal per Kabel): `pio run -e cyd2usb -t upload`
- Tests (am PC): `pio test -e native` (Logik unter `src/logic/`) und `node --test tools/*.test.mjs`
- Sätze: `docs/superpowers/specs/2026-10-25-saetze.json` → `node tools/gen-phrases.mjs` erzeugt `src/logic/phrases.h` (mit committen; `--check` prüft, ob er aktuell ist)
- Veröffentlichen (nur lokal): `pio run -e cyd2usb && node tools/ota-publish.mjs --release`
- Seriell (115200): alle 60 s `heap=… min=… fps=… emo=…`

Aufbau: reine, am PC getestete Logik in `src/logic/*.h` (keine Arduino-Includes, Zeit als Parameter); Hardware in `src/hw/`, Bildschirme in `src/ui/`, Netz in `src/net/` (eigener Task auf Kern 0), Update in `src/ota/`; `src/app.cpp` verbindet alles.
Pins und Messwerte (Panel, Touch-Kalibrierung, Lichtsensor) stehen in `src/config.h`.

Design und Pläne: `docs/superpowers/`, Fortschritt: `docs/FORTSCHRITT.md`, Online-Update: `docs/ota.md`. Vorlage für das Online-Update: `reference/ha_buddy_ota/`.
