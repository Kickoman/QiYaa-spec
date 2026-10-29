# Feature parity

Every feature of either app and its status on each platform:

- **yes**: the app has it.
- **no**: the app lacks it, and the linked issue adds it.
- **differs**: both apps have it but behave differently. The link names the scenario gap in
  [player/README.md](player/README.md#known-gaps) or the issue.
- **n/a**: by design only on one platform (a desktop window system, a phone's audio focus). There
  is no issue.
- **not checked**: the app has it, but nobody has checked it on a device yet. The linked issue
  holds the device check, and the row becomes **yes** (or **differs** with a gap) after it.

When a feature lands on one platform, add its row here and open an issue for the other; see the
[top README](README.md#changing-the-behaviour). When the issue is closed, set the row to **yes**.

## Sources and library

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Liked tracks | yes | yes | |
| User playlists | yes | yes | |
| "Похожие треки" for a playlist (recommendations) | yes | yes | `GET /users/<uid>/playlists/<kind>/recommendations` |
| "Для вас": personal playlists (Плейлист дня, Дежавю, Премьера…) | yes | yes | `GET /landing3?blocks=personalplaylists` |
| Liked artists and their popular tracks | yes | yes | Top 100 by rating ([SRC-09](player/sources.md#search)) |
| Liked albums (podcasts left out) | yes | yes | |
| Stations, grouped by type | yes | yes | |
| My Wave | yes | yes | [wave.md](player/wave.md) |
| Wheel of waves (waves matched to the current track) | yes | yes | `POST /wheel/new` |
| Search shows its results: the best result, tracks, artists and albums; a found track plays the found tracks from it; an artist or album opens to its tracks | yes | not checked: Kickoman/QiYaa-android#118 | [SRC-13 to SRC-19](player/sources.md#search), [SPOT-11](player/spotify.md#picks); Spotify's artists and albums on the desktop are not checked against a live reply yet (Kickoman/QiYaa#73) |
| Play next and Add to the end, for a track or an album | yes | differs: gap A6, Kickoman/QiYaa-spec#17 | [ADD-01 to ADD-09](player/queue.md); during a jam the search adds to the jam ([HOST-38](jam/host.md#adding-as-the-host)) |
| Empty source, or one whose tracks are all unavailable, keeps the queue | yes | yes | [SRC-07, SRC-08, WAVE-03](player/sources.md) |
| The last source picked wins over slower earlier ones | yes | yes | [SRC-01 to SRC-03](player/sources.md) |
| Open the track in the browser | yes | yes | |
| Local files: open files or a folder | yes | n/a | [LOCAL-01 to LOCAL-03](player/local.md#opening); Android takes them from the phone's media library instead |
| Local files from the phone's media library: all tracks, albums, artists and folders, with the permission to read audio | n/a | not checked: Kickoman/QiYaa-android#118 | [LOCAL-10 to LOCAL-14](player/local.md#the-phones-media-library-android) |
| Local files: tags and covers, read from the disk; no wave from history after them | yes | not checked: Kickoman/QiYaa-android#118 | [local.md](player/local.md), [CONT-12](player/continuation.md#the-flag). Android: the media library's tags (one line of artists) and pictures ([LOCAL-04, LOCAL-05](player/local.md#tracks)); a codec the phone cannot decode fails when it plays, and FLAC without a decoder is not taken ([LOCAL-13](player/local.md#the-phones-media-library-android)) |
| Ogg Vorbis among local files | yes | not checked: Kickoman/QiYaa-android#118 | [LOCAL-09](player/local.md#tracks) |
| Spotify (Premium): Liked Songs, playlists (Spotify's own too), albums, artists, "For you", search, radio by a track | yes | not checked: Kickoman/QiYaa-android#118 | [SPOT-07 to SPOT-12](player/spotify.md#picks) |
| Spotify tracks: Ogg Vorbis 320 or 96 kbps by the quality, refused audio keys stop playback, Spotify's autoplay after a queue | yes | not checked: Kickoman/QiYaa-android#118 | [SPOT-13 to SPOT-22](player/spotify.md#playing-a-track) |

## Playback

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Play, pause, stop, next, previous, seek | yes | yes | |
| Seeking: the position shows the target at once; a seek into the downloaded part does not wait for a stalled download | yes | yes | [TR-08, TR-09](player/transport.md); Android keeps the last 10 minutes in memory and the current track's download on disk, up to 512 MiB (about 3 h 43 min at 320 kbps) |
| Previous: the previous track within 3 s, otherwise restart | yes | yes | [TR-01, TR-02](player/transport.md) |
| Previous while shuffle is on | yes | yes | Without repeats: the track before in the pass; Random track: the head's track before in source order ([SHUF-02, SHUF-07](player/shuffle.md)) |
| Shuffle | yes | yes | Orders the head only ([shuffle.md](player/shuffle.md)). Android: at the head's wrap of CONT-10 while vibe tracks wait, the track fetched ahead is the first vibe track, while the wrap plays the head's first one after a pause (see Gapless). The switch is kept with the queue: Android restores it with its saved queue; the desktop restores no queue and starts with shuffle off |
| Shuffle algorithm: without repeats or random track | yes | yes | [SHUF-06 to SHUF-09](player/shuffle.md); the desktop chooses in the main menu, Android in the library |
| The algorithm is saved | yes | yes | [SHUF-10](player/shuffle.md) |
| Repeat (whole queue; no repeat-one) | yes | yes | [TR-06](player/transport.md) |
| Gapless: the next track is fetched ahead and follows without silence | yes | yes | Android: ExoPlayer's playlist. The next track's link is signed and its download starts once the current file is downloaded, about a minute or more before the end (up to about 3 min at economy), and the next track plays on in the same audio output. Not across the head's wrap of CONT-10 while vibe tracks wait: Android pauses at the head's end |
| Volume and balance | yes | yes | |
| Bitrate, sample rate and channels shown | yes | yes | |
| Quality: standard or economy (HE-AAC 64 kbps) | yes | yes | [quality.md](player/quality.md) |
| Separate quality for Wi-Fi and mobile data | n/a | yes | [QUAL-07](player/quality.md) |
| Lossless | no | no | Waits for a tester with access to lossless ([quality.md](player/quality.md#lossless-not-yet), #1) |
| Elapsed or remaining time | yes | yes | |
| Lyrics: the current track's text from Yandex Music (`supplement`) or the file (`.lrc`, tags), and from LRCLIB when the user turns it on; the synced line follows the position and a click seeks to it | yes | not checked: Kickoman/QiYaa-android#118 | [lyrics.md](player/lyrics.md); Yandex's synced lyrics not yet ([lyrics.md](player/lyrics.md#synced-lyrics-not-yet)). Android: in the cover's place, after a tap on the cover; no `.lrc` on Android 10 |
| Queue editing: select, remove, play a chosen track | yes | yes | |
| Audio focus; pause when headphones are unplugged | n/a | yes | |

## Waves

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Loads more when 2 tracks are left | yes | yes | [WAVE-05, WAVE-06](player/wave.md) |
| A reply for a replaced queue is dropped silently | yes | yes | [WAVE-07](player/wave.md) |
| Next at the end asks for more again after a failure | yes | yes | [WAVE-09](player/wave.md) |
| Shuffle does not apply in a wave | yes | yes | [WAVE-10, WAVE-11](player/wave.md) |
| Repeat does not apply in a wave | yes | yes | [WAVE-12](player/wave.md) |
| A queue goes on with a wave from history after its head ("continue with a wave", on by default) | yes | yes | [CONT-01 to CONT-11](player/continuation.md) |

## Play tracking

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| `/play-audio` when a track starts | yes | yes | [TRK-01, TRK-02](player/tracking.md) |
| Rotor feedback: `radioStarted`, `trackStarted`, `trackFinished`, `skip`, station fallback | yes | yes | [TRK-03 to TRK-11](player/tracking.md) |

## Errors

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| No network: pause, continue when it returns, as the same play | yes | yes | [ERR-01 to ERR-03, ERR-09](player/errors.md) |
| Broken track: next, stop after 3 in a row | yes | yes | [ERR-04 to ERR-07](player/errors.md) |
| Broken track while paused: the next one, still paused | no: Kickoman/QiYaa#70 | yes | [ERR-10](player/errors.md#broken-tracks) |
| Rejected token: back to login | differs | not checked: Kickoman/QiYaa-android#118 | Desktop stops with "Ошибка доступа" ([ERR-08](player/errors.md)) and does not open the login dialog. Android opens its login, which can be left for the Library (Kickoman/QiYaa-android#115) |

## Likes

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Like and unlike the current track | yes | yes | |
| Dislike and skip | yes | yes | [TRK-07, TR-07](player/tracking.md) |
| The current track shows whether it is liked | yes | yes | |
| Like, dislike and open any track of the playlist | yes | yes | Desktop: a right click on a line; Android: a long press. A dislike skips only when that track is current ([TR-10](player/transport.md)) |

## Equalizer

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| 10 bands ±12 dB, preamp, on/off | yes | yes | Same filters: [dsp/eq-response.json](dsp/README.md) |
| The 17 Winamp presets | yes | yes | [dsp/eq-presets.json](dsp/README.md) |
| `.eqf` / `.q1` import and `.eqf` export | yes | n/a | Winamp files on a desktop |
| AUTO button | unclear | unclear | Saved on both; what it should do is not specified |
| Settings saved | yes | yes | |

## Visualisation

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Spectrum (19 bars), oscilloscope, off | yes | yes | Same bars: [dsp/spectrum.json](dsp/README.md) |
| Colours from the skin's `viscolor.txt` | yes | n/a | Android uses its accent theme |
| Milkdrop (projectM) | yes | n/a | |

## Look

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Winamp `.wsz` skins | yes | n/a | Android has its own mobile design |
| Accent themes | n/a | yes | |
| Russian UI | yes | yes | |
| English UI | yes | yes | Yandex's "волна" is "vibe" in English ("My Vibe") |
| Belarusian UI, the default language | yes | yes | |
| Picking the language in the app, applied at once | yes | yes | |

## Desktop windows

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Separate skinned windows: docking, snapping, positions saved | yes | n/a | |
| Minimising or focusing the main window takes the others along | yes | n/a | |
| Shade mode, scale, always on top | yes | n/a | |
| "Сейчас играет" window: cover, album, year | yes | differs | Android shows the cover on the player screen |
| Hotkeys | yes | n/a | |

## System integration

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Media keys and the system media panel, with cover | yes (MPRIS on Linux, SMTC on Windows; none on macOS) | yes (MediaSession, notification, headset) | |
| Plays in the background | yes | yes | |

## Login and data

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Login with a device code | yes | yes | |
| Paste a token or a URL with `#access_token=` | yes | yes | |
| Token from a file or `QIYAA_TOKEN` | yes | n/a | |
| Log out | yes | yes | |
| Runs without a Yandex Music login: the app starts, the login opens from the menu (desktop) or the Library (Android) and can be left | yes | not checked: Kickoman/QiYaa-android#118 | The desktop opens its login dialog at the start on its own; Android starts on the Library (Kickoman/QiYaa-android#115) |
| Spotify: login with a code at spotify.com/pair or in the browser, log out | yes | not checked: Kickoman/QiYaa-android#118 | [SPOT-01 to SPOT-06](player/spotify.md#account) |
| Covers cached on disk | yes | yes | Android: Coil's disk cache in the app's cache folder (2% of the size of the data partition, 10 to 250 MiB, so 250 MiB on a data partition of 12.5 GiB or more; least recently used out first), for the player screen and the notification. Android may empty it when storage runs low |
| Volume, balance, equalizer, visualisation and time mode saved | yes | yes | |
| Telemetry: usage statistics and crash reports, off by a menu switch | yes (builds with `QIYAA_WITH_TELEMETRY`) | yes (builds with `-PqiyaaWithTelemetry=true`) | [telemetry/](telemetry/README.md) |

## Jam

A shared queue for a party: one app hosts and plays, guests add tracks from a browser or from
their own QiYaa. The server and the web guest live in
[Kickoman/QiYaa-jam](https://github.com/Kickoman/QiYaa-jam): the protocol, room and host
scenarios in Kickoman/QiYaa-jam#3, #4, #5, the server in #6 to #12, the web guest in #13 to #16.

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Host a jam: room with a QR link, the jam queue and the jam wave in the player, guest search through the host's account, jam tracks not reported as plays | yes | yes | Android starts a jam only with a Yandex Music login; the desktop starts one without it, and the guests' searches get `unauthorized` ([HOST-29](jam/host.md#search-and-check-for-guests)) |
| The jam plays over the host's own queue: the guests' items on top, the host's source (the base) under them, no wave of the jam's own; the host edits the base during the jam | yes | yes | [HOST-10 to HOST-14, HOST-21, HOST-32, HOST-36](jam/host.md#the-base) |
| Guests listen along on the jam page: the host shares the files it plays | yes | yes | [jam/listen.md](jam/listen.md); off by default, the host's subscription's files |
| A base track that is not from the Yandex catalog (a local file, a Spotify track) plays in the jam: guests see its title and artists, with no cover and no listening along | yes | not checked: Kickoman/QiYaa-android#118 | [HOST-37](jam/host.md#the-base), [LISTEN-14](jam/listen.md#the-host), `track.source: host` in the [protocol](jam/protocol/README.md#tracks); on the desktop these are local files ([local.md](player/local.md)) and Spotify tracks ([spotify.md](player/spotify.md)). Android reports such a track since Kickoman/QiYaa-android#116, checked by JVM tests only; its first such tracks come with local files (Kickoman/QiYaa-android#111) |
| Join a jam as a guest from the app, searching with your own account | no: Kickoman/QiYaa#16 | yes | [jam/guest.md](jam/guest.md); browser guests need no app |

## Desktop only: command-line options

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Command-line options: offline mode, local files, demo, skin, scale, screenshot | yes | n/a | |
