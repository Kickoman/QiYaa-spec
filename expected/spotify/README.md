# Expected Spotify parse results

For each fixture in [fixtures/spotify/](../../fixtures/spotify/README.md), what parsing it must give,
at the same path. The form is a neutral JSON, as for [expected/yandex/](../yandex/README.md): the
field names are this spec's, and each app's test converts what it parsed to this form and compares
the two objects. The desktop's own parsers write these files (see [Changing them](#changing-them)).

Every file is one JSON object. Bytes are lower-case hex. A track, album or artist id is base62, the
`<id>` of `spotify:track:<id>`.

## Shapes

| Fixtures | Expected object |
|---|---|
| `extended-metadata/tracks` | `{"entities": [{"uri", "status", "track": Track \| null}]}` |
| `extended-metadata/albums` | `{"entities": [{"uri", "status", "album": {"id", "name", "artists", "trackIds"} \| null}]}` |
| `extended-metadata/artists` | `{"entities": [{"uri", "status", "artist": {"id", "name", "topTracks": [{"country", "trackIds"}], "topTracksIn": {"BY", "PL", ""}} \| null}]}` |
| `collection-paging/*` | `{"items": [{"uri", "addedAt"}], "nextPageToken", "last"}` |
| `playlist-user-rootlist/*` | `{"name", "length", "from", "truncated", "items": [{"uri", "name"}], "more", "playlists": [{"uri", "name"}]}` |
| `playlist/*` | `{"name", "length", "from", "truncated", "items": [{"uri", "name"}], "more", "trackUris"}` |
| `storage-resolve-files-audio-interactive/*` | `{"result", "urls", "usable"}` |
| `context-resolve/*`, `context-resolve-autoplay/*`, `radio-apollo-tracks/*` | `{"trackUris", "next"}`, or `{"unreadable": true}` |
| `pathfinder-home/*` | `{"stale": false, "playlists": [{"uri", "name"}]}`, `{"stale": false, "unreadable": true}`, or `{"stale": true}` |
| `pathfinder-searchDesktop/*` | `{"stale": false, "kind", "best": {"uri", "name"} \| null, "trackUris"}`, or `{"stale": true}` |
| `apresolve/*` | `{"accessPoints", "spclients"}` |

`artists` is `[{"id", "name"}]` in order. `trackIds` are ids in order.

### Track

| Field | Rule |
|---|---|
| `id`, `name` | Fields 1 (the gid) and 2 |
| `artists` | `[{"id", "name"}]`: every field 4 {1 gid, 2 name}, in order |
| `album` | `{"id", "name", "year", "covers": [{"id", "pixels"}]}`: field 3 {1 gid, 2 name, 6 date {1 year, zigzag}, 17 {1 image, repeated}}; `year` is 0 without a date |
| `durationMs` | Field 7, zigzag |
| `files` | `[{"id", "format"}]`: every field 12 {1 file id, 2 format}, in order |
| `alternatives` | Every field 13, a track of its own, in order. It has the fields above, not the three below |
| `available` | Whether the track plays: an Ogg Vorbis file of its own or of an alternative |
| `coverUrls` | `{"64", "400", "1000"}`: the cover URL for that many pixels, or `null` without covers |
| `play` | `{"96", "160", "320"}`: `{"trackId", "fileId", "format"}`, the file to play when that bitrate is asked, or `null` |

## The parsing rules

What an app must get right, beyond the field numbers above.

**extended-metadata**
- The reply is field 2 {3 entity, repeated}. An entity is {1 header {1 status}, 2 URI, 3 {1 type
  URL, 2 payload}}.
- **The entities come in the server's order, not the request's: match them by `uri`.** An entity
  counts only with status 200 and a payload. Any other entity (404, or 200 without a payload) is
  missing: a list of tracks leaves that track out. An entity whose reader fails makes the whole
  reply unreadable.
- The desktop asks at most 100 URIs per request, sends the batches at once and asks each URI once.
- Everything not listed is ignored. That includes the restrictions (field 11): a track the
  account's country may not play comes without files, relinked to an alternative that has them.
- **A file or an image without a 20-byte id is dropped.** So is a file, an image or an alternative
  whose reader fails. A failing reader elsewhere in a track (album, artist, date) keeps what it read.
  Ids of other lengths stay all zero.

**Covers**
- An image's `pixels` is its width (field 3, zigzag) when that is above 0. Otherwise it comes from
  the size class (field 2): 1 is 64, 2 is 640, 3 is 1280, and anything else (0, the default) is 300.
- `coverUrls[n]` is `https://i.scdn.co/image/<id>` of the smallest cover at least n pixels wide, or
  of the largest one when none is. Among equals, the first wins.
- The desktop asks for 400 (`kCoverPixels`), and so does Android (SPOT-19).

**Playing a track (SPOT-13, SPOT-15)**
- Only Ogg Vorbis plays: format 0 is 96 kbps, 1 is 160, 2 is 320. MP3, FLAC, AAC and the rest never
  do.
- `play[k]` is the Ogg Vorbis file whose bitrate is closest to k, the first among equals. It comes
  from the track's own files; without any, from its alternatives in order, each searched the same
  way. `trackId` is the gid of the track that has the file, the alternative's for a relinked track:
  the audio key is asked for that gid.
- `available` is `play["320"] != null`. A track that is not available stays in its list, marked.

**Albums and artists**
- An album is {1 gid, 2 name, 3 artist, 11 disc {1 number, 3 {1 track gid}}}. `trackIds` are disc by
  disc, then in the disc's order. A disc whose reader fails makes the album unreadable.
- An artist is {1 gid, 2 name, 4 {1 country, 2 {1 track gid}}}. An entry whose reader fails makes the
  artist unreadable.
- `topTracksIn[c]` is the first non-empty list for country c, else for `ZZ` (worldwide), else for
  `""` (no country); an empty list does not count. The country is the account's, from the access
  point. Until it arrives it is `""`, which also tries the list without a country first.
- The desktop plays the first 10 (`kTopTracks`).

**The collection**
- A page is {1 item {1 URI, 2 added at, 3 removed}, 2 next page token, 3 sync token, ignored}.
- Removed items, items without a URI and items whose reader fails are dropped.
- `last` is whether the token is empty or the page has no items: the app stops there, and otherwise
  asks again with the token.
- Liked Songs are the `spotify:track:` items of the `collection` set, saved albums its
  `spotify:album:` items, followed artists the `artist` set. They stay in Spotify's order, newest
  first.

**The rootlist and playlists**
- A page is {2 length, 3 {1 name}, 5 {1 from, 2 truncated, 3 item {1 URI}, 4 meta item {2 {1 name},
  9 status, zigzag}}}.
- When there are as many meta items as items, item i takes meta item i's name, and it is left out
  unless that status is 0 or 200. When the counts differ (a request without attributes), every item
  stays, without a name.
- `more` is `from + 1000 < length`: the app then asks again from `from + 1000`. `truncated` is not
  read.
- `playlists` are the rootlist's `spotify:playlist:` items, so a folder's start and end markers fall
  away and folders flatten. A playlist plays its `trackUris`, the `spotify:track:` items: episodes
  and local files are never asked for.

**storage-resolve:** {1 result, 2 CDN URL, repeated}. Results: 0 CDN, 1 storage, 3 restricted.
`usable` holds only for 0 with at least one URL, and the app tries the URLs in order.

**Contexts** (a station, autoplay, their next pages)
- When the root has `pages`, the first page alone is read. Otherwise the root itself is the page.
- `trackUris` are the `uri` of its `tracks` that start with `spotify:track:`; other items are dropped.
- `next` is its `next_page_url`, `""` at the end. The next page is `GET` on spclient of that URL
  without its `hm:/` (`hm://radio-apollo/v5/…` is `/radio-apollo/v5/…`). A URL that does not start with
  `hm://` cannot be followed.
- `{"loading": true}`, or a body that is not a JSON object, is unreadable.

**Pathfinder**
- `stale` holds for status 400, 405 or 412, and for a 2xx reply whose `data` is not an object. The
  app then asks with the next known query hash; with none left it fails, and telemetry reports the
  `spotify` error `query-outdated` ([telemetry/](../../telemetry/README.md)). Any other reply is read
  from `data`.
- **Search**, from `data.searchV2`; without it the reply is unreadable.
  - `trackUris` are `tracksV2.items[].item.data.uri` (or `item.uri` when `item` has no `data`) that
    start with `spotify:track:`.
  - The best result is `topResultsV2.itemsV2[0]` alone, through the same `data`:
    - `__typename` `Artist` with a `spotify:artist:` URI is `kind` `artist`, its name
      `profile.name`;
    - `Album` with a `spotify:album:` URI is `album`, its name `name`;
    - anything else is `tracks` with `best` `null`.
  - Nothing found is `tracks` without `trackUris` (SRC-12).
- **Home**, from `data.home.sectionContainer`; without it the reply is unreadable.
  - The section is the item of `sections.items` whose `uri` is
    `spotify:section:0JQ5DAUnp4wcj0bCb3wh3S`, Made For You.
  - `playlists` are its `sectionItems.items[].content.data` with `__typename` `Playlist`, in order.
    Without the section the list is empty.

**apresolve:** the strings of `accesspoint` and `spclient`, without empty ones. An empty access point
list becomes `ap.spotify.com:443`. An empty spclient list becomes `spclient.wg.spotify.com:443`,
unless the session already had spclient hosts, which it then keeps; the files show a new session.

## Comparing

As for Yandex: an app compares the whole object. If its model does not keep a field, it removes that
field from both sides, and its test says which ones and why. Android, for example, keeps one cover
URL and compares it with `coverUrls["400"]`. An app never removes a field that its model does have.

## Changing them

The generator writes the fixtures and these files together:

```bash
make -C scripts/spotify-vectors catalog DESKTOP=/path/to/QiYaa BUILD=/tmp/spotify-vectors QT=/path/to/Qt/6.x/gcc_64
```

- It is the `catalog` target of [scripts/spotify-vectors](../../scripts/spotify-vectors), built against
  the desktop like the `spotify/` vectors, and compiled anew on every run. It also needs Qt 6
  (QtCore and QtNetwork, the installation the desktop builds with, `moc` included). It opens no
  socket.
- `SPEC` (default `../..`, this repository) is where it writes `fixtures/spotify/` and
  `expected/spotify/`. It replaces their endpoint folders whole, and leaves the READMEs alone.
- The output is deterministic: running it again on unchanged code gives the same files. The files
  were last generated against Kickoman/QiYaa `b99c1dfb8a8eb90b49316b2e146a9c753448a0d3` (v0.9.0).

**Where each value comes from.** The generator parses each fixture's very bytes with the desktop's
code:
- `ParseMetadataReply`, `ParseTrack`, `ParseAlbum`, `ParseArtist`, `TopTracksIn`, `PickOggVorbis`,
  `ParseCollectionPage`, `ParseListPage`, `PlaylistsOf` and `ParseStorageResolve` of
  `src/spotify/protocol`;
- `TrackOf` and `CoverUrlOf` of `src/sources/spotify/tracks.cpp`;
- `ContextOf`, `FoundOf`, `MadeForYouOf`, `StaleQuery` and `WithPrefix` of
  `src/sources/spotify/library.cpp`, and `StringsOf` of `src/spotify/client/session.cpp`.

  These live in anonymous namespaces, so the generator includes the two `.cpp` files. The `Web`
  methods they would need are stubs that abort if called; none is.

A few one-line rules sit inside the desktop's network callbacks, which the generator cannot call
without a network. It writes them out from the desktop's lines:
- `last` (`Library::collectionPage`);
- `more` (`Library::listUris`);
- `usable` (`FileDownload`, `file_download.cpp`);
- the order stale, then `data`, then parse (`Library::pathfinder`);
- apresolve's fallbacks (`Session::resolve`).

`npm run check` ([scripts/check-spotify-fixtures.mjs](../../scripts/check-spotify-fixtures.mjs))
recomputes every expected file from its fixture with parsers of its own, written from this README.

A change in these files is a change in behaviour: it needs an issue for the other app.
