# Transport

Previous, next, repeat and shuffle in an ordinary, finite queue. Terms and the status columns are
explained in the [README](README.md). In a wave, shuffle and repeat do not apply
([WAVE-10 to WAVE-12](wave.md)).

The desktop hotkeys, media keys and system media panels (MPRIS, SMTC, the Android notification)
act the same way as the buttons.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| TR-01 | The current track has played for at most 3 s | Previous | The previous track plays. On the first track it goes to the last track if repeat is on, and restarts the first track if repeat is off. | yes | yes |
| TR-02 | The current track has played for more than 3 s | Previous | The current track restarts from 0. This is a seek, not a new start: no events and no `/play-audio` (unlike Play's restart, [TRK-02](tracking.md)). | yes | yes |
| TR-03 | A track is current | Next, or the current track ends | The next track in play order plays. Without shuffle that is the following one. With shuffle it is a random other track of the queue. | yes | yes |
| TR-04 | Repeat is off, the last track is current | Next, or the last track ends | Playback stops. The cursor stays on the last track, and the queue stays. | yes | yes |
| TR-05 | Repeat is on, the last track is current | Next, or the last track ends | The first track plays. | yes | yes |
| TR-06 | — | The user switches repeat | Repeat has two states, off and on (the whole queue). There is no repeat of one track. | yes | yes |
| TR-07 | A finite queue | The user dislikes the current track | The dislike request of [TRK-07](tracking.md), then the same as Next: TR-03, TR-04 or TR-05. | yes | yes |

Not fixed yet: which track Previous goes to when shuffle is on. The desktop goes to the one before
it in the queue, and Android to the one before it in the shuffled order.
