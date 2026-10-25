# CYD-Buddy – Mehr Interaktion (Design)

Stand 2026-10-25, freigegeben vom Nutzer. Vier Funktionen, jede als eigenes OTA-Update: **F1** Augen folgen + High-Five, **F2** Sprechblasen + Rituale + Tagesrückblick, **F3** Füttern + Krankheit + Charakter, **F4** Pomodoro-Timer. Danach (eigene Spec, nicht hier) der „Claude-Modus“.

Grundlage: Gesicht nach `docs/superpowers/specs/2026-10-25-gesicht-design.md` (FaceShape, 22 Emotionen, `mood_quirks`). Sätze: `docs/superpowers/specs/2026-10-25-saetze.json` (kuratiert, nicht ändern ohne Rückfrage).

## Gemeinsames
- **Menü** wird 2×3 Kacheln (je ~96×90 px, Abstand 8): Spielen (grün), **Füttern** (orange), **Timer** (rot-violett), Uhr (gelb), Einstellungen (violett), Schließen (rot). Symbole aus Primitiven (Apfel, Sanduhr/Tomate), keine Textkürzel.
- Neue **Emotionen** (vor `COUNT`, mit Werten in `face_expression`): `HUNGRY` (pupil_dy +8, Mund WAVE, mouth_w 40, Brauen leicht innen hoch bt 0.6, Farbe orange 255,150,40), `FULL` (Bauchweh: open .6, top_cut .3, Mund WAVE, Farbe grünlich 140,200,80), `SICK` (open .55, tilt .4, Mund CURVE mc -.4, Farbe blassgrün 120,170,120), `FOCUSED` (open .7, top_cut .25, bt -.3, Mund CURVE mc 0 mw 30, Farbe Timer-Farbe 255,90,140).
- **Neue Dauerwerte** (`Persist`, NVS, `kVersion` erhöhen mit Übernahme alter Werte): `satiety` (Sättigung 0..100, Start 70), `sick` (bool), `cuddly` und `playful` (je −100..100, Start 0), `favorite_food` (0..3, beim ersten Start zufällig), Tageszähler (s. F2), `last_day` (YYYYMMDD).
- Alles Logische in `src/logic/*.h` (header-only, Zeit als Parameter, Unity-Tests), UI in `src/ui/*`, Verdrahtung `src/app.cpp`.

## F1 – Augen folgen + High-Five
- `src/logic/look.h`: `void look_at(LookState&, int16_t x, int16_t y, uint32_t now)` setzt ein Blickziel `gx = (x-160)/160*30`, `gy = (y-100)/100*15` (geklemmt auf ±30/±15) für 3000 ms; `bool look_active(const LookState&, uint32_t now)`; `void look_target(const LookState&, float *gx, float *gy)`. In `app.cpp`: bei jedem Touch-Beginn und während des Streichelns `look_at`; solange aktiv, ersetzt das Ziel den Gazer.
- **High-Five:** TAP in einer oberen Ecke (`x < 64 || x > 256`, `y < 56`) auf dem EYES-Bildschirm → `MoodEvent::HIGHFIVE`: Unterhaltung +5, Zuneigung +2, Emotion `EXCITED` 2 s, Overlay `OV_HAND` (gezeichnete Hand in Augenfarbe in der getippten Ecke, 1,5 s, kurzes Aufblitzen der LED weiß) und (ab F2) Sprechblase „highfive“. `bool highfive_hit(int16_t x, int16_t y)` in `look.h` (testbar). High-Five ersetzt in der Ecke den normalen TAP (kein SURPRISED/CONFUSED).

## F2 – Sprechblasen, Rituale, Tagesrückblick
- `tools/gen-phrases.mjs` erzeugt aus `saetze.json` → `src/logic/phrases.h` (`enum class Say : uint8_t { MORGEN, NACHT, … }` in JSON-Reihenfolge ohne `_quelle`, `const char *const` Tabellen, UTF-8). Test: jeder Satz nach `ascii`-Wandlung (`logic/ascii.h`) ≤ 28 Zeichen, jede Kategorie ≥ 6 Sätze.
- `src/logic/speech.h`: `SpeechState`; `const char *speech_pick(SpeechState&, Say, uint32_t seed)` (nie zweimal hintereinander derselbe Satz je Kategorie); `bool speech_due(SpeechState&, Emotion base, uint32_t now)` Regeln: höchstens eine Blase alle 90 s (Rituale/Ereignisse ausgenommen); beim Wechsel des Grundgefühls zu BORED/SAD/HUNGRY/SICK/HAPPY mit 40 % Chance die passende Kategorie (gelangweilt/traurig/hungrig/krank/froh); sonst alle 5–10 min „gedanken“ (bei |Charakter| > 40 stattdessen „frech“ bzw. „verschmust“).
- **Ereignis-Sätze:** STROKE → „verliebt“ (25 % Chance, Cooldown 90 s gilt), POKE → „genervt“, Gewitter-Schreck → „gewitter“, nach neuem Wetter (Himmel geändert) einmal regen/sonne/hitze/kalt.
- **Blase zeichnen** (`src/ui/bubble.*`): oben mittig, abgerundetes Rechteck mit kleinem Zipfel Richtung Gesicht, weiß/hellgrau auf Schwarz, Text schwarz `FreeSansBold9pt7b`, Breite nach Text, y 2..32 (Brauen ggf. kurz überdeckt), 4 s sichtbar, ins Sprite gezeichnet (Parameter an `eyes_draw`). Umlaute über `ascii.h`.
- **Rituale** (`src/logic/rituals.h`, testbar mit Stunde/Minute/Tag als Parameter):
  - *Morgen:* erstes Aufwachen (Ende Nachtschlaf oder erste Berührung) eines Tages zwischen 5 und 12 Uhr → GREETING + Blase „morgen“, 4 s später zweite Blase mit Tageswetter, z. B. „Heute 14° und Regen“ (Format aus Temperatur + Himmel-Wort: Sonne, Wolken, Nebel, Regen, Schnee, Gewitter).
  - *Nacht:* um 22:00 (einmal pro Tag, wenn wach) → YAWN + Blase „nacht“.
  - *Tagesrückblick:* um 20:00 einmal pro Tag (wenn wach, sonst bei der nächsten Berührung bis 23 Uhr) Bildschirm `REVIEW` für 12 s: Überschrift „Dein Tag mit mir“, Zeilen mit Symbol + Zahl: gestreichelt, gestupst, Spiele (gewonnen/gesamt), gegessen, Pomodoros; darunter kleines Augenpaar mit Gefühl (viel Zuneigung → HAPPY, viel Stupsen → ANNOYED, sonst CONTENT). Tippen schließt.
- `src/logic/daystats.h`: Zähler `strokes, pokes, games, wins, meals, pomodoros`, `daystats_roll(DayStats&, uint32_t yyyymmdd)` setzt bei neuem Tag zurück. In NVS gespeichert (mit dem 10-min-Speichern).

## F3 – Füttern, Krankheit, Charakter
- `src/logic/food.h`: `enum class Food : uint8_t { APPLE, PIZZA, CAKE, COFFEE }`; Wirkung `food_effect(Food)`: Apfel Sättigung +15, Pizza +35, Kuchen +25 & Unterhaltung +5, Kaffee +5 & Energie +15. Lieblingsessen zusätzlich Zuneigung +5, Unterhaltung +5 → Emotion EXCITED 3 s + Blase „lieblingsessen“; sonst LOVE-ähnlich kurz (CONTENT→HAPPY 2 s) + Blase „lecker“. Liegt Sättigung danach > 95 → `FULL` 5 s + Blase „satt“.
- Sättigung sinkt wach −0,12/min, schlafend −0,04/min. Grundgefühl `HUNGRY`, wenn Sättigung < 25 (Rang: nach SLEEPY, vor SAD).
- **Krankheit:** `sick` wird wahr, wenn 2 h lang ununterbrochen Sättigung < 15 **und** Zuneigung < 20. Krank: Grundgefühl `SICK` (Rang direkt nach SLEEPING), Energie-Abbau doppelt, alle 1–3 min „Hatschi“: Overlay `OV_SNEEZE` (Tropfen an der Nase + Augen kurz zu) + Blase „krank“. Heilung: Sättigung > 60 **und** danach 3× Streicheln → gesund, HAPPY 3 s + Blase „froh“. Nie Tod.
- **Fütter-Bildschirm** `FEED` (Menü): 4 Kacheln mit gezeichneten Speisen (Apfel rot, Pizza-Dreieck, Kuchen-Stück, Kaffeetasse), oben Sättigungsbalken (Farbe grün/gelb/rot); Tipp → zurück zum Gesicht, Speise fliegt in 600 ms von unten zum Mund, Mund kaut 3× (mouth_open 0↔0,6 alle 200 ms), dann Reaktion. Krank: Fütter-Bildschirm zeigt zusätzlich ein Thermometer-Symbol.
- **Charakter** (`src/logic/character.h`): `cuddly` +0,5 je Streicheln, −1,5 je Stupsen; `playful` +1 je Spiel, +0,3 je High-Five, −0,02 je Minute Langeweile (BORED); geklemmt ±100. Wirkung: Eigenarten-Abstand × (1 − playful/200); CHEEKY statt MISCHIEVOUS mit Wahrscheinlichkeit 0,5 − cuddly/200; LOVE-Dauer × (1 + cuddly/200); Gedanken-Sätze s. F2. Anzeige im Info-Bereich der Einstellungen: zwei Balken mit Symbolen (Herz ↔ Blitz, Ball ↔ Kissen).

## F4 – Pomodoro-Timer
- `src/logic/pomodoro.h`: Zustände `IDLE, WORK, BREAK`; `pomo_start(Pomo&, uint8_t minutes, uint32_t now)` (15/25/45), Pause fest 5 min; `PomoEvent pomo_tick(Pomo&, uint32_t now)` liefert `NONE, BREAK_START, BREAK_END`; `pomo_stop`; `float pomo_progress(const Pomo&, uint32_t now)` 0..1; `uint32_t pomo_remaining_s`. Nach BREAK_END → IDLE (nicht automatisch neu).
- **Timer-Bildschirm** `TIMER` (Menü): drei große Kacheln „15“, „25“, „45“ (Zahlen sind hier erlaubt, darunter Sanduhr-Symbol) + Zurück; läuft schon ein Timer: Kacheln „Stopp“ (Quadrat-Symbol) und „Zurück“.
- **Während WORK:** Grundgefühl `FOCUSED` (überschreibt alles außer SLEEPING; keine Eigenarten, keine Sprechblasen außer Start), dünner Fortschrittsring am Sprite-Rand (Bogen um das Gesicht, Timer-Farbe), Restzeit klein im Statusstreifen (mm:ss); Start → Blase „pomodoro_start“. Antippen: kein SURPRISED, nur Blick folgt.
- **BREAK_START:** Emotion EXCITED 3 s, Blase „pomodoro_pause“, LED blinkt 5× grün, dann Grundgefühl normal; Ring zeigt Pause-Rest in Grün. **BREAK_END:** Blase „pomodoro_fertig“, LED 2× blau. `pomodoros`-Zähler +1 bei BREAK_START.
- Nachtschlaf und Schlaf werden während WORK nicht ausgelöst.

## Prüfung
- Unity-Tests für `look.h`, `phrases.h`, `speech.h`, `rituals.h`, `daystats.h`, `food.h`, Krankheit/Sättigung in `mood.h`, `character.h`, `pomodoro.h`; jede Gruppe mit Gegenprobe. Bestehende 78 Tests bleiben grün.
- `pio run -e cyd2usb` SUCCESS, Flash < 1 966 080 B, RAM-Zuwachs notieren.
- Am Gerät (lokaler Claude + Nutzer): je Funktion ein OTA-Release, fps ≥ 25.
