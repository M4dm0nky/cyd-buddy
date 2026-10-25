// Tests für tools/claude-hooks/buddy-status.mjs (node:test, nur Standardbibliothek).
import { test, describe } from 'node:test';
import assert from 'node:assert/strict';
import http from 'node:http';
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { toBody } from './buddy-status.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const scriptPath = path.join(here, 'buddy-status.mjs');

function tmpDir(prefix) {
  return fs.mkdtempSync(path.join(os.tmpdir(), prefix));
}

// Startet das Skript als Kindprozess. stdin = Hook-JSON, env = Umgebung.
function runScript(file, input, env) {
  return new Promise((resolve, reject) => {
    const started = Date.now();
    const child = spawn(process.execPath, [file], {
      env: { ...process.env, ...env },
      stdio: ['pipe', 'pipe', 'pipe'],
    });
    let stdout = '';
    let stderr = '';
    child.stdout.on('data', (d) => (stdout += d));
    child.stderr.on('data', (d) => (stderr += d));
    child.on('error', reject);
    child.on('close', (code) => {
      resolve({ code, stdout, stderr, ms: Date.now() - started });
    });
    child.stdin.end(input);
  });
}

describe('toBody', () => {
  test('PreToolUse Edit wird auf Kurzkörper abgebildet', () => {
    const body = toBody({
      hook_event_name: 'PreToolUse',
      tool_name: 'Edit',
      session_id: 'abcdef1234567890',
    });
    assert.deepEqual(body, { e: 'PreToolUse', t: 'Edit', n: '', sid: 'abcdef12', i: 0 });
  });

  test('Notification permission_prompt setzt n', () => {
    const body = toBody({
      hook_event_name: 'Notification',
      notification_type: 'permission_prompt',
      session_id: 'deadbeefcafe0000',
    });
    assert.deepEqual(body, {
      e: 'Notification',
      t: '',
      n: 'permission_prompt',
      sid: 'deadbeef',
      i: 0,
    });
  });

  test('PostToolUseFailure mit is_interrupt true setzt i = 1', () => {
    const body = toBody({
      hook_event_name: 'PostToolUseFailure',
      tool_name: 'Bash',
      session_id: '0123456789abcdef',
      is_interrupt: true,
    });
    assert.deepEqual(body, { e: 'PostToolUseFailure', t: 'Bash', n: '', sid: '01234567', i: 1 });
  });

  test('is_interrupt false oder fehlend ergibt i = 0', () => {
    assert.equal(toBody({ hook_event_name: 'Stop', is_interrupt: false }).i, 0);
    assert.equal(toBody({ hook_event_name: 'Stop' }).i, 0);
  });

  test('fehlende Felder ergeben leere Strings und i = 0', () => {
    assert.deepEqual(toBody({}), { e: '', t: '', n: '', sid: '', i: 0 });
    assert.deepEqual(toBody(undefined), { e: '', t: '', n: '', sid: '', i: 0 });
  });

  test('sid wird auf 8 Zeichen gekürzt', () => {
    assert.equal(toBody({ session_id: '1234567890' }).sid.length, 8);
    assert.equal(toBody({ session_id: 'abc' }).sid, 'abc');
  });

  test('t und n werden auf je 40 Zeichen gekürzt', () => {
    const long = 'x'.repeat(200);
    const body = toBody({ tool_name: long, notification_type: long });
    assert.equal(body.t.length, 40);
    assert.equal(body.n.length, 40);
  });

  test('JSON-Körper bleibt bei extrem langen Feldern unter 256 Byte', () => {
    const long = 'x'.repeat(300);
    const json = JSON.stringify(
      toBody({
        hook_event_name: long,
        tool_name: long,
        notification_type: long,
        session_id: long,
        is_interrupt: true,
      }),
    );
    assert.ok(Buffer.byteLength(json, 'utf8') <= 256, `zu lang: ${Buffer.byteLength(json)} Byte`);
  });
});

describe('buddy-status.mjs als Prozess', () => {
  const hookInput = JSON.stringify({
    hook_event_name: 'PreToolUse',
    tool_name: 'Edit',
    session_id: 'abcdef1234567890',
  });

  test('ohne Konfiguration: Exit 0 und leeres stdout', async () => {
    const emptyHome = tmpDir('buddy-home-');
    const scriptDir = tmpDir('buddy-noconf-');
    const copy = path.join(scriptDir, 'buddy-status.mjs');
    fs.copyFileSync(scriptPath, copy);

    const res = await runScript(copy, hookInput, { USERPROFILE: emptyHome, BUDDY_DEBUG: '' });
    assert.equal(res.code, 0);
    assert.equal(res.stdout, '');
  });

  test('Timeout: Server antwortet nie → Exit 0 in < 3 s, stdout leer', async () => {
    const server = http.createServer(() => {
      // antwortet absichtlich nie
    });
    await new Promise((r) => server.listen(0, '127.0.0.1', r));
    const port = server.address().port;

    const home = tmpDir('buddy-home-');
    const scriptDir = tmpDir('buddy-hang-');
    const copy = path.join(scriptDir, 'buddy-status.mjs');
    fs.copyFileSync(scriptPath, copy);
    fs.writeFileSync(
      path.join(scriptDir, 'buddy-status.json'),
      JSON.stringify({ host: `127.0.0.1:${port}`, key: 'TESTKEY123' }),
    );

    try {
      const res = await runScript(copy, hookInput, { USERPROFILE: home });
      assert.equal(res.code, 0);
      assert.equal(res.stdout, '');
      assert.ok(res.ms < 3000, `dauerte ${res.ms} ms`);
    } finally {
      server.closeAllConnections?.();
      server.close();
    }
  });

  test('Server antwortet 204: genau ein POST mit Header und Körper', async () => {
    const received = [];
    const server = http.createServer((req, res) => {
      let body = '';
      req.on('data', (d) => (body += d));
      req.on('end', () => {
        received.push({ method: req.method, url: req.url, headers: req.headers, body });
        res.writeHead(204);
        res.end();
      });
    });
    await new Promise((r) => server.listen(0, '127.0.0.1', r));
    const port = server.address().port;

    const home = tmpDir('buddy-home-');
    const scriptDir = tmpDir('buddy-ok-');
    const copy = path.join(scriptDir, 'buddy-status.mjs');
    fs.copyFileSync(scriptPath, copy);
    fs.writeFileSync(
      path.join(scriptDir, 'buddy-status.json'),
      JSON.stringify({ host: `127.0.0.1:${port}`, key: 'TESTKEY123' }),
    );

    try {
      const res = await runScript(copy, hookInput, { USERPROFILE: home });
      assert.equal(res.code, 0);
      assert.equal(res.stdout, '');
      assert.equal(received.length, 1, `Anzahl POSTs: ${received.length}`);
      const req = received[0];
      assert.equal(req.method, 'POST');
      assert.equal(req.url, '/claude');
      assert.equal(req.headers['content-type'], 'application/json');
      assert.equal(req.headers['x-buddy-key'], 'TESTKEY123');
      assert.deepEqual(JSON.parse(req.body), {
        e: 'PreToolUse',
        t: 'Edit',
        n: '',
        sid: 'abcdef12',
        i: 0,
      });
    } finally {
      server.closeAllConnections?.();
      server.close();
    }
  });
});
