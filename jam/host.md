# Host

What the QiYaa app does while it hosts a jam, the same on desktop and Android. The server keeps the
room ([room.md](room.md)). The host plays, reports what plays, and runs the Yandex requests that
guests need. Everything not written here is the ordinary player of [player/](../player/README.md).

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
  parts: first the **jam part**, the mirror of the last state's `queue`, then the **wave part**,
  tracks of the jam wave.
- **Jam item**: a track the player got from `state.queue`. It keeps its `itemId` and `addedBy`.
- **Jam wave**: a rotor wave started from `state.room.fallback.seeds`. The ordinary wave rules of
  [wave.md](../player/wave.md) apply to it unless a scenario here says otherwise.
- **Jam track**: a jam item or a jam wave track.
- **Connected**: the host has a connection whose `resume` or `create` was accepted.

## Queue mirror

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-01 | A jam part [A, B, C] | A state with `queue` [A, B, D, E] | The jam part becomes [A, B, D, E]. The longest common prefix (A, B) stays in place and is not reloaded; the rest is replaced. The wave part stays behind it. | no: #13 | no: #60 |
| HOST-02 | A jam part [A, B], B is the next track and its link is already fetched | A state with `queue` [A, B, C] | A stays the next track: its link and buffer are kept. | no: #13 | no: #60 |
| HOST-03 | A jam part [A, B] | A state with `queue` [B, A] or [C, A, B] | The next track changes, so the prefetch of A is dropped and the new first track is prefetched, as after any queue change. | no: #13 | no: #60 |
| HOST-04 | Item A is current in the player | A state still lists A in `queue` (it was sent before the server applied `started{A}`) | A is not put into the tail again: the jam part never holds the current item. | no: #13 | no: #60 |

## Track change

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-05 | The jam part is not empty | The current track ends, or the user presses Next | The first jam item becomes current. The host sends `started{itemId}` and then `playing{source: item, itemId, positionMs: 0, paused: false}`. | no: #13 | no: #60 |
| HOST-06 | A jam track is current | Pause, resume, seek, or 10 s of playing since the last report | `playing` with the current `source`, `positionMs` and `paused`. A wave track also carries `track`. | no: #13 | no: #60 |
| HOST-07 | A jam session | The user presses Previous | The current track restarts, whatever its position. During a jam, Previous never goes to an earlier track, which already left the queue. Not a new `started`. | no: #13 | no: #60 |
| HOST-08 | The player is stopped or idle, the tail is empty | A state brings a jam item | The item starts playing at once, as in HOST-05. | no: #13 | no: #60 |
| HOST-09 | A jam item hits a track failure | — | The error rules of [errors.md](../player/errors.md) apply (ERR-04 to ERR-07). The item was already reported with `started`, so the server moves on as well. A network failure pauses playback as in ERR-01 to ERR-03. | no: #13 | no: #60 |

## Jam wave

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-10 | The jam part is empty, the last state has seeds | The current track ends, or Next | The host starts the jam wave: `POST /rotor/session/new` with the seeds as in WAVE-01, and plays its available tracks. It remembers the session and the `seedsVersion` it started with. Each wave track is reported as `playing{source: wave, track}`, with no `started`. | no: #13 | no: #60 |
| HOST-11 | A jam wave session exists | The jam part becomes empty again | If the last state's `seedsVersion` is the one the session was started with, the host goes on with that session: first its deferred tracks (HOST-12), then load-more as in WAVE-05 to WAVE-09. Otherwise it starts a new session (HOST-10) and drops the old session's deferred tracks. | no: #13 | no: #60 |
| HOST-12 | A jam wave track is current, more wave tracks are in the tail | A state brings a jam item | The current wave track plays to its end. The jam items go in front of the remaining wave tracks, which are deferred for HOST-11. | no: #13 | no: #60 |
| HOST-13 | The jam part is empty and the last state has no seeds | The current track ends, or Next | If a track was playing, the jam wave starts from `track:<id>` of that track. If nothing has played yet, the player stops and the host sends `playing{source: idle}`. | no: #13 | no: #60 |
| HOST-14 | The jam starts while a track plays that is not a jam item | — | That track plays on and is reported as `playing{source: wave, track}`. The rest of the old queue is replaced by the jam's tail. | no: #13 | no: #60 |

## Not learning

The host's account must not learn the party's taste.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-15 | A jam session | Any jam track starts or closes | No `/play-audio` (TRK-01, TRK-02 are not applied). No feedback to My Wave or to any station. | no: #13 | no: #60 |
| HOST-16 | A jam wave session | Its tracks start and close | Rotor feedback (TRK-03 to TRK-11) goes to the jam wave's own session only, so that the wave follows the party. The setting "feedback to the jam wave", on by default, turns it off: then no rotor feedback at all. | no: #13 | no: #60 |
| HOST-17 | A jam track is current | The user likes or dislikes it with the player's own buttons | The like goes to the host's account as usual: that is the user's own choice. A dislike also skips, as TR-07. | no: #13 | no: #60 |

## Skip

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-18 | Jam item I is current | `command{kind: skip, itemId: I}` | As Next (HOST-05, HOST-10). | no: #14 | no: #61 |
| HOST-19 | Item I is not current: another item, a wave track, or nothing plays | `command{kind: skip, itemId: I}` | Ignored. So two quick skips from guests skip one track. | no: #14 | no: #61 |

## Adding as the host

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-20 | Connected | The user adds a track to the jam from its library or search | `add{track}` to the server. The track reaches the tail only with the next state, in its place in the order, never directly. "Play next" is `add` followed by `pin`, or `pin` for a jam item. | no: #14 | no: #61 |
| HOST-21 | A jam session | The user picks a source that would replace the queue (a playlist, an album, a wave) | The queue is not replaced. The app offers to add tracks to the jam instead. | no: #14 | no: #61 |

## Storage and "Continue the jam?"

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-22 | A jam session | `created`, `linkRotated`, `snapshot`, or a change of the outbox | The app stores `roomId`, `hostSecret`, `joinUrl`, the latest snapshot `data` and the outbox on disk, atomically (a temporary file and a rename, or the platform's equivalent). | no: #12, #14 | no: #59, #60 |
| HOST-23 | A stored jam session | The app starts, or Android restores it after the process was killed | The app asks "Continue the jam?". Yes: it connects and sends `resume` with the stored snapshot and outbox. No: it connects, resumes and sends `end`, then clears the storage. | no: #14 | no: #60, #61 |
| HOST-24 | A stored jam session | `resume` is refused with `room-not-found`, `bad-secret` or `bad-key` | The storage is cleared, the jam part of the tail becomes ordinary tracks as in HOST-32, and a status says the jam is over. | no: #14 | no: #61 |

## Without the server

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-25 | Connected | The connection closes or dies | Playback goes on from the tail and the jam wave (HOST-10, with the last state's seeds). The `started` events that should be sent go to the outbox instead, and `playing` is not queued. Removing, pinning, kicking, settings and adding are disabled, and a status says there is no connection to the jam server. | no: #14 | no: #61 |
| HOST-26 | Not connected | — | The host reconnects after 1, 2, 4 … 30 s, and at once when the network comes back. Then `resume` with the snapshot and the outbox. After `resumed`, the outbox is cleared, the controls are enabled again, and the host sends `playing` for the current track. | no: #14 | no: #61 |
| HOST-27 | Not connected | A Yandex request of the player fails for the network | As ERR-01 to ERR-03: the jam changes nothing here. | no: #13 | no: #60 |

## Search and check for guests

A guest's track is built like a track of the app's own parsing ([expected/yandex](../expected/yandex/README.md)): `id`, the first album's id as `albumId`, `title` with the version, the artists' names, `durationMs`, and `coverUri` in the same order of sources (left out when there is none). Artists are cut to 10 and names to 64 characters.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-28 | Connected | `searchRequest{requestId, text}` | `GET /search?text=<text>&type=track&page=0`. `searchResult{requestId, tracks}` with the available tracks of `tracks.results`, in the order of the reply, at most 20. A reply without tracks gives `tracks: []`. | no: #14 | no: #61 |
| HOST-29 | As HOST-28 | The search fails; or the token is rejected (401) | `searchResult{requestId, error: failed}`; or `error: unauthorized`, and the app shows the login problem to its user as usual. | no: #14 | no: #61 |
| HOST-30 | Connected | `validateRequest{requestId, trackIds}` | `POST /tracks` with those ids. `validateResult{requestId, results}` with one result per id, in order: `track` for a track that is available, `reason: track-unavailable` for one that is missing or unavailable. If the request fails: `reason: failed` for every id. | no: #14 | no: #61 |
| HOST-31 | Connected | Either request | The answer goes out within the host timeout ([limits](limits.md#requests-through-the-host)), even while the player is busy: these requests do not wait behind playback work. | no: #14 | no: #61 |

## The end

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-32 | A jam session | The user ends the jam; or `ended` arrives | Connected: `end` is sent (not for `ended`). The current track plays to its end. The jam part of the tail stays as ordinary tracks, and the deferred and remaining wave tracks are dropped. The jam wave does not go on. The storage is cleared. Not connected: the same locally, and the server ends the room on its own when the host has been away long enough (REC-05). | no: #14 | no: #61 |
| HOST-33 | After HOST-32 | — | Ordinary playback rules again: `/play-audio` and feedback for tracks that start from now on. | no: #13 | no: #60 |

## What the host shows

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| HOST-34 | A jam session | The playlist shows the tail | Each jam item carries the name of whoever added it, from `participants` by `addedBy`. Wave tracks are marked as the jam wave. The current track's line says who added it. | no: #15 | no: #62 |
| HOST-35 | A jam session | — | The jam window or screen shows a QR code of `joinUrl`, the link with a copy or share action, the participants with their online state and a kick action, the settings, "new link" and "end", and whether the host is connected. | no: #15 | no: #62 |
