# Listening along

A guest may listen to the jam on its own device: the web page plays the very file the host plays,
roughly in time with the host. The host's app sends the links of its files, the server passes them
on, and the guest's browser fetches the file from Yandex Music's storage itself: the sound never
goes through the jam server, and the host's token never leaves the host.

The files are the host's subscription's, which Yandex Music licenses for the host's own listening.
So the host's user switches sharing on in the app, it is off by default, and the setting says
whose files they are.

The protocol is in [protocol/](protocol/README.md#listening-along), the numbers in
[limits.md](limits.md#listening-along).

## The host

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| LISTEN-01 | A jam session, sharing off (the default) | Any `playing` | No `listenUrl`, no `listenNextUrl`. | yes | no: Kickoman/QiYaa-android#73 |
| LISTEN-02 | Sharing on | `playing` for an item or a wave track | `listenUrl` is the link of the file the host plays, once the host has it. `playing{source: idle}` carries none. | yes | no: Kickoman/QiYaa-android#73 |
| LISTEN-03 | Sharing on, the host already has the link of the file that plays next (a preloaded track) | `playing` | `listenNextUrl` is that link. Without one the field is left out; the next `playing` brings it once the host has it. | no: #17 | no: Kickoman/QiYaa-android#73 |
| LISTEN-04 | Sharing on | A link that is not a file of Yandex Music's storage (a test server) | It is left out: the server refuses a `playing` with such a link. | yes | no: Kickoman/QiYaa-android#73 |
| LISTEN-05 | A jam session | The user switches sharing on or off | The next `playing` follows the new choice. The choice is the app's setting, not the room's: it outlives the jam. | yes | no: Kickoman/QiYaa-android#73 |

## The server

| ID | Given | When | Then |
|---|---|---|---|
| LISTEN-06 | A room | The host's `playing` with `listenUrl` and `listenNextUrl` | `room.nowPlaying` carries them until the next `playing`, which replaces or drops them, and until another item starts. `idle` carries none. A snapshot never holds them: they expire within the hour. |

## The web guest

| ID | Given | When | Then |
|---|---|---|---|
| LISTEN-07 | `room.nowPlaying.listenUrl` is set | — | "Listen here" is offered. Nothing plays until the guest taps it: browsers start sound only after a gesture. |
| LISTEN-08 | Listening | A `state`, and every second | The page plays `listenUrl` at the room's progress (`positionMs + (server now − reportedAt)` while not `paused`), pauses with the host, and seeks again when it is off by more than 2 s. |
| LISTEN-09 | Listening, `listenNextUrl` known | The file ends before a `state` with the next track | The page plays `listenNextUrl` from its start at once, without waiting for the server: a locked phone may not run the page's timers. The next `state` corrects the file and the position. |
| LISTEN-10 | Listening | `room.nowPlaying` without `listenUrl` (nothing plays, or the host stopped sharing) | The sound pauses; listening stays on and goes on with the next link. |
| LISTEN-11 | Listening | A pause from outside the page: the lock screen, a headset, a call | Listening ends, and later states do not start the sound again. The page's own pauses and the end of a file do not end it. |
| LISTEN-12 | Listening | — | The phone's lock screen and media controls show the title, the artists and the cover. Their pause ends listening. |
