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
| [room.md](room.md) | The room on the server: creating, joining, the link, adding, search, removing, pinning, kicking, what plays, skipping, settings, state, the end, abuse, privacy | `ROOM-` |
| [ordering/](ordering/README.md) | The queue order: the rule and its reference cases | file names |
| [recovery.md](recovery.md) | The host gone and back, raising a room from a snapshot, server restarts, guests reconnecting | `REC-` |
| [seeds.md](seeds.md) | How the jam wave's seeds are chosen and when `seedsVersion` changes | `SEED-` |
| [host.md](host.md) | What the host app does: the queue mirror, the jam wave, not learning, storage, offline, search for guests, the end | `HOST-` |
| [listen.md](listen.md) | Guests listening along on their own devices: what the host sends, the server keeps and the web guest plays | `LISTEN-` |

## Rules

The [top README](../README.md#rules-for-files) applies: text in English, permanent scenario IDs,
tests that name the ID they check. Also:

- The server generates its protocol types from `protocol/schemas`. The desktop and Android
  parsers are tested on `protocol/examples`: the valid ones parse, the `invalid-*` ones are
  refused.
- Numbers come from [limits.md](limits.md). A schema that enforces one of them says the same
  number, and the limits table says so.
