# CYD-Buddy – Claude-Modus (Design)

Stand 2026-10-25, freigegeben vom Nutzer. Der Buddy zeigt, was Claude Code gerade tut (alle Projekte auf dem Windows-PC des Nutzers). Grundlage Hooks: https://code.claude.com/docs/en/hooks (Ereignisse u. a. UserPromptSubmit, PreToolUse, PostToolUse, PostToolUseFailure, PermissionRequest, Notification mit `notification_type`, SubagentStart/Stop, Stop, SessionStart/End; Handler `async: true` blockiert nicht).

## Zustände
`enum class ClaudeState : uint8_t { OFF, IDLE, LISTENING, THINKING, READING, CODING, RUNNING, FAILED, NEEDS_YOU, DONE };`
`IDLE` = Sitzung aktiv, aber gerade nichts zu zeigen (nach DONE): normales Pet-Gesicht, Claude-Symbol sichtbar, Pet-Eigenarten wieder erlaubt.

| Hook (PC) | Zustand | Gesicht / Zeichen am Buddy | Dauer |
|---|---|---|---|
| UserPromptSubmit | LISTENING | SURPRISED 1,2 s, Blase „Okay!“ (bzw. „Bin dran!“), dann THINKING | – |
| PostToolUse, SubagentStop, SessionStart | THINKING | neue Emotion **THINKING** (Pupillen oben links, Braue asymmetrisch, kleiner Mund), alle 20–40 s Blase „Hmm...“ | bis nächstes Ereignis |
| PreToolUse Read/Grep/Glob/WebSearch/WebFetch/ToolSearch/LS | READING | THINKING-Gesicht, Pupillen springen alle 250 ms links↔rechts | dto. |
| PreToolUse Edit/Write/NotebookEdit/MultiEdit | CODING | FOCUSED, Overlay „Tipp-Punkte“ (drei Punkte unter dem Mund, wandernd) | dto. |
| PreToolUse Bash/PowerShell (sonstige Werkzeuge ebenso) | RUNNING | FOCUSED + drehendes Ring-Segment am Rand (vorhandener Pomodoro-Ring, Farbe Claude-Orange 217,119,87) | dto. |
| PostToolUseFailure (ohne `is_interrupt`) | FAILED | SCARED 3 s, Blase „Mist!“/„Huch, Fehler!“, danach THINKING | 3 s |
| PermissionRequest; PreToolUse AskUserQuestion; Notification `permission_prompt`, `idle_prompt`, `elicitation_dialog`, `agent_needs_input` | NEEDS_YOU | Blick gerade auf den Betrachter (Pupillen Mitte, pupil_r 18), Brauen hoch, LED gelb blinkend (1 Hz), Blase „Hey, du!“/„Ich brauch dich!“ alle 30 s wiederholt | bis nächstes Ereignis, max. 30 min; Tippen stoppt nur das LED-Blinken |
| Stop | DONE | EXCITED + OV_HAND, Blase „Fertig!“ | 8 s, danach Pet-Modus mit Claude-Symbol |
| SessionEnd | OFF | – | – |
| SubagentStart/SubagentStop | (Zähler) | Statusstreifen: Helfer-Symbol + Anzahl laufender Helfer | – |

- Kein Ereignis seit 15 min → OFF (normaler Pet-Modus). Während Claude-Zuständen (außer OFF) laufen Bedürfnisse, Streicheln, Füttern, Menü normal; das angezeigte Gefühl kommt aus dem Claude-Zustand, kurzzeitige Gefühle aus Berührung (LOVE, ANNOYED, GIGGLE …) haben 3 s Vorrang. Keine Eigenarten, keine Pet-Sprechblasen, kein Nachtschlaf, solange nicht OFF. Pomodoro (FOCUSED) hat Vorrang vor Claude-Gesichtern, NEEDS_YOU-LED und -Blase gelten trotzdem.
- Mehrere Sitzungen: das jüngste Ereignis gewinnt, außer NEEDS_YOU einer anderen Sitzung (`sid`) ist offen – das bleibt, bis von dieser Sitzung ein neues Ereignis kommt.
- Statusstreifen: Claude-Symbol (kleiner orangefarbener Stern/Funke) solange nicht OFF.

## Schnittstelle Buddy (`src/net/claude_api.*`)
- `WebServer` (Arduino-Core) Port 80, mDNS-Name `cyd-buddy` (`cyd-buddy.local`), Start nach WLAN-Verbindung; `handleClient()` aus dem Hauptschleifen-Takt (nicht im net_task, damit keine Thread-Probleme mit dem Zustand).
- `POST /claude`, Header `X-Buddy-Key: <Schlüssel>`, Körper JSON ≤ 256 B: `{"e":"<Ereignis>","t":"<Werkzeug>","n":"<notification_type>","sid":"<8 Zeichen>","i":<0|1 is_interrupt>}`. Antwort 204; falscher/fehlender Schlüssel 401; Körper zu groß 413; JSON fehlerhaft 400. `GET /claude/ping` mit Schlüssel → 200 `{"ok":true,"state":"…"}` (für die Einrichtung).
- **Schlüssel:** beim ersten Start 12 Hex-Zeichen aus `esp_random()`, im NVS (`ckey`, nicht in `Persist`-Migration verlieren), angezeigt in Einstellungen → Info und seriell beim Start (`claude: Schlüssel …`). Kein Schlüssel im Quelltext.
- Zuordnung Ereignis → Zustand als reine Logik in `src/logic/claude_state.h` (testbar): `void claude_event(ClaudeLogic&, const char *e, const char *tool, const char *ntype, const char *sid, bool interrupt, uint32_t now)`, `ClaudeState claude_state(const ClaudeLogic&, uint32_t now)`, `uint8_t claude_helpers(const ClaudeLogic&)`, `bool claude_needs_you(...)`, `bool claude_active(...)`.

## PC-Seite (macht der lokale Claude, nicht die Cloud)
- `C:\Users\madma\.claude\hooks\buddy-status.mjs`: liest Hook-JSON von stdin, baut den Kurzkörper, `fetch` mit `AbortSignal.timeout(800)`, beendet sich immer mit Exit 0 (auch ohne Netz/Buddy). Konfiguration `buddy-status.json` daneben (`host`, `key`); fehlt sie → sofort Exit 0. Fallback-Host: IP aus der Konfiguration, falls `cyd-buddy.local` nicht auflöst.
- Globale `~/.claude/settings.json`: Hooks für die Ereignisse der Tabelle, `type: command`, `command: node "C:/Users/madma/.claude/hooks/buddy-status.mjs"`, `async: true`. Einrichtung über den Skill `update-config`, vorher Sicherung der Datei.
- Kopie des Skripts im Repo unter `tools/claude-hooks/` (ohne Schlüssel) mit Kurzanleitung.

## Prüfung
- Unity-Tests für `claude_state.h` (jede Tabellenzeile, 15-min-Timeout, NEEDS_YOU-Vorrang über Sitzungen, Helferzähler, FAILED → THINKING nach 3 s, DONE → Pet nach 8 s).
- Node-Test für die Abbildung Hook-JSON → Kurzkörper (`tools/claude-hooks/buddy-status.test.mjs`).
- Am Gerät: `curl` mit Schlüssel → 204, ohne → 401; danach echte Claude-Sitzung: Gesicht folgt den Phasen, NEEDS_YOU bei einer Freigabe-Abfrage.
