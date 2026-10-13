# Continuation

What plays after a queue's head. Terms and the status columns are explained in the
[README](README.md). The wave rules this file refers to are in [wave.md](wave.md), the feedback in
[tracking.md](tracking.md).

A queue's source has two parts:

- The **head**: a fixed list of tracks. Likes, a playlist, an album, an artist's tracks and search
  results are heads. A head may be empty.
- The **continuation**, which a source may have: an endless supply of tracks that follows the head
  and hears the events of its tracks, for feedback. My Wave and a station are an empty head with a
  wave as the continuation ([wave.md](wave.md)).

A queue whose source has no continuation of its own can go on with a **wave from history**: a rotor
wave seeded with the tracks this queue has played. Each queue has a flag, "continue with a wave",
that says whether it does.

## The flag

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| CONT-01 | Any queue | A source request replaces the queue ([SRC-06](sources.md#what-gets-queued), [WAVE-01](wave.md#start)) | The new queue's flag "continue with a wave" takes the value of the setting of the same name. The setting is on by default. For a source with a continuation of its own (My Wave, a station) the flag changes nothing. | no: #28 | no: #79 |
| CONT-02 | A queue whose source has no continuation of its own | The user switches the flag | Only this queue's flag changes, not the setting. Switched on: the wave from history follows the head, and starts at once if remaining ≤ 2 already (CONT-04). Switched off: the wave from history asks for no more tracks; the tracks it gave stay in the queue and play, and after the last one playback stops as in [TR-04](transport.md). | no: #28 | no: #79 |
| CONT-03 | The flag is off, playback stopped at the end of the head (TR-04) | The user switches the flag on and presses Next | The wave from history starts (CONT-04), and its first track plays when it comes, as in [WAVE-08](wave.md#loading-more). | no: #28 | no: #79 |

## The wave from history

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| CONT-04 | The flag is on, repeat is off, the wave from history has no session yet, and CONT-05 gives at least one seed | A track becomes current or starts ([tracking.md](tracking.md)), and remaining ≤ 2 | The wave from history starts: `POST /rotor/session/new` as in WAVE-01, with the seeds of CONT-05. Unlike WAVE-01, the queue is not replaced and no status appears: the available tracks of the reply are appended after the head ([SRC-05](sources.md#what-gets-queued)). The app keeps `radioSessionId`, the first seed as the station id and each track's `batchId`, and sends `radioStarted` ([TRK-03](tracking.md#rotor-feedback)). If the head ends before the reply, playback waits for it as in WAVE-08. From then on the queue loads more as a wave (WAVE-05 to WAVE-09). At most one request is in flight (WAVE-06), and a reply for a queue that was replaced meanwhile is dropped (WAVE-07). Outside a jam the queue keeps this one session; a jam may start others ([HOST-11, HOST-32](../jam/host.md#the-base)). | no: #28 | no: #79 |
| CONT-05 | As CONT-04 | The seeds are chosen | The rule of [jam/seeds.md](../jam/seeds.md#the-rule) applied to the tracks of this queue that have started, the latest start first: each track id once, at most 5, as `track:<id>` without an album. Tracks a continuation gave are not candidates, and neither are tracks that are not from Yandex Music. | no: #28 | no: #79 |
| CONT-06 | As CONT-04, but no track can be a seed yet: none has started, or none of those that did is from Yandex Music | A track becomes current with remaining ≤ 2 | No request. The wave from history waits for a track that starts (CONT-04). If none does before the head ends, playback stops at its end as in TR-04. | no: #28 | no: #79 |
| CONT-07 | The first request of the wave from history fails, or brings no available track | — | Nothing is appended. A failure shows an error status, an empty reply none. When the head ends, playback stops at its end as in TR-04, and the next Next sends `rotor/session/new` again with the seeds as they are then, as in WAVE-09. | no: #28 | no: #79 |
| CONT-08 | A wave from history | The tracks of the queue start and close | `/play-audio` for every track as usual ([TRK-01, TRK-02](tracking.md#play-audio)). Rotor feedback (TRK-03 to TRK-11) only for the tracks the wave gave, to the session each came from; the head's tracks send none. | no: #28 | no: #79 |

## Shuffle and repeat

Shuffle and repeat act on the head. In a queue whose head is empty (My Wave, a station),
[WAVE-10 to WAVE-12](wave.md#shuffle-and-repeat) apply.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| CONT-09 | Shuffle is on, the flag is on | The head plays | Shuffle orders the head only ([TR-03](transport.md)). The head runs out when its shuffled order has played each of its tracks once; then the tracks of the wave from history follow in their own order. A shuffle that may play a track again before every track of the head has played has no end, so the wave from history never starts. | no: #28 | no: #79 |
| CONT-10 | Repeat is on, the flag is on | The head's last track in play order ends, or Next | The head's first track plays ([TR-05](transport.md)): the head wraps, and the wave from history does not start while repeat is on. If its tracks are already in the queue (repeat was switched on after they came), they wait behind the head and follow it once repeat is off. | no: #28 | no: #79 |
| CONT-11 | A track that a continuation gave is current, shuffle or repeat is on | Next, or the track ends | The continuation's next track plays: shuffle does not reorder the continuation, and repeat does not wrap the queue, as in WAVE-10 and WAVE-12. | no: #28 | no: #79 |
