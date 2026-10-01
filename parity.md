# Feature parity

Every feature of either app and its status on each platform:

- **yes**: the app has it.
- **no**: the app lacks it, and the linked issue adds it.
- **differs**: both apps have it but behave differently. The link names the scenario gap in
  [player/README.md](player/README.md#known-gaps) or the issue.
- **n/a**: by design only on one platform (a desktop window system, a phone's audio focus). There
  is no issue.

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
| Search with the best result | yes | yes | [SRC-09 to SRC-12](player/sources.md#search) |
| Empty source, or one whose tracks are all unavailable, keeps the queue | yes | yes | [SRC-07, SRC-08, WAVE-03](player/sources.md) |
| The last source picked wins over slower earlier ones | yes | yes | [SRC-01 to SRC-03](player/sources.md) |
| Open the track in the browser | yes | yes | |

## Playback

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Play, pause, stop, next, previous, seek | yes | yes | |
| Previous: the previous track within 3 s, otherwise restart | yes | yes | [TR-01, TR-02](player/transport.md) |
| Previous while shuffle is on | differs | differs | Not decided yet ([transport.md](player/transport.md)) |
| Shuffle | yes | yes | Android keeps the choice in memory only |
| Repeat (whole queue; no repeat-one) | yes | yes | [TR-06](player/transport.md) |
| Gapless: the next track is fetched ahead and follows without silence | yes | unclear | Android relies on ExoPlayer's playlist |
| Volume and balance | yes | yes | |
| Bitrate, sample rate and channels shown | yes | yes | |
| Elapsed or remaining time | yes | yes | |
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

## Play tracking

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| `/play-audio` when a track starts | yes | yes | [TRK-01, TRK-02](player/tracking.md) |
| Rotor feedback: `radioStarted`, `trackStarted`, `trackFinished`, `skip`, station fallback | yes | yes | [TRK-03 to TRK-11](player/tracking.md) |

## Errors

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| No network: pause, continue when it returns | yes | yes | [ERR-01 to ERR-03](player/errors.md) |
| Broken track: next, stop after 3 in a row | yes | yes | [ERR-04 to ERR-07](player/errors.md) |
| Rejected token: back to login | differs | yes | Desktop stops with "Ошибка доступа" ([ERR-08](player/errors.md)) and does not open the login dialog |

## Likes

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Like and unlike the current track | yes | yes | |
| Dislike and skip | yes | yes | [TRK-07, TR-07](player/tracking.md) |
| The current track shows whether it is liked | yes | yes | |

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
| English UI | n/a | yes | The desktop UI is Russian only |

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
| Covers cached on disk | yes | unclear | |
| Volume, balance, equalizer, visualisation and time mode saved | yes | yes | |

## Jam

A shared queue for a party: one app hosts and plays, guests add tracks from a browser or from
their own QiYaa. The server and the web guest live in
[Kickoman/QiYaa-jam](https://github.com/Kickoman/QiYaa-jam): the protocol, room and host
scenarios in Kickoman/QiYaa-jam#3, #4, #5, the server in #6 to #12, the web guest in #13 to #16.

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Host a jam: room with a QR link, the jam queue and the jam wave in the player, guest search through the host's account, jam tracks not reported as plays | no: Kickoman/QiYaa#12, #13, #14, #15 | yes | Desktop needs Kickoman/QiYaa#5 first |
| Join a jam as a guest from the app, searching with your own account | no: Kickoman/QiYaa#16 | no: Kickoman/QiYaa-android#63 | Browser guests need no app |

## Desktop only: running without an account

| Feature | Desktop | Android | Notes |
|---|---|---|---|
| Command-line options: offline mode, local files, demo, skin, scale, screenshot | yes | n/a | |
