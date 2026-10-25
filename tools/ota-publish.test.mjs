import test from 'node:test';
import assert from 'node:assert/strict';
import { inspectImage } from './ota-publish.mjs';

function img(chip, proj) {
  const b = Buffer.alloc(300);
  b[0] = 0xe9; b.writeUInt16LE(chip, 12); b.writeUInt32LE(0xabcd5432, 32);
  b.write(proj, 32 + 48, 'latin1');
  for (let i = 0; i < 32; i++) b[32 + 144 + i] = i;
  return b;
}
test('ESP32-Image wird erkannt', () => {
  const r = inspectImage(img(0, 'cyd'), 'cyd');
  assert.equal(r.error, undefined);
  assert.equal(r.elf, '000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f');
});
test('S3-Image wird abgelehnt', () => assert.match(inspectImage(img(9, 'cyd'), 'cyd').error, /falscher Chip/));
test('falsches Projekt', () => assert.match(inspectImage(img(0, 'x'), 'cyd').error, /falsches Projekt/));
