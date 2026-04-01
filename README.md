# @yskomur/node-sdjournal

Native Node.js bindings for the systemd **sd-journal** API — read *and* write.

Built with pure **NAPI** (C), no C++ wrapper, ABI-stable across Node versions.

## Features

| | |
|---|---|
| ✍️ Write | `print()`, `send()` (structured fields via `sd_journal_sendv`) |
| 📖 Read | `seekHead/Tail/Cursor/Time`, `getCursor`, `readEntry`, `readEntries` |
| 🔍 Filter | `addMatch`, `setUnit`, `setIdentifier`, `setPriority`, `clearFilters` |
| 👀 Follow | `reader.follow()` with `tail/head/current` start modes |
| 🟦 Types | Full TypeScript definitions (`.d.ts`) |

## Requirements

- Linux with systemd
- `libsystemd-dev` (Debian/Ubuntu) or `systemd-devel` (Fedora/RHEL)
- Node.js ≥ 18

## Install

```bash
npm install @yskomur/node-sdjournal
```

## Usage

### Write

```js
const { print, send, Priority } = require('@yskomur/node-sdjournal');

print(Priority.INFO, 'VM başlatıldı');

send({
  MESSAGE:           'VM started',
  PRIORITY:          String(Priority.INFO),
  SYSLOG_IDENTIFIER: 'myapp',
  VM_ID:             'vm-42',
});
```

### Read

```js
const { JournalReader, OpenFlags, Priority } = require('@yskomur/node-sdjournal');

const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
r.setIdentifier('myapp');
r.setPriority(Priority.INFO);
r.seekTime(new Date(Date.now() - 5 * 60_000));

for (const entry of r.readEntries(50)) {
  console.log(`[${entry.timestamp.toISOString()}] ${entry.message}`);
}
r.close();
```

### Follow (tail -f)

```js
const ac = new AbortController();
process.on('SIGINT', () => ac.abort());

await r.follow(
  entry => console.log(entry.message, entry.cursor),
  { signal: ac.signal, start: 'tail' },
);
```

## API

| Method | Description |
|---|---|
| `print(priority, message)` | sd_journal_print wrapper |
| `send(fields)` | sd_journal_sendv — structured |
| `JournalReader.open(flags?)` | Journal aç |
| `seekHead/Tail/Cursor/Time` | Konumlan |
| `getCursor()` | Mevcut cursor'ı al |
| `next() / previous()` | İterate |
| `getEntry()` | Entry oku → normalized `{ message, priority, fields, cursor, timestamp }` |
| `readEntry()` / `readEntries(limit?)` | High-level read helper |
| `addMatch(str)` | Filter: `"FIELD=value"` |
| `setUnit / setIdentifier / setPriority` | Common filter helper'ları |
| `clearFilters()` | Tüm filter'ları temizle |
| `wait(usec?)` | Yeni veri bekle |
| `follow(fn, opts?)` | `start: 'tail' | 'head' | 'current'` ile takip |

### Normalized Entry Shape

`getEntry()`, `readEntry()`, and `readEntries()` return:

```js
{
  message: 'VM started',
  priority: 6,
  identifier: 'myapp',
  unit: 'myapp.service',
  cursor: 's=...',
  realtimeUsec: 1710000000000000,
  timestamp: new Date(),
  fields: {
    MESSAGE: 'VM started',
    PRIORITY: '6',
    SYSLOG_IDENTIFIER: 'myapp'
  }
}
```

### Filter Helpers

```js
const r = JournalReader.open();
r.setUnit('nginx.service');
r.setIdentifier('myapp');
r.setPriority(Priority.ERR);
```

These helpers use `addMatch()` internally, so multiple calls are combined as journal matches. Use `clearFilters()` to reset them.

### Follow Start Modes

- `tail` (default): seek to end and emit only newly appended entries
- `head`: seek to beginning and stream forward from the oldest available entry
- `current`: keep the current reader position and continue from there

## Contributing

Contributions are welcome! Please open an issue or pull request on [GitHub](https://github.com/yskomur/node-sdjournal).

## Credits

Designed and written by [Claude.ai](https://claude.ai) (Anthropic) in collaboration with [@yskomur](https://github.com/yskomur).

Read API ergonomics, test updates, and documentation refresh contributions by Codex.

## License

MIT
