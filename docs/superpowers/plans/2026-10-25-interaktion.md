# Interaktion (F1–F4) Implementation Plan

> **For agentic workers:** Umsetzung durch den Online-Agenten nach `docs/superpowers/cloud-anleitung-interaktion.md`. TDD (superpowers:test-driven-development) für alles unter `src/logic/`. Nach **jedem Task** Eintrag in `docs/FORTSCHRITT.md` (Format unten), commit, push.

**Goal:** Die vier Funktionen aus `docs/superpowers/specs/2026-10-25-interaktion-design.md` (F1 Augen folgen + High-Five, F2 Sprechblasen + Rituale + Rückblick, F3 Füttern + Krankheit + Charakter, F4 Pomodoro).
**Architecture:** Reine Logik als header-only in `src/logic/` (Zeit, Stunde, Datum, Zufalls-Seed als Parameter) mit Unity-Tests; UI-Module in `src/ui/`; Verdrahtung in `src/app.cpp`; Dauerwerte in `src/hw/store.*`.
**Tech Stack:** PlatformIO (pioarduino 55.03.312-1, Arduino-Core 3.3.12), LovyanGFX 1.2.34, ArduinoJson 7, Unity, Node ≥ 20.

## Global Constraints
- Spec gilt; bei Widerspruch Spec vor Plan. Sätze nur aus `docs/superpowers/specs/2026-10-25-saetze.json`.
- `src/logic/` nur Standardheader, keine Arduino-Includes; vorhandene Helfer nutzen: `logic/rng.h` (`rng_next`, `rng_range`), `logic/clamp.h`, `logic/ascii.h`.
- Bestehende 78 Tests müssen grün bleiben. Bestehende Tests nur ändern, wenn die Spec das Verhalten ändert – im FORTSCHRITT-Eintrag nennen.
- Das 8-Bit-Sprite (320×200) bleibt die einzige große Allokation; neue Bildschirme zeichnen direkt aufs Display. Kein `new`/`malloc` pro Bild.
- Bildrate ≥ 25 fps: keine Pixel-Schleifen über große Flächen pro Bild (Ausnahme: Sonnenbrillen-Raster existiert schon).
- `Persist`-Änderung: `kVersion` erhöhen und alte Werte beim Laden übernehmen (fehlende Schlüssel → Startwerte), damit WLAN-unabhängige Werte (Bedürfnisse, Helligkeit, Ort) nach dem Update erhalten bleiben. Touch-Kalibrierung (`touchcal`) nie anfassen.
- Nicht ändern: `src/ota/*`, `src/logic/ota_policy.h`, `tools/ota-publish.mjs`, `src/hw/display.h`, `src/config.h`, Touch-Kalibrierung in `src/main.cpp`.
- Kein Release, kein Flash, kein PR, kein Push auf `main`.

## Fortschritts-Protokoll (Pflicht)
Nach jedem Task (bei Tasks mit UI zusätzlich nach jedem Step mit Dateiänderungen) **oben** in `docs/FORTSCHRITT.md`:
```markdown
## Interaktion Task <N> – <Name> – <ERLEDIGT | TEILWEISE | BLOCKIERT>
- Zeit: <UTC>
- Gemacht: <1–3 Sätze>
- Dateien: <Liste>
- Prüfung: <Befehl> → <Ergebnis wörtlich: Tests/Fehler, Build-Größe> oder „nicht geprüft, weil …“
- Gegenprobe: <welcher absichtliche Fehler, Ergebnis>
- Nächster Schritt: <Task/Step>
- Offen/Fragen: <oder „–“>
```
Dann `git add <Dateien> docs/FORTSCHRITT.md && git commit -m "<typ>: Interaktion Task <N> <kurz>" && git push`.

## Tasks

### Task 1 (F1): `src/logic/look.h` + Tests `test/test_look`
`LookState`, `look_at`, `look_active`, `look_target`, `highfive_hit` laut Spec. Tests: Abbildung Mitte → (0,0), Ecken → Grenzen ±30/±15, Ablauf nach 3000 ms, Ecken-Treffer oben links/rechts ja, unten/Mitte nein. `MoodEvent::HIGHFIVE` in `mood.h` + Test (fun +5, affection +2, EXCITED 2 s). Commit `feat: Interaktion Task 1 Blick folgt und High-Five (Logik)`.

### Task 2 (F1): UI + Verdrahtung
`OV_HAND` (weather_mood.h Overlay-Bit 1024 + Test), Hand zeichnen in `eyes.cpp`, `app.cpp`: Blick folgt bei Touch-Beginn und Streicheln, High-Five in Ecken statt TAP, LED kurz weiß. Build. Commit `feat: Interaktion Task 2 Blick folgt und High-Five (UI)`.

### Task 3 (F2): Sätze
`tools/gen-phrases.mjs` (+ `tools/gen-phrases.test.mjs`, läuft mit `node --test tools/*.test.mjs`) erzeugt `src/logic/phrases.h`; Unity-Test `test/test_phrases`: alle Kategorien ≥ 6 Sätze, jeder Satz nach ASCII-Wandlung ≤ 28 Zeichen. Die erzeugte Datei wird mit committet (das Gerät baut ohne Node). Commit `feat: Interaktion Task 3 Sätze als Header`.

### Task 4 (F2): `speech.h`, `rituals.h`, `daystats.h` + Tests
Regeln laut Spec (Abstand 90 s, 40 %-Chance beim Gefühlswechsel, Gedanken alle 5–10 min, keine Wiederholung, Morgen 5–12 Uhr einmal pro Tag, Nacht 22:00 einmal, Rückblick 20:00 bzw. Nachholen bis 23 Uhr, Tageswechsel setzt Zähler zurück, Wetter-Satz „Heute 14° und Regen“). Commit `feat: Interaktion Task 4 Sprechregeln, Rituale, Tageszähler`.

### Task 5 (F2): UI
`src/ui/bubble.*` (ins Sprite, Parameter an `eyes_draw`), Bildschirm `REVIEW` (`src/ui/review.*`), Verdrahtung inkl. Ereignis-Sätzen, Tageszähler in NVS. Build. Commit `feat: Interaktion Task 5 Sprechblasen und Rückblick (UI)`.

### Task 6 (F3): `food.h`, Sättigung/Krankheit in `mood.h`, `character.h` + Tests
Alle Zahlen laut Spec; Tests je Regel (inkl. Krankheit erst nach 2 h ununterbrochen, Heilung Sättigung > 60 + 3× Streicheln, FULL bei > 95, Lieblingsessen, Rang HUNGRY/SICK, Charakter-Klemmung und Wirkung auf Eigenarten-Abstand). Neue Emotionen + `face_expression`-Werte + Test in `test_face_model`. Commit `feat: Interaktion Task 6 Füttern, Krankheit, Charakter (Logik)`.

### Task 7 (F3): UI
Menü 2×3 (`menu.*`), Bildschirm `FEED` (`src/ui/feed.*`) mit Speise-Symbolen und Sättigungsbalken, Flug-/Kau-Animation, `OV_SNEEZE`, Charakter-Balken in den Einstellungen (Info), `Persist` erweitern. Build. Commit `feat: Interaktion Task 7 Füttern und Charakter (UI)`.

### Task 8 (F4): `pomodoro.h` + Tests
Zustände, Ereignisse, Fortschritt, Restzeit laut Spec. Commit `feat: Interaktion Task 8 Pomodoro (Logik)`.

### Task 9 (F4): UI
Bildschirm `TIMER` (`src/ui/timer_view.*`), FOCUSED-Gesicht, Fortschrittsring im Sprite, Restzeit im Statusstreifen, LED-Signale, Zähler. Build. Commit `feat: Interaktion Task 9 Pomodoro (UI)`.

### Task 10: Abschluss
`README.md` (Bedienung ergänzen), Abschlussprüfung (alle Tests, Node-Tests, Build mit Größe), FORTSCHRITT mit Liste „am Gerät zu prüfen“ je F1–F4. Commit `docs: Interaktion Task 10 Abschluss`.
