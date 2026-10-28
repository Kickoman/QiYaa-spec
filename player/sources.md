# Sources

Loading a source into the queue. Terms and the status columns are explained in the
[README](README.md).

## Requests

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SRC-01 | Source request A is in flight | The user picks source B before A answers | Only B is applied. When A's reply arrives, it is dropped: the queue does not change and no status appears. A's HTTP request does not have to be aborted. | yes | yes |
| SRC-02 | Source request A is in flight | The user picks B, then A fails | A's failure is dropped silently. There is no error status. | yes | yes |
| SRC-03 | An artist or album the user played waits for its tracks (SRC-09, SRC-10) | The user picks another source | Its reply is dropped as in SRC-01. Playing an artist or an album is one source request, however many requests it makes. | yes | yes |
| SRC-04 | Any queue | A source request fails (network, HTTP error, bad JSON) | Error status. The queue and playback stay as they were. | yes | yes |

## What gets queued

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SRC-05 | A source reply lists tracks, some with `available: false` | It is applied | Tracks with `available: false` are left out of the queue. A track without an `available` field is available. | yes | yes |
| SRC-06 | Any queue, possibly playing | A source with at least one available track answers | The queue is replaced by the source's available tracks in the source's order. The previous track is closed ([TRK-06](tracking.md)). The first track starts playing. A status names the source and its track count. | yes | yes |
| SRC-07 | Any queue, possibly playing | A source answers with no tracks | Status "<source>: empty". The queue and playback stay as they were. This applies to every source: likes, playlist, album, artist, station, local files ([LOCAL-03](local.md#opening), [LOCAL-12](local.md#the-phones-media-library-android)) and the picks of Spotify ([SPOT-07 to SPOT-11](spotify.md#picks)). A wave is covered by [WAVE-03](wave.md). | yes | yes |
| SRC-08 | Any queue, possibly playing | A source answers with tracks that are all unavailable | Same as SRC-07: "<source>: empty", and nothing changes. The empty check runs after the filter of SRC-05. | yes | yes |

## Search

A search shows what it found and plays nothing by itself: the queue changes only when the user
plays or adds something from it. It is no source request (SRC-14). The user searches Yandex Music
or Spotify ([SPOT-11](spotify.md#picks)), choosing one for each search.

`GET /search?text=<text>&type=all&page=0`. The text is trimmed, and an empty text sends nothing.
Of the reply:

- **The best result**, `best`: its type, `best.result.id` and a name, `best.result.name` or
  `best.result.title` if there is no name. It is shown when its type is `artist`, `album` or
  `track` and the id is not empty; a best track is shown as a track (`best.result` is a track
  object), and only when it is available (SRC-05). A best playlist, podcast or anything else is
  not shown.
- **Tracks**: the available ones of `tracks.results`, in the reply's order.
- **Artists**: `artists.results`, `id` and `name`, in order.
- **Albums**: `albums.results` with `metaType` `music` (no podcasts or audiobooks) and `available`
  not `false`, in order: `id`, `title`, the artists' names, `year` (may be missing) and whether
  `type` is `single`.

The best result is shown once: the tracks, artists or albums leave out the entry with its id.
Playlists, podcasts, episodes and clips are not shown.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SRC-09 | Logged in | The user plays an artist: a liked one, or one of a search (SRC-13) | `GET /artists/<id>/track-ids-by-rating`. The first 100 ids are loaded with `POST /tracks/` (`track-ids`, at most 250 per request). They are queued under the artist's name. | yes | yes |
| SRC-10 | Logged in | The user plays an album: a liked one, or one of a search (SRC-13) | `GET /albums/<id>/with-tracks`. The tracks of all volumes are joined in order and queued under the album's title. | yes | yes |
| SRC-11 | — | — | Removed: a search no longer plays its tracks by itself (SRC-13, SRC-18). It queued `tracks.results` under "Search: <text>" when the best result was no artist or album. | — | — |
| SRC-12 | — | — | Removed: a search that finds nothing is SRC-19 now. It showed the status "Nothing found" when the list SRC-09, SRC-10 or SRC-11 chose was empty. | — | — |
| SRC-13 | Search reply | It has something to show | The best result, then the tracks, the artists and the albums, each part with its count; a part with nothing is left out. A long part shows its first few, and the rest of the same reply on request. Each track shows its title, artists and duration, and the one that is current in the queue is marked. The user can play a track (SRC-18) or add it (SRC-17); play an artist (SRC-09) or open it (SRC-16); play an album (SRC-10), open it or add it. Nothing is queued until then. | no: Kickoman/QiYaa#73 | no: Kickoman/QiYaa-android#144 |
| SRC-14 | A search in flight, or its results shown | The user searches again | The newer search replaces the older one: the older reply is dropped and its results go. A search does not drop a source request in flight and is not dropped by one: they are independent. Playing or adding something from the results leaves them shown as they are. | no: Kickoman/QiYaa#73 | no: Kickoman/QiYaa-android#144 |
| SRC-15 | Any queue | A search fails (network, HTTP error, bad JSON) | The failure is shown in place of the results, with a way to search again. The queue and playback stay as they were, and no status appears. | no: Kickoman/QiYaa#73 | no: Kickoman/QiYaa-android#144 |
| SRC-16 | A search's results | The user opens an artist or an album | Its tracks are shown in place of the results, nothing is queued: an artist's are the ones SRC-09 would queue, an album's the ones SRC-10 would. While they load, another open, Back or a new search wins: the older reply is dropped. Playing one of them is a source request with no request of its own: the queue becomes those tracks under the artist's name or the album's title, with the chosen one current and playing, as in SRC-18. Back shows the results as they were, with no new request. An empty list says so; a failure is shown as in SRC-15. | no: Kickoman/QiYaa#73 | no: Kickoman/QiYaa-android#144 |
| SRC-17 | A search's results, or an artist or album opened from them | The user adds a track, or an album, to the queue | "Play next" or "Add to the end" ([ADD-01 to ADD-09](queue.md)): a track by itself, an album with its available tracks in order (`GET /albums/<id>/with-tracks` first when they are not shown; Spotify's as in SPOT-09). An album's tracks are added when they arrive, by the rules for the queue as it is then; a failure gives an error status and changes nothing, and an album with no available track the status "<album>: empty". During a jam session the search adds to the jam instead ([HOST-38](../jam/host.md#adding-as-the-host)). | no: Kickoman/QiYaa#73 | no: Kickoman/QiYaa-android#144 |
| SRC-18 | A search's results | The user plays one of its tracks (the best track too) | A source request with no request of its own: the queue becomes the search's tracks, the best track first when it is one, under the title "Search: <text>", with the chosen track current and playing; the tracks before it stay in the queue. Otherwise as SRC-06: with shuffle on, the pass begins with the chosen track ([SHUF-01](shuffle.md#without-repeats)). | no: Kickoman/QiYaa#73 | no: Kickoman/QiYaa-android#144 |
| SRC-19 | Search reply | It has nothing to show: no best result is shown, and there is no track, artist or album | "Nothing found" in place of the results. The queue stays, and no status appears. | no: Kickoman/QiYaa#73 | no: Kickoman/QiYaa-android#144 |
