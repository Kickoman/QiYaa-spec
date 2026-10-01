# Jam protocol, version 1

The host app, the guests (a browser or QiYaa) and the jam server exchange JSON messages, each over
its own WebSocket. The server owns the room: participants, queue, order, rights and limits. The
host plays the music and runs the Yandex requests that need its account. The schemas here are the
source of truth. The server generates its types from them. The desktop and Android parsers are
tested on the examples.

| Path | What it holds |
|---|---|
| `schemas/defs.schema.json` | Identifiers, secrets, names, times, the reason list and the no-secrets rule |
| `schemas/track.schema.json`, `settings.schema.json`, `room.schema.json`, `snapshot-data.schema.json` | The objects inside messages |
| `schemas/messages/<type>.schema.json` | One schema per message `type` |
| `schemas/client-message.schema.json`, `server-message.schema.json` | Any message of one direction |
| [examples/](examples/README.md) | Examples per `type`. The `invalid-*` files are explained in its README |
| [../../scripts/check-jam-protocol.mjs](../../scripts/check-jam-protocol.mjs) | `npm ci && npm run check` at the spec root: valid examples pass, invalid ones fail |

Numbers such as sizes, timeouts and rates are in [../limits.md](../limits.md).

## Transport

- `wss://<server>/ws`. Text frames only. Each frame is one UTF-8 JSON object with a `type`.
- Frame size limits depend on the connection ([limits](../limits.md)). The large host limit
  exists because of the snapshot in `resume`.
- The server sends a WebSocket ping every 25 s and drops a connection that has not answered for
  60 s. Browsers answer pings themselves. The apps' WebSocket libraries do too.
- Every time is an integer: milliseconds since the Unix epoch, on the server clock. A client
  estimates its clock offset from `serverTime` in `welcome` and in every `state`.

## Handshake

1. The client sends `hello{protocol, app, appVersion}` within 5 s of the connection opening.
   Otherwise the server closes the connection with 1008.
2. The server answers `welcome{protocol, serverTime}`. If it does not speak `hello.protocol`, it
   sends `rejected{reason: "update-required", serverProtocol}` and closes with 1000.
3. The connection takes a role with its first request after `welcome`. `create` or `resume` makes
   it the host. `join` makes it a guest: `app: web` gives a web guest, `desktop` or `android` a
   QiYaa guest. A browser (`app: web`) cannot be a host. A role is kept for the connection's life.

Over the per-IP connection limits, the server refuses the WebSocket upgrade with HTTP 429. A
browser must connect from a page of the server's own origin; another `Origin` gets HTTP 403.
Connections without `Origin` (the apps) are accepted.

## Who may send what

| Sender | Messages |
|---|---|
| Any connection | `hello` (first, once) |
| No role yet | `create`, `resume` (desktop and android only), `join` |
| Host | `playing`, `started`, `add` with `track`, `pin`, `remove`, `kick`, `settings`, `rotateLink`, `end`, `searchResult`, `validateResult` |
| Web guest | `add` with `trackId`, `remove`, `skip`, `search` |
| QiYaa guest | `add` with `track`, `remove`, `skip` |

A known message from the wrong sender is refused with `rejected{not-allowed}`, and the connection
stays open. Which items a guest may remove or skip is a room rule, and so are the settings that
allow it.

## Requests and replies

A client message with an `id` is a request. The server answers it exactly once, with the same `id`.
The answer is the success reply from the table below or `rejected{id, reason, detail?}`. The
client picks the `id` (1–36 characters of `A-Z a-z 0-9 _ -`), unique among its pending requests.
The server does not interpret it.

| Request | Success reply |
|---|---|
| `create` | `created`, then `state` |
| `resume` | `resumed{restored}`, then `state` |
| `join` | `joined`, then `state` |
| `search` | `searchResults` |
| `rotateLink` | `linkRotated` |
| `add`, `pin`, `remove`, `kick`, `settings`, `end`, `skip` | `ack` |

`hello`, `playing`, `started`, `searchResult` and `validateResult` carry no `id` and get no reply.
A reply comes before the `state` that shows its effect.

The server also asks the host: `searchRequest{requestId, text}` for a web guest's `search`, and
`validateRequest{requestId, trackIds}` for a QiYaa guest's `add`. The host answers with
`searchResult` or `validateResult` with the same `requestId`. The server chooses `requestId`. An
answer that comes after the 10 s host timeout, or names an unknown `requestId`, is ignored. The
guest's request then ends with `host-timeout`.

## State

- After every change, the server sends `state{version, serverTime, room}` to every connection in
  the room. It is the whole room, never a diff, and each connection gets its own `room.you`.
- `version` grows by 1 with each change and is the same for all. On one connection, a client
  ignores a `state` whose `version` is not greater than the last one it applied. After a
  reconnect, it takes the first `state` as it is: a room raised from a snapshot may count from an
  older `version`.
- `room.queue` is already in play order: the items the host pinned first, then the room's order.
- `room.nowPlaying.source`:
  - `item`: a queue item, with `itemId`, `track` and `addedBy`;
  - `wave`: a track of the jam wave, with `track`. It was never in the queue;
  - `idle`: nothing plays.
- Progress: `positionMs + (server now − reportedAt)` while not `paused`, where server now = local
  now + clock offset.
- `room.fallback.seeds` holds up to 5 `track:<id>` seeds. The host starts the jam wave from them
  when the queue is empty. `seedsVersion` changes exactly when the set of seeds changes; their
  order does not matter.
- `room.hostOnline` is `false` while the host has no connection.

## Snapshot and resume

- After changes, the server sends the host `snapshot{data}`, at most once every 2 s. The host keeps
  the latest `data` without reading it and sends it back in `resume`.
- `data` is the whole room, `{format: 1, room}`: history, counters, the kicked list and every
  timestamp that the order needs. Secrets appear only as lower-case hex SHA-256: `joinSecretHash`,
  `hostSecretHash`, each participant's `idHash`, and `kicked`.
- The server therefore cannot tell the join secret after raising a room from a snapshot. The host
  keeps its `joinUrl` (from `created` or the last `linkRotated`) with its jam session, next to
  `roomId` and `hostSecret`.
- `resume{roomId, hostSecret, snapshot, outbox}`:
  - **the room is live**: `hostSecret` must match. The snapshot is ignored. The `started` events in
    `outbox` are applied in order, and an item that is no longer in the queue is skipped, so a
    repeated `started` changes nothing. Reply `resumed{restored: false}`;
  - **the server does not know the room**: the snapshot must pass this schema and the limits, its `room.id` must be `roomId`, and SHA-256 of `hostSecret` must equal
    `hostSecretHash`. The room comes back with the same id and join secret, then `outbox` is
    applied. Reply `resumed{restored: true}`;
  - otherwise: `room-not-found` or `bad-secret`. Raising a room counts as creating one for the
    limits on rooms ([ROOM-02, ROOM-03](../room.md#create)): `rate-limited` or `server-full`.

## Identifiers and secrets

| Field | Format | Made by | Secret |
|---|---|---|---|
| `roomId` | 8 characters of lower-case Crockford base32 (40 bits) | server | no; it is in the link path |
| `joinSecret` | 128 bits, base64url without padding (22 characters) | server | yes |
| `hostSecret` | 256 bits, base64url (43 characters) | server | yes |
| `participantId` | UUID v4, lower case | the guest's client, kept per room | yes: it lets the guest come back as the same participant |
| `publicId` | 6 characters of lower-case Crockford base32 | server | no; others see the participant by it |
| `itemId` | `i` + a counter of the room; never reused, since the counter is in the snapshot | server | no |

The join link is `https://<server>/j/<roomId>#<joinSecret>`. The secret is in the fragment, so a
browser never sends it in the HTTP request.

Where each secret may appear:

| Secret | Only in |
|---|---|
| `hostSecret` | `created` (to the host), `resume` |
| `joinSecret` | `created`, `linkRotated` (to the host), their `joinUrl`, `join` |
| `participantId` | `join` |

No secret appears in `state`, in the snapshot, in `rejected.detail` or in server logs. For `state`
and the snapshot the schemas enforce it: no object at any depth may have a property named
`hostKey`, `hostSecret`, `joinSecret` or `participantId`.

**No host keys.** Anyone with the app may create a room; the limits on rooms per IP and per server
([limits](../limits.md#rooms)) keep that cheap. Until 2026-10-01 `create` and `resume` carried a
`hostKey` issued by the server's owner. Apps from before that (Android 0.2.3) still send it: it is
an unknown field now and ignored, and the name stays on the list above so that it never leaks.

## Tracks

`track = {id, albumId?, title, artists[], durationMs, coverUri?}`:

- `id` and `albumId`: catalogue ids as strings.
- `title`: the title as the apps show it, with the version in parentheses when there is one.
- `artists`: artist names, at most 10 of up to 64 characters each. A sender cuts longer lists and
  names, so that a guest's `add` fits in a guest frame.
- `coverUri`: the Yandex template without a scheme. `%%` stands for the size (`200x200`), and
  clients add `https://`.

The server never takes a guest's word for a track's metadata. A web guest sends only a `trackId`
from the search results the server gave it. The host replaces a QiYaa guest's track with its own
canonical metadata (`validateResult`).

## Reasons

| `reason` | When |
|---|---|
| `bad-secret` | `join`: wrong join secret. `resume`: wrong host secret |
| `room-not-found` | `join`, `resume`: no such room, and for `resume` no usable snapshot |
| `room-full` | `join`: the room has its maximum of guests |
| `join-closed` | `join`: the host closed the room to new guests |
| `kicked` | `join`: this participant was removed by the host |
| `rate-limited` | any request over a rate limit; `create`, or a `resume` that would raise a room, over the rooms of one IP |
| `queue-limit` | `add`: the guest has `maxPendingPerGuest` items waiting, or the queue is full |
| `duplicate` | `add`: the track is already waiting in the queue |
| `not-allowed` | the sender's role or the room's settings do not allow it |
| `stale` | the item is no longer in the queue, or a `skip` names an item that is not playing |
| `unknown-track` | `add` from a web guest: the `trackId` is not among the search results the server gave that guest recently |
| `track-unavailable` | `add` from a QiYaa guest: the host cannot play the track |
| `host-offline` | `search`, or a QiYaa guest's `add`, while the host has no connection |
| `host-timeout` | the host did not answer a `searchRequest` or `validateRequest` in time |
| `host-error` | the host answered with an error (`searchResult.error`, `validateResult` reason `failed`) |
| `invalid-message` | the message does not match its schema; the connection is closed next |
| `update-required` | `hello` names a protocol this server does not speak; carries `serverProtocol` |
| `server-full` | `create`, or a `resume` that would raise a room from its snapshot: the server has its maximum of rooms |

A client that meets a `reason` it does not know shows a general failure.

## Closing

| Close | When | The client |
|---|---|---|
| 1000 | after `ended`, `kicked` or `rejected{update-required}`, or when a newer connection took over the host role ([REC-04](../recovery.md#the-host-goes-away-and-comes-back)) | does not reconnect |
| 1001 | the server is shutting down (a restart) | reconnects |
| 1008 | protocol violation: no `hello` in time, a binary frame, an unknown `type`, `invalid-message` | reconnects with backoff; a correct client never sees it |
| 1009 | a frame over the size limit | as 1008 |
| anything else, or a lost network | — | reconnects |

Every close with 1008 or 1009 counts as a violation of the client's IP ([limits](../limits.md)).
Reconnecting waits 1, 2, 4 … up to 30 s, and happens at once when the network comes back.

## Versions and unknown content

- This is protocol **1**. A change that an old client could not safely ignore bumps the number.
  Adding optional fields, or messages that only the other side needs, does not.
- Unknown fields are ignored by everyone, and the schemas allow them. The exception is the
  secret names in `state` and in the snapshot.
- An unknown `type` from a client makes the server close the connection with 1008. An unknown
  `type` from the server is ignored by the client.

## Generating code from the schemas

- Every schema has an `$id` under `https://github.com/Kickoman/QiYaa-spec/jam/protocol/schemas/`.
  These are names, not addresses to fetch. Relative `$ref`s resolve against the referencing file's
  folder. json-schema-to-typescript needs `cwd` set to the folder of the file it compiles.
- `title` is the type name (`AddMessage`, `Track`, `SnapshotRoom`), `description` says what it is.
- A `$ref` never has sibling keywords; composition goes through `allOf`.
- A large `maxItems` (the queue's 300) turns into a union of tuples unless the generator is told not
  to (`maxItems: -1` in json-schema-to-typescript).

## Changing the protocol

Change the schema and the examples here first, and keep `npm run check` green. Then change the
server and the apps, each in its own repository, with the spec submodule bumped.
