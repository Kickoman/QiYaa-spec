# Room

What the jam server does with a room, as scenarios the server's tests check. Messages and reasons
are in [protocol/](protocol/README.md), numbers in [limits.md](limits.md), the queue order in
[ordering/](ordering/README.md), the wave seeds in [seeds.md](seeds.md). Losing and regaining
connections is in [recovery.md](recovery.md).

## Reading a scenario

| ID | Given | When | Then | Server |
|---|---|---|---|---|

- **Then** lists what the server sends and how the room changes. Anything not listed stays as it
  was. "State to all" means a `state` with the next `version` to every connection in the room.
  Each connection gets its own `you`.
- **Server**: whether the jam server does this today. `no: #N` names the Kickoman/QiYaa-jam issue
  that adds it.

## Terms

- **Participant**: the host or a guest. A guest is known by the hash of its `participantId`, and
  others see it by its `publicId`. A participant stays in the room when its connections close and
  is **online** while it has at least one open connection.
- **Item**: a track in the jam, with `itemId`, `addedBy`, `addedAt` and, once pinned, `pinnedAt`.
  An item is **waiting** while it is in the queue. It is **current** while it is
  `nowPlaying` with `source: item`. It is **recent** after that, with `playedAt` = the time it
  stopped being current; the latest 10 are kept.
- **Waiting items of a guest**: the items in the queue that the guest added, pinned or not. This is
  `participants[].pending`.
- **Host action**: any request from the host connection.

## Create

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-01 | Fewer rooms than the limits (ROOM-02, ROOM-03) | A desktop or android connection sends `create` | `created{roomId, hostSecret, joinSecret, joinUrl, publicId}`, then `state` version 1. The room has the host as its only participant (`kind: host`, named `hostName`). Settings are the defaults `round-robin`, `guestsCanSkip: false`, `joinOpen: true`, `maxPendingPerGuest: 10`, overridden by `create.settings`. `nowPlaying` is `idle`, the queue and `recent` are empty, and `fallback` is `{seeds: [], seedsVersion: 0}`. The room's age counts from now. | no: #7, #8 |
| ROOM-02 | The connection's IP has 2 live rooms, or created 5 rooms in the last hour ([limits](limits.md#rooms)) | `create` | `rejected{rate-limited}`. No room. A room counts for the IP of the connection that created or raised it (REC-06), for the room's whole life; a refused `create` does not count. | no: #8 |
| ROOM-03 | The server has its maximum of rooms | `create` | `rejected{server-full}`. The server checks this before ROOM-02. | no: #8 |

## Join

The server checks a `join` in this order and stops at the first failure: room exists → participant
not kicked → known participant (then admitted at once) → join secret → room open → room not full.

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-04 | An open room with free places | A new `participantId` joins with the right secret | `joined{publicId}` with a new `publicId`. The participant is added with `kind` from `hello.app` (`web`, or `qiyaa` for desktop and android), online, 0 waiting items. State to all. | no: #7 |
| ROOM-05 | A guest who joined before, online or not | The same `participantId` joins again, from the same or another connection | `joined` with the same `publicId`. The name becomes the new one. Its items stay. It is admitted even if the room is closed or full and even with an old join secret: the `participantId` itself proves who it is. Several connections of one participant are allowed and all get `state`. | no: #7 |
| ROOM-06 | A room | A new `participantId` joins with a wrong secret | `rejected{bad-secret}`. The failure counts toward the IP's failed joins (ROOM-58). | no: #7, #8 |
| ROOM-07 | — | `join` names a room the server does not have | `rejected{room-not-found}`. Counts toward the IP's failed joins. | no: #8 |
| ROOM-08 | The host kicked this participant (ROOM-35) | Its `participantId` joins, with any secret | `rejected{kicked}`. | no: #7 |
| ROOM-09 | `joinOpen: false` | A new `participantId` joins with the right secret | `rejected{join-closed}`. Known participants still come back (ROOM-05). | no: #7 |
| ROOM-10 | The room has its maximum of guests, counting offline ones and not the host | A new `participantId` joins | `rejected{room-full}`. Kicking a guest frees a place. | no: #7 |
| ROOM-11 | A guest with one connection | The connection closes | The guest becomes offline. Its items stay. State to all. | no: #7 |

## The link

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-12 | A room | The host sends `rotateLink` | A new join secret. `linkRotated{joinSecret, joinUrl}` to the host only, then state to all. The old secret no longer admits new participants. | no: #7 |
| ROOM-13 | The link was rotated | A guest who joined before reconnects with the old secret | Admitted (ROOM-05). A new participant with the old secret gets `bad-secret`. | no: #7 |

## Add

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-14 | A web guest got track T in its own `searchResults` within the last 30 min, among its latest 200 result tracks | `add{trackId: T}` | A new item: `itemId` = `i` + the room's next number, the track's metadata from those results, `addedBy` = the guest, `addedAt` = now, not pinned. `ack`, then state to all. | no: #7, #9 |
| ROOM-15 | — | A web guest sends `add{trackId}` for a track that it did not get in its own results, or got more than 30 min ago, or that dropped out of its latest 200 | `rejected{unknown-track}`. Another guest's results do not count. | no: #9 |
| ROOM-16 | The host is online | A QiYaa guest sends `add{track}` | The server sends the host `validateRequest{trackIds: [track.id]}`. If the host answers with a `track`, the item is added with the host's metadata, not the guest's. Then `ack` and state to all. | no: #9 |
| ROOM-17 | As ROOM-16 | The host answers `reason: track-unavailable`, or `reason: failed` | `rejected{track-unavailable}`, or `rejected{host-error}`. | no: #9 |
| ROOM-18 | The host is offline, or does not answer in 10 s | A QiYaa guest sends `add{track}` | `rejected{host-offline}`, or `rejected{host-timeout}`. | no: #9 |
| ROOM-19 | — | The host sends `add{track}` | The item is added with the host's metadata as sent, without a check and without a per-person limit. `ack`, state to all. | no: #7 |
| ROOM-20 | An item with the same track id is waiting | Anyone, the host included, adds that track | `rejected{duplicate}`. The same track may be added again once it is current or recent. | no: #7 |
| ROOM-21 | A guest has `maxPendingPerGuest` waiting items | It adds | `rejected{queue-limit}`. | no: #7 |
| ROOM-22 | The queue holds its maximum of items | Anyone adds | `rejected{queue-limit}`. | no: #7 |
| ROOM-23 | A guest sent its maximum of `add` requests in the last minute, accepted or not | It sends another | `rejected{rate-limited}`. | no: #8 |

## Search

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-24 | The host is online | A web guest sends `search{text}` | The text is trimmed, and runs of spaces become one. The server sends the host `searchRequest{requestId, text}` with its own `requestId`. The host's `searchResult.tracks` go to that guest as `searchResults{id, tracks}` and into its result cache (ROOM-14). | no: #9 |
| ROOM-25 | As ROOM-24 | The host is offline; or does not answer in 10 s; or answers `error` | `rejected{host-offline}`; `rejected{host-timeout}`; `rejected{host-error}`. An answer that comes later, or with a `requestId` the server does not expect, is dropped. | no: #9 |
| ROOM-26 | A web guest sent its maximum of searches in the last minute | It searches | `rejected{rate-limited}`. No request to the host. | no: #8 |
| ROOM-27 | — | A QiYaa guest sends `search` | `rejected{not-allowed}`: it searches with its own account. | no: #8 |

## Remove, pin, kick

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-28 | A waiting item added by this guest, pinned or not | The guest sends `remove{itemId}` | The item leaves the queue. `ack`, state to all. | no: #7 |
| ROOM-29 | A waiting item added by someone else | A guest removes it | `rejected{not-allowed}`. | no: #7 |
| ROOM-30 | The item is current, recent, removed, or unknown | Anyone removes it | `rejected{stale}`. | no: #7 |
| ROOM-31 | Any waiting item | The host removes it | Removed, `ack`, state to all. | no: #7 |
| ROOM-32 | A waiting item that is not pinned | The host sends `pin{itemId}` | `pinnedAt` = now. The item moves ahead of every unpinned item and behind items pinned earlier ([ordering](ordering/README.md)). `ack`, state to all. | no: #7 |
| ROOM-33 | The item is already pinned | The host pins it again | `ack`. `pinnedAt` stays, nothing moves. | no: #7 |
| ROOM-34 | The item is not waiting | The host pins it | `rejected{stale}`. | no: #7 |
| ROOM-35 | A guest | The host sends `kick{publicId}` | The guest's connections get `kicked` and are closed with 1000. The guest leaves the participants, its waiting items leave the queue, and its `participantId` hash is remembered as kicked. Its current and recent items stay. `ack`, state to all. | no: #7 |
| ROOM-36 | — | The host kicks itself, or a `publicId` not in the room | `rejected{not-allowed}`, or `rejected{stale}`. | no: #7 |

## What plays

Only the host reports what plays. Its reports move items; nothing else does.

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-37 | Item I is waiting | The host sends `started{itemId: I}` | I leaves the queue and becomes current: `nowPlaying = {source: item, itemId, track, addedBy, positionMs: 0, paused: false, reportedAt: now}`. Its adder's `lastServedAt` = now. The item that was current before goes to the front of `recent` with its `playedAt`. State to all. | no: #7 |
| ROOM-38 | Item I is current, recent, removed, or unknown | `started{itemId: I}` | Nothing changes, and no state is sent. A repeated `started` is harmless. | no: #7 |
| ROOM-39 | Item I is current | `playing{source: item, itemId: I, positionMs, paused}` | `positionMs`, `paused` and `reportedAt` = now are updated. State to all. | no: #7 |
| ROOM-40 | Item I is waiting | `playing{source: item, itemId: I, …}` | As `started{I}` (ROOM-37), then as ROOM-39. | no: #7 |
| ROOM-41 | Item I is neither current nor waiting | `playing{source: item, itemId: I, …}` | Ignored. | no: #7 |
| ROOM-42 | Anything plays | `playing{source: wave, track, …}` or `playing{source: idle, …}` | `nowPlaying` becomes that wave track, or idle, with `reportedAt` = now. A current item goes to the front of `recent`. `lastServedAt` does not change. State to all. | no: #7 |

## Skip

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-43 | `guestsCanSkip: true`, item I is current, the host is online | A guest sends `skip{itemId: I}` | `command{kind: skip, itemId: I}` to the host, `ack` to the guest. The room changes only when the host reports the next track. Two quick skips of I send two commands; the host acts on the first ([HOST-17](host.md#skip)). | no: #7 |
| ROOM-44 | `guestsCanSkip: false` | A guest skips | `rejected{not-allowed}`. | no: #7 |
| ROOM-45 | `guestsCanSkip: true` | A guest skips an item that is not current, or while a wave track or nothing plays | `rejected{stale}`. Guests cannot skip wave tracks in protocol 1. | no: #7 |
| ROOM-46 | `guestsCanSkip: true`, item I current | A guest skips I while the host is offline | `rejected{host-offline}`. | no: #7 |

## Settings

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-47 | Waiting items | The host changes `order` | The queue is ordered by the new mode at once. Pinned items and the current item do not move. `ack`, state to all. | no: #7 |
| ROOM-48 | A guest has 8 waiting items | The host sets `maxPendingPerGuest: 5` | The 8 items stay. The guest's next adds get `queue-limit` until it has fewer than 5. | no: #7 |
| ROOM-49 | — | The host changes `joinOpen` or `guestsCanSkip` | Applies to the next `join` (ROOM-09) or `skip` (ROOM-44). `ack`, state to all. | no: #7 |

## State

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-50 | A room | Anything in the room changes: a participant, an item, the settings, `nowPlaying`, the link | `version` + 1, then state to all: the whole room, the queue in play order, each connection with its own `you`. No field of any `state` is a secret ([protocol](protocol/README.md#identifiers-and-secrets)). | no: #7 |
| ROOM-51 | — | A request is refused, or a report changes nothing (ROOM-38, ROOM-41) | No state, and `version` stays. | no: #7 |

## The end

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-52 | A room | The host sends `end` | `ack` to the host, then `ended{host-ended}` to every connection, which are then closed with 1000. The room is gone: `join` and `resume` get `room-not-found`. | no: #7 |
| ROOM-53 | A room reaches the maximum room age | — | `ended{expired}` to every connection, closed with 1000. The room is gone and cannot be raised from a snapshot either ([REC-07](recovery.md)). | no: #10 |

## Connections and abuse

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-54 | A new connection | No `hello` within the handshake time | Closed with 1008. | no: #8 |
| ROOM-55 | Any connection | A frame over its size limit ([limits](limits.md#connections)) | Closed with 1009. | no: #8 |
| ROOM-56 | Any connection | A binary frame, text that is not a JSON object, an unknown `type`, or a message that does not match its schema | For a known `type`: `rejected{id?, invalid-message}`. Then, in every case, closed with 1008. | no: #8 |
| ROOM-57 | An IP has had its maximum of connections closed with 1008 or 1009 | It opens another connection within the ban time | The WebSocket upgrade is refused with HTTP 429 until the ban ends. | no: #8 |
| ROOM-58 | An IP has its maximum of open connections, or opened its maximum in the last minute, or had its maximum of failed joins in the last minute | It opens a connection, or joins | The upgrade is refused with HTTP 429, or the join gets `rejected{rate-limited}` even with the right secret. | no: #8 |
| ROOM-59 | — | A browser opens a WebSocket from a page of another origin (`Origin` is not the server's own) | The upgrade is refused with HTTP 403. Connections without `Origin` (the apps) are accepted. | no: #8 |
| ROOM-60 | — | A request carries `X-Real-IP` | The server uses it as the client's IP only when the connection comes from `TRUSTED_PROXY`; otherwise it uses the connection's address. | no: #8 |
| ROOM-61 | A connection | It does not answer the server's pings for the dead-connection time | Closed. Its participant goes offline as in ROOM-11. | no: #8 |
| ROOM-62 | — | Any HTTP request other than `GET /`, `GET /j/<roomId>`, the web guest's own files under `GET /assets/`, `GET /healthz`, `GET /.well-known/assetlinks.json` and the upgrade on `/ws` | 404 with an empty body. A path never reaches outside the web guest's files. | no: #8 |

## Privacy

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-63 | — | The server logs anything | Logs hold connections, refusals with their `reason`, and counters. They never hold names, track titles, search texts, secrets or `participantId`s. | no: #8 |
| ROOM-64 | — | The server keeps a secret | It keeps only hashes of `hostSecret`, `joinSecret` and `participantId`. It does not need the secrets themselves after it has sent them to the host. | no: #7, #8 |

## Rights

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| ROOM-65 | A guest connection | It sends a message that only the host may send: `pin`, `kick`, `settings`, `rotateLink`, `end`, `playing`, `started`, `searchResult`, `validateResult`, or `add` of the kind that is not its own ([protocol](protocol/README.md#who-may-send-what)) | `rejected{id?, not-allowed}`, with the `id` when the message has one. Nothing changes and the connection stays open. The same goes for a host connection that sends `join`, `search` or `skip`. | no: #8 |
