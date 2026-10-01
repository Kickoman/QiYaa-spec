# Examples

One folder per message `type`, one message per file, exactly as it goes over the wire.
`check` validates every file against `schemas/messages/<type>.schema.json`: the others must
pass, `invalid-*` must fail. Each invalid example is listed here with the reason.

| File | Why it is invalid |
|---|---|
| `hello/invalid-unknown-app.json` | `app` is not one of `desktop`, `android`, `web`. |
| `hello/invalid-no-protocol.json` | `protocol` is missing. |
| `welcome/invalid-no-server-time.json` | `serverTime` is missing: clients need it for the clock offset. |
| `rejected/invalid-unknown-reason.json` | `reason` is not in the reason table. |
| `ack/invalid-no-id.json` | `id` is missing: an `ack` always answers a request. |
| `create/invalid-long-name.json` | `hostName` is longer than 24 characters. |
| `created/invalid-url-without-secret.json` | `joinUrl` has no `#<joinSecret>` fragment. |
| `resume/invalid-snapshot-with-join-secret.json` | The snapshot carries `joinSecret`; a snapshot holds secrets only as hashes. |
| `resume/invalid-outbox-with-playing.json` | The outbox holds only `started` events. |
| `resumed/invalid-no-restored.json` | `restored` is missing. |
| `playing/invalid-wave-without-track.json` | A wave track is not in the queue, so `track` is required. |
| `playing/invalid-item-without-item-id.json` | `itemId` is required for `source: item`. |
| `playing/invalid-negative-position.json` | `positionMs` is negative. |
| `started/invalid-number-id.json` | `itemId` is not `i` + a number. |
| `add/invalid-both.json` | Both `trackId` and `track`: exactly one of them is allowed. |
| `add/invalid-neither.json` | Neither `trackId` nor `track`. |
| `add/invalid-cover-without-size.json` | `coverUri` is not a template: it has no `%%` for the size. |
| `pin/invalid-no-item-id.json` | `itemId` is missing. |
| `remove/invalid-no-id.json` | `id` is missing: every request carries one. |
| `kick/invalid-participant-id.json` | A guest is named by `publicId`; the host never sees a `participantId`. |
| `settings/invalid-empty.json` | `settings` changes nothing. |
| `settings/invalid-too-many-pending.json` | `maxPendingPerGuest` is above 50. |
| `settings/invalid-flat.json` | Settings go inside `settings`, not next to `id`. |
| `rotateLink/invalid-no-id.json` | `id` is missing: the reply `linkRotated` needs it. |
| `searchResult/invalid-tracks-and-error.json` | Both `tracks` and `error`: exactly one of them is allowed. |
| `searchResult/invalid-too-many.json` | More than 20 tracks. |
| `validateResult/invalid-track-and-reason.json` | A result carries either `track` or `reason`, not both. |
| `validateResult/invalid-no-results.json` | `results` is empty. |
| `join/invalid-name-with-spaces.json` | `name` has spaces at its ends: the client trims it before sending. |
| `join/invalid-participant-not-uuid.json` | `participantId` is not a UUID v4. |
| `join/invalid-upper-case-room.json` | `roomId` is upper case; room ids are lower-case Crockford base32. |
| `search/invalid-blank.json` | `text` is only spaces. |
| `search/invalid-too-long.json` | `text` is longer than 100 characters. |
| `skip/invalid-no-item-id.json` | `itemId` is missing: a skip names the item it means. |
| `searchResults/invalid-no-tracks.json` | `tracks` is missing; an empty result is `[]`. |
| `validateRequest/invalid-empty.json` | `trackIds` is empty. |
| `command/invalid-unknown-kind.json` | `kind` is not `skip`. |
| `snapshot/invalid-participant-id.json` | A participant carries its `participantId`; the snapshot keeps only `idHash`. |
| `snapshot/invalid-host-secret.json` | The room carries `hostSecret`; the snapshot keeps only `hostSecretHash`. |
| `snapshot/invalid-format-2.json` | `format` is not 1. |
| `state/invalid-join-secret.json` | The room carries `joinSecret`. |
| `state/invalid-host-secret.json` | `you` carries `hostSecret`, even though this is the host's own state. |
| `state/invalid-host-key.json` | `settings` carries `hostKey`, the field older apps still send in `create` and `resume`. |
| `state/invalid-participant-id.json` | A participant carries its `participantId`. |
| `state/invalid-item-without-track.json` | `nowPlaying` with `source: item` has no `track`. |
| `ended/invalid-unknown-reason.json` | `reason` is not `host-ended` or `expired`. |
