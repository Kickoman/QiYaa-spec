# Spotify fixtures

Replies of Spotify's servers that the apps parse for the catalogue and for playback
([player/spotify.md](../../player/spotify.md)), one file per case. What parsing each must give is in
[expected/spotify/](../../expected/spotify/README.md), under the same path. The access point's
messages and the request bodies are test vectors in [spotify/](../../spotify/README.md).

## Files

`<endpoint>/<case>.json`, for example `pathfinder-searchDesktop/best-artist.json`.

- **`<endpoint>`** is the request path without the leading slash, its version segment (`v0` to
  `v5`) and its parameters (an id, a username, a context URI), with `/` replaced by `-`, and a word
  repeated right after itself written once. `/extended-metadata/v0/extended-metadata` becomes
  `extended-metadata`, and `/playlist/v2/user/<username>/rootlist` becomes `playlist-user-rootlist`.
  Pathfinder answers every operation at one path, so its endpoints are `pathfinder-<operationName>`.
  apresolve is a host of its own: `apresolve`.
- **The HTTP status** is 200 unless the case name starts with another one:
  `412-outdated.json` is answered with 412.
- **A list page's offset.** The rootlist and a playlist are asked page by page with `from=<offset>`,
  and what follows a page depends on the offset asked, not on anything in the reply. A case named
  `page-<n>…` answers the request with `from=(n − 1) × 1000`; any other case of those endpoints
  answers `from=0`.
- **A JSON reply** is the body byte for byte, as in [fixtures/yandex/](../yandex/README.md#files).
  An empty body is an empty file.
- **A protobuf reply** cannot sit in a JSON file byte for byte, so it is wrapped:

  ```json
  {"contentType": "application/x-protobuf", "base64": "…", "fields": […]}
  ```

  - `base64` is the body, in standard base64 with padding: what a test serves.
  - `contentType` is the `Content-Type` to serve it with.
  - `fields` is the same body as protobuf fields, there to be read and reviewed, in the form of the
    `write` cases of [spotify/protobuf.json](../../spotify/README.md#protobufjson-the-wire-format):
    `{field, varint}` (a decimal string), `{field, string}`, `{field, bytes}` (hex) or
    `{field, message: […]}`. Writing them in order gives exactly the `base64` body, and
    `npm run check` makes sure of it.

  The endpoints with protobuf replies are those with a content type in the table below. A test
  helper for them serves `base64` decoded, with `contentType`; the JSON fixtures' helper would set
  `application/json`.

## Cases

| Endpoint | Request | Body | Cases |
|---|---|---|---|
| `extended-metadata` | `POST /extended-metadata/v0/extended-metadata` (spclient) | `application/x-protobuf` | `tracks`, `tracks-broken-entity`, `albums`, `artists` |
| `collection-paging` | `POST /collection/v2/paging` (spclient) | `application/vnd.collection-v2.spotify.proto` | `liked-page-1`, `liked-page-2`, `artists`, `empty` |
| `playlist-user-rootlist` | `GET /playlist/v2/user/<username>/rootlist?decorate=…&from=0&length=1000` (spclient) | `application/x-protobuf` | `folders`, `without-attributes` |
| `playlist` | `GET /playlist/v2/playlist/<id>?decorate=…&from=…&length=1000` (spclient) | `application/x-protobuf` | `page-1`, `page-2`, `page-2-without-position` |
| `storage-resolve-files-audio-interactive` | `GET /storage-resolve/files/audio/interactive/<file id>` (spclient) | `application/x-protobuf` | `cdn`, `cdn-without-urls`, `restricted`, `storage` |
| `context-resolve` | `GET /context-resolve/v1/spotify:station:track:<id>` (spclient) | JSON | `station` |
| `context-resolve-autoplay` | `POST /context-resolve/v1/autoplay` (spclient) | JSON | `liked-songs`, `loading` |
| `radio-apollo-tracks` | `GET /radio-apollo/v5/tracks/<station>?…` (spclient), a `next_page_url` | JSON | `last-page` |
| `pathfinder-home` | `POST /pathfinder/v2/query` (Pathfinder), operation `home` | JSON | `made-for-you`, `no-made-for-you`, `no-section-container` |
| `pathfinder-searchDesktop` | `POST /pathfinder/v2/query` (Pathfinder), operation `searchDesktop` | JSON | `412-outdated`, `500-with-data`, `all-parts`, `best-album`, `best-artist`, `no-data`, `nothing-found`, `tracks-only` |
| `apresolve` | `GET /?type=accesspoint&type=spclient` (apresolve.spotify.com) | JSON | `503-no-body`, `blank-entries`, `empty`, `ok` |

What the cases hold:

- **`extended-metadata`:** each reply answers in the reverse of the order its URIs were asked in, as
  the desktop's test server does, so that matching by URI is needed.
  - `tracks`: `TRACK_V4` for nine URIs.
    - `Plays`: Ogg Vorbis 96, 160 and 320, MP3 320, FLAC and AAC; two artists; the number, disc and
      popularity fields, which nothing reads.
    - `Relinked`: no files, and an alternative with Ogg Vorbis 320.
    - `Gone`: no files and no alternative.
    - `Lossless only`: FLAC, FLAC 24-bit and AAC, with covers of the size classes 3 and 2.
    - `Middle`: FLAC, Ogg Vorbis 160 and MP3 320, without covers or a date.
    - `Restricted elsewhere`: a restriction (field 11) and an Ogg Vorbis 160 file.
    - `Width before size class`: covers whose width does not match their size class: class 1 at
      500 px, class 2 with a width of 0, class 3 with a negative width, class 0 at 120 px.
    - A 404 entity, and a 200 entity without a payload.
  - `tracks-broken-entity`: a whole entity, then one whose bytes are cut short.
  - `albums`: `ALBUM_V4`. `Double` has two discs, `Single` one, and `Discs out of order` writes
    disc 2 before disc 1. There is also a 404 entity.
  - `artists`: `ARTIST_V4`.
    - `Band` has top tracks for `ZZ` and `BY`.
    - `Worldwide` has an empty `BY` list and twelve `ZZ` tracks.
    - `No country` has a list without a country.
- **`collection-paging`:** the `collection` set in two pages: tracks, an album and a removed track,
  then the last page. Also the `artist` set, and a page without items that still carries a token.
- **`playlist-user-rootlist`:**
  - `folders` is the rootlist with `decorate=…attributes…status_code`: a folder with a playlist and a
    playlist of status 403 in it, a playlist outside it, and one with an explicit status 200.
  - `without-attributes` is the same list without meta items.
- **`playlist`:** two pages of a playlist of 1003 entries.
  - The first page holds a track, an episode, a local file and a track. The second, asked from 1000,
    holds three tracks.
  - `page-2-without-position` is the second page without the reply's position (field 5.1).
  - The pages are shorter than the 1000 entries a real page has. Only `length` and the offset asked
    decide whether another page follows.
- **`storage-resolve-files-audio-interactive`:** CDN links; CDN without links; `RESTRICTED`;
  `STORAGE`.
- **`context-resolve`, `-autoplay`, `radio-apollo-tracks`:**
  - a station with two pages, the first holding an episode;
  - Liked Songs' autoplay with a next page;
  - an autoplay that is still loading;
  - the last page reached through a `next_page_url`.
- **`pathfinder-home`:** the Made For You section among others, with a card that is not a playlist;
  a home without that section; a home without a section container.
- **`pathfinder-searchDesktop`:**
  - the best result an artist, or an album;
  - the best result a track, and a found track without its `data` wrapper;
  - every part: a best track, tracks, artists and albums (an album, a single, one without a date),
    each list with an entry that is not of its kind;
  - nothing found;
  - an outdated query hash, as 412 with no body and as 200 without `data`;
  - a 500 whose body has `data` all the same.
- **`apresolve`:** access points and spclients; empty lists; lists with empty entries; a 503 without
  a body.

## Where they come from

All are synthetic, hand-written in the shape of the replies the desktop reads (its parsers' field
numbers and JSON paths, `src/spotify/protocol` and `src/sources/spotify` of Kickoman/QiYaa). The
generator in [scripts/spotify-vectors](../../scripts/spotify-vectors) builds them and writes their
expected files ([expected/spotify/](../../expected/spotify/README.md#changing-them)).

- **From the desktop's tests** (`tests/spotify_source_test.cpp`, `tests/spotify_protocol_test.cpp`):
  - the shapes of tracks, albums and artists: the `Band` artist, the `Album` of 1991 with covers of
    64, 300 and 640 px, files keyed by mark, and the relinked track;
  - the double album and the `ZZ`/`BY` top tracks;
  - the reverse-order metadata answers;
  - the rootlist with a folder;
  - the Made For You section with a `NotFound` card;
  - the search replies and the autoplay context with its `hm://radio-apollo/…` next page;
  - the lossless-only and the FLAC, 160, MP3 sets of files.
- **Made for these fixtures:** the rest. Ids are the base62 form of gids filled with one byte, as in
  the desktop's tests. Usernames (`owner`), tokens, uids and CDN links are made up. The apresolve
  host names follow Spotify's public pattern. Nothing comes from a real account or a real reply.

Adding a case: add it to the generator, run it, make the desktop and Android tests read it, and add
it to the table above.
