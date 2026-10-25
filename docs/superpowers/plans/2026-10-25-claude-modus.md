# Claude-Modus Implementation Plan

> **For agentic workers:** TDD (superpowers:test-driven-development) für `src/logic/claude_state.h` und das Node-Skript. Ein Commit pro Task, Trailer mit eigenem Modell, nicht pushen.

**Goal:** Buddy zeigt den Arbeitszustand von Claude Code laut `docs/superpowers/specs/2026-10-25-claude-modus-design.md`.
**Architecture:** Reine Zustandslogik (`src/logic/claude_state.h`, Unity) + kleiner HTTP-Empfänger (`src/net/claude_api.*`, Arduino `WebServer` + `ESPmDNS`) + Darstellung (Gesicht/Overlays/Statusstreifen) + PC-Hook-Skript (`tools/claude-hooks/buddy-status.mjs`, Node ≥ 20, nur Standardbibliothek).

## Global Constraints
- Werkzeuge nur über das PowerShell-Tool; pio `& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe"`, Ausgaben nicht mit `Select-Object -First` kürzen; Tests mit gcc-PATH wie in früheren Plänen.
- Kein Schlüssel im Quelltext; Schlüssel im NVS (`ckey`), unabhängig von `Persist`-Versionen.
- HTTP nur `handleClient()` aus der Hauptschleife; Körper > 256 B ablehnen (413), Header `X-Buddy-Key` vergleichen (Länge + Inhalt).
- Bildrate ≥ 25 fps bleibt; keine Allokation pro Anfrage über ArduinoJson-Standard hinaus (Filter/kleines Dokument).
- Nicht ändern: `src/ota/*`, `ota_policy.h`, `tools/ota-publish.mjs`, `display.h`, `config.h`, Touch-Kalibrierung.

### Task 1: `src/logic/claude_state.h` + `test/test_claude_state`
Tests zuerst für jede Zeile der Zustandstabelle der Spec (Werkzeugnamen-Gruppen), Timeout 15 min → OFF, FAILED → THINKING nach 3 s, LISTENING → THINKING nach 1,2 s, DONE → nach 8 s Zustand `IDLE` (Pet-Gesicht, `claude_active` bleibt wahr bis 15 min nach dem letzten Ereignis, Claude-Symbol sichtbar); NEEDS_YOU-Vorrang über andere Sitzungen, Helferzähler (Start +1, Stop −1, nie < 0, Reset bei SessionEnd/Timeout), `is_interrupt` → kein FAILED. Neue Emotion `THINKING` + Werte in `face_expression` (+ Test). Commit `feat: Claude-Modus Zustandslogik`.

### Task 2: Empfänger `src/net/claude_api.*`
`claude_api_begin()` nach WLAN (mDNS `cyd-buddy`, WebServer Port 80), `claude_api_loop()` aus `app_loop`, Schlüssel erzeugen/laden (`ckey`), Info-Anzeige + serielle Ausgabe, Endpunkte `POST /claude` und `GET /claude/ping` laut Spec, Übergabe an `claude_event`. Build. Commit `feat: Claude-Modus HTTP-Empfänger`.

### Task 3: Darstellung
`app.cpp`: angezeigtes Gefühl aus `claude_state` (Regeln der Spec: Berührungs-Gefühle 3 s Vorrang, Pomodoro Vorrang, keine Eigenarten/Pet-Blasen/Nachtschlaf solange aktiv), Pupillen-Springen (READING), Tipp-Punkte-Overlay (CODING, neues Bit `OV_TYPING`), Ring-Segment drehend (RUNNING), Blick + LED gelb 1 Hz + wiederholte Blase (NEEDS_YOU; Tippen stoppt LED), DONE mit OV_HAND + „Fertig!“, Claude-Symbol + Helferzahl im Statusstreifen. Sätze: in `saetze.json` neue Kategorien `claude_okay`, `claude_hmm`, `claude_fehler`, `claude_hey`, `claude_fertig` (je ≥ 4, ≤ 28 Zeichen) ergänzen und `node tools/gen-phrases.mjs` ausführen. Build, fps notieren. Commit `feat: Claude-Modus Darstellung`.

### Task 4: PC-Skript (Datei im Repo, Einrichtung macht der Orchestrator)
`tools/claude-hooks/buddy-status.mjs` (stdin-JSON → `{e,t,n,sid,i}`; `sid` = erste 8 Zeichen von `session_id`; `i` aus `is_interrupt`; Konfiguration aus `buddy-status.json` im selben Ordner wie das Skript oder `%USERPROFILE%\.claude\hooks\buddy-status.json`; `fetch` POST mit Timeout 800 ms; immer Exit 0, nie etwas auf stdout), `tools/claude-hooks/buddy-status.test.mjs` (Abbildung, fehlende Konfiguration → stilles Ende, Timeout → Exit 0 mit einem lokalen Server, der nicht antwortet), `tools/claude-hooks/README.md` (Einrichtung: Skript kopieren, `buddy-status.json` mit Host/Schlüssel, Hook-Block für `settings.json`). Commit `feat: Claude-Modus Hook-Skript`.
