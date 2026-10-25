#!/usr/bin/env node
// Veröffentlicht eine gebaute CYD-Buddy-Firmware für den Abruf-Update des Geräts (docs/ota.md).
//
//   node tools/ota-publish.mjs --check              nur prüfen und anzeigen (Projektname, ELF, Größe), nichts schreiben
//   node tools/ota-publish.mjs                      prüfen + nach ota-out/ schreiben (firmware.bin, zuletzt manifest.json)
//   node tools/ota-publish.mjs --release            zusätzlich als neuestes GitHub-Release von M4dm0nky/cyd-buddy veröffentlichen (gh)
//   node tools/ota-publish.mjs --bin pfad.bin       bestimmte Datei statt .pio/build/cyd2usb/firmware.bin
//
// Das Gerät stimmt am Display zu und holt dann manifest.json + firmware.bin aus dem neuesten Release. Dieses Skript schreibt NIE auf das Gerät.
// Nur Node-Standardbibliothek (und für --release die gh-CLI). Die .bin enthält keine Geheimnisse (WLAN kommt aus dem Portal).
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { execFileSync } from 'node:child_process';
import { fileURLToPath, pathToFileURL } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const BIN = path.join(ROOT, '.pio', 'build', 'cyd2usb', 'firmware.bin');
// Projektname im Image: kommt vom Boardpaket (vorgebaute Core-Bibliotheken), nicht vom Projektordner.
// Aus erstem Build per --check ermittelt (pioarduino 55.03.312-1, Core 3.3.12): "arduino-lib-builder".
const PROJECT = 'arduino-lib-builder';
const CHIP_ESP32 = 0;
const CHECK_BYTES = 32 + 176;          // bis zum Ende von app_elf_sha256 (wie OTA_IMG_CHECK_BYTES in src/logic/ota_policy.h)
const SLOT_SIZE = 1966080;             // App-Slot der Partition min_spiffs
const MANIFEST_NAME = 'manifest.json';
const FIRMWARE_NAME = 'firmware.bin';
const REPO = 'M4dm0nky/cyd-buddy';

// --- Image prüfen (dieselben Offsets wie src/logic/ota_policy.h) ---
export function inspectImage(buf, project = PROJECT) {
  if (buf.length < CHECK_BYTES) return { error: 'Datei zu kurz' };
  if (buf[0] !== 0xe9) return { error: 'kein Firmware-Image (Magic 0xE9 fehlt; merged/bootloader-Datei?)' };
  if (buf.readUInt16LE(12) !== CHIP_ESP32) return { error: 'falscher Chip (erwartet ESP32)' };
  if (buf.readUInt32LE(32) !== 0xabcd5432) return { error: 'keine App-Kennung (esp_app_desc_t)' };
  const name = imageProject(buf);
  if (name !== project) return { error: `falsches Projekt "${name}" (erwartet "${project}")` };
  const str = (o, n) => buf.subarray(o, o + n).toString('latin1').replace(/\0.*$/s, '');
  return {
    project: name,
    version: str(32 + 16, 32),
    time: str(32 + 80, 16),
    date: str(32 + 96, 16),
    idf: str(32 + 112, 32),
    elf: buf.subarray(32 + 144, 32 + 176).toString('hex'),
  };
}

// Projektname aus esp_app_desc_t, ohne weitere Prüfung (für --check).
export function imageProject(buf) {
  if (buf.length < 32 + 80) return '';
  return buf.subarray(32 + 48, 32 + 80).toString('latin1').replace(/\0.*$/s, '');
}

function newestSource() {
  let t = 0;
  const walk = (d) => {
    if (!fs.existsSync(d)) return;
    for (const e of fs.readdirSync(d, { withFileTypes: true })) {
      const p = path.join(d, e.name);
      if (e.isDirectory()) walk(p);
      else if (/\.(cpp|c|h|hpp)$/.test(e.name)) t = Math.max(t, fs.statSync(p).mtimeMs);
    }
  };
  walk(path.join(ROOT, 'src'));
  t = Math.max(t, fs.statSync(path.join(ROOT, 'platformio.ini')).mtimeMs);
  return t;
}

function stamp(d = new Date()) {
  const p = (n) => String(n).padStart(2, '0');
  return `${d.getFullYear()}${p(d.getMonth() + 1)}${p(d.getDate())}-${p(d.getHours())}${p(d.getMinutes())}`;
}

// wie __DATE__ __TIME__ ohne Sekunden, z. B. "Oct 25 2026 10:50"
function buildStamp(d) {
  const m = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'][d.getMonth()];
  const p = (n) => String(n).padStart(2, '0');
  return `${m} ${p(d.getDate())} ${d.getFullYear()} ${p(d.getHours())}:${p(d.getMinutes())}`;
}

function main() {
  const argv = process.argv.slice(2);
  const flag = (n) => argv.includes(n);
  const opt = (n) => { const i = argv.indexOf(n); return i >= 0 && argv[i + 1] && !argv[i + 1].startsWith('--') ? argv[i + 1] : null; };
  const die = (m) => { console.error('FEHLER: ' + m); process.exit(1); };

  const binPath = path.resolve(opt('--bin') || BIN);
  if (!fs.existsSync(binPath)) die(`${binPath} fehlt: erst bauen (pio run -e cyd2usb)`);
  const buf = fs.readFileSync(binPath);

  if (flag('--check')) {
    const found = imageProject(buf);
    console.log(`Projektname im Image: "${found}"${found === PROJECT ? '' : `  (Skript erwartet "${PROJECT}" -> PROJECT anpassen)`}`);
  }
  const info = inspectImage(buf, flag('--check') ? imageProject(buf) : PROJECT);
  if (info.error) die(`${binPath}: ${info.error}`);
  if (buf.length < 200 * 1024) die('Image unter 200 KB: unvollständiger Build?');
  if (buf.length > SLOT_SIZE) die(`Image (${buf.length} B) größer als der App-Slot (${SLOT_SIZE} B, min_spiffs)`);

  // Sendergates: das Gerät beweist, dass ein Image GÜLTIG ist; hier geht es darum, dass es das RICHTIGE ist.
  const binTime = fs.statSync(binPath).mtimeMs;
  if (!opt('--bin') && !flag('--allow-stale') && newestSource() > binTime + 2000) {
    die('Quellcode ist neuer als die .bin (veralteter Build). Neu bauen oder --allow-stale.');
  }
  let dirty = '';
  try { dirty = execFileSync('git', ['status', '--porcelain', '--', 'src', 'tools', 'platformio.ini'], { cwd: ROOT }).toString().trim(); } catch { /* kein git */ }
  if (dirty && !flag('--allow-dirty') && !flag('--check')) die('Arbeitsverzeichnis hat uncommittete Änderungen (git status). Committen oder --allow-dirty.\n' + dirty.split('\n').slice(0, 8).join('\n'));
  let commit = 'unbekannt';
  try { commit = execFileSync('git', ['rev-parse', '--short', 'HEAD'], { cwd: ROOT }).toString().trim(); } catch { /* egal */ }

  const sha256 = crypto.createHash('sha256').update(buf).digest('hex');
  // Datum/Zeit in esp_app_desc_t stammen aus den vorgebauten Core-Bibliotheken, nicht vom eigenen Build -> Zeit der .bin nehmen.
  const built = buildStamp(new Date(binTime));
  const manifest = { project: info.project, elf: info.elf, sha256, size: buf.length, file: FIRMWARE_NAME, built: built.slice(0, 23), commit };
  console.log(`Quelle:   ${binPath}`);
  console.log(`Gebaut:   ${built}  (IDF ${info.idf}, Commit ${commit}; Datum im Image ${info.date} ${info.time})`);
  console.log(`ELF:      ${info.elf.slice(0, 16)}…  Größe ${buf.length} B (${(100 * buf.length / SLOT_SIZE).toFixed(1)} % des Slots)  SHA-256 ${sha256.slice(0, 16)}…`);
  if (flag('--check')) { console.log('--check: nichts geschrieben.'); return; }

  const dest = path.join(ROOT, 'ota-out');
  fs.mkdirSync(dest, { recursive: true });
  // Erst die .bin, zuletzt das Manifest (atomar): es gibt nie ein Manifest ohne die zugehörige Datei.
  fs.copyFileSync(binPath, path.join(dest, FIRMWARE_NAME + '.tmp'));
  fs.renameSync(path.join(dest, FIRMWARE_NAME + '.tmp'), path.join(dest, FIRMWARE_NAME));
  fs.writeFileSync(path.join(dest, MANIFEST_NAME + '.tmp'), JSON.stringify(manifest, null, 2) + '\n');
  fs.renameSync(path.join(dest, MANIFEST_NAME + '.tmp'), path.join(dest, MANIFEST_NAME));
  console.log(`Geschrieben nach ${dest}`);

  if (flag('--release')) {
    const tag = `v${stamp()}`;
    // Tag auf genau den gebauten Commit setzen (ohne --target nähme GitHub den Stand von main). Der Commit muss gepusht sein.
    const full = execFileSync('git', ['rev-parse', 'HEAD'], { cwd: ROOT }).toString().trim();
    try { execFileSync('git', ['branch', '-r', '--contains', full], { cwd: ROOT }).toString().trim() || die('Commit ist nicht gepusht: erst git push.'); }
    catch { die('Commit ist nicht gepusht: erst git push.'); }
    execFileSync('gh', ['release', 'create', tag, path.join(dest, FIRMWARE_NAME), path.join(dest, MANIFEST_NAME),
      '--latest', '--target', full, '--repo', REPO, '--title', `Firmware ${built}`, '--notes', `Commit ${commit}`], { cwd: ROOT, stdio: 'inherit' });
    console.log(`Release ${tag} veröffentlicht. Am Gerät erscheint innerhalb von ca. 10 min "Update bereit".`);
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(path.resolve(process.argv[1])).href) main();
