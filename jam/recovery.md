# Recovery

What happens when connections drop, when the host goes away, and when the server restarts. The
format and the terms are those of [room.md](room.md). The `resume` rules are in
[protocol/](protocol/README.md#snapshot-and-resume). What the host app does while offline is in
[host.md](host.md#without-the-server). **Client** marks scenarios that a guest client follows,
with the issue of the web guest.

## The host goes away and comes back

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| REC-01 | A room with its host online | The host's last connection closes or dies | `hostOnline: false`, state to all. The room keeps working. Web guests add from their cached results (ROOM-14). `search`, a QiYaa guest's `add` and `skip` get `host-offline`. The room's timer for its host starts. | no: #10 |
| REC-02 | The host is offline, the room is live | The host sends `resume` with the right `hostSecret` and a valid `hostKey` | `hostOnline: true`. The snapshot is ignored and the `outbox` is applied (REC-03). `resumed{restored: false}`, then state to all. The host timer stops. | no: #10 |
| REC-03 | The outbox holds `started` for items I1, I2, I3 in that order | `resume` is accepted (REC-02, REC-06) | Each `started` is applied in order, as ROOM-37 at the current server time. An item that is not waiting (already started, removed) is skipped (ROOM-38). The same outbox sent twice changes nothing the second time. | no: #10 |
| REC-04 | The host has a connection | A second connection sends `resume` for the same room and is accepted | The new connection becomes the host. The old one is closed with 1000 and gets no more messages. | no: #10 |
| REC-05 | The host has been offline for the room-without-host time ([limits](limits.md#rooms)) | — | `ended{expired}` to every connection, closed with 1000. The room is gone, but the host can raise it from its snapshot until the room's maximum age (REC-06). | no: #10 |

## Raising a room from the host's snapshot

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| REC-06 | The server does not know the room: it expired (REC-05) or the server lost it (REC-11) | The host sends `resume` with a valid `hostKey`, the right `hostSecret` and a snapshot that passes its schema and the limits, where `snapshot.room.id` = `roomId` and the room is younger than its maximum age | The room comes back as the snapshot has it: the same `roomId`, the same join secret (guests' links work), participants, kicked list, queue, recent, settings, counters and `createdAt`. Every guest is offline until it reconnects, the host is online. `version` = snapshot's + 1. Then the outbox (REC-03). `resumed{restored: true}`, state. The room counts toward the server's rooms (`server-full` if there is no place). | no: #10 |
| REC-07 | The server does not know the room | The snapshot's `createdAt` is older than the maximum room age | `rejected{room-not-found}`. The jam is over for good. | no: #10 |
| REC-08 | The server does not know the room | `snapshot` is `null`, does not pass the schema or the limits, or its `room.id` is another room | `rejected{room-not-found}`. | no: #10 |
| REC-09 | Live room or snapshot | `resume` with a `hostSecret` that does not match, or a `hostKey` that is unknown or revoked | `rejected{bad-secret}`, or `rejected{bad-key}`. A revoked key ends the host's ability to resume even a live room. | no: #10 |

## Server restarts

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| REC-10 | Rooms are live | The server gets SIGTERM (a planned restart) | It writes every room, in the snapshot format, to a file on its data volume (a temporary file, then a rename), closes every connection with 1001 and exits. On start it reads the file, deletes it, and has the same rooms: every participant offline, the host timer counting from the start. Clients reconnect; the host's `resume` finds a live room (REC-02). | no: #10 |
| REC-11 | Rooms are live | The server dies without SIGTERM, or the file is missing | The rooms are lost. A host that resumes raises its room from its snapshot (REC-06). What guests did after that snapshot was taken is lost; this is the only loss, and only in a crash. | no: #10 |
| REC-12 | — | After changes in a room | The host gets `snapshot{data}` at most once every 2 s. The last change is always followed by a snapshot, at most 2 s later. | no: #10 |

## Guests reconnect

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| REC-13 | A guest's connection dropped | It reconnects and joins with its `participantId` | Its `publicId`, name and items are as before (ROOM-05). The first `state` of the new connection is taken whatever its `version` ([protocol](protocol/README.md#state)). | no: #7 |
| REC-14 | A web guest was in a room | On reconnecting it gets `room-not-found` | It keeps reconnecting with the usual backoff for 10 min from the first `room-not-found`, since the host may raise the room (REC-06). After that it shows that the jam is not there and stops. **Client**: Kickoman/QiYaa-jam#16. | — |
| REC-15 | A guest | It gets `ended` or `kicked` | It shows why and does not reconnect. **Client**: Kickoman/QiYaa-jam#16. | — |
