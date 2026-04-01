'use strict';

const { print, send, JournalReader, Priority, OpenFlags } = require('../lib');

let passed = 0, failed = 0;

function test(name, fn) {
  try { fn(); console.log(`  ✓ ${name}`); passed++; }
  catch (e) { console.error(`  ✗ ${name}: ${e.message}`); failed++; }
}

// ── Writer ────────────────────────────────────────────────────────────────────
console.log('\nWriter');

test('print() info', () => {
  print(Priority.INFO, 'node-sdjournal test: print()');
});

test('send() structured', () => {
  send({
    MESSAGE:  'node-sdjournal test: send()',
    PRIORITY: String(Priority.DEBUG),
    TEST_FIELD: 'hello',
    SYSLOG_IDENTIFIER: 'node-sdjournal-test',
  });
});

// ── Reader ────────────────────────────────────────────────────────────────────
console.log('\nReader');

test('open + close', () => {
  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  r.close();
});

test('seekHead + next + getEntry', () => {
  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  r.seekHead();
  const ok = r.next();
  if (!ok) throw new Error('no entries in journal');
  const e = r.getEntry();
  if (!e.fields || !e.cursor) throw new Error('missing fields/cursor');
  if (!(e.timestamp instanceof Date)) throw new Error('timestamp not a Date');
  r.close();
});

test('addMatch + next (PRIORITY=6)', () => {
  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  r.addMatch(`PRIORITY=${Priority.INFO}`);
  r.seekHead();
  let count = 0;
  while (r.next() && count < 5) {
    const e = r.getEntry();
    if (e.fields.PRIORITY !== '6') throw new Error(`unexpected priority: ${e.fields.PRIORITY}`);
    count++;
  }
  r.flushMatches();
  r.close();
});

test('seekTail + previous', () => {
  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  r.seekTail();
  const ok = r.previous();
  if (!ok) throw new Error('no entries');
  r.getEntry();
  r.close();
});

test('seekTime (last 10s)', () => {
  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  r.seekTime(new Date(Date.now() - 10_000));
  r.next();
  r.close();
});

// ── Sonuç ─────────────────────────────────────────────────────────────────────
console.log(`\n${passed} passed, ${failed} failed\n`);
if (failed > 0) process.exit(1);
