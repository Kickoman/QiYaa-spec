# Sources

Loading a source into the queue. Terms and the status columns are explained in the
[README](README.md).

## Requests

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SRC-01 | Source request A is in flight | The user picks source B before A answers | Only B is applied. When A's reply arrives, it is dropped: the queue does not change and no status appears. A's HTTP request does not have to be aborted. | yes | yes |
| SRC-02 | Source request A is in flight | The user picks B, then A fails | A's failure is dropped silently. There is no error status. | yes | yes |
| SRC-03 | A search whose best result is an artist or album is waiting for its second request | The user picks another source | The search's second reply is dropped too. A search is one source request, however many requests it makes. | yes | yes |
| SRC-04 | Any queue | A source request fails (network, HTTP error, bad JSON) | Error status. The queue and playback stay as they were. | yes | yes |

## What gets queued

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SRC-05 | A source reply lists tracks, some with `available: false` | It is applied | Tracks with `available: false` are left out of the queue. A track without an `available` field is available. | yes | yes |
| SRC-06 | Any queue, possibly playing | A source with at least one available track answers | The queue is replaced by the source's available tracks in the source's order. The previous track is closed ([TRK-06](tracking.md)). The first track starts playing. A status names the source and its track count. | yes | yes |
| SRC-07 | Any queue, possibly playing | A source answers with no tracks | Status "<source>: empty". The queue and playback stay as they were. This applies to every source: likes, playlist, album, artist, station and search (search: [SRC-12](#search)). A wave is covered by [WAVE-03](wave.md). | no: likes clear the queue (gap D4) | yes |
| SRC-08 | Any queue, possibly playing | A source answers with tracks that are all unavailable | Same as SRC-07: "<source>: empty", and nothing changes. The empty check runs after the filter of SRC-05. | no (gap D4) | yes |

## Search

`GET /search?text=<text>&type=all&page=0`. The text is trimmed, and an empty text sends nothing.
The reply's `best` gives a type, `best.result.id` and a name: `best.result.name`, or
`best.result.title` if there is no name.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SRC-09 | Search reply | `best.type` is `artist` and the id is not empty | `GET /artists/<id>/track-ids-by-rating`. The first 100 ids are loaded with `POST /tracks/` (`track-ids`, at most 250 per request). They are queued under the artist's name. | yes | yes |
| SRC-10 | Search reply | `best.type` is `album` and the id is not empty | `GET /albums/<id>/with-tracks`. The tracks of all volumes are joined in order and queued under the album's title. | yes | yes |
| SRC-11 | Search reply | `best` is missing or has any other type (`track`, `playlist`, `podcast`, …), or its id is empty | `tracks.results` is queued under the title "Search: <text>". A best track is not moved to the front, and a best playlist is not opened. | yes | yes |
| SRC-12 | Search reply | The list chosen by SRC-09, SRC-10 or SRC-11 is empty | Status "Nothing found". The queue stays. There is no fallback from an empty artist or album to `tracks.results`. | yes | yes |
