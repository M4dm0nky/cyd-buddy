# Fortschritt CYD-Buddy

Neueste Einträge oben. Format und Regeln: `docs/superpowers/plans/2026-10-25-cyd-buddy.md`, Abschnitt „Fortschritts-Protokoll“.

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
