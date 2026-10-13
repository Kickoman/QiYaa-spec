# Transport

Previous, next, repeat, shuffle and seeking. Terms and the status columns are explained in the
[README](README.md). Shuffle and repeat act on the queue's head: in a wave with an empty head (My
Wave, a station) they do not apply ([WAVE-10 to WAVE-12](wave.md#shuffle-and-repeat)), and what
follows a head plays in its own order ([CONT-09 to CONT-11](continuation.md#shuffle-and-repeat)).

The desktop hotkeys, media keys and system media panels (MPRIS, SMTC, the Android notification)
act the same way as the buttons.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| TR-01 | The current track has played for at most 3 s | Previous | The previous track plays. On the first track it goes to the last track if repeat is on, and restarts the first track if repeat is off. | yes | yes |
| TR-02 | The current track has played for more than 3 s | Previous | The current track restarts from 0. This is a seek, not a new start: no events and no `/play-audio` (unlike Play's restart, [TRK-02](tracking.md)). | yes | yes |
| TR-03 | A track is current | Next, or the current track ends | The next track in play order plays. Without shuffle that is the following one. With shuffle it is a random other track of the head (CONT-09). | yes | yes |
| TR-04 | Repeat is off, the last track is current, and no continuation follows: the flag "continue with a wave" is off ([CONT-01](continuation.md#the-flag)), or the wave from history could not start (CONT-06, CONT-07) | Next, or the last track ends | Playback stops. The cursor stays on the last track, and the queue stays. | yes | yes |
| TR-05 | Repeat is on, the head's last track is current | Next, or that track ends | The first track plays. A wave from history does not start while repeat is on (CONT-10). | yes | yes |
| TR-06 | — | The user switches repeat | Repeat has two states, off and on (the whole queue). There is no repeat of one track. | yes | yes |
| TR-07 | A track of the head is current (a wave track: [TRK-07](tracking.md)) | The user dislikes the current track | The dislike request of [TRK-07](tracking.md), then the same as Next: TR-03, TR-04 or TR-05. | yes | yes |
| TR-08 | A track plays or is paused | The user seeks | The position shows the target at once and stays on it until audio from the target plays, then runs on from there. It never goes back to the position from before the seek, also during a series of seeks. A paused track stays paused at the target. | yes | not checked |
| TR-09 | The download of the current track has stalled midway | The user seeks into the part already downloaded, at least 3 s before its end | Audio plays from the target at once. It does not wait for the download to move on or to fail. Closer to the end of the downloaded part a seek may wait for more bytes: a decoder can need data past the target. | yes | not checked |

Not fixed yet: which track Previous goes to when shuffle is on. The desktop goes to the one before
it in the queue, and Android to the one before it in the shuffled order.
