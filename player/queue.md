# Adding to the queue

Putting tracks into the queue without replacing it: "Play next" and "Add to the end". Terms and
the status columns are explained in the [README](README.md).

The tracks come from a search or from an artist or album opened in it
([SRC-17](sources.md#search)): one track, or the available tracks of an album in its order. Only
available tracks are added ([SRC-05](sources.md#what-gets-queued)), and each add makes rows of its
own, also for a track the queue already has. Adding is not a source request: it does not drop a
source request in flight, and a source reply that comes later replaces the queue, the added tracks
included ([SRC-06](sources.md#what-gets-queued)).

An added track keeps its own service, Yandex Music or Spotify: it opens and reports through it.
Adding changes neither the queue's flag "continue with a wave" nor its continuation: a queue of local
files keeps its flag off ([CONT-12](continuation.md#the-flag)), and a Spotify queue keeps its
autoplay ([SPOT-20](spotify.md#continuation)). A continuation takes only its own service's tracks:
the seeds of the wave from history ([CONT-05](continuation.md#the-wave-from-history)), the recent
tracks of Spotify's autoplay, and the ids of the queue's last tracks that a wave sends when it loads
more ([WAVE-05](wave.md#loading-more)) leave the others out.

During a jam session the search adds to the jam instead ([HOST-38](../jam/host.md#adding-as-the-host)),
and neither action here is offered.

Whether the queue is **in its head** decides where "Add to the end" puts the tracks: it is while the
last of its own tracks (head or continuation) to become current is a head track, or none has yet and
the head is not empty. Once a continuation's track (a wave, the wave from history, Spotify's
autoplay or radio) has become current, or when the head is empty, the queue is in its continuation.
Waiting tracks change neither.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| ADD-01 | A queue that is not empty | The user picks "Play next" for tracks | They come right behind the current track, in their order, in front of the tracks that already wait there: they are **waiting tracks**. The current track plays on; when it ends, or on Next, the first of them starts. With no current track (it was removed with a waiting track behind it, ADD-07) they come in front of the waiting tracks, and Play or Next starts the first of them. A status says what was added. | yes | yes |
| ADD-02 | Waiting tracks | They play, or the user picks another track | Each plays once, in the order the playlist shows. When the user picks another track, the ones still waiting move right behind it and play next. Shuffle does not take them into its order, repeat does not play them again, and a draw of Random track made before them stands ([SHUF-05](shuffle.md#either-algorithm)): after the last of them the queue goes on in its own order from where it was, in the head ([SHUF-01](shuffle.md#without-repeats)) or in the continuation. Previous from a head track goes back in the head's order and passes them by ([TR-01](transport.md), [SHUF-02](shuffle.md#without-repeats)); from any other track it goes to the row above, which may be one of them that has played ([TR-01](transport.md)). A waiting track that has played stays in the playlist and plays again only when the user picks it; after it, too, the queue goes on from where it was. | yes | yes |
| ADD-03 | A queue in its head | The user picks "Add to the end" for tracks | They are added to the end of the head, after its last track and in front of the continuation's tracks, and are head tracks from then on. In order they play after the head's last track; with Without repeats they join the current pass among the tracks it has still to play, each at a random place ([SHUF-01](shuffle.md#without-repeats)); with Random track the next draw may pick them, and a draw already made stands ([SHUF-05](shuffle.md#either-algorithm), [SHUF-06](shuffle.md#random-track)); repeat plays them with the rest of the head ([TR-05](transport.md)). The current track plays on. A status says what was added. | yes | yes |
| ADD-04 | A queue in its continuation | The user picks "Add to the end" for tracks | They wait as in ADD-01, but behind the tracks that already wait: they play after them, and the continuation goes on after them. A status says what was added. | yes | yes |
| ADD-05 | An empty queue | The user picks "Play next" or "Add to the end" for tracks | They become the queue's head, as a source request would make it but with no ticket and nothing started: under the title of what they came from (the search's "Search: <text>", the album's title, the artist's name), with the flag of a new queue ([CONT-01](continuation.md#the-flag)); Spotify tracks get Spotify's autoplay with the first of them as the context ([SPOT-20](spotify.md#continuation)). The first of them is the current track. A status says what was added. An add with no available track changes nothing. | yes | yes |
| ADD-06 | A queue whose playback stopped at its end ([TR-04](transport.md)) | The user picks "Play next" or "Add to the end" | As ADD-01, ADD-03 or ADD-04. Nothing starts: Next plays the first added track, Play the current one again. | yes | yes |
| ADD-07 | Waiting tracks, or tracks added to the end | The user removes rows, or clears the playlist | They are removed like any row, and a removed waiting track does not play; clearing drops them with the rest of the queue. Removing the current track stops playback, and a waiting track behind it goes on waiting until Play or Next. | yes | no (gap A6) |
| ADD-08 | A queue waiting for its continuation's tracks after Next at its end ([WAVE-08](wave.md#loading-more), [CONT-04](continuation.md#the-wave-from-history)) | The user picks "Play next" or "Add to the end" | The first added track that now comes next starts at once; the continuation's reply is appended when it comes. | yes | yes |
| ADD-09 | Tracks that wait by ADD-01 or ADD-04 | A jam session starts, or runs | They are the base's: the jam's items come in front of them ([HOST-12](../jam/host.md#the-base)), and a new base ([HOST-21](../jam/host.md#adding-as-the-host)) or clearing the playlist ([HOST-36](../jam/host.md#editing-the-playlist)) drops them. | yes | yes |
