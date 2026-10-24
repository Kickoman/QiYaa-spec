# Lyrics

The text of the current track, shown while the user has the lyrics view open: on the desktop the
LYRICS window; on Android the text in the cover's place on the player screen, after a tap on the
cover. There the view is open while it is shown and the player screen is on screen: another tab,
the app in the background or the screen off close it. Terms and the status columns are explained
in the [README](README.md).

## Where the text comes from

- **The track's source.**
  - For a Yandex Music track, Yandex Music ([below](#yandex-music)).
  - For a local file ([local.md](local.md)), the file itself: an `.lrc` file next to it and the
    lyrics in its tags (LYR-10).
  - For a Spotify track ([spotify.md](spotify.md)), none: Spotify's own lyrics are not asked for,
    so only a lyrics service can give a text.
- **A lyrics service**, only when the user turns it on; it is off until then. The one service is
  [LRCLIB](#lrclib), an open database of lyrics with and without times.
  - It learns what the user listens to: the title and the first artist of each track it is asked
    about, and the computer's address.
  - The apps say so where the user turns it on.

A text is **synced** when its lines carry times and **plain** when they do not
([lyrics/](../lyrics/README.md) has the rules). The view shows the first text available in this
order:

1. the source's synced text;
2. the service's synced text;
3. the source's plain text;
4. the service's plain text.

When nothing has a text and the service says the track is instrumental, the view says that the
track has no words, not that it has no lyrics.

## Yandex Music

```
GET /tracks/<id>/supplement
Authorization: OAuth <token>
```

- **`<id>`** is the track's id without its album. `<id>:<albumId>` works as well.
- **The reply** ([fixtures](../fixtures/yandex/tracks-supplement)) is `result.lyrics`:
  - `fullLyrics`: the whole text, lines separated by `\n`, stanzas by an empty line, with no times;
  - `lyrics`: its first lines;
  - `hasRights`, `textLanguage`, `showTranslation`, `id`.
- **A track without a text** has a `result` without `lyrics`, and the status is still 200. An id
  the server does not know gets 400 `validate`.
- **Taking the text.** It is taken when `fullLyrics` is not empty and `hasRights` is not `false`.
  It is always plain.
- **`lyricsAvailable`.** The old field of a track matched `supplement` on every track checked on
  2026-10-21. The apps ask `supplement` anyway and do not read it.

### Synced lyrics: not yet

Yandex Music has synced lyrics (licensed from LyricFind), but the apps do not ask for them: the
account the apps are built with never gets them. Each app keeps a place where the request would
go, and it answers "none" for now.

What is known, from probes on 2026-10-21:

- **Whether a track has them.** A track says so in `lyricsInfo {hasAvailableSyncLyrics,
  hasAvailableTextLyrics}`. Both were `false` on every track checked, even for tracks whose
  `supplement` has a text.
- **The request.**

  ```
  GET /tracks/<id>/lyrics?format=LRC&timeStamp=<unix seconds>&sign=<sign>
  Authorization: OAuth <token>
  X-Yandex-Music-Client: YandexMusicDesktopAppWindows/5.95.0
  ```

  `format=TEXT` asks for the plain text the same way.
- **`sign`** is base64 of HMAC-SHA256 over `<id> + <timeStamp>`, with the key of
  [get-file-info](quality.md#fetching-the-economy-file). Nothing is dropped from the base64 text,
  unlike there.
  - The server checks the key together with the client header: without the header, or with the
    time in milliseconds, it answers 403 `Invalid Sign`.
  - The Android app's key goes with a `YandexMusicAndroid/…` header.
- **The answer for our account** was 404 `No lyrics found for track` for both formats. The same
  came from outside Russia; Russia was not tried.
- **The 200 reply** has not been seen. In the model of the open client
  [yandex-music-api](https://github.com/MarshalX/yandex-music-api) it is:
  - `result.downloadUrl`: a link to the text itself;
  - `writers`: the authors;
  - `major`: the provider;
  - `lyricId` and `externalLyricId`.

**To start:** someone whose account gets a 200 (`lyricsInfo.hasAvailableSyncLyrics: true`)
captures a reply and the text behind its `downloadUrl`. That decides the parsing; then the
scenarios here change and the place in the apps gets the request.

## LRCLIB

```
GET https://lrclib.net/api/search?track_name=<title>&artist_name=<first artist>
User-Agent: <app name>/<version> (<the app's repository URL>)
```

- **The reply** ([fixtures](../fixtures/lrclib/search)) is a JSON array of up to 20 records. Each
  record has `id`, `trackName`, `artistName`, `albumName`, `duration` in seconds, `instrumental`,
  `plainLyrics` and `syncedLyrics` (LRC, or `null`), and fields the apps do not read.
- **The choice.**
  - Take the records whose `duration` is within 2 s of the track's, or all of them when the
    track's duration is unknown.
  - Of those, the first one with a synced text gives it. A `syncedLyrics` without a single time
    tag counts as plain.
  - Otherwise the first one with `plainLyrics` gives a plain text.
  - Otherwise, when one of them is `instrumental`, the track has no words.
  - Otherwise nothing is found.
- **Why the search and not `/api/get`.**
  - `/api/get` needs the album exactly as LRCLIB has it. The catalog's album often differs, and
    the answer is 404.
  - Several artists joined in one name ("A, B") find nothing; the first artist alone does.
- **Retries.**
  - A 5xx answer or a network failure is tried again once, after 1 s.
  - LRCLIB often answers a search that finds nothing with 503 `ServerOverloaded` instead of `[]`
    (seen 2026-10-21). So a 503 after the retry counts as nothing found.
- **Without the version.** When nothing is found and the title ends with ` (…)` or ` […]`, the
  search runs once more with that part cut off, and the retry rule applies again.
- **No request** goes out for a track without a title or an artist.

## Limits

| What | Limit |
|---|---|
| A text, in UTF-8 | 256 KiB: a larger one is not taken, as if there were none. A local `.lrc` file over it is not read |
| Lines of a text | 2000, after time tags are expanded: the rest is left out |
| A line | 500 characters: the rest of it is cut |
| An LRCLIB reply | 2 MiB: a larger one is a failure |
| Waiting for one answer | 8 s: a source or service that has not answered by then has nothing |
| Answers kept in memory | The last 10 tracks |

## Scenarios

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| LYR-01 | The lyrics view is closed | The current track changes, or anything else happens | No request for lyrics goes out, neither to the source nor to the service. | yes | not checked: Kickoman/QiYaa-android#118 |
| LYR-02 | The view is open | A Yandex track is current | `supplement` as [above](#yandex-music); its text is shown as plain. Without a text, the source has none. | yes | yes |
| LYR-03 | The view is open, the service is on | A track is current | The source is asked. The service is asked when the source has no synced text: when it answered without one, failed, or has not answered within 1 s. The view shows the first text available in the order [above](#where-the-text-comes-from). A text that arrives first shows at once and gives way to a better one when that comes. While a better one may still come, the view says it is still looking. | yes | yes |
| LYR-04 | The service is asked | — | The request, the retries and the choice as in [LRCLIB](#lrclib). | yes | yes |
| LYR-05 | The source has no text | The service says the track is instrumental | The view says the track has no words. When the source has a text, that text shows, whatever the service says. | yes | yes |
| LYR-06 | A synced text is shown, the track plays or is paused | The position moves, or the user seeks | The current line is the last line whose time is at or before the position. It is marked and kept in the middle of the view. After a seek it follows the target at once, as the position does ([TR-08](transport.md)). Before the first line no line is current. | yes | not checked: Kickoman/QiYaa-android#118 |
| LYR-07 | A synced text is shown, the track plays or is paused | The user clicks or taps a line | A seek to the line's time, as in TR-08; a paused track stays paused. The lines of a stopped player, of a track waiting for the network ([ERR-01 to ERR-03](errors.md#network)) and of a plain text do nothing. On Android a stopped player is one with nothing loaded (after ERR-05, ERR-07 or ERR-08) or at the end; after Stop, which pauses at the start, the lines seek as on a pause. | yes | not checked: Kickoman/QiYaa-android#118 |
| LYR-08 | A synced text is shown | The user scrolls the view | The view stops following the current line until 5 s after the last scroll, then shows the current line again. A plain text starts at its top and just scrolls. | yes | not checked: Kickoman/QiYaa-android#118 |
| LYR-09 | A track's text is being looked up | Another track becomes current | The answers for the earlier track are dropped, a request to the service for it is cancelled, and the view at once shows that it is looking up the new one. | yes | yes |
| LYR-10 | The view is open | A local file is current | The source's synced text is the file's `.lrc` (the same name with `.lrc` in any case, in the same folder), else the lyrics in its tags when they carry time tags. Its plain text is the lyrics in its tags, else an `.lrc` without time tags. The lyrics in the tags are USLT in the ID3v2 tag of an mp3 or WAV file, whatever its description, the `LYRICS` Vorbis comment of a FLAC file, `©lyr` in MP4. An `.lrc` is UTF-8, UTF-16 or UTF-32 with a byte order mark, else UTF-8, else the system's code page. Then as LYR-03. On Android an `.lrc` is read on Android 8–9 and 11+, not on Android 10, where the system does not give it (the tags still are); without a byte order mark and not UTF-8 it is read as windows-1251, since Android has no other code page, so a Western single-byte file comes out as Cyrillic. | yes | not checked: Kickoman/QiYaa-android#118 |
| LYR-11 | A text, an `.lrc` file or a reply over a [limit](#limits) | It is read | As the limits say. | yes | yes |
| LYR-12 | A text | It is shown | Its lines as [lyrics/lrc.json](../lyrics/README.md) gives them. An empty line of a synced text is a pause and shows as a note sign. | yes | yes |
| LYR-13 | A jam runs, the host's view is open | — | The view shows the text of the host's current track, as without a jam. Nothing about lyrics goes to the jam server or to the guests. | yes | not checked: Kickoman/QiYaa-android#118 |
| LYR-14 | A track's lookup has ended, and it is one of the last 10 tracks looked up | It is current again | Its answers come from memory, with no request: the source's and the service's apart, so turning the service on asks only the service. A network failure and a missing login are not kept, and logging in or out forgets everything. Nothing is kept on disk. | yes | yes |
| LYR-15 | The view is open | The user turns the service on, or off | On: when the source has no synced text for the current track, the service is asked at once. Off: a text of the service gives way to the source's (or to "no lyrics"), a request still running is cancelled, and nothing more goes to the service. On Android it is turned on in Library or from the view when it has no text, and off only in Library: a text of the service then gives way when the view is opened again. | yes | yes |
| LYR-16 | Not logged in to Yandex Music | A Yandex track is current | Nothing is asked of Yandex Music. When nothing else has a text, the view says that its lyrics need a login. Local files and the service work as usual. | yes | yes |
| LYR-17 | The source failed on the network, and no other text is shown | The network comes back | Until then the view says that the text could not be loaded. Then the source is asked again. | yes | yes |
