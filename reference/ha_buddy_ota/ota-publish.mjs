#!/usr/bin/env node
// Veröffentlicht eine gebaute 7"-Firmware für den Abruf-Update des Geräts (docs/ota-7in.md).
//
//   node tools/ota-publish.mjs                      prüfen + nach ota-out/ schreiben (neueste HA_Buddy_7in/build/*.ino.bin)
//   node tools/ota-publish.mjs --bin pfad.bin       bestimmte Datei
//   node tools/ota-publish.mjs --dest Z:\ha_buddy   in diesen Ordner schreiben (z. B. Freigabe \\homeassistant\media\ha_buddy)
//   node tools/ota-publish.mjs --ha                 zusätzlich in HAs Medienordner ha_buddy hochladen (Zugang aus secrets.h)
//   node tools/ota-publish.mjs --check              nur prüfen und anzeigen, nichts schreiben
//   node tools/ota-publish.mjs --serve [8765]       ota-out/ mit Anmeldung (Bearer) im LAN anbieten (SECRET_OTA_BASE_URL/TOKEN am Gerät)
//
// Das Gerät stimmt am Display zu und holt dann manifest.json + firmware.bin. Dieses Skript schreibt NIE auf das Gerät.
// Nur Node-Standardbibliothek. Die .bin enthält WLAN-Zugang und HA-Token: nie ohne Anmeldung ablegen, nie in ein Release hängen.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import http from 'node:http';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const SKETCH = path.join(ROOT, 'HA_Buddy_7in');
// Projektname im Image: das Waveshare-Boardpaket (esp32-XIP) baut mit "arduino-lib-builder", nicht mit dem Sketchnamen.
const PROJECT = 'arduino-lib-builder';
const CHECK_BYTES = 176;
// HAs Medienordner liefert nur Audio/Video/Bild aus (nach Endung, mit Anmeldung): die beiden Dateien heißen deshalb .png, der Inhalt bleibt unverändert.
const MANIFEST_NAME = 'manifest.png';
const FIRMWARE_NAME = 'firmware.png';

const argv = process.argv.slice(2);
const flag = (n) => argv.includes(n);
const opt = (n) => { const i = argv.indexOf(n); return i >= 0 && argv[i + 1] && !argv[i + 1].startsWith('--') ? argv[i + 1] : null; };
const die = (m) => { console.error('FEHLER: ' + m); process.exit(1); };

// --- Image prüfen (dieselben Offsets wie src/logic/ota_policy.h) ---
export function inspectImage(buf, project = PROJECT) {
  if (buf.length < CHECK_BYTES) return { error: 'Datei zu kurz' };
  if (buf[0] !== 0xe9) return { error: 'kein Firmware-Image (Magic 0xE9 fehlt; merged/bootloader-Datei?)' };
  if (buf.readUInt16LE(12) !== 9) return { error: 'falscher Chip (erwartet ESP32-S3)' };
  if (buf.readUInt32LE(32) !== 0xabcd5432) return { error: 'keine App-Kennung (esp_app_desc_t)' };
  const str = (o, n) => buf.subarray(o, o + n).toString('latin1').replace(/\0.*$/s, '');
  const name = str(32 + 48, 32);
  if (name !== project) return { error: `falsches Projekt "${name}" (erwartet "${project}")` };
  return {
    project: name,
    version: str(32 + 16, 32),
    time: str(32 + 80, 16),
    date: str(32 + 96, 16),
    idf: str(32 + 112, 32),
    elf: buf.subarray(32 + 144, 32 + 176).toString('hex'),
  };
}

function newestBin() {
  const dirs = [path.join(SKETCH, 'build')];
  let best = null;
  const walk = (d, depth) => {
    if (depth > 3 || !fs.existsSync(d)) return;
    for (const e of fs.readdirSync(d, { withFileTypes: true })) {
      const p = path.join(d, e.name);
      if (e.isDirectory()) walk(p, depth + 1);
      else if (e.name.endsWith('.ino.bin') && !e.name.includes('merged')) {
        const t = fs.statSync(p).mtimeMs;
        if (!best || t > best.t) best = { p, t };
      }
    }
  };
  dirs.forEach((d) => walk(d, 0));
  return best && best.p;
}

function newestSource() {
  let t = 0;
  const walk = (d) => {
    for (const e of fs.readdirSync(d, { withFileTypes: true })) {
      if (['build', 'logs', 'node_modules'].includes(e.name)) continue;
      const p = path.join(d, e.name);
      if (e.isDirectory()) walk(p);
      else if (/\.(ino|cpp|c|h|hpp)$/.test(e.name) && e.name !== 'secrets.h') t = Math.max(t, fs.statSync(p).mtimeMs);
    }
  };
  walk(SKETCH);
  return t;
}

function secretToken() {
  try {
    const m = fs.readFileSync(path.join(SKETCH, 'src/config/secrets.h'), 'utf8').match(/^\s*#define\s+SECRET_OTA_TOKEN\s+"([^"]+)"/m);
    return m ? m[1] : null;
  } catch { return null; }
}

function serve(dir, port) {
  const token = opt('--token') || process.env.OTA_TOKEN || secretToken();
  if (!token) die('kein Token: --token, Umgebungsvariable OTA_TOKEN oder SECRET_OTA_TOKEN in secrets.h setzen (z. B. 64 Hex-Zeichen: node -e "console.log(require(\'crypto\').randomBytes(32).toString(\'hex\'))")');
  const want = Buffer.from('Bearer ' + token);
  const srv = http.createServer((req, res) => {
    const got = Buffer.from(req.headers.authorization || '');
    const ok = got.length === want.length && crypto.timingSafeEqual(got, want);
    if (!ok) { res.writeHead(401); return res.end(); }
    const name = decodeURIComponent((req.url || '').split('?')[0]).replace(/^\/+/, '');
    if (req.method !== 'GET' || ![MANIFEST_NAME, FIRMWARE_NAME].includes(name)) { res.writeHead(404); return res.end(); }
    const f = path.join(dir, name);
    if (!fs.existsSync(f)) { res.writeHead(404); return res.end(); }
    res.writeHead(200, { 'Content-Length': fs.statSync(f).size, 'Content-Type': 'application/octet-stream' });
    fs.createReadStream(f).pipe(res);
  });
  srv.listen(port, '0.0.0.0', () => console.log(`Biete ${dir} an auf Port ${port} (nur ${MANIFEST_NAME} + ${FIRMWARE_NAME}, nur mit Bearer-Token). Strg+C beendet.`));
}

// Lädt die beiden Dateien in HAs Medienordner ha_buddy (media_source/local_source/upload), Zugang aus secrets.h. Erst die Firmware, zuletzt das Manifest.
async function uploadHa(dest) {
  const sec = fs.readFileSync(path.join(SKETCH, 'src', 'config', 'secrets.h'), 'utf8');
  const g = (n) => (sec.match(new RegExp('#define\\s+' + n + '\\s+"?([^"\\s]+)"?')) || [])[1];
  const host = g('SECRET_HA_HOST'), port = g('SECRET_HA_PORT') || '8123', tok = g('SECRET_HA_TOKEN');
  if (!host || !tok) die('SECRET_HA_HOST/SECRET_HA_TOKEN nicht in secrets.h gefunden');
  for (const n of [FIRMWARE_NAME, MANIFEST_NAME]) {
    const fd = new FormData();
    fd.set('media_content_id', 'media-source://media_source/local/ha_buddy');
    fd.set('file', new Blob([fs.readFileSync(path.join(dest, n))], { type: 'image/png' }), n);
    const r = await fetch(`http://${host}:${port}/api/media_source/local_source/upload`, { method: 'POST', headers: { Authorization: 'Bearer ' + tok }, body: fd });
    if (!r.ok) die(`Upload ${n}: HTTP ${r.status}`);
    console.log(`In HA hochgeladen: ha_buddy/${n}`);
  }
}

// --- Hauptteil ---
const dest = path.resolve(opt('--dest') || path.join(ROOT, 'ota-out'));
if (flag('--serve')) {
  const p = parseInt(opt('--serve') || '8765', 10);
  if (!fs.existsSync(path.join(dest, MANIFEST_NAME))) die(`${dest} enthält noch keine ${MANIFEST_NAME}: erst veröffentlichen`);
  serve(dest, p);
} else {
  const binPath = path.resolve(opt('--bin') || newestBin() || die('keine HA_Buddy_7in.ino.bin gefunden: arduino-cli compile --fqbn <FQBN aus HA_Buddy_7in/README.md> --output-dir HA_Buddy_7in/build HA_Buddy_7in'));
  const buf = fs.readFileSync(binPath);
  const info = inspectImage(buf);
  if (info.error) die(`${binPath}: ${info.error}`);
  if (buf.length < 200 * 1024) die('Image unter 200 KB: unvollständiger Build?');
  if (buf.length > 3 * 1024 * 1024) die('Image größer als der 3-MB-Slot der Partitionstabelle');

  // Sendergates (wie bei claudescreen: das Gerät beweist, dass ein Image GÜLTIG ist; hier geht es darum, dass es das RICHTIGE ist)
  const binTime = fs.statSync(binPath).mtimeMs;
  if (!opt('--bin') && !flag('--allow-stale') && newestSource() > binTime + 2000) {
    die('Quellcode ist neuer als die .bin (veralteter Build). Neu bauen oder --allow-stale.');
  }
  let dirty = '';
  try { dirty = execFileSync('git', ['status', '--porcelain', '--', 'HA_Buddy_7in', 'tools', 'docs'], { cwd: ROOT }).toString().trim(); } catch { /* kein git */ }
  if (dirty && !flag('--allow-dirty')) die('Arbeitsverzeichnis hat uncommittete Änderungen (git status). Committen oder --allow-dirty.\n' + dirty.split('\n').slice(0, 8).join('\n'));
  let commit = 'unbekannt';
  try { commit = execFileSync('git', ['rev-parse', '--short', 'HEAD'], { cwd: ROOT }).toString().trim(); } catch { /* egal */ }

  const sha256 = crypto.createHash('sha256').update(buf).digest('hex');
  const built = `${info.date} ${info.time}`.trim();
  const manifest = { project: PROJECT, elf: info.elf, sha256, size: buf.length, file: FIRMWARE_NAME, built: built.slice(0, 23), commit };
  console.log(`Quelle:   ${binPath}`);
  console.log(`Gebaut:   ${built}  (IDF ${info.idf}, Commit ${commit})`);
  console.log(`ELF:      ${info.elf.slice(0, 16)}…  Größe ${buf.length} B  SHA-256 ${sha256.slice(0, 16)}…`);
  if (flag('--check')) { console.log('--check: nichts geschrieben.'); process.exit(0); }

  fs.mkdirSync(dest, { recursive: true });
  // Erst die .bin, zuletzt das Manifest (atomar): das Gerät sieht nie ein Manifest ohne die zugehörige Datei.
  fs.copyFileSync(binPath, path.join(dest, FIRMWARE_NAME + '.tmp'));
  fs.renameSync(path.join(dest, FIRMWARE_NAME + '.tmp'), path.join(dest, FIRMWARE_NAME));
  fs.writeFileSync(path.join(dest, MANIFEST_NAME + '.tmp'), JSON.stringify(manifest, null, 2) + '\n');
  fs.renameSync(path.join(dest, MANIFEST_NAME + '.tmp'), path.join(dest, MANIFEST_NAME));
  console.log(`Veröffentlicht in ${dest}`);
  console.log('Am Gerät erscheint innerhalb von ca. 10 min "Update bereit"; zustimmen mit "Jetzt aktualisieren".');
  if (flag('--ha')) await uploadHa(dest);
}
