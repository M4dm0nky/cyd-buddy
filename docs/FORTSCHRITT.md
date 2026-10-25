# Fortschritt CYD-Buddy

Neueste Einträge oben. Format und Regeln: `docs/superpowers/plans/2026-10-25-cyd-buddy.md`, Abschnitt „Fortschritts-Protokoll“.

## Task 8 – Augen zeichnen und Hauptschleife – ERLEDIGT
- Zeit: 2026-10-25 11:20 UTC
- Gemacht: Augen-Renderer mit 8-Bit-Sprite 320×200 (Fallback direkt), Overlays (Herz, Sonnenbrille, Schweiß, Träne, zZz, Zittern, Regen-Tönung), Symbole aus Primitiven, Statusstreifen (zeichnet nur bei Änderung), `app.cpp` mit Bildtakt 33 ms, Gesten → Stimmung, Licht → Schlaf, Blinzeln/Blick/Tween, LED 15 %, Hintergrundlicht, Speichern alle 10 min, Seriell-Log alle 60 s. Testbild nach `ui/testscreen.cpp` ausgelagert (beim Start gedrückt halten).
- Dateien: src/ui/eyes.h, src/ui/eyes.cpp, src/ui/icons.h, src/ui/icons.cpp, src/ui/status_bar.h, src/ui/status_bar.cpp, src/ui/testscreen.h, src/ui/testscreen.cpp, src/app.h, src/app.cpp, src/main.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 10,6 % (34 736 B statisch; Sprite 64 000 B kommt zur Laufzeit dazu), Flash 27,3 % (537 303 B). `pio test -e native` → 45 Tests, 0 Fehler (7 Gruppen)
- Gegenprobe: – (Renderer nicht unit-getestet)
- Nächster Schritt: Task 9 Step 1
- Offen/Fragen: (1) Symbol-Funktionen nehmen `LovyanGFX &` statt `LGFX &` (Basisklasse; `LGFX` passt weiter, dazu gehen Sprites). Zusätzliche Symbole: `icon_target`, `icon_simon`, `icon_brightness`, `icon_back`. Zusätzlich `status_invalidate()` und `eyes_draw_pair()` (für Spiele/OTA). (2) Lid-Schnitte beziehen sich auf die *geöffnete* Höhe `h*open`, nicht auf `h` – sonst wäre SLEEPY (open 0,35, top_cut 0,5) ganz zu. (3) Sonnenbrillen-Rahmen grau statt schwarz, weil Schwarz auf schwarzem Grund unsichtbar wäre. (4) Mond-Symbol setzt schwarzen Hintergrund voraus.
- Am Gerät zu prüfen: Augen flüssig (fps im Seriell-Log, Ziel ≥ 25), Gesten (Tippen/Streicheln/Stupsen/Lang drücken), LDR-Richtung/-Schwelle, Testbild beim Start gedrückt halten.

## Task 7 – Hardware-Schicht (LED, Licht, Speicher) – ERLEDIGT
- Zeit: 2026-10-25 11:00 UTC
- Gemacht: `led.*` (ledcAttach/ledcWrite, im Core-Header `esp32-hal-ledc.h` geprüft: `bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution)`, `bool ledcWrite(uint8_t pin, uint32_t duty)`), `light.*` (Ringpuffer 16, alle 60 ms, Hysterese), `store.*` (Preferences, Namensraum `buddy`, Versions-Schlüssel `ver`).
- Dateien: src/hw/led.h, src/hw/led.cpp, src/hw/light.h, src/hw/light.cpp, src/hw/store.h, src/hw/store.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 10,4 % (33 984 B), Flash 25,2 % (494 623 B) – Module noch nicht benutzt
- Gegenprobe: – (keine Unit-Tests für Hardware vorgesehen)
- Nächster Schritt: Task 8 Step 1
- Offen/Fragen: `LDR_DARK_IS_HIGH` fehlt in config.h → in light.cpp Standard 1 mit Kommentar „am Gerät messen“. Sichere Variante: Solange `LDR_DARK_THRESHOLD` ≤ 0 (Platzhalter), meldet `light_update` nie „dunkel“, sonst würde der Buddy mit Schwelle 0 dauernd schlafen.

## Task 6 – OTA-Regeln portieren (ota_policy.h) – ERLEDIGT
- Zeit: 2026-10-25 10:53 UTC
- Gemacht: Vorlage kopiert, `OTA_CHIP_ESP32 = 0`, Kommentar auf `test/test_ota_policy`, neu `ota_redirect_ok(url)` (nur `https://`, Host `github.com` oder Endung `.githubusercontent.com`, Groß/Klein egal, Host nur aus Buchstaben/Ziffern/`-`/`.`).
- Dateien: src/logic/ota_policy.h, test/test_ota_policy/test_main.cpp
- Prüfung: `pio test -e native -f test_ota_policy` vor Implementierung → ERRORED (Header fehlt); mit unveränderter Vorlage → SIGSEGV; nach Korrektur → 5 Tests, 0 Fehler
- Gegenprobe: Chip-Konstante 0 → 9 → `test_image_ok_esp32: Expected 0 Was 3` und `test_image_rejects` FAILED; zurück → 5 PASS
- Nächster Schritt: Task 7 Step 1
- Offen/Fragen: **Abweichung von der Vorlage:** `OTA_IMG_CHECK_BYTES` war 176, `ota_image_check` liest die ELF-SHA aber aus Byte 176…207 (`32 + 144 + i`). Mit genau 176 Byte Puffer (wie im Test) liest es über das Ende hinaus → Absturz. Jetzt `32 + 176 = 208`. Dasselbe steckt in der Vorlage `ota-publish.mjs` (`CHECK_BYTES = 176`, liest bis 208) und betrifft vermutlich auch den HA-Buddy (dort harmlos, weil der Puffer größer ist). In Task 13 im Skript ebenfalls auf 208 gesetzt.

## Task 5 – Spiel-Logik (simon.h, catch_game.h) – ERLEDIGT
- Zeit: 2026-10-25 10:46 UTC
- Gemacht: Test aus dem Plan wörtlich; `src/logic/simon.h` und `src/logic/catch_game.h` nach den Interfaces im Plan (Zufall über `rng_range` aus eye_model.h).
- Dateien: src/logic/simon.h, src/logic/catch_game.h, test/test_games/test_main.cpp
- Prüfung: `pio test -e native -f test_games` vor Implementierung → ERRORED (Header fehlt); danach → 7 Tests, 0 Fehler
- Gegenprobe: Gewinnprüfung in `simon_press` auf `len == 5` → `test_simon_win: Expected 0 Was 2 [FAILED]`; zurück → 7 PASS
- Nächster Schritt: Task 6 Step 1
- Offen/Fragen: –

## Task 4 – Wetter-Laune und Overlays (weather_mood.h) – ERLEDIGT
- Zeit: 2026-10-25 10:42 UTC
- Gemacht: Test aus dem Plan wörtlich; `src/logic/weather_mood.h` mit WMO-Zuordnung, Flags hot/cold, `weather_fresh` (< 3 h), `overlays_for`.
- Dateien: src/logic/weather_mood.h, test/test_weather_mood/test_main.cpp
- Prüfung: `pio test -e native -f test_weather_mood` vor Implementierung → ERRORED (Header fehlt); danach → 4 Tests, 0 Fehler
- Gegenprobe: `> 28` → `>= 28` → `test_flags: Expected FALSE Was TRUE [FAILED]`; zurück → 4 PASS
- Nächster Schritt: Task 5 Step 1
- Offen/Fragen: –

## Task 3 – Augenmodell (eye_model.h) – ERLEDIGT
- Zeit: 2026-10-25 10:38 UTC
- Gemacht: Test aus dem Plan wörtlich; `src/logic/eye_model.h` mit Tabelle (switch), xorshift32, `eye_tween`/`rgb_tween` (k = 1 − e^(−dt/150)), Blinker und Gazer nach Plan.
- Dateien: src/logic/eye_model.h, test/test_eye_model/test_main.cpp
- Prüfung: `pio test -e native -f test_eye_model` vor Implementierung → ERRORED (Header fehlt); danach → 6 Tests, 0 Fehler
- Gegenprobe: Blinzelabstand 2000–6000 → 7000–9000 → `test_blinker_closes_within_window` und `test_blinker_reopens` FAILED; zurück → 6 PASS
- Nächster Schritt: Task 4 Step 1
- Offen/Fragen: –

## Task 2 – Gesten (gesture.h) – ERLEDIGT
- Zeit: 2026-10-25 10:33 UTC
- Gemacht: Test aus dem Plan wörtlich, `src/logic/gesture.h` nach den Regeln im Plan (Tipp-Ringpuffer der letzten 5, Streichel-Fenster 1,5 s, Bewegungsschwelle 15 px).
- Dateien: src/logic/gesture.h, test/test_gesture/test_main.cpp
- Prüfung: `pio test -e native -f test_gesture` vor Implementierung → ERRORED (Header fehlt); danach → 7 Tests, 0 Fehler
- Gegenprobe: `GESTURE_STROKE_PX` 60 → 200 → `test_stroke: Expected 2 Was 0 [FAILED]`; zurück → 7 PASS
- Nächster Schritt: Task 3 Step 1
- Offen/Fragen: –

## Task 1 – Stimmung (mood.h) – ERLEDIGT
- Zeit: 2026-10-25 10:28 UTC
- Gemacht: Test aus dem Plan wörtlich übernommen, `src/logic/mood.h` exakt nach Plan.
- Dateien: src/logic/mood.h, test/test_mood/test_main.cpp
- Prüfung: `pio test -e native -f test_mood` vor Implementierung → ERRORED (`logic/mood.h: No such file`); danach → 13 Tests, 0 Fehler
- Gegenprobe: `+= 8` → `+= 7` in STROKE → `test_stroke_cooldown: Expected 58 Was 57 [FAILED]`; zurückgeändert → 13 PASS
- Nächster Schritt: Task 2 Step 1
- Offen/Fragen: –

## Task 0 – Werkzeuge (Cloud) – ERLEDIGT mit Umweg
- Zeit: 2026-10-25 10:25 UTC
- Gemacht: PlatformIO 6.2.0 per pip. Die PlatformIO-Registry (`api.registry.platformio.org`, `collector.platformio.org`) ist im Cloud-Netz gesperrt (403), GitHub geht. Umweg nur außerhalb des Repos: Proxy-CA an `~/.platformio/penv/.../certifi/cacert.pem` angehängt (pioarduino überschreibt `REQUESTS_CA_BUNDLE`), `tool-scons` 4.11.1 aus PyPI nach `~/.platformio/packages/tool-scons` gelegt, `platform-native` aus GitHub nach `~/.platformio/platforms/native`, Bibliotheken per `git clone` mit `.piopm` nach `.pio/libdeps/` (LovyanGFX 1.2.34, WiFiManager 2.0.17, ArduinoJson 7.4.3, Unity 2.6.1; bei Unity Kommentarzeilen aus `library.json` entfernt). `platformio.ini` unverändert.
- Dateien: keine im Repo
- Prüfung: `pio test -e native -f test_smoke` → 3 Tests, 0 Fehler. `pio run -e cyd2usb` → SUCCESS, RAM 10,4 % (33 984 B), Flash 25,2 % (494 623 B von 1 966 080 B)
- Gegenprobe: –
- Nächster Schritt: Task 1 Step 1
- Offen/Fragen: Lokal mit normalem Internet ist kein Umweg nötig. Branch: Die Cloud-Sitzung darf nur auf `claude/dreamy-galileo-bvdw39` pushen, nicht auf `buddy-umsetzung` (Vorgabe der Sitzung). Der lokale Claude übernimmt also `claude/dreamy-galileo-bvdw39` (ggf. lokal in `buddy-umsetzung` umbenennen).

## Task 0 – Gerüst und Testbild – ERLEDIGT (lokal)
- Zeit: 2026-10-25
- Gemacht: PlatformIO-Gerüst (cyd2usb + native), Pins/Display-Klasse, Testbild, Smoke-Test, OTA-Vorlagen aus dem HA-Buddy unter reference/.
- Dateien: platformio.ini, src/config.h, src/hw/display.h, src/main.cpp, src/logic/clamp.h, test/test_smoke/, reference/ha_buddy_ota/
- Prüfung: siehe Commit „chore: Gerüst“ (Build und Smoke-Test lokal)
- Gegenprobe: Smoke-Test mit falschem Erwartungswert schlägt fehl
- Nächster Schritt: Task 1 Step 1
- Offen/Fragen: Panel-Variante, Touch-Kalibrierung, LDR-Richtung/-Schwelle misst der lokale Claude am Gerät.
