# Shuffle

The order in which a queue's head plays when shuffle is on. Terms and the status columns are
explained in the [README](README.md). Next, Previous and repeat are in [transport.md](transport.md);
this file says which track they go to with shuffle on. Shuffle orders the head only: the tracks
of a continuation follow it in their own order
([CONT-09 to CONT-11](continuation.md#shuffle-and-repeat)), and in a wave with an empty head
(My Wave, a station) shuffle does not apply ([WAVE-10 to WAVE-12](wave.md#shuffle-and-repeat)).

Shuffle is two choices of the user: the switch, on or off, and the algorithm. The algorithm applies
while the switch is on; switching off does not change it.

| Algorithm | Stored value | Next plays |
|---|---|---|
| Without repeats, the default | `without-repeats` | The next track of the pass: every track of the head once, in a random order (SHUF-01) |
| Random track | `random` | Another track of the head, drawn at random; a track may come again before every track has played (SHUF-06) |

Any other stored value reads as `without-repeats`. With either algorithm the playlist shows the
queue in its own order; only the play order changes. A head that holds the same track twice holds
two tracks, and each plays once in a pass. The pass is not saved: a queue that the app restores
after a restart starts a new pass from the restored track. The switch is kept with the queue:
Android restores it with its saved queue; the desktop restores no queue and starts with shuffle off.

## Without repeats

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SHUF-01 | Shuffle is on with Without repeats, the head has tracks | Next, or a track ends, again and again | The head plays as a pass: each of its tracks once, in a random order. A pass begins with the track that is current when it is made: a new queue's first track ([SRC-06](sources.md#what-gets-queued)), or the current track when the order starts afresh (SHUF-03, SHUF-04). After the pass's last track the head has run out: playback stops as in [TR-04](transport.md), or the continuation follows ([CONT-09](continuation.md#shuffle-and-repeat)). With repeat on, the pass's first track follows its last and the same pass plays again; it is not shuffled anew ([TR-05](transport.md), [CONT-10](continuation.md#shuffle-and-repeat)). Tracks that a continuation appends never join the pass, and waiting jam items play first and leave it as it is ([HOST-12](../jam/host.md#the-base)). | yes | yes |
| SHUF-02 | Shuffle is on with Without repeats | Previous, the current track has played for at most 3 s; or the user picks a track of the head | Previous on a track of the head: the track before it in the pass plays, whether it has played or not. On the pass's first track Previous goes to the pass's last if repeat is on, and restarts the current track if repeat is off, as [TR-01](transport.md) does without shuffle; after 3 s, [TR-02](transport.md). Previous on the first track that a continuation gave: the head's track that was current last. A picked track plays, and the pass goes on from its place: Next plays the track after it in the pass, and tracks before that place that have not played do not play in this pass. | yes | no (gap A5) |

## Either algorithm

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SHUF-03 | A track plays or is paused, in a queue with a head | The user switches shuffle on or off | The current track, its position and its pause stay as they are: no new start, no events. On: the order of the head starts afresh from the current track by the chosen algorithm, a new pass that begins with it (SHUF-01) or a new draw (SHUF-06). Switching on while it is on changes nothing. Off: the head plays in its source order from the current track. In a wave, WAVE-10 and WAVE-11 apply. | yes | yes |
| SHUF-04 | Shuffle is on, a track of the head is current | The user removes tracks of the head | The removed tracks do not play. The order starts afresh as in SHUF-03, from the current track or, if it was removed, from the track that becomes current: with Without repeats a new pass of every track left in the head, so tracks already heard may play again. Removing tracks that a continuation gave, or jam items, leaves the order as it is. | yes | no (gap A4) |
| SHUF-05 | Shuffle is on | The app fetches the next track ahead; the user presses Next; or the track ends and the next one follows without a gap | All three take the same next track. Looking ahead does not move the cursor or use the track up: it becomes current only when it starts. With Random track the next track is drawn when a track becomes current or the order starts afresh (SHUF-03, SHUF-04), and looking ahead again gives the same one. When the next track changes (shuffle, the algorithm, repeat, a removal), a track fetched ahead for the old one is dropped. | yes | not checked |

## Random track

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SHUF-06 | Shuffle is on with Random track, the head has at least two tracks, a track of the head is current | Next, or the track ends | Another track of the head plays, drawn at random among all its tracks but the current one, each with the same chance. A track may play again before every track of the head has played. Repeat does not change which track comes next. A head of one track plays as without shuffle ([TR-04, TR-05](transport.md), [CONT-09](continuation.md#shuffle-and-repeat)). | yes | no: #NNN |
| SHUF-07 | Shuffle is on with Random track, a track of the head is current and has played for at most 3 s | Previous | The track before the current one in the head's source order plays, not the track that played before it. On the head's first track Previous goes to the head's last if repeat is on, and restarts the first track if repeat is off, as in [TR-01](transport.md). After 3 s, [TR-02](transport.md). From the first track that a continuation gave, as in SHUF-02. | yes | no: #NNN |
| SHUF-08 | Shuffle is on with Random track, the head has at least two tracks, a track of the head is current | Tracks play on | The head never runs out. Playback does not stop at its end ([TR-04](transport.md)), remaining is endless, and the wave from history never starts, whatever the flag ([CONT-04](continuation.md#the-wave-from-history), [CONT-09](continuation.md#shuffle-and-repeat)). Tracks of a continuation that are already in the queue (they came before Random track took effect) wait behind the head: one plays only when the user picks it, and from there the continuation goes on in its own order ([CONT-11](continuation.md#shuffle-and-repeat)). | yes | no: #NNN |

## The choice

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| SHUF-09 | — | The user chooses the algorithm (on the desktop in the main menu, "Shuffle") | Two choices, Without repeats and Random track; Without repeats until the user chooses. Choosing does not switch shuffle on or off. With shuffle in effect the order starts afresh from the current track by the new algorithm, and the current track, its position and its pause stay, as in SHUF-03. With shuffle off, or in a wave, the choice is kept and applies when shuffle does. | yes | no: #NNN |
| SHUF-10 | An algorithm was chosen | The app starts again | The algorithm is the one chosen last. With nothing stored, or a stored value other than `without-repeats` and `random`, it is Without repeats. | yes | no: #NNN |
