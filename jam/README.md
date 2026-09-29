# Jam

A shared queue for a party. One QiYaa (desktop or Android) hosts the jam and plays the music with
its Yandex account. Guests scan a QR code and add tracks from a browser or from their own QiYaa.
A small jam server holds the room and its rules. The server and the web guest live in
[Kickoman/QiYaa-jam](https://github.com/Kickoman/QiYaa-jam). Everything here is shared by the
server and both apps.

| Path | What it holds | IDs |
|---|---|---|
| [protocol/](protocol/README.md) | Protocol v1: transport, handshake, messages as JSON Schema, examples, secrets, reasons | — |
| [limits.md](limits.md) | Every size, timeout and rate of the jam | — |

Coming next, with their issues:

| Path | What it will hold | IDs | Issue |
|---|---|---|---|
| `room.md` | The room: joining, adding, removing, pinning, skipping, settings, the end | `ROOM-` | Kickoman/QiYaa-jam#4 |
| `ordering/*.json` | Reference cases of the queue order | — | Kickoman/QiYaa-jam#4 |
| `recovery.md` | Host gone and back, server restarts, snapshots, the outbox | `REC-` | Kickoman/QiYaa-jam#4 |
| `seeds.md` | How the wave seeds are chosen | — | Kickoman/QiYaa-jam#4 |
| `host.md` | What the host app does: the queue mirror, the jam wave, what it does not report | `HOST-` | Kickoman/QiYaa-jam#5 |

## Rules

The [top README](../README.md#rules-for-files) applies: text in English, permanent scenario IDs,
tests that name the ID they check. Also:

- The server generates its protocol types from `protocol/schemas`. The desktop and Android
  parsers are tested on `protocol/examples`: the valid ones parse, the `invalid-*` ones are
  refused.
- Numbers come from [limits.md](limits.md). A schema that enforces one of them says the same
  number, and the limits table says so.
