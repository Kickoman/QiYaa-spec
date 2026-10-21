# Transport

Previous, next, repeat, shuffle and seeking. Terms and the status columns are explained in the
[README](README.md). Shuffle and repeat act on the queue's head: in a wave with an empty head (My
Wave, a station) they do not apply ([WAVE-10 to WAVE-12](wave.md#shuffle-and-repeat)), and what
follows a head plays in its own order ([CONT-09 to CONT-11](continuation.md#shuffle-and-repeat)).
How shuffle orders the head, with its two algorithms, is in [shuffle.md](shuffle.md).

The desktop hotkeys, media keys and system media panels (MPRIS, SMTC, the Android notification)
act the same way as the buttons.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| TR-01 | The current track has played for at most 3 s, shuffle is off or does not apply | Previous | The previous track plays. On the first track it goes to the last track if repeat is on, and restarts the first track if repeat is off. With shuffle on, [SHUF-02](shuffle.md#without-repeats) and [SHUF-07](shuffle.md#random-track) apply instead. | yes | yes |
| TR-02 | The current track has played for more than 3 s | Previous | The current track restarts from 0. This is a seek, not a new start: no events and no `/play-audio` (unlike Play's restart, [TRK-02](tracking.md)). | yes | yes |
| TR-03 | A track is current | Next, or the current track ends | The next track in play order plays. Without shuffle that is the following one. Shuffle orders the head only (CONT-09). With shuffle on, the next track is that of [SHUF-01](shuffle.md#without-repeats) or [SHUF-06](shuffle.md#random-track). | yes | yes |
| TR-04 | Repeat is off, the last track in play order is current (with shuffle, the pass's last: [SHUF-01](shuffle.md#without-repeats); Random track with two or more head tracks has no last track: [SHUF-08](shuffle.md#random-track)), and no continuation follows: the flag "continue with a wave" is off ([CONT-01](continuation.md#the-flag)), or the wave from history could not start (CONT-06, CONT-07) | Next, or the last track ends | Playback stops. The cursor stays on the last track, and the queue stays. | yes | yes |
| TR-05 | Repeat is on, the head's last track in play order is current | Next, or that track ends | The head's first track in play order plays; with shuffle that is the pass's first, and the same pass plays again ([SHUF-01](shuffle.md#without-repeats)). A wave from history does not start while repeat is on (CONT-10). | yes | yes |
| TR-06 | — | The user switches repeat | Repeat has two states, off and on (the whole queue). There is no repeat of one track. | yes | yes |
| TR-07 | A track of the head is current (a wave track: [TRK-07](tracking.md)) | The user dislikes the current track | The dislike request of [TRK-07](tracking.md), then the same as Next: TR-03, TR-04 or TR-05. | yes | yes |
| TR-08 | A track plays or is paused | The user seeks | The position shows the target at once and stays on it until audio from the target plays, then runs on from there. It never goes back to the position from before the seek, also during a series of seeks. A paused track stays paused at the target. | yes | yes |
| TR-09 | The download of the current track has stalled midway, and no network wait of [ERR-01 to ERR-03](errors.md) has begun | The user seeks into the part already downloaded, at least 3 s before its end | Audio plays from the target at once. It does not wait for the download to move on or to fail. Closer to the end of the downloaded part a seek may wait for more bytes: a decoder can need data past the target. | yes | yes |
| TR-10 | A track of the playlist was picked for an action (on the desktop a right click, on Android a long press) | The user dislikes it | The dislike request of [TRK-07](tracking.md). Which track is current is decided when the action runs, by the track's id. If the current track has that id, then the same as TR-07 (TRK-07 for a wave track). Otherwise nothing that plays changes, and no event goes to a wave. | yes | yes |
