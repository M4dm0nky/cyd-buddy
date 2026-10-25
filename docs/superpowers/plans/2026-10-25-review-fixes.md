# Review-Fixes Implementation Plan

> Ergebnis des Code-Reviews vom 2026-10-25 (Funde von Opus am Code bzw. an den Arduino-Core-Quellen bestätigt). Drei Pakete, je ein Agent in eigenem Worktree. **Datei-Besitz ist strikt** – nur die eigenen Dateien ändern, damit die Pakete konfliktfrei zusammengeführt werden können. TDD (superpowers:test-driven-development) für alles Testbare: zuerst ein Test, der den Fehler zeigt (FAIL), dann Fix (PASS), dann Gegenprobe.

## Global Constraints
- Werkzeuge nur über das PowerShell-Tool; pio `& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe"`; Ausgaben nie mit `Select-Object -First` kürzen; Tests mit gcc im PATH (`C:\Users\madma\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin`).
- Alle bestehenden Tests (153 nativ, 18 Node) bleiben grün. Nur Edit/Write. Ein Commit pro Fund, Trailer mit eigenem Modell, nicht pushen.
- Verhalten nur wie unten beschrieben ändern; keine Umbauten nebenbei.

## Paket A – Logik (`src/logic/*`, `test/*`) · Besitz: src/logic/mood.h, ota_policy.h, claude_state.h, gesture.h, eye_model.h, pomodoro.h, rng.h + zugehörige Tests
1. **Nickerchen weckbar** (mood.h ~133): Nickerchen nur beim *Unterschreiten* von Energie 10 einschalten (Flanke: `bool low_prev`), Berührung beendet es; erneutes Nickerchen erst, wenn Energie wieder ≥ 10 war und erneut < 10 fällt **oder** 10 min nach der letzten Berührung. Test: Energie 5, Nickerchen, TAP, danach 100× `mood_tick(20 ms)` → wach.
2. **Umleitungsprüfung** (ota_policy.h ~66): Authority = Teil bis zum ersten `/`, `?`, `#` oder Ende; enthält er `@` → ablehnen; danach optionalen `:Port` nur mit Ziffern erlauben und abtrennen. Tests: `https://github.com:@evil.com/x`, `https://github.com:x@evil.com/`, `https://github.com?@evil.com/`, `https://github.com#@evil.com/`, `https://user@github.com/` → false; `https://github.com:443/x` → true; bestehende Fälle unverändert.
3. **Claude-Reihenfolge** (claude_state.h): DONE darf nur durch UserPromptSubmit, PreToolUse, PermissionRequest, Notification(needs-Typen) oder SessionStart verlassen werden (nicht durch späte PostToolUse/SubagentStop/SubagentStart); NEEDS_YOU derselben Sitzung wird durch PostToolUse/SubagentStop/SubagentStart **nicht** aufgehoben, durch PreToolUse erst ≥ 1500 ms nach dem Auslöser (verspätete Vor-Ereignisse). Helferzähler bei UserPromptSubmit und Stop auf 0 setzen (alle Helfer gehören zur Runde). Tests je Regel.
4. **Aufräumen**: ungenutzte Felder `Blinker.closing`/`start_ms` und `Pomo.minutes` entfernen (vorher per grep sicherstellen, dass nichts außer Tests sie liest; Tests anpassen), `rng_range` bei vollem Bereich ohne Division durch 0. Nachtgrenzen: eine Funktion `mood_is_night(hour)` (22–7) für Nachtschlaf und SLEEPY (SLEEPY dann ebenfalls 22–7; bestehenden Test anpassen und im Bericht nennen).

## Paket B – Netz/OTA/Speicher (`src/net/*`, `src/ota/*`, `src/hw/store.*`, `src/main.cpp`, `src/hw/light.cpp`, `docs/ota.md`) · keine Änderung an app.cpp
1. **Körper vor Schlüssel** (claude_api.cpp): eigenen `RequestHandler` (WebServer, `canHandle`/`canRaw`/`raw` bzw. `upload`) so nutzen, dass Schlüssel (`X-Buddy-Key`) und `Content-Length ≤ 256` **vor** dem Lesen des Körpers geprüft werden; sonst 401/413 und Verbindung schließen, ohne den Körper in den Heap zu lesen. API-Namen in `~/.platformio/packages/framework-arduinoespressif32/libraries/WebServer/src` nachsehen. Am Ende im Bericht beschreiben, wie es geprüft wurde.
2. **Handler nur einmal registrieren** (claude_api.cpp): `on()`/`addHandler`/`collectHeaders` nur beim ersten Start.
3. **Rollback wirksam** (main.cpp + ota_pull.*): `bool verifyRollbackLater() { return true; }` definieren; Gesundheitsnachweis in `ota_pull` (UI-Takt + 20 s) **unabhängig vom WLAN** laufen lassen: neue Funktion `ota_pull::health_tick()` aus der Hauptschleife (Aufruf in app.cpp macht Paket C); `ota_pull::begin()` liest den Zustand weiter. Nach einem Kabel-Flash ist der Zustand UNDEFINED/VALID → nichts zu tun (Verhalten wie jetzt).
4. **Vor OTA-Neustart speichern**: in `ota_pull` einen Rückruf `void ota_pull::on_restart(void (*fn)())`, der vor `ESP.restart()` aufgerufen wird (Registrierung in app.cpp macht Paket C).
5. **Download-Zeitüberwachung** (ota_pull.cpp ~197–208): Stall-Prüfung auch, wenn `available() > 0`, aber `read() <= 0`.
6. **main.cpp**: Warten auf „Finger weg“ vor der Kalibrierung mit 10-s-Zeitlimit; Kalibrierwerte plausibilisieren (nicht alle gleich/0) sonst neu kalibrieren.
7. **Kommentare/Doku**: „Idle-Priorität“ (ota_pull.cpp), „am Gerät messen“ (light.cpp), Planreste „3./4./5./8.“ entfernen; docs/ota.md: Rollback und Speicherwerte auf den gemessenen Stand bringen, Grenzen (keine Signatur, Downgrade möglich, Projektkennung generisch) nennen.

## Paket C – App/UI (`src/app.cpp`, `src/app.h`, `src/ui/*`) · keine Änderung an logic/net/ota (nur deren neue Funktionen aufrufen: `ota_pull::health_tick()`, `ota_pull::on_restart(save_now)` – bis Paket B zusammengeführt ist, mit `#if __has_include`-freiem Weg: lege in app.cpp die Aufrufe an und deklariere sie nicht neu; der Orchestrator führt B vor C zusammen. Deshalb C erst NACH B starten.)
1. **24,8-Tage-Zeitstempel** (app.cpp: `s_touch_prio_until`, `s_hand_until`, `s_led_flash_until`, `s_bubble_until`, `s_sneeze_until` und alle weiteren `(int32_t)(x - now) > 0`-Prüfungen in app.cpp/ui): kleiner Helfer `struct Timer { uint32_t until; bool on; }` mit `arm(now, dur)`, `active(now)` (schaltet sich beim Ablauf selbst aus). Reine Logik als `src/ui/deadline.h`? → nein, als `src/logic/deadline.h` **mit Test** (Ausnahme vom Besitz: diese eine neue Datei + `test/test_deadline` gehören Paket C).
2. **Fünfter Tipp** (app.cpp:493): außerhalb von EYES zählt POKE als Tipp (`tap = TAP || (screen != EYES && POKE)`).
3. **Simon-Ausgang**: 30 s ohne Eingabe in AWAIT_INPUT → Spiel endet als LOST (in game_simon.cpp/app.cpp).
4. **Rückblick**: Tipps in den ersten 1000 ms nach dem Öffnen ignorieren (wie `kIgnoreTapsMs` der OTA-Karte).
5. **Sprite-Neuversuch**: ist `!eyes_ready()` im EYES-Bildschirm, alle 5 s `eyes_begin()` erneut versuchen; beim Start das Sprite vor `net_begin()` anlegen.
6. **OTA-Karte** nicht anbieten während Pomodoro-Arbeit oder Claude aktiv-arbeitend (State nicht OFF/IDLE); Einstellungen → Firmware bleibt immer möglich.
7. `s_manual_ota` in `open_settings()` und `go_eyes()` zurücksetzen.
8. Einstellungen: `settings_tick` zeichnet Zeilen nur bei geändertem Text neu.
9. Animationen: `now` erst ganzzahlig auf die Periode reduzieren (`now % P`), dann float (eyes.cpp sinf/fmodf-Stellen).
10. `ota_pull::health_tick()` jedes Bild aufrufen (statt nur `ui_alive()` im EYES-Bildschirm, beides behalten), `ota_pull::on_restart(save_now)` im Setup registrieren.
11. Aufräumen: `src/ui/testscreen.*` löschen (und FORTSCHRITT-Hinweis nicht ändern – Doku macht der Orchestrator), `kEyeY` entfernen, `CYD_LOG_LDR` mit `#ifndef … 0` absichern, verrutschten Kommentar über `claude_frame` korrigieren.
