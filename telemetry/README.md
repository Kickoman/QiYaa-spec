# Telemetry

Anonymous usage statistics and crash reports, so the authors see which systems QiYaa runs on, what
fails and what people use. Only builds made with telemetry have it (the desktop app's
`QIYAA_WITH_TELEMETRY`, the Android app's `-PqiyaaWithTelemetry=true`, both off by default; the
releases on GitHub have it). In such a build it is on from the first start, and a switch turns it
off: in the menu on the desktop, in the settings at the bottom of Library on Android. The Android
app has it since Kickoman/QiYaa-android#75 ([parity](../parity.md), [Android](#android)).

The app posts batches of events to the jam server, which writes each event as a line of its log;
the host's log store keeps them and draws the dashboard. The landing counts clicks on its download
buttons the same way ([below](#the-landing)).

## Transport

`POST <server>/api/telemetry` with `Content-Type: application/json` and a batch
([schemas/batch.schema.json](schemas/batch.schema.json), [examples](examples/)). `<server>` is
the jam server the build names (the desktop app's `QIYAA_TELEMETRY_URL`; the Android app's Gradle
property `qiyaaTelemetryUrl`, by default `<qiyaaJamUrl>/api/telemetry`), not the jam server chosen
in the app. A batch is of one app: `app` is `desktop` or `android`, and each carries only its own
app's `start`, `exit` and `crash`.

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
- On Android, `machine` is the same HMAC over `Settings.Secure.ANDROID_ID` as ASCII bytes. Since
  Android 8 that id is 16 hex digits, one per app signing key, user and device. When there is
  none, a random UUID is kept in the setting `telemetry/machine` and hashed the same way.
- Test vectors for the hash: the id `abc` gives `3a8271ff2b263f300989f47f507a3804`; the Android
  examples' machine, `97eaa8cc2fbe9a456389e6bd9d44a3fb`, is the `ANDROID_ID` `3f2b9c1d7a6e5048`.
- `session`: a random UUID per run of the app.
- `version`: the app's version in that run.

Every event carries its own `session` and `version`: a batch may hold events of an earlier run,
which the app could not send then (a crash report, the last `exit`).

## Events

| `type` | When | What it adds |
|---|---|---|
| `start` | The app started | The system (`os`, `osName`, `osVersion`, `kernel`, `arch`, the `qt` it runs on, its `locale`), the build (`package`, `milkdropBuilt`, `jamBuilt`), the state it starts in (`language`, `scale`, `screens`, `dpr`, `skin`, `vis`, `equalizer`, `milkdropVisible`, `audioBackend`, `loggedIn`, `spotifyLoggedIn`) and `firstRun`. Android: the system (`os` `android`, `osVersion`, `sdk`, `device`, `kernel`, `arch`, the system's `locale`), the build (`package`: `apk` for a release build, `debug` otherwise), the state it starts in (`language`, `dpr`, `theme`, `vis`, `equalizer`, `quality`, `qualityMetered`, `loggedIn`) and `firstRun` |
| `exit` | The app closed normally; Android: the next run, for a run that ended without a crash ([TEL-16](#the-app)) | `seconds` it ran (Android: on screen or playing); `tracks` started; `playMinutes`; `milkdropMinutes` with the Milkdrop window shown (desktop only); `jams` hosted; `errorsDropped` over the per-session limit; the [lyrics counts](#lyrics-counts) when the lyrics view looked anything up (desktop only) |
| `crash` | The next run, after a crash | `signal` (`SIGSEGV`, `SIGABRT`…, `EXCEPTION_ACCESS_VIOLATION`…, or `TERMINATE` for an uncaught exception; Android: `SIGNAL_<n>` for a signal with no name), `frames` (none for a native crash on Android), `seconds` the crashed run lasted ([Android](#android): two kinds), `exceptionType` for `TERMINATE` |
| `unclean_exit` | The next run, after a run that neither closed nor left a crash report; Android: after an ANR, or a Java crash that left no report ([TEL-16](#the-app)) | `seconds` it ran, to the last minute it noted (Android: on screen or playing) |
| `error` | Something failed | `area` and `kind` from the table below, `httpStatus` for `api` and for `spotify` `http` |
| `feature` | A switch or a choice | `name` and `value` from the table below |

A frame is `<the module's file name>+0x<offset in it>` (`QiYaa+0x4f2a10`,
`libQt6Widgets.so.6+0x1d2f40`), or `unknown+0x<address>` when the module is unknown. Characters a
module name may not have become `_`. Offsets do not
change between machines for one build, so one bug gives one list of frames everywhere; with the
build's debug symbols they turn into function names.

On Android a frame is `<class>.<method>:<line>` of the root cause, the last exception of the
`cause` chain (`io.github.kickoman.qiyaa.queue.QueueController.onFailure:291`); when the root
cause has no frames, the outer exception's. The line is `0` when it is unknown or the method is
native. Characters outside `A-Za-z0-9_$.<>-` become `_`; a frame over the length cuts its class
and method and keeps `:<line>`. `exceptionType` is the root cause's class name
(`java.lang.IllegalStateException`), left out when it does not fit; the exception's message is
never sent. Once release builds are minified with R8 (Kickoman/QiYaa-android#22), their frames arrive obfuscated as
`<R8 class>.<R8 method>:<pc>` (ART gives the instruction's offset in place of the line), apart
from the system's own (`java.*`, `android.*`). Like the desktop's debug symbols, the
`mapping.txt` published with each release turns them back: write each frame `c.m:N` as a stack
trace line `at c.m(SourceFile:N)` and run R8's `retrace` with the mapping of the event's
`version`.

| `error.area` | `kind` |
|---|---|
| `playback` | `network`, `auth`, `track`: why a track failed (as in [ERR-](../player/errors.md)) |
| `audio` | `init` (no sound device could be opened), `device` (the device failed while playing), `undecodable` |
| `api` | `network`, `http` (with `httpStatus`), `content` (the answer could not be read) |
| `login` | `failed` |
| `spotify` | `login-expired` (saved credentials refused, [SPOT-04](../player/spotify.md#account)), `premium-required` (SPOT-05), `key-refused` (an audio key refused for the account, SPOT-17), `key-failed` (an audio key failed twice, SPOT-16), `access-point` (the AP connection failed without a network failure), `http` (with `httpStatus`: a request to Spotify's servers, a CDN's included), `content` (an answer could not be read), `query-outdated` (Pathfinder refused every query hash it was given) |
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
| `lyrics` | `on`, `off` (the lyrics view shown or hidden) |
| `lyrics-services` | `on`, `off` (lyrics from a service such as LRCLIB turned on or off, [lyrics.md](../player/lyrics.md)) |
| `spotify` | `login-code`, `login-browser` (a Spotify login succeeded that way, [spotify.md](../player/spotify.md#account)), `logout` |

Android has no skins and no Milkdrop: it sends neither the `skin` and `milkdrop` areas nor those
features. It has no lyrics view yet (Kickoman/QiYaa-android#112), so it sends neither the `lyrics`
features nor the lyrics counts.

### Lyrics counts

What the lyrics view ([player/lyrics.md](../player/lyrics.md)) found, as counts in `exit`. They are
there only when the session looked up at least one track, and then all of them are.

- **A lookup** is one track in the open view that the app asked its source or the service about.
  A track whose answers all come from memory (LYR-14) is not a lookup. A Yandex track asked about
  without a login is one, and ends with no text.
  - Opening the view during a track starts a lookup of it.
  - Turning the service on during a track (LYR-15) starts a new lookup of the same track.
- **A lookup ends** when everyone asked has answered or the waits are over, when the track
  changes, or when the view closes. Then it is counted once, by what the view shows at that
  moment.

| Field | Counts lookups that ended with |
|---|---|
| `lyricsLookups` | anything: the sum of the next six |
| `lyricsSourceSynced` | the source's synced text |
| `lyricsThirdPartySynced` | the service's synced text |
| `lyricsSourceText` | the source's plain text |
| `lyricsThirdPartyText` | the service's plain text |
| `lyricsInstrumental` | no text, and the service says the track is instrumental |
| `lyricsNone` | no text: nothing found, not logged in, a failure, or the lookup ended before an answer came |
| `lyricsThirdPartyFailed` | a failure of the service after its retry, a final 503 included (whatever was shown) |

Nothing else about lyrics is sent: no text, no title, no track id.

## Never sent

The Yandex token, the account's name or id, Spotify's credentials, tokens and username, track titles, artists, lyrics, search texts, file paths (a
skin from a file is `custom`), any error's or exception's text (on Android, any exception's
message or `toString()`), the OS's own machine id (on Android, the raw `ANDROID_ID`), and the
user's or the computer's name (on Android, the name the user gave the device). The schema allows
only the fields above, each with a pattern, so a batch with anything else is refused. The landing
sends no id of the visitor and, of the page it was opened from, only the host.

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

## Android

- A run is the app's process. Android ends processes by itself, so the app never "closes": the
  app is in use while it is on screen or playing, and where the desktop closes, Android stops
  being in use.
- It sends at the start, every 5 minutes while in use, when it stops being in use and when the
  network comes back; never while offline.
- A run saves its counts every minute it is in use. The next run sends that run's `exit` from
  them, or its `crash` or `unclean_exit` ([TEL-16](#the-app)).
- Two kinds of `seconds`: a `crash` from the crash report counts the time since the process
  started; `exit`, `unclean_exit` and a `crash` from Android's report count the time the run was
  in use, to the last minute it saved.
- It has no Milkdrop, no skins and no screenshot runs.

## The app

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| TEL-01 | A build without telemetry (the default) | — | Nothing is recorded or sent and there is no switch. The desktop build has no telemetry code; the Android app has it but never starts it. | yes | yes |
| TEL-02 | A build with telemetry | The first start | Telemetry is on. The switch (desktop: the menu; Android: the bottom of Library) turns it off: nothing more is sent, events waiting on disk are deleted. On again, new events go from then on. The choice is the app's setting. | yes | yes |
| TEL-03 | On | The app starts | `start` is queued. Waiting events go at the start, every 5 minutes and when the app closes (within the wait it already makes for its own requests; on Android, "closes" is "stops being in use", see [Android](#android)). What did not go stays on disk for the next run. | yes | yes |
| TEL-04 | On, events sent | The answer | As in the transport table. More than 500 waiting: the oldest are dropped. | yes | yes |
| TEL-05 | On | The app crashes (a signal, an unhandled exception, an uncaught C++ exception) | The crash handler writes the signal or the exception's code, the frames and, for an uncaught exception, its type to a file. The next run sends `crash` with the crashed run's `session` and `version` before its own `start`. | yes | yes |
| TEL-06 | On | A run starts and the previous one neither closed nor left a crash report | `unclean_exit` with the previous run's `session`, `version` and `seconds`. | yes | differs: TEL-16 |
| TEL-07 | On | Something fails | `error` with the category; at most 50 in a session, the rest only counted. | yes | yes |
| TEL-08 | On | The app closes normally | `exit` with the session's counts. | yes | differs: TEL-16 |
| TEL-09 | On | The user shows Milkdrop, starts or ends a jam, picks a skin, a language, the equalizer, a visualization | `feature`. | yes | yes |
| TEL-10 | Any build | A screenshot run, tests, temporary settings | Nothing is recorded or sent. | yes | yes |
| TEL-17 | On | The user shows or hides the lyrics view, turns the lyrics service on or off; the app closes | `feature` `lyrics` or `lyrics-services`; `exit` with the [lyrics counts](#lyrics-counts). | yes | no: #112 |
| TEL-18 | On | The app starts; the user logs in to Spotify or logs out; a Spotify login, key or request fails | `start` with `spotifyLoggedIn` (desktop only; builds before Spotify leave it out); `feature` `spotify`; `error` with area `spotify` and its kind. Nothing about the account or its tracks. | yes | no: #113 |
| TEL-16 | Android, on | A run starts and an earlier run's files are on disk | Before its own `start`, the earlier run's waiting events and then how it ended: `crash` from its crash report; otherwise, on Android 11+, `crash` with the signal and no frames when Android reports a native crash, `unclean_exit` when it reports an ANR or a Java crash that left no report; otherwise `exit` with the counts the run saved, `seconds` being the time it was on screen or playing. A run that left neither a crash report nor counts gets no such event. | n/a | yes |

## The server

| ID | Given | When | Then |
|---|---|---|---|
| TEL-11 | — | A valid batch | 204. Each event becomes one log line: `service: qiyaa-desktop` or `qiyaa-android`, by the batch's `app`; `stream: telemetry`, `event: app_<type>`, the level `error` for `crash`, `warn` for `error` and `unclean_exit`, `info` for the rest. The line holds `machine`, the client's address, the event's `at` as `client_at`, and the event's fields in snake_case. |
| TEL-12 | — | Not JSON, not a batch of the schema, over 64 KiB, or over a rate limit | 400, 400, 413 or 429 with `Retry-After`. Nothing of the body is logged, only the request itself. |

## The landing

The landing's download buttons (`/`, `/ru`, `/en` on the jam server) go straight to the files on
GitHub. A click also tells the jam server what was clicked: `POST /api/download` to the landing's
own server, with a JSON object of the fields below; the page does not wait for the answer. The
click carries no id of the visitor and the page keeps nothing in the browser. The server logs it
with the request's client address and user agent, like a page view ([ROOM-63](../jam/room.md#privacy)).

| Field | Value |
|---|---|
| `file` | The button's file: `windows-setup`, `windows-zip`, `appimage`, `deb`, `dmg` or `apk` |
| `place` | The page's section the button is in: `download` or `android` |
| `language` | The page's language: `be`, `ru` or `en` |
| `version` | Only when known: the release the button points to, `X.Y.Z`. Left out while the button still points to the newest release's page because GitHub has not answered |
| `browserLanguage` | Only when known: the browser's first language as it names it (`ru-RU`) |
| `referrer` | Only when known: the host of the site the landing was opened from (`t.me`); never the landing's own host, and nothing but the host |

| What | Limit | Kept by |
|---|---|---|
| Request body | 1 KiB | server (413) |
| Clicks from one IP | 10 a minute | server (429) |
| `version` | `X.Y.Z`, at most 20 characters | server (400) |
| `browserLanguage` | 1 to 35 of `A-Z a-z 0-9 -` | page, server (400) |
| `referrer` | 1 to 253 of `a-z 0-9 . : -` | page, server (400) |

The page leaves out an optional field that does not fit, not the whole click.

| ID | Given | When | Then |
|---|---|---|---|
| TEL-13 | The landing | The visitor clicks a download button: the main or the middle mouse button, a tap, Enter | The page posts the click once. The button works as it would without the post, whether the post succeeds, fails or is blocked: the page never waits for it and never stops the link. |
| TEL-14 | — | A valid click | 204. The request's own log line (`stream: http_request`, level `info`) holds the click: `file`, `place`, `page_language` and, when sent, `app_version`, `browser_language`, `referrer_host`. No other line is written. |
| TEL-15 | — | Not JSON; not an object of the fields above, or a value outside them; over 1 KiB; over the rate limit | 400, 400, 413 or 429 with `Retry-After`. Nothing of the body is logged, only the request itself. |
