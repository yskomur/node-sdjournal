# node-sdjournal

Native Node.js bindings for the systemd **sd-journal** API — read *and* write.

Built with pure **NAPI** (C), no C++ wrapper, ABI-stable across Node versions.

## Features

| | |
|---|---|
| ✍️ Write | `print()`, `send()` (structured fields via `sd_journal_sendv`) |
| 📖 Read | `seekHead/Tail/Cursor/Time`, `next/previous`, `getEntry` |
| 🔍 Filter | `addMatch / flushMatches` |
| 👀 Follow | `reader.follow()` — tail -f with AbortSignal |
| 🟦 Types | Full TypeScript definitions (`.d.ts`) |

## Requirements

- Linux with systemd
- `libsystemd-dev` (Debian/Ubuntu) or `systemd-devel` (Fedora/RHEL)
- Node.js ≥ 18

## Install

```bash
npm install node-sdjournal
```

## Usage

### Write

```js
const { print, send, Priority } = require('node-sdjournal');

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
const r = JournalReader.open(OpenFlags.LOCAL_ONLY);
r.addMatch('SYSLOG_IDENTIFIER=myapp');
r.seekTime(new Date(Date.now() - 5 * 60_000));

while (r.next()) {
  const { fields, timestamp } = r.getEntry();
  console.log(`[${timestamp.toISOString()}] ${fields.MESSAGE}`);
}
r.close();
```

### Follow (tail -f)

```js
const ac = new AbortController();
process.on('SIGINT', () => ac.abort());

await r.follow(
  entry => console.log(entry.fields.MESSAGE),
  { signal: ac.signal },
);
```

## API

| Method | Description |
|---|---|
| `print(priority, message)` | sd_journal_print wrapper |
| `send(fields)` | sd_journal_sendv — structured |
| `JournalReader.open(flags?)` | Journal aç |
| `seekHead/Tail/Cursor/Time` | Konumlan |
| `next() / previous()` | İterate |
| `getEntry()` | Entry oku → `{ fields, cursor, timestamp }` |
| `addMatch(str)` | Filter: `"FIELD=value"` |
| `wait(usec?)` | Yeni veri bekle |
| `follow(fn, opts?)` | tail -f, AbortSignal destekli |

## Contributing

Contributions are welcome! Please open an issue or pull request on [GitHub](https://github.com/yskomur/node-sdjournal).

## Credits

Designed and written by [Claude.ai](https://claude.ai) (Anthropic) in collaboration with [@yskomur](https://github.com/yskomur).

## License

MIT
