# Telemetry

Anonymous usage statistics and crash reports, so the authors see which systems QiYaa runs on, what
fails and what people use. Only builds made with telemetry have it (the desktop app's
`QIYAA_WITH_TELEMETRY`, off by default; the releases on GitHub have it). In such a build it is on
from the first start, and a switch in the menu turns it off. The Android app has none yet
([parity](../parity.md)).

The app posts batches of events to the jam server, which writes each event as a line of its log;
the host's log store keeps them and draws the dashboard.

## Transport

`POST <server>/api/telemetry` with `Content-Type: application/json` and a batch
([schemas/batch.schema.json](schemas/batch.schema.json), [examples](examples/)). `<server>` is
the jam server the build names (`QIYAA_TELEMETRY_URL`), not the jam server chosen in the app.

| Answer | Meaning | The app |
|---|---|---|
| 204 | Accepted | Drops the events it sent |
| 400 | Not a batch of the schema | Drops them too: sending them again would not help |
| 413 | The body is over the limit | Never happens if the app keeps to the limit; drops them |
| 429 | Over a rate limit; `Retry-After` in seconds | Sends nothing before then |
| anything else, no answer | | Keeps them and tries again later: 1 min, doubling up to 1 h |

## Who sent it

- `machine`: the first 16 bytes, in hex, of HMAC-SHA256 with the key `QiYaa telemetry` over the
  OS's machine id (Qt's `QSysInfo::machineUniqueId()`: `/etc/machine-id` or
  `/var/lib/dbus/machine-id` on Linux, `MachineGuid` on Windows, `IOPlatformUUID` on macOS). The
  OS's id itself never leaves the machine, as `machine-id(5)` asks: an app derives its own. When
  the OS has no id, a random UUID is kept in the app's settings and hashed the same way.
- `session`: a random UUID per run of the app.
- `version`: the app's version in that run.

Every event carries its own `session` and `version`: a batch may hold events of an earlier run,
which the app could not send then (a crash report, the last `exit`).

## Events

| `type` | When | What it adds |
|---|---|---|
| `start` | The app started | The system (`os`, `osName`, `osVersion`, `kernel`, `arch`, the `qt` it runs on, its `locale`), the build (`package`, `milkdropBuilt`, `jamBuilt`), the state it starts in (`language`, `scale`, `screens`, `dpr`, `skin`, `vis`, `equalizer`, `milkdropVisible`, `audioBackend`, `loggedIn`) and `firstRun` |
| `exit` | The app closed normally | `seconds` it ran; `tracks` started; `playMinutes`; `milkdropMinutes` with the Milkdrop window shown; `jams` hosted; `errorsDropped` over the per-session limit |
| `crash` | The next run, after a crash | `signal` (`SIGSEGV`, `SIGABRT`…, `EXCEPTION_ACCESS_VIOLATION`…, or `TERMINATE` for an uncaught exception), `frames`, `seconds` the crashed run lasted, `exceptionType` for `TERMINATE` |
| `unclean_exit` | The next run, after a run that neither closed nor left a crash report | `seconds` it ran, to the last minute it noted |
| `error` | Something failed | `area` and `kind` from the table below, `httpStatus` for `api` |
| `feature` | A switch or a choice | `name` and `value` from the table below |

A frame is `<the module's file name>+0x<offset in it>` (`QiYaa+0x4f2a10`,
`libQt6Widgets.so.6+0x1d2f40`), or `unknown+0x<address>` when the module is unknown. Characters a
module name may not have become `_`. Offsets do not
change between machines for one build, so one bug gives one list of frames everywhere; with the
build's debug symbols they turn into function names.

| `error.area` | `kind` |
|---|---|
| `playback` | `network`, `auth`, `track`: why a track failed (as in [ERR-](../player/errors.md)) |
| `audio` | `init` (no sound device could be opened), `device` (the device failed while playing), `undecodable` |
| `api` | `network`, `http` (with `httpStatus`), `content` (the answer could not be read) |
| `login` | `failed` |
| `skin` | `load` |
| `milkdrop` | `no-opengl`, `gles`, `old-opengl`, `projectm` |
| `jam` | the server's refusal reason (`server-full`, `rate-limited`, `update-required`, `not-allowed`, `queue-limit`, `duplicate`, `stale`), or `gone`, `expired`, `ended-by-server` when a jam ends that way |

| `feature.name` | `value` |
|---|---|
| `milkdrop` | `on`, `off` (the window shown or hidden) |
| `jam` | `on`, `off` (a jam started or ended) |
| `skin` | `base`, a built-in skin's name, `custom` |
| `language` | `be`, `ru`, `en` |
| `equalizer` | `on`, `off` |
| `vis` | `spectrum`, `oscilloscope`, `off` |

## Never sent

The Yandex token, the account's name or id, track titles, artists, search texts, file paths (a
skin from a file is `custom`), any error's or exception's text, the OS's own machine id, and the
user's or the computer's name. The schema allows only the fields above, each with a pattern, so
a batch with anything else is refused.

## Limits

| What | Limit | Kept by |
|---|---|---|
| Request body | 64 KiB | app, server (413) |
| Events in a batch | 1 to 50 | app, schema |
| Frames in a crash | 32 | app, schema |
| Events waiting on disk | 500, the oldest dropped first | app |
| `error` events per session | 50, the rest counted in `exit.errorsDropped` | app |
| Batches from one IP | 30 a minute | server (429) |
| Batches from one `machine` | 10 a minute | server (429) |

## The app

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| TEL-01 | A build without telemetry (the default) | — | No telemetry code, nothing is sent, the menu has no switch. | yes | n/a |
| TEL-02 | A build with telemetry | The first start | Telemetry is on. The menu's switch turns it off: nothing more is sent, events waiting on disk are deleted. On again, new events go from then on. The choice is the app's setting. | yes | no |
| TEL-03 | On | The app starts | `start` is queued. Waiting events go at the start, every 5 minutes and when the app closes (within the wait it already makes for its own requests). What did not go stays on disk for the next run. | yes | no |
| TEL-04 | On, events sent | The answer | As in the transport table. More than 500 waiting: the oldest are dropped. | yes | no |
| TEL-05 | On | The app crashes (a signal, an unhandled exception, an uncaught C++ exception) | The crash handler writes the signal or the exception's code, the frames and, for an uncaught exception, its type to a file. The next run sends `crash` with the crashed run's `session` and `version` before its own `start`. | yes | no |
| TEL-06 | On | A run starts and the previous one neither closed nor left a crash report | `unclean_exit` with the previous run's `session`, `version` and `seconds`. | yes | no |
| TEL-07 | On | Something fails | `error` with the category; at most 50 in a session, the rest only counted. | yes | no |
| TEL-08 | On | The app closes normally | `exit` with the session's counts. | yes | no |
| TEL-09 | On | The user shows Milkdrop, starts or ends a jam, picks a skin, a language, the equalizer, a visualization | `feature`. | yes | no |
| TEL-10 | Any build | A screenshot run, tests, temporary settings | Nothing is recorded or sent. | yes | no |

## The server

| ID | Given | When | Then |
|---|---|---|---|
| TEL-11 | — | A valid batch | 204. Each event becomes one log line: `service: qiyaa-desktop`, `stream: telemetry`, `event: app_<type>`, the level `error` for `crash`, `warn` for `error` and `unclean_exit`, `info` for the rest. The line holds `machine`, the client's address, the event's `at` as `client_at`, and the event's fields in snake_case. |
| TEL-12 | — | Not JSON, not a batch of the schema, over 64 KiB, or over a rate limit | 400, 400, 413 or 429 with `Retry-After`. Nothing of the body is logged, only the request itself. |
