# Yandex Music API fixtures

Response bodies as the server sends them, one file per case. What parsing each must give is in
[expected/yandex/](../../expected/yandex/README.md), under the same path.

## Files

`<endpoint>/<case>.json`, for example `search/best-artist.json`.

- **The body, byte for byte.** A reply that comes wrapped (`{"invocationInfo":…,"result":…}`)
  is kept wrapped. `wheel-new`, `oauth-*` and `storage-download-info` are not wrapped, because
  the server does not wrap them. An empty body is an empty file. A body that is not JSON (the
  XML in `storage-download-info/xml.json`) is kept as it is, despite the extension.
- **`<endpoint>`** is the request path without the leading slash and without its parameters
  (uid, ids, playlist kind, session id), with `/` replaced by `-`.
  `/users/42/likes/tracks` becomes `users-likes-tracks`, and
  `/rotor/session/<id>/tracks` becomes `rotor-session-tracks`. Requests to other hosts carry the
  host's role: `oauth-*` for oauth.yandex.ru, and `storage-download-info` for the link that a
  `downloadInfoUrl` points to.
- **The HTTP status** is 200 unless the case name starts with another one:
  `401-session-expired.json` is answered with 401.

## Cases

| Endpoint | Request | Cases |
|---|---|---|
| `account-status` | `GET /account/status` | `ok`, `500-empty` |
| `users-likes-tracks` | `GET /users/<uid>/likes/tracks` | `string-ids`, `number-ids` |
| `tracks` | `POST /tracks/` | `two-tracks`, `with-unavailable`, `version-and-artists`, `cover-from-album`, `cover-order` |
| `users-playlists-list` | `GET /users/<uid>/playlists/list` | `ok` |
| `users-playlists` | `GET /users/<uid>/playlists/<kind>` | `embedded-tracks`, `ids-only` |
| `users-playlists-recommendations` | `GET /users/<uid>/playlists/<kind>/recommendations` | `ok` |
| `landing3` | `GET /landing3?blocks=personalplaylists` | `personal-playlists` |
| `users-likes-artists` | `GET /users/<uid>/likes/artists` | `ok`, `401-session-expired` |
| `artists-track-ids-by-rating` | `GET /artists/<id>/track-ids-by-rating` | `ok`, `more-than-100` |
| `users-likes-albums` | `GET /users/<uid>/likes/albums` | `ok` |
| `albums` | `POST /albums` | `with-podcast` |
| `albums-with-tracks` | `GET /albums/<id>/with-tracks` | `two-volumes` |
| `rotor-stations-list` | `GET /rotor/stations/list` | `ok` |
| `rotor-session-new` | `POST /rotor/session/new` | `ok` |
| `rotor-session-tracks` | `POST /rotor/session/<id>/tracks` | `ok` |
| `rotor-session-feedback` | `POST /rotor/session/<id>/feedback` | `ok`, `404-not-found`, `503-string-error` |
| `rotor-station-feedback` | `POST /rotor/station/<id>/feedback` | `ok` |
| `wheel-new` | `POST /wheel/new` | `ok` |
| `search` | `GET /search?type=all` | `best-artist`, `best-album`, `best-track`, `best-playlist`, `best-podcast`, `no-best` |
| `tracks-download-info` | `GET /tracks/<id>/download-info` | `variants`, `previews-only`, `empty` |
| `storage-download-info` | `GET <downloadInfoUrl>&format=json` | `ok`, `number-ts`, `xml` |
| `users-likes-tracks-add-multiple`, `users-likes-tracks-remove`, `users-dislikes-tracks-add-multiple` | `POST /users/<uid>/…` | `ok` |
| `play-audio` | `POST /play-audio` | `ok` |
| `oauth-device-code` | `POST /device/code` | `ok`, `400-invalid-client` |
| `oauth-token` | `POST /token` | `ok`, `400-authorization-pending`, `400-bad-verification-code` |

The error bodies cover every form the apps must read: `{"error":{"message"}}` (401, 404),
`{"error":"…"}` (503) and an empty body (500). Any fixture can stand in for any endpoint's error,
since both apps read errors the same way everywhere.

## Where they come from

All fixtures so far are hand-written in the shape of real responses: the same envelope, field
names and types. The account is uid 42, login `kick`. When a real response replaces one, scrub
it first. That means tokens, uids, logins, names, e-mails, `req-id`, and the `sign`, `ts` and
`nonce` of links. Then update its expected file in the same commit.

Adding a case: add the fixture and its expected file, make the desktop and Android tests read
it, and add it to the table above.
