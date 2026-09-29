# Expected parse results

For each fixture in [fixtures/yandex/](../../fixtures/yandex/README.md), what parsing it must
give, at the same path. The form is a neutral JSON: the field names are this spec's, not either
app's. Each app's test converts what it parsed to this form and compares the two objects. The
desktop converters are in `tests/support/yandex_json.cpp` of Kickoman/QiYaa.

Every file is one JSON object.

## Shapes

| Fixtures | Expected object |
|---|---|
| `account-status/ok` | `{"uid", "login", "displayName"}` |
| `users-likes-tracks`, `users-playlists/ids-only`, `artists-track-ids-by-rating` | `{"trackIds": [...]}`: the ids the app then asks `POST /tracks/` for, in order. For an artist that is at most the first 100 |
| `users-likes-albums` | `{"albumIds": [...]}`: the ids the app then asks `POST /albums` for |
| `tracks`, `users-playlists/embedded-tracks`, `users-playlists-recommendations`, `albums-with-tracks` | `{"tracks": [Track]}` |
| `users-playlists-list`, `landing3` | `{"playlists": [{"ownerUid", "kind", "title", "trackCount"}]}` |
| `users-likes-artists` | `{"artists": [{"id", "name"}]}` |
| `albums` | `{"albums": [{"id", "name"}]}`: podcasts left out; the name is `"<first artist> - <title>"`, or the title alone |
| `rotor-stations-list` | `{"stations": [{"id", "type", "name"}]}`: `id` is `"<type>:<tag>"` |
| `rotor-session-new`, `rotor-session-tracks` | `{"sessionId", "batchId", "tracks": [Track]}` |
| `wheel-new` | `{"waves": [{"name", "description", "seeds"}]}`: only items of type `WAVE` with a name and seeds |
| `search/*` | `{"bestType", "bestId", "bestName", "tracks": [Track]}` |
| `tracks-download-info/*` | `{"variants": [{"codec", "bitrateKbps", "preview", "downloadInfoUrl"}], "best"}`: `best` is the `downloadInfoUrl` of the chosen variant, or `null` |
| `storage-download-info/*` | `{"host", "path", "ts", "s", "trackUrl"}`: `trackUrl` is the signed `https://<host>/get-mp3/<sign>/<ts><path>`; `{"invalid": true}` when the reply is unusable |
| `oauth-device-code/ok` | `{"deviceCode", "userCode", "verificationUrl", "intervalSeconds", "expiresInSeconds"}` |
| `oauth-token/ok` | `{"accessToken"}` |
| `oauth-token/400-authorization-pending` | `{"pending": true}`: keep polling, not an error |
| replies whose body does not matter (likes, dislikes, `play-audio`, feedback) | `{"ok": true}` |
| error cases (status ≥ 400) | `{"error": {"status", "message"}}`: the app's error must name the status and contain the message. A `null` message means the body has none, and the app's own text is accepted |

## Track

| Field | Type | Rule |
|---|---|---|
| `id` | string | `id`; a number becomes its decimal string |
| `albumId` | string | `id` of the first of `albums`. In `albums-with-tracks`, a track without albums gets the requested album's id |
| `title` | string | `title`, plus `" (<version>)"` when `version` is not empty |
| `artists` | string[] | `name` of each of `artists`, in order |
| `durationMs` | number | `durationMs`, 0 when missing |
| `available` | bool | `available`, `true` when missing |
| `albumTitle`, `year`, `genre` | string, number, string | from the first of `albums`; `""`, 0, `""` when missing |
| `coverUri` | string | as in the API, with `%%` still in it: the first album's `coverUri`, else the track's `coverUri`, else its `ogImage`, else `""` |

`bestType` in search is `artist`, `album`, `track`, `playlist`, `other` for any other type, or
`""` without `best`. `bestName` is `name`, or `title` when there is no `name`. `bestId` is `""`
when the result has no `id`.

## Comparing

An app compares the whole object. If its model does not keep a field, it removes that field from
both sides, and its test says which ones and why. For example, a model that stores a cover URL
instead of `coverUri` compares the URL built from the expected `coverUri`. An app never removes a
field that its model does have.
