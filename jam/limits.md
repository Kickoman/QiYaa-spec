# Jam limits

Every number of the jam in one place. Code does not repeat a number without a reference to this
file. **Schema** means the protocol schemas enforce it. **Server**, **host** and **client** mean
that side enforces or follows it.

## Connections

| What | Value | Enforced by |
|---|---|---|
| Frame size: a web app (`hello.app: web`) or a guest | 4 KiB | server |
| Frame size: a desktop or android app before it has a role, and a host | 256 KiB | server |
| `hello` after the connection opens | within 5 s | server |
| WebSocket ping from the server | every 25 s | server |
| A connection is dead after no pong for | 60 s | server |
| Per IP: open connections | 20 | server |
| Per IP: new connections | 30 a minute | server |
| Per IP: failed `join` (wrong secret, unknown room) | 10 a minute | server |
| Protocol violations (closes with 1008 or 1009) before a ban | 3 from one IP | server |
| Ban | 10 min | server |
| Reconnect backoff | 1, 2, 4 … 30 s; at once when the network returns | client |

## Rooms

| What | Value | Enforced by |
|---|---|---|
| Rooms on one server | 20 | server (`server-full`) |
| Live rooms created or raised from one IP | 2 | server (`rate-limited`) |
| Rooms created or raised from one IP | 5 an hour | server (`rate-limited`) |
| Guests in a room | 30, the host not counted | server (`room-full`) |
| Participants in `state` and in the snapshot | 31 | schema |
| Queue length | 300 items | schema, server (`queue-limit`) |
| Waiting items per guest, `maxPendingPerGuest` | 10 by default, the host sets 1–50 | schema, server (`queue-limit`) |
| Host's own items | no per-person limit, only the queue length | server |
| `recent` played items in `state` | 10 | schema |
| Wave seeds | 5 | schema |
| Kicked participants remembered | 100 | schema |
| A room without its host | ends (`expired`) after 1 h | server |
| Room age | ends (`expired`) after 12 h | server |

## Rates

| What | Value | Enforced by |
|---|---|---|
| `add` by a guest | 10 a minute | server (`rate-limited`) |
| `search` by a web guest | 20 a minute | server (`rate-limited`) |
| `snapshot` to the host | at most once every 2 s | server |
| `playing` from the host | on a track change, pause, resume and seek, and every 10 s | host |

## Requests through the host

| What | Value | Enforced by |
|---|---|---|
| Host answer to `searchRequest` or `validateRequest` | 10 s, then `host-timeout` | server |
| Search results per search | 20 | schema, host |
| Tracks per `validateRequest` | 20 | schema |
| Search results a web guest may add from | its own, for 30 min, the latest 200 tracks | server (`unknown-track`) |
| `started` events in `resume.outbox` | 500 | schema |

## Fields

| What | Value | Enforced by |
|---|---|---|
| Participant name, `hostName` | 1–24 characters, no spaces at the ends, no control characters; clients trim | schema |
| Search text | 1–100 characters, not only spaces | schema |
| Request `id`, `requestId` | 1–36 characters of `A-Z a-z 0-9 _ -` | schema |
| Track `title` | 1–150 characters | schema |
| Track `artists` | at most 10 names of 1–64 characters; senders cut longer ones | schema |
| Track `coverUri` | at most 300 characters, contains `%%` | schema |
| `rejected.detail` | at most 200 characters | schema |
