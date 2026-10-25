# Spotify

Playing Spotify's catalogue with the user's own Spotify Premium account. Terms and the status
columns are explained in the [README](README.md).

The app speaks Spotify's client protocol the way [librespot](https://github.com/librespot-org/librespot)
does: an access point (AP) connection for the login and the audio keys, `spclient` and Pathfinder for
the catalogue, Spotify's CDN for the files. Nothing about a Spotify track goes to the Yandex API: a
wave from history never takes it as a seed ([CONT-05](continuation.md#the-wave-from-history)), and in
a jam it is a track that only the host can play ([HOST-37](../jam/host.md#the-base)). Only Premium
accounts play, and Spotify refuses the audio keys to some of them (SPOT-17).

A Spotify track's id is its base62 id (`6PCoZdjSPbRMehHviTrpB3`), and its URI
`spotify:track:<id>`.

## What the app tells Spotify about itself

The fields the servers check are what librespot sends; the free-form strings name QiYaa.

| Field | Value |
|---|---|
| Client ID (OAuth, client-token, login5) | `65b708073fc0480ea92a077233ca87bd`, Spotify's desktop client. Since 2026-08-10 login5 refuses stored credentials made with any other ID. |
| AP `build_info`: product, product flags, platform, version | `PRODUCT_CLIENT`, none, the platform of the OS and CPU as in librespot, `124200290` |
| client-token `client_version` | `1.2.52.442` |
| AP login `system_information_string` and `version_string`; the HTTP `User-Agent` | `QiYaa X.Y.Z`; `Spotify/124200290 <OS>/0 (QiYaa X.Y.Z)` |
| Device id | 20 random bytes as 40 hex digits, made once per installation and kept |

These values were accepted on 2026-10-21. If Spotify starts refusing them, librespot's own strings
are the first thing to try; the desktop's `src/spotify/protocol/README.md` keeps them and where
librespot defines the checked ones.

On Android the app presents itself as the desktop on Linux does: the AP platform is
`PLATFORM_LINUX_ARM` (`PLATFORM_LINUX_X86_64` on an x86-64 phone or emulator), client-token's
platform is the desktop's Linux one (the kernel release, the Android version as the
distribution's, the CPU `aarch64`, `arm` or `x86_64`), the login's CPU family `CPU_ARM` (`CPU_X86_64`)
and OS `OS_LINUX`, and the `User-Agent` says `Linux`. librespot does the same in the AP handshake on
Android, since the APs answer an Android platform with `TryAnotherAP`; Android's own values together
with the desktop's client ID were never tried. These Android values are not tried on a phone yet
(Kickoman/QiYaa-android#118).

## Account

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SPOT-01 | Not logged in to Spotify | The user logs in with a code | `POST https://accounts.spotify.com/oauth2/device/authorize` (form: `client_id`, `scope=streaming`). The app shows the reply's `user_code` and the page to enter it at, `verification_uri_complete` (`https://spotify.com/pair?code=…`), and polls `POST https://accounts.spotify.com/api/token` (form: `grant_type=urn:ietf:params:oauth:grant-type:device_code`, `device_code`, `client_id`) every `interval` seconds, 5 s longer after each `slow_down`, while it answers `authorization_pending`. A token: SPOT-03. `access_denied`, `expired_token` or closing the dialog end it with nothing saved. | yes | not checked: Kickoman/QiYaa-android#118 |
| SPOT-02 | Not logged in to Spotify | The user logs in in the browser | The app listens on a free port of `127.0.0.1` and opens `https://accounts.spotify.com/authorize` with `client_id`, `response_type=code`, `redirect_uri=http://127.0.0.1:<port>/login`, `scope=streaming` and a PKCE challenge (S256). The redirect's `code` is exchanged at `POST /api/token` with the verifier; the browser is told it can be closed. A token: SPOT-03. A redirect with `error`, or one whose `state` does not match, saves nothing. On Android the page opens in the phone's browser and the user comes back to the app on their own; if the system stops the app meanwhile, the login starts over. | yes | not checked: Kickoman/QiYaa-android#118 |
| SPOT-03 | A token from SPOT-01 or SPOT-02 | — | The app logs in at the AP with it and keeps only the reusable credentials of `APWelcome`: the username and the credentials blob, in a file of the app's own. The token itself is never stored. A status says the login worked, and the Spotify menu offers its picks. | yes | not checked: Kickoman/QiYaa-android#118 |
| SPOT-04 | Saved credentials | The app starts | It logs in at the AP with them; no dialog. `APLoginFailed` with `BadCredentials` deletes them: a status says the Spotify login has expired, and the menu offers to log in again. A network failure keeps them and tries again as in ERR-02. | yes | yes |
| SPOT-05 | An account without Premium | A login at the AP | `APLoginFailed` with `PremiumAccountRequired`: a status says Spotify Premium is needed. Nothing is saved. | yes | yes |
| SPOT-06 | Logged in to Spotify | The user logs out | The saved credentials are deleted and the AP connection closes. Spotify tracks left in the queue fail as an account failure ([ERR-08](errors.md#account)) when they need their file. | yes | yes |

## Picks

Each pick is a source request: [SRC-01 to SRC-08](sources.md) apply, and its tracks are queued in
the order given. A track Spotify marks unavailable in the account's country, or with no Ogg Vorbis
file (SPOT-15), is `available: false` (SRC-05). Episodes and the local files of a playlist are left
out. Track metadata comes from `POST /extended-metadata/v0/extended-metadata` (`TRACK_V4`).

On Android a reply that would make the app work without end is cut short: a track's alternatives
are followed 4 levels deep, and a file only deeper counts as none (SPOT-15); a list or a collection
of more than 1000 pages is unreadable, a `content` failure. The desktop has no such limits.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SPOT-07 | Logged in | The user picks Liked Songs | The `spotify:track:` items of the `collection` set of `POST /collection/v2/paging`, every page, newest first as Spotify shows it, under the title "Liked Songs". | yes | not checked: Kickoman/QiYaa-android#118 |
| SPOT-08 | Logged in | The user opens the list of playlists | `GET /playlist/v2/user/<username>/rootlist`: every playlist the user made or follows, in the user's order, folders flattened. Spotify's own (Discover Weekly, Daily Mix, Release Radar) are among them when the user follows them. Picking one: `GET /playlist/v2/playlist/<id>` and its tracks under its name. | yes | not checked: Kickoman/QiYaa-android#118 |
| SPOT-09 | Logged in | The user opens the saved albums, or the followed artists | The saved albums are the `spotify:album:` items of the `collection` set of `/collection/v2/paging` (there is no `albums` set: it answers 403), the followed artists the `artist` set; newest first. An album plays all its discs in order under its name; an artist plays its popular tracks in the account's country (`ARTIST_V4`, up to 10) under its name. | yes | not checked: Kickoman/QiYaa-android#118 |
| SPOT-10 | Logged in | The user opens "For you" | Pathfinder's `home`: the playlists of its "Made For You" section (`spotify:section:0JQ5DAUnp4wcj0bCb3wh3S`) in its order. Picking one is as in SPOT-08. | yes | not checked: Kickoman/QiYaa-android#118 |
| SPOT-11 | Logged in | The user searches | Pathfinder `searchDesktop` with the trimmed text; an empty text sends nothing. The best result is the first of its top results: an artist plays as in SPOT-09, an album as in SPOT-09, anything else the found tracks under "Search: <text>". No tracks found: status "Nothing found" ([SRC-12](sources.md#search)). | yes | not checked: Kickoman/QiYaa-android#118 |
| SPOT-12 | A Spotify track, current or picked in the playlist | The user picks "Radio" for it | A queue with an empty head and the track's station as its continuation (`spotify:station:track:<id>`), as a station of Yandex Music ([WAVE-01](wave.md#start)): its first tracks replace the queue and play, more come when remaining ≤ 2, under the title "<track>: radio". The flag of [CONT-01](continuation.md#the-flag) changes nothing. | yes | not checked: Kickoman/QiYaa-android#118 |

## Playing a track

On Android a login refused while a track needs its file, at the AP or at login5, stops playback on
that track as an account failure ([ERR-08](errors.md#account)), as SPOT-17 does; `BadCredentials` also
forgets the saved login (SPOT-04), and `TryAnotherAP` is no refusal: the next AP is tried. login5's
temporary refusals (`TIMEOUT`, `TOO_MANY_ATTEMPTS`, `TRY_AGAIN_LATER`) wait as a network failure
([ERR-01](errors.md#network)) instead.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SPOT-13 | A Spotify track | It needs its file (to start or to preload) | The file of the quality in effect from the track's `TRACK_V4` metadata: `standard` Ogg Vorbis 320 kbps, `economy` 96 kbps; when that one is missing, 160, then any Ogg Vorbis ([quality.md](quality.md)). MP3, AAC and FLAC files are never taken. Then the audio key over the AP (`RequestKey` with the file id and the track's id), the CDN links from `GET /storage-resolve/files/audio/interactive/<file id>`, and the file from the first link with range requests. It is decrypted with AES-128-CTR (the key, Spotify's fixed initial counter block) and its first 167 bytes, Spotify's own header, are dropped: the rest is Ogg Vorbis and plays as any downloaded file ([TR-09](transport.md)). The bitrate shown is the file's. On Android the file is not downloaded whole: it is read in ranges as it plays, and the decrypted Ogg goes to the same disk cache as a Yandex Music track's download (TR-09). | yes | not checked: Kickoman/QiYaa-android#118 |
| SPOT-14 | A Spotify track's file is downloading | A CDN link fails: a network failure, or a status other than 200 or 206 | The next link of the list continues from the same byte. A 401 or 403 from the CDN is such a failed link, not the account failure of ERR-08. When every link has failed: a network failure ([ERR-01 to ERR-03](errors.md#network)) if the last one failed by the network, otherwise a track failure ([ERR-04](errors.md#broken-tracks)). On Android the file is read in ranges as it plays (SPOT-13), so a seek or a read long after the start can reach links that have expired: when every link answers 403 or 410, `storage-resolve` is asked once more and its links continue from the same byte. That happens at most once per run of failed links; a run starts again with every seek and every byte read, so a later expiry in the same play asks again. A reply that breaks after it has delivered bytes is not a failed link: the next link continues from the same byte. When every link has failed, it is a network failure if any of them failed by the network, not only the last; a track failure by status ends the play, and the file is not prepared again for it. | yes | yes |
| SPOT-15 | A Spotify track | Its metadata has no Ogg Vorbis file, or it is restricted in the account's country and has no playable alternative | A track failure (ERR-04 to ERR-07). | yes | yes |
| SPOT-16 | A Spotify track needs its audio key | The AP answers with `AesKeyError` code 2, or nothing comes in 5 s | It asks once more after 1 s. A second failure is a track failure (ERR-04). | yes | yes |
| SPOT-17 | A Spotify track needs its audio key | The AP answers with `AesKeyError` code 1 | Spotify does not let this account play outside its own apps. Playback stops on the track and a status says so: no next track, not a track failure (ERR-05), as an account failure ([ERR-08](errors.md#account)). The same happens again for any Spotify track until the account changes. | yes | yes |
| SPOT-18 | Logged in | The AP connection drops, or the app wakes up | It connects again with the saved credentials, retrying as in ERR-02. A track that waits for its key or its links meanwhile waits as a network failure (ERR-01). On Android, while the app is in the background and plays nothing, a dropped connection is not restored: playback starting, the next audio key it needs, or the app coming back to the screen connects again. | yes | yes |
| SPOT-19 | A Spotify track | It starts and closes | Nothing is reported, to Spotify or to Yandex: no `/play-audio` ([TRK-01](tracking.md#play-audio)), no rotor feedback. The loudness values of Spotify's header are not applied. Dislike is not offered; like is not offered yet. Opening the track in the browser opens `https://open.spotify.com/track/<id>`. Its cover is the album's image from `https://i.scdn.co/image/<image id>`, the smallest one at least as large as shown. | yes | not checked: Kickoman/QiYaa-android#118 |

## Continuation

A queue of Spotify tracks has no wave from history: Spotify's autoplay takes its place under the
same flag "continue with a wave" ([CONT-01 to CONT-03](continuation.md#the-flag)), and CONT-04 to
CONT-08 do not apply to it. Shuffle and repeat act as in [CONT-09 to CONT-11](continuation.md#shuffle-and-repeat).

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SPOT-20 | A queue of a pick of SPOT-07 to SPOT-11, the flag on, repeat off, no autoplay tracks yet | A track becomes current or starts, and remaining ≤ 2 | `POST /context-resolve/v1/autoplay` with `{context_uri, recent_track_uri}`: the pick's URI (`spotify:user:<username>:collection` for Liked Songs, the playlist's, the album's or the artist's own URI, a search's first track's) and the URIs of the queue's tracks that started, oldest first, at most 50. The available tracks of the reply's first page are appended after the head; the next page comes when remaining ≤ 2 again. No status. If the head ends before the reply, playback waits for it as in [WAVE-08](wave.md#loading-more). | yes | yes |
| SPOT-21 | As SPOT-20 | The request fails, or brings no available track | As [CONT-07](continuation.md#the-wave-from-history): nothing is appended, a failure shows an error status, playback stops at the end of the head, and the next Next asks again. | yes | yes |
| SPOT-22 | A queue of Spotify tracks in a jam, as its base | The base asks for more | Its continuation is the autoplay of SPOT-20 from the base's own tracks: the last state's seeds start no jam wave under it ([HOST-11](../jam/host.md#the-base)). With the flag off or the autoplay failed, HOST-13 applies. | yes | yes |
