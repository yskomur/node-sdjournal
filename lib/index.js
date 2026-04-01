'use strict';

const native = require('../build/Release/sdjournal');

// ── Sabitler ──────────────────────────────────────────────────────────────────

const Priority = Object.freeze({
  EMERG:   0,
  ALERT:   1,
  CRIT:    2,
  ERR:     3,
  WARNING: 4,
  NOTICE:  5,
  INFO:    6,
  DEBUG:   7,
});

const OpenFlags = Object.freeze({
  LOCAL_ONLY:   1 << 0,
  RUNTIME_ONLY: 1 << 1,
  SYSTEM:       1 << 2,
  CURRENT_USER: 1 << 3,
});

const WaitResult = Object.freeze({
  NOP:        0,
  APPEND:     1,
  INVALIDATE: 2,
});

const FollowStart = Object.freeze({
  TAIL: 'tail',
  HEAD: 'head',
  CURRENT: 'current',
});

function parsePriority(value) {
  if (value === undefined) return null;
  const parsed = Number.parseInt(String(value), 10);
  return Number.isNaN(parsed) ? null : parsed;
}

function normalizeEntry(entry) {
  const fields = entry.fields ?? {};
  const realtimeUsec = entry.realtimeUsec ?? 0;
  return {
    ...entry,
    fields,
    cursor: entry.cursor ?? null,
    realtimeUsec,
    timestamp: new Date(Number(realtimeUsec) / 1000),
    message: fields.MESSAGE ?? '',
    priority: parsePriority(fields.PRIORITY),
    identifier: fields.SYSLOG_IDENTIFIER ?? null,
    unit: fields._SYSTEMD_UNIT ?? null,
  };
}

// ── Writer ────────────────────────────────────────────────────────────────────

function print(priority, message) {
  native.print(priority, String(message));
}

function send(fields) {
  const normalized = {};
  for (const [k, v] of Object.entries(fields)) {
    normalized[k.toUpperCase()] = String(v);
  }
  native.send(normalized);
}

// ── Reader ────────────────────────────────────────────────────────────────────

class JournalReader {
  #h; // native handle

  constructor(handle) { this.#h = handle; }

  static open(flags = OpenFlags.LOCAL_ONLY) {
    return new JournalReader(native.open(flags));
  }

  close()            { native._close.call(this.#h); }
  seekHead()         { native._seekHead.call(this.#h); }
  seekTail()         { native._seekTail.call(this.#h); }
  seekCursor(c)      { native._seekCursor.call(this.#h, c); }
  getCursor()        { return native._getCursor.call(this.#h); }

  seekTime(t) {
    const ms = t instanceof Date ? t.getTime() : t;
    native._seekTime.call(this.#h, BigInt(ms) * 1000n);
  }

  next()             { return native._next.call(this.#h); }
  previous()         { return native._previous.call(this.#h); }

  getEntry() {
    return normalizeEntry(native._getEntry.call(this.#h));
  }

  readEntry() {
    return this.next() ? this.getEntry() : null;
  }

  readEntries(limit = 100) {
    if (!Number.isInteger(limit) || limit < 0) {
      throw new TypeError('limit must be a non-negative integer');
    }

    const entries = [];
    while (entries.length < limit) {
      const entry = this.readEntry();
      if (!entry) break;
      entries.push(entry);
    }
    return entries;
  }

  addMatch(match)    { native._addMatch.call(this.#h, match); }
  flushMatches()     { native._flushMatches.call(this.#h); }
  clearFilters()     { this.flushMatches(); }

  setUnit(unit) {
    this.addMatch(`_SYSTEMD_UNIT=${unit}`);
  }

  setIdentifier(identifier) {
    this.addMatch(`SYSLOG_IDENTIFIER=${identifier}`);
  }

  setPriority(priority) {
    this.addMatch(`PRIORITY=${priority}`);
  }

  wait(timeoutUsec = -1n) {
    return native._wait.call(this.#h, timeoutUsec);
  }

  /**
   * tail -f — AbortSignal ile durdurulabilir.
   * @param {(entry: object) => void} onEntry
   * @param {{ signal?: AbortSignal, pollUsec?: bigint, start?: 'tail' | 'head' | 'current' }} [opts]
   */
  async follow(onEntry, { signal, pollUsec = 500_000n, start = FollowStart.TAIL } = {}) {
    if (start === FollowStart.TAIL) {
      this.seekTail();
      this.next();
    } else if (start === FollowStart.HEAD) {
      this.seekHead();
    } else if (start !== FollowStart.CURRENT) {
      throw new TypeError(`invalid follow start: ${start}`);
    }

    while (!signal?.aborted) {
      if (this.next()) {
        onEntry(this.getEntry());
      } else {
        // event loop'u bloke etmemek için tick ver, sonra kısa wait
        await new Promise(r => setImmediate(r));
        this.wait(pollUsec);
      }
    }
  }
}

module.exports = {
  print,
  send,
  JournalReader,
  Priority,
  OpenFlags,
  WaitResult,
  FollowStart,
};
