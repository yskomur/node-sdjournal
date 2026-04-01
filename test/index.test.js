'use strict';

const {
  print,
  send,
  JournalReader,
  Priority,
  OpenFlags,
  FollowStart,
} = require('../lib');

let passed = 0, failed = 0;
const tests = [];

function test(name, fn) {
  tests.push({ name, fn });
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
  if (typeof e.message !== 'string') throw new Error('message not normalized');
  if (e.priority !== null && typeof e.priority !== 'number') throw new Error('priority not normalized');
  r.close();
});

test('getCursor()', () => {
  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  r.seekHead();
  if (!r.next()) throw new Error('no entries');
  const cursor = r.getCursor();
  if (!cursor || typeof cursor !== 'string') throw new Error('cursor missing');
  r.close();
});

test('readEntry() + readEntries()', () => {
  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  r.seekHead();
  const first = r.readEntry();
  if (!first) throw new Error('missing first entry');
  const rest = r.readEntries(2);
  if (!Array.isArray(rest)) throw new Error('readEntries did not return array');
  if (rest.length > 2) throw new Error('readEntries exceeded limit');
  r.close();
});

test('addMatch + next (PRIORITY=6)', () => {
  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  r.setPriority(Priority.INFO);
  r.seekHead();
  let count = 0;
  while (r.next() && count < 5) {
    const e = r.getEntry();
    if (e.fields.PRIORITY !== '6') throw new Error(`unexpected priority: ${e.fields.PRIORITY}`);
    count++;
  }
  r.clearFilters();
  r.close();
});

test('setIdentifier()', () => {
  send({
    MESSAGE: 'node-sdjournal test: setIdentifier()',
    PRIORITY: String(Priority.INFO),
    SYSLOG_IDENTIFIER: 'node-sdjournal-filter-id',
  });

  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  r.setIdentifier('node-sdjournal-filter-id');
  r.seekTail();
  let attempts = 0;
  let matched = false;
  while (attempts < 20) {
    if (r.previous()) {
      const e = r.getEntry();
      if (e.identifier === 'node-sdjournal-filter-id') {
        matched = true;
        break;
      }
    } else {
      break;
    }
    attempts++;
  }
  if (!matched) throw new Error('identifier filter did not match');
  r.close();
});

test('follow() validates start mode', async () => {
  const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
  let threw = false;
  try {
    await r.follow(() => {}, { start: 'invalid' });
  } catch (e) {
    threw = /invalid follow start/.test(e.message);
  } finally {
    r.close();
  }
  if (!threw) throw new Error('invalid start did not throw');
});

test('FollowStart exports values', () => {
  if (FollowStart.TAIL !== 'tail') throw new Error('TAIL export mismatch');
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
async function run() {
  for (const { name, fn } of tests) {
    try {
      await fn();
      console.log(`  ✓ ${name}`);
      passed++;
    } catch (e) {
      console.error(`  ✗ ${name}: ${e.message}`);
      failed++;
    }
  }

  console.log(`\n${passed} passed, ${failed} failed\n`);
  if (failed > 0) process.exit(1);
}

void run();
