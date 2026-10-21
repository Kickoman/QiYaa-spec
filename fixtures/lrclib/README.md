# LRCLIB fixtures

Replies of [LRCLIB](https://lrclib.net), the lyrics service the apps ask when the user turns it on
([player/lyrics.md](../../player/lyrics.md#lrclib)). What each must give is in
[expected/lrclib/](../../expected/lrclib/README.md), under the same path.

`<endpoint>/<case>.json`, as in [fixtures/yandex/](../yandex/README.md#files): the body byte for
byte, the status 200 unless the case name starts with another one.

| Endpoint | Request | Cases |
|---|---|---|
| `search` | `GET /api/search?track_name=…&artist_name=…` | `several-durations`, `plain-only`, `instrumental`, `empty`, `503-server-overloaded` |

The records are hand-written in the shape of the replies of 2026-10-21: the same fields, in the
same order, with made-up titles and texts (real lyrics are not ours to copy). `lyricsfile`, which
the apps do not read, is `null` here.
