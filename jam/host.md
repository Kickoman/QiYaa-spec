# Host

What the QiYaa app does while it hosts a jam, the same on desktop and Android. The server keeps the
room ([room.md](room.md)). The host plays, reports what plays, and runs the Yandex requests that
guests need. During a jam the player's queue is the current track, the guests' items, and under
them the host's own source, the base: the jam has no wave of its own. Everything not written here
is the ordinary player of [player/](../player/README.md).

## Reading a scenario

The table is the one from [player/README.md](../player/README.md#reading-a-scenario):

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|

`no: #N` names the issue in that app's repository that adds the behaviour.

## Terms

- **Jam session**: from `created` (or an accepted `resume`) until the jam ends. The app keeps it on
  disk (HOST-22).
- **Last state**: the latest `state` the host applied.
- **Tail**: the tracks of the player's queue after the current track. During a jam it has two
  parts: first the **jam part**, the mirror of the last state's `queue`, then the **base**.
- **Jam item**: a track the player got from `state.queue`. It keeps its `itemId` and `addedBy`.
- **Base**: what the host's own source still has to play: the rest of its head, then its
  continuation ([player/continuation.md](../player/continuation.md)), under the queue's flag
  "continue with a wave", shuffle and repeat. A **base track** is any track of the base. The
  protocol reports it as `source: wave` ([protocol](protocol/README.md#state)), and one that is
  not from the Yandex catalog as in HOST-37.
- **Jam wave**: the base's wave from history while the jam is on. It takes its seeds from
  `state.room.fallback.seeds` (HOST-11). The rules of
  [continuation.md](../player/continuation.md) and [wave.md](../player/wave.md) apply to it unless
  a scenario here says otherwise. My Wave or a station as the base is not the jam wave.
- **Connected**: the host has a connection whose `resume` or `create` was accepted.

## Queue mirror

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-01 | A jam part [A, B, C] | A state with `queue` [A, B, D, E] | The jam part becomes [A, B, D, E]. The longest common prefix (A, B) stays in place and is not reloaded; the rest is replaced. The base stays behind it. | yes | yes |
| HOST-02 | A jam part [A, B], B is the next track and its link is already fetched | A state with `queue` [A, B, C] | A stays the next track: its link and buffer are kept. | yes | yes |
| HOST-03 | A jam part [A, B] | A state with `queue` [B, A] or [C, A, B] | The next track changes, so the prefetch of A is dropped and the new first track is prefetched, as after any queue change. | yes | yes |
| HOST-04 | Item A is current in the player | A state still lists A in `queue` (it was sent before the server applied `started{A}`) | A is not put into the tail again: the jam part never holds the current item. | yes | yes |

## Track change

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-05 | The jam part is not empty | The current track ends, or the user presses Next | The first jam item becomes current. The host sends `started{itemId}` and then `playing{source: item, itemId, positionMs: 0, paused: false}`. | yes | yes |
| HOST-06 | A jam item or a base track is current | Pause, resume, seek, or 10 s of playing since the last report | `playing` with the current `source`, `positionMs` and `paused`. A base track also carries `track`. | yes | yes |
| HOST-07 | A jam session | The user presses Previous | The current track restarts, whatever its position. During a jam, Previous never goes to an earlier track. Not a new `started`. | yes | yes |
| HOST-08 | The player is stopped or idle, the tail is empty | A state brings a jam item | The item starts playing at once, as in HOST-05. | yes | yes |
| HOST-09 | A jam item hits a track failure | — | The error rules of [errors.md](../player/errors.md) apply (ERR-04 to ERR-07). The item was already reported with `started`, so the server moves on as well. A network failure pauses playback as in ERR-01 to ERR-03. | yes | yes |

## The base

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-10 | The jam part is empty | The current track ends, or Next | The next track of the base plays, by the ordinary rules: the rest of the head, then the continuation ([TR-03](../player/transport.md), [CONT-04](../player/continuation.md#the-wave-from-history), [WAVE-05 to WAVE-09](../player/wave.md#loading-more)). Each base track is reported as `playing{source: wave, track}`, with no `started`; one that is not from the Yandex catalog as in HOST-37. | yes | yes |
| HOST-11 | The base's continuation is the jam wave | It asks for tracks (its first session, CONT-04, or a load-more, WAVE-05), or a state comes | Its seeds are the last state's seeds when there are any, and those of the queue's history ([CONT-05](../player/continuation.md#the-wave-from-history)) otherwise. A session remembers the `seedsVersion` it started with. When the last state has seeds and the session did not start with its `seedsVersion`, the session is outdated: its next request is `rotor/session/new` with the last state's seeds instead of a load-more, and whenever the jam part is empty and the current track is not a jam wave track, its tracks still in the tail (deferred by HOST-12) are dropped. The head's tracks are never dropped. A base of local files has no jam wave: its flag is off ([CONT-12](../player/continuation.md#the-flag)), so the last state's seeds start nothing under it, and when it ends HOST-13 applies. A base of Spotify tracks has none either: its continuation is Spotify's autoplay from its own tracks ([SPOT-22](../player/spotify.md#continuation)). | yes | yes |
| HOST-12 | A base track is current, more base tracks are in the tail | A state brings a jam item | The current base track plays to its end. The jam items go in front of the rest of the base, which keeps its order and waits: it is deferred, and plays when the jam part is empty again (HOST-10, HOST-11). | yes | yes |
| HOST-13 | The jam part is empty, and the base has nothing more: its head has played out and no continuation follows (the queue's flag is off, which it always is for local files by CONT-12; the jam wave has no seeds in the last state nor from CONT-05; or its first request failed, CONT-07, or Spotify's autoplay's, SPOT-21) | The current track ends, or Next | The player stops at the end as in TR-04, and the host sends `playing{source: idle}`. An item that comes then starts at once (HOST-08). | yes | yes |
| HOST-14 | The host's queue, playing or not | The jam starts | The current track stays and plays on if it plays; it is reported as `playing{source: wave, track}`. The rest of the queue stays as the base, with its flag, shuffle and repeat, and the jam items go in front of it as they come. An empty queue is an empty base: when the jam part runs out, the flag decides (HOST-11, HOST-13). | yes | yes |
| HOST-37 | A base track that is not from the Yandex catalog: a local file or a Spotify track | It becomes current, and every report about it (HOST-06) | `playing{source: wave, track}` with `track = {id, source: host, title, artists, durationMs}` ([protocol](protocol/README.md#tracks)): the title and the artists as the playlist shows them, cut to the limits of a track ([limits](limits.md#fields)), no `albumId` and no `coverUri`. The `id` is the host's own: the same for that track during the jam session, never a path, a file name or a Spotify id. No `listenUrl` and no `listenNextUrl` (LISTEN-14). Such a track cannot be added to the jam (HOST-20, ROOM-66): the action is disabled for it. | yes | yes |

## Not learning

The host's account must not learn the party's taste.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-15 | A jam session | Any track starts or closes: a jam item or a base track | No `/play-audio` (TRK-01, TRK-02 are not applied). No feedback to My Wave or to any station, also when it is the base. | yes | yes |
| HOST-16 | A jam session, the base's continuation is the jam wave | Its tracks start and close | Rotor feedback (TRK-03 to TRK-11) goes to the session each track came from, so that the wave follows the party. The setting "feedback to the jam wave", on by default, turns it off: then no rotor feedback at all. | yes | yes |
| HOST-17 | A jam item or a base track is current | The user likes or dislikes it with the player's own buttons | The like goes to the host's account as usual: that is the user's own choice. A dislike also skips, as TR-07. | yes | yes |

## Skip

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-18 | Jam item I is current | `command{kind: skip, itemId: I}` | As Next (HOST-05, HOST-10). | yes | yes |
| HOST-19 | Item I is not current: another item, a base track, or nothing plays | `command{kind: skip, itemId: I}` | Ignored. So two quick skips from guests skip one track. | yes | yes |

## Adding as the host

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-20 | Connected | The user adds a track to the jam from its library or search | `add{track}` to the server. The track reaches the tail only with the next state, in its place in the order, never directly. "Play next" is `add` followed by `pin`, or `pin` for a jam item. | yes | yes |
| HOST-21 | A jam session | The user picks a source that would replace the queue: likes, a playlist, an album, an artist, a station, a wave, a pick of a search ([SRC-09, SRC-10, SRC-16, SRC-18](../player/sources.md#search)), local files ([LOCAL-01 to LOCAL-03](../player/local.md#opening), [LOCAL-11, LOCAL-12](../player/local.md#the-phones-media-library-android)), a pick of Spotify ([SPOT-07 to SPOT-12](../player/spotify.md#picks)) | The request and its statuses are as outside a jam (SRC-01 to SRC-10, WAVE-01 to WAVE-04), and an empty or failed one leaves the base as it was. Otherwise the source becomes the new base, with the flag of a new queue ([CONT-01](../player/continuation.md#the-flag), off for local files by CONT-12), and the old base, its continuation included, is dropped. Unlike SRC-06 and WAVE-01 nothing starts at once: the current track plays to its end, then the jam part, then the new base. Adding to the jam (HOST-20) still makes a track a jam item. | yes | yes |
| HOST-38 | A jam session | The user adds a found track from the search ([SRC-17](../player/sources.md#search)) | The search adds to the jam, not to the queue: "Add" is HOST-20's `add`, "Play next" its `add` followed by `pin`. The queue's own "Play next" and "Add to the end" ([ADD-01 to ADD-09](../player/queue.md)) are not offered, and an album or an artist is not added as a whole. A found track that waits in the jam part shows who added it; "Add" is gone for it, and "Play next" pins its item (HOST-20). Right after the user adds a track its row shows that it is being sent, until a state brings an item of that track, the server refuses the `add` (the refusal is shown on the row), or the connection drops. A track only the host can play (HOST-37) has the add actions disabled. Playing a found track is HOST-21 with SRC-18's queue: the new base is all the found tracks and begins with the chosen one, in order and with shuffle alike (the pass begins with it). Not connected, adding is disabled as in HOST-25. | yes | yes |

## Editing the playlist

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-36 | A jam session | The user removes lines from the playlist, or clears it | Removing a jam item of the tail sends `remove{itemId}`: the item leaves the tail with the next state, as when a guest removes it, never directly. Any other line (a base track, the current track, a track that already played) is removed from the queue at once, as outside a jam: the base plays on by its own rules without it, and removing the current track stops playback. Clearing the playlist keeps the current track and the jam part and drops the rest: the base, its continuation included, becomes an empty base with the flag of a new queue ([CONT-01](../player/continuation.md#the-flag)), also after a base of local files (CONT-12), and when the jam part runs out that flag decides (HOST-11, HOST-13). Not connected, removing a jam item is disabled (HOST-25). | yes | yes |

## Storage and "Continue the jam?"

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-22 | A jam session | `created`, `linkRotated`, `snapshot`, or a change of the outbox | The app stores `roomId`, `hostSecret`, `joinUrl`, the latest snapshot `data` and the outbox on disk, atomically (a temporary file and a rename, or the platform's equivalent). | yes | yes |
| HOST-23 | A stored jam session | The app starts, or Android restores it after the process was killed | The app asks "Continue the jam?". Yes: it connects and sends `resume` with the stored snapshot and outbox. No: it connects, resumes and sends `end`, then clears the storage. The queue is not stored between runs, so the base is empty after Yes, and when the jam part runs out the queue's flag decides (HOST-11, HOST-13). | yes | yes |
| HOST-24 | A stored jam session | `resume` is refused with `room-not-found` or `bad-secret` | The storage is cleared, the jam part of the tail becomes ordinary tracks as in HOST-32, and a status says the jam is over. | yes | yes |

## Without the server

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-25 | Connected | The connection closes or dies | Playback goes on from the tail: the jam part, then the base (HOST-10, HOST-11 with the last state's seeds). The `started` events that should be sent go to the outbox instead, and `playing` is not queued. Removing, pinning, kicking, settings and adding are disabled, and a status says there is no connection to the jam server. | yes | yes |
| HOST-26 | Not connected | — | The host reconnects after 1, 2, 4 … 30 s, and at once when the network comes back. Then `resume` with the snapshot and the outbox. After `resumed`, the outbox is cleared, the controls are enabled again, and the host sends `playing` for the current track. | yes | yes |
| HOST-27 | Not connected | A Yandex request of the player fails for the network | As ERR-01 to ERR-03: the jam changes nothing here. | yes | yes |

## Search and check for guests

A guest's track is built like a track of the app's own parsing ([expected/yandex](../expected/yandex/README.md)): `id`, the first album's id as `albumId`, `title` with the version, the artists' names, `durationMs`, and `coverUri` in the same order of sources (left out when there is none). Artists are cut to 10 and names to 64 characters.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-28 | Connected | `searchRequest{requestId, text}` | `GET /search?text=<text>&type=track&page=0`. `searchResult{requestId, tracks}` with the available tracks of `tracks.results`, in the order of the reply, at most 20. A reply without tracks gives `tracks: []`. | yes | yes |
| HOST-29 | As HOST-28 | The search fails; or the token is rejected (401) | `searchResult{requestId, error: failed}`; or `error: unauthorized`, and the app shows the login problem to its user as usual. | yes | yes |
| HOST-30 | Connected | `validateRequest{requestId, trackIds}` | `POST /tracks` with those ids. `validateResult{requestId, results}` with one result per id, in order: `track` for a track that is available, `reason: track-unavailable` for one that is missing or unavailable. If the request fails: `reason: failed` for every id. | yes | yes |
| HOST-31 | Connected | Either request | The answer goes out within the host timeout ([limits](limits.md#requests-through-the-host)), even while the player is busy: these requests do not wait behind playback work. | yes | yes |

## The end

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-32 | A jam session | The user ends the jam; or `ended` arrives | Connected: `end` is sent (not for `ended`). The current track plays to its end. The jam part stays: its items become waiting tracks ([ADD-02](../player/queue.md)), in front of any that waited before, and the base plays on by its own rules. A jam wave whose session took the last state's seeds is ended: its tracks in the tail are dropped, and the next request of the wave from history starts a new session from the queue's history ([CONT-05](../player/continuation.md#the-wave-from-history)). The storage is cleared. Not connected: the same locally, and the server ends the room on its own when the host has been away long enough (REC-05). | yes | yes |
| HOST-33 | After HOST-32 | — | Ordinary playback rules again: `/play-audio` and feedback for tracks that start from now on. | yes | yes |

## What the host shows

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-34 | A jam session | The playlist shows the tail | Each jam item carries the name of whoever added it, from `participants` by `addedBy`. A base track carries no name: the jam wave's tracks are marked as the jam wave, and the rest of the base looks as it does outside a jam. The current track's line says who added it. | yes | yes |
| HOST-35 | A jam session | — | The jam window or screen shows a QR code of `joinUrl`, the link with a copy or share action, the participants with their online state and a kick action, the settings, "new link" and "end", and whether the host is connected. | yes | yes |
