# Claude-Modus: Hook-Skript für den Buddy

`buddy-status.mjs` wird von Claude Code bei jedem Ereignis aufgerufen und
schickt den Arbeitszustand (Lesen, Schreiben, Fehler, Rückfrage …) an den
CYD-Buddy. Das Skript braucht nur Node ≥ 20 und beendet sich immer mit Exit 0,
auch wenn der Buddy nicht erreichbar ist.

## Einrichtung

1. Skript kopieren: `buddy-status.mjs` nach `%USERPROFILE%\.claude\hooks\`.
2. Konfiguration anlegen: `buddy-status.example.json` als
   `%USERPROFILE%\.claude\hooks\buddy-status.json` kopieren und ausfüllen:
   - `host`: `cyd-buddy.local`
   - `key`: Schlüssel aus dem Buddy unter Einstellungen → Info
   - `fallbackHost` (optional): IP des Buddy, falls `cyd-buddy.local` nicht auflöst
3. Hooks in `%USERPROFILE%\.claude\settings.json` eintragen (vorher sichern),
   Block unten einfügen. Bestehende Einträge bleiben erhalten, nur die
   `hooks`-Abschnitte ergänzen.

Die Konfiguration darf nicht ins Repo. Im Repo liegt nur die Vorlage mit Platzhaltern.

## Hook-Block für settings.json

```json
{
  "hooks": {
    "UserPromptSubmit": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "PreToolUse": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "PostToolUse": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "PostToolUseFailure": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "PermissionRequest": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "Notification": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "SubagentStart": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "SubagentStop": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "Stop": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "SessionStart": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }],
    "SessionEnd": [{ "hooks": [{ "type": "command", "command": "node \"C:/Users/madma/.claude/hooks/buddy-status.mjs\"", "async": true }] }]
  }
}
```

## Prüfen

- Tests im Repo: `node --test tools/claude-hooks/buddy-status.test.mjs`
- Am Buddy: `curl -H "X-Buddy-Key: <Schlüssel>" http://cyd-buddy.local/claude/ping` → `200` (ohne Schlüssel `401`).
- Debug: Umgebungsvariable `BUDDY_DEBUG=1` schreibt Fehler auf stderr.
