// Claude-Modus: Hook-Skript für Claude Code. Schickt den Arbeitszustand an den MadBuddy.
// Liest das Hook-JSON von stdin, bildet einen Kurzkörper und sendet ihn per POST.
// Beendet sich IMMER mit Exit 0 und schreibt NIE auf stdout (Claude würde es als Hook-Antwort lesen).
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const MAX_FIELD = 40;
const TIMEOUT_MS = 800;
const debug = process.env.BUDDY_DEBUG === '1';

function clip(value, max) {
  return typeof value === 'string' ? value.slice(0, max) : '';
}

// Reine Abbildung Hook-JSON → Kurzkörper {e, t, n, sid, i}.
export function toBody(hook) {
  const h = hook && typeof hook === 'object' ? hook : {};
  return {
    e: clip(h.hook_event_name, MAX_FIELD),
    t: clip(h.tool_name, MAX_FIELD),
    n: clip(h.notification_type, MAX_FIELD),
    sid: clip(h.session_id, 8),
    i: h.is_interrupt ? 1 : 0,
  };
}

// Konfiguration: zuerst neben dem Skript, dann %USERPROFILE%\.claude\hooks\.
function loadConfig(scriptDir) {
  const home = process.env.USERPROFILE || os.homedir();
  const candidates = [
    path.join(scriptDir, 'buddy-status.json'),
    path.join(home, '.claude', 'hooks', 'buddy-status.json'),
  ];
  for (const file of candidates) {
    try {
      const cfg = JSON.parse(fs.readFileSync(file, 'utf8'));
      if (cfg && typeof cfg.host === 'string' && typeof cfg.key === 'string') return cfg;
    } catch {
      // Datei fehlt oder ist kaputt: nächste Stelle probieren
    }
  }
  return null;
}

async function post(host, key, body) {
  const res = await fetch(`http://${host}/claude`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', 'X-Buddy-Key': key },
    body,
    signal: AbortSignal.timeout(TIMEOUT_MS),
  });
  // Antwort wird nicht ausgewertet; Körper verwerfen, damit die Verbindung frei wird.
  await res.body?.cancel().catch(() => {});
}

async function readStdin() {
  const chunks = [];
  for await (const chunk of process.stdin) chunks.push(chunk);
  return Buffer.concat(chunks).toString('utf8');
}

async function main() {
  const scriptDir = path.dirname(fileURLToPath(import.meta.url));
  const cfg = loadConfig(scriptDir);
  if (!cfg) return; // ohne Konfiguration still beenden

  let hook;
  try {
    hook = JSON.parse(await readStdin());
  } catch {
    hook = {};
  }
  const body = JSON.stringify(toBody(hook));

  const hosts = [cfg.host];
  if (typeof cfg.fallbackHost === 'string' && cfg.fallbackHost) hosts.push(cfg.fallbackHost);

  for (const host of hosts) {
    try {
      await post(host, cfg.key, body);
      return;
    } catch (err) {
      if (debug) process.stderr.write(`buddy-status: ${host}: ${err?.message ?? err}\n`);
    }
  }
}

const isMain = process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href;
if (isMain) {
  main()
    .catch((err) => {
      if (debug) process.stderr.write(`buddy-status: ${err?.message ?? err}\n`);
    })
    .finally(() => {
      process.exitCode = 0;
    });
}
