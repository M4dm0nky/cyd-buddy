#!/usr/bin/env node
// Erzeugt src/logic/phrases.h aus docs/superpowers/specs/2026-10-25-saetze.json (Sätze für die Sprechblasen).
//
//   node tools/gen-phrases.mjs          Header neu schreiben
//   node tools/gen-phrases.mjs --check  nur prüfen, ob der eingecheckte Header aktuell ist (Exit 1 sonst)
//
// Die erzeugte Datei wird mit committet: das Gerät baut ohne Node. Nur Node-Standardbibliothek.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
export const SOURCE = 'docs/superpowers/specs/2026-10-25-saetze.json';
export const TARGET = 'src/logic/phrases.h';

const cstr = (s) => '"' + s.replace(/\\/g, '\\\\').replace(/"/g, '\\"') + '"';

// obj: { kategorie: [Sätze], ... }; Schlüssel mit "_" am Anfang (z. B. _quelle) sind Kommentare.
export function generate(obj) {
  const cats = Object.keys(obj).filter((k) => !k.startsWith('_'));
  for (const k of cats) {
    if (!/^[a-z][a-z0-9_]*$/.test(k)) throw new Error(`Name "${k}" taugt nicht als Bezeichner`);
    if (!Array.isArray(obj[k]) || obj[k].length === 0) throw new Error(`Kategorie "${k}" ist leer`);
    if (obj[k].length > 255) throw new Error(`Kategorie "${k}" hat zu viele Sätze`);
  }
  const up = (k) => k.toUpperCase();
  const out = [];
  out.push('#pragma once');
  out.push('#include <stdint.h>');
  out.push('');
  out.push(`// ERZEUGT von tools/gen-phrases.mjs aus ${SOURCE} – nicht von Hand ändern.`);
  out.push('// Sätze in UTF-8; vor dem Zeichnen über logic/ascii.h wandeln. Host-Test test/test_phrases.');
  out.push(`enum class Say : uint8_t { ${[...cats.map(up), 'COUNT'].join(', ')} };`);
  out.push('');
  out.push('namespace phrases_detail {');
  for (const k of cats) out.push(`inline const char *const ${up(k)}[] = {${obj[k].map(cstr).join(', ')}};`);
  out.push('}  // namespace phrases_detail');
  out.push('');
  out.push('struct SayList { const char *const *items; uint8_t n; };');
  out.push('inline SayList say_list(Say s) {');
  out.push('  using namespace phrases_detail;');
  out.push('  switch (s) {');
  for (const k of cats) out.push(`    case Say::${up(k)}: return SayList{${up(k)}, ${obj[k].length}};`);
  out.push('    default: return SayList{nullptr, 0};');
  out.push('  }');
  out.push('}');
  return out.join('\n') + '\n';
}

function main() {
  const json = JSON.parse(fs.readFileSync(path.join(ROOT, SOURCE), 'utf8'));
  const h = generate(json);
  const target = path.join(ROOT, TARGET);
  if (process.argv.includes('--check')) {
    const cur = fs.existsSync(target) ? fs.readFileSync(target, 'utf8') : '';
    if (cur !== h) { console.error(`${TARGET} ist veraltet: node tools/gen-phrases.mjs`); process.exit(1); }
    console.log(`${TARGET} ist aktuell.`);
    return;
  }
  fs.writeFileSync(target, h);
  console.log(`${TARGET} geschrieben (${Object.keys(json).filter((k) => !k.startsWith('_')).length} Kategorien).`);
}

if (process.argv[1] && import.meta.url === pathToFileURL(path.resolve(process.argv[1])).href) main();
