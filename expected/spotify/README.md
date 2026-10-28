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
| `extended-metadata/tracks*` | `{"entities": [{"uri", "status", "track": Track \| null}]}` |
| `extended-metadata/albums` | `{"entities": [{"uri", "status", "album": {"id", "name", "artists", "trackIds"} \| null}]}` |
| `extended-metadata/artists` | `{"entities": [{"uri", "status", "artist": {"id", "name", "topTracks": [{"country", "trackIds"}], "topTracksIn": {"BY", "PL", ""}} \| null}]}` |
| `collection-paging/*` | `{"items": [{"uri", "addedAt"}], "nextPageToken", "last"}` |
| `playlist-user-rootlist/*` | `{"requestedFrom", "name", "length", "from", "truncated", "items": [{"uri", "name"}], "more", "playlists": [{"uri", "name"}]}` |
| `playlist/*` | `{"requestedFrom", "name", "length", "from", "truncated", "items": [{"uri", "name"}], "more", "trackUris"}` |
| `storage-resolve-files-audio-interactive/*` | `{"result", "urls", "usable"}` |
| `context-resolve/*`, `context-resolve-autoplay/*`, `radio-apollo-tracks/*` | `{"trackUris", "next"}`, or `{"unreadable": true}` |
| `pathfinder-home/*` | `{"stale": false, "playlists": [{"uri", "name"}]}`, or one of Pathfinder's other forms |
| `pathfinder-searchDesktop/*` | `{"stale": false, "kind", "best": {"uri", "name"} \| null, "trackUris", "artists": [{"uri", "name"}], "albums": [{"uri", "name", "artists", "year", "single"}]}`, or one of Pathfinder's other forms |
| `apresolve/*` | `{"accessPoints", "spclients"}` |

- `artists` is `[{"id", "name"}]` in order. `trackIds` are ids in order.
- **A protobuf reply that cannot be read as a whole** is `{"unreadable": true}`, at any of the
  protobuf endpoints (`extended-metadata/tracks-broken-entity`).
- **Pathfinder's other forms** (see [Pathfinder](#pathfinder)):
  - `{"stale": false, "unreadable": true}`: `data` without the part to read;
  - `{"stale": false, "error": {"status"}}`: an HTTP error, its body not read;
  - `{"stale": true, "withNoHashLeft": {"error": {"status"}} | {"unreadable": true}}`: a stale
    reply, and what it gives when it answers the last known hash.
- **`requestedFrom` is not in the reply.** It is the `from` the page was asked with, given because
  `more` depends on it. The fixtures' case names carry it: `page-<n>` is
  `(n − 1) × 1000`, any other case 0.

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
  The list of tracks it answers follows the URIs it was given, duplicates included: a track twice in
  a playlist plays twice (`library.cpp:246–261`).
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
- An album is {1 gid, 2 name, 3 artist, 11 disc {1 number, not read; 3 {1 track gid}}}. `trackIds`
  are disc by disc in the order of the fields 11, not by their numbers, then in the disc's order. A
  disc whose reader fails makes the album unreadable.
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
- Saved albums and followed artists (SPOT-09) take their names from their `ALBUM_V4` and `ARTIST_V4`
  metadata (`Library::names`, `library.cpp:424–449`). One without metadata, with a payload that does
  not parse, or with an empty name is left out.

**The rootlist and playlists**
- A page is {2 length, 3 {1 name}, 5 {1 from, 2 truncated, 3 item {1 URI}, 4 meta item {2 {1 name},
  9 status, zigzag}}}.
- When there are as many meta items as items, item i takes meta item i's name, and it is left out
  unless that status is 0 or 200. When the counts differ (a request without attributes), every item
  stays, without a name.
- **`more` is `requestedFrom + 1000 < length`**, with the offset the app asked: 0, then 1000 more
  each time (`library.cpp:335–338`). The app then asks again from `requestedFrom + 1000`. The reply's
  own position (field 5.1, `from`) and `truncated` are parsed here, but the desktop reads neither: a
  reply without a position (`playlist/page-2-without-position`) pages the same way.
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

**Pathfinder.** A reply is taken in this order (`library.cpp:499–512`):
1. **Stale:** status 400, 405 or 412, or a 2xx reply whose `data` is not an object. The app asks again
   with the next known query hash. With none left, telemetry reports the `spotify` error
   `query-outdated` ([telemetry/](../../telemetry/README.md)), and the reply goes on to the steps
   below: a stale 412 then ends as an HTTP error, a stale 200 as unreadable (`withNoHashLeft`).
   These statuses are those of the Pathfinder request itself: a client-token or login5 request that
   fails before it with the same status is not a stale hash, and Android reports it as what it is.
   The desktop counts it as stale too (Kickoman/QiYaa#72).
2. **Any other status that is not 2xx** is an HTTP error ("Spotify: HTTP <status>"); its body is not
   read, even when it has `data` (`500-with-data`).
3. **A 2xx reply whose `data` is not an object** is unreadable. Only a stale reply with no hash left
   gets here.
4. **Otherwise** the reply is read from `data`, as below.
- **Search**, from `data.searchV2`; without it the reply is unreadable.
  - `trackUris` are `tracksV2.items[].item.data.uri` (or `item.uri` when `item` has no `data`) that
    start with `spotify:track:`.
  - The best result is `topResultsV2.itemsV2[0]` alone, through the same `data`:
    - `__typename` `Artist` with a `spotify:artist:` URI is `kind` `artist`, its name
      `profile.name`;
    - `Track` with a `spotify:track:` URI is `track`, its name `name`;
    - `Album` with a `spotify:album:` URI is `album`, its name `name`;
    - anything else is `tracks` with `best` `null`.
  - `artists` are `artists.items[]`, through `item` when there is one and then `data`, whose `uri`
    starts with `spotify:artist:`: the `uri` and `profile.name`, in order.
  - `albums` are `albumsV2.items[]` the same way, with `spotify:album:` URIs: `uri`, `name`, the
    `profile.name` of each of `artists.items`, `date.year` (`null` when missing or 0), and `single`
    when `type` is `SINGLE`.
  - The shape of `artists` and `albumsV2` is written from Pathfinder's web client, not checked
    against a live reply yet.
  - Nothing found is `tracks` without `trackUris` ([SRC-19](../../player/sources.md#search)); with
    the parts of [SPOT-11](../../player/spotify.md#picks), also without `artists` and `albums`.
- **Home**, from `data.home.sectionContainer`; without it the reply is unreadable.
  - The section is the item of `sections.items` whose `uri` is
    `spotify:section:0JQ5DAUnp4wcj0bCb3wh3S`, Made For You.
  - `playlists` are its `sectionItems.items[].content.data` with `__typename` `Playlist`, in order.
    Without the section the list is empty.

**apresolve:** the strings of `accesspoint` and `spclient`, without empty ones. An empty access point
list becomes `ap.spotify.com:443`. An empty spclient list becomes `spclient.wg.spotify.com:443`,
unless the session already had spclient hosts, which it then keeps; the files show a new session.
The status is never looked at. A failed request, a network failure or any HTTP status, is read the
same way: without usable lists in its body it gets both fallbacks (`session.cpp:120–138`,
`503-no-body`).

**Two kinds of unreadable.** Lines are those of `src/sources/spotify/library.cpp` at the commit
below.
- **A reply that cannot be used as a whole is an error.** The call fails with "Spotify sent an
  answer QiYaa cannot read", and telemetry reports the `spotify` error `content`. This is
  `{"unreadable": true}` in these files:
  - the extended-metadata envelope (`:235`);
  - a collection page (`:293`) and a list page (`:330`);
  - a context (`:593`);
  - Pathfinder's search and home (`:569`, `:539`).

  Pathfinder's step 3 (`:510`) is the same error, with `query-outdated` reported instead.
- **A payload inside a readable reply that does not parse is skipped, silently.**
  - A track whose payload fails is left out of its list (`:258`).
  - An album or artist whose payload fails plays nothing, without an error (`:386–388`, `:410–412`).
  - A saved album or followed artist whose payload fails is left out of the list (`:437–445`).

  No fixture has such a payload.

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
  were last generated against the desktop's `search-sources-73` branch (Kickoman/QiYaa#73), whose
  commit is named in the spec commit that changed them.

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
