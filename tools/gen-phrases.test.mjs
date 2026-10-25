import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { generate, SOURCE, TARGET } from './gen-phrases.mjs';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

test('Enum in JSON-Reihenfolge, _quelle wird ausgelassen', () => {
  const h = generate({ _quelle: 'x', morgen: ['A'], pomodoro_start: ['B', 'C'] });
  assert.match(h, /enum class Say : uint8_t \{ MORGEN, POMODORO_START, COUNT \};/);
  assert.doesNotMatch(h, /QUELLE|_quelle/);
  assert.match(h, /"B",\s*"C"/);
});
test('Anführungszeichen und Backslash werden maskiert, UTF-8 bleibt', () => {
  const h = generate({ a: ['Er sagt "hi" \\ ok', 'Tschüß'] });
  assert.ok(h.includes('"Er sagt \\"hi\\" \\\\ ok"'));
  assert.ok(h.includes('"Tschüß"'));
});
test('ungültige Eingaben werden abgelehnt', () => {
  assert.throws(() => generate({ a: [] }), /leer/);
  assert.throws(() => generate({ 'a-b': ['x'] }), /Name/);
});
test('eingecheckte phrases.h ist aktuell', () => {
  const json = JSON.parse(fs.readFileSync(path.join(ROOT, SOURCE), 'utf8'));
  assert.equal(fs.readFileSync(path.join(ROOT, TARGET), 'utf8').replace(/\r\n/g, '\n'), generate(json));   // CRLF-Checkout unter Windows
});
