# Player scenarios

How the player behaves, as scenarios that the tests of both apps check. One file per area:

| File | Area | IDs |
|---|---|---|
| [sources.md](sources.md) | Loading a source into the queue: requests, empty sources, unavailable tracks, search | `SRC-` |
| [wave.md](wave.md) | Waves (rotor): start, loading more, shuffle and repeat in a wave | `WAVE-` |
| [continuation.md](continuation.md) | What follows a queue's head: the flag "continue with a wave", the wave from history, shuffle and repeat with a continuation | `CONT-` |
| [tracking.md](tracking.md) | Play reports: `/play-audio` and the rotor feedback events | `TRK-` |
| [errors.md](errors.md) | Network failures and broken tracks during playback | `ERR-` |
| [transport.md](transport.md) | Previous, next, repeat, shuffle and seeking | `TR-` |
| [shuffle.md](shuffle.md) | Shuffle: the pass without repeats, the random track, Previous with shuffle, the choice of algorithm | `SHUF-` |
| [quality.md](quality.md) | The audio quality the user chooses, and what is known about lossless | `QUAL-` |

## Reading a scenario

Each row is one scenario:

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|

- **Given**: the state before the action.
- **When**: the action or event.
- **Then**: what must happen. This includes API requests, events the app shows or sends, and the
  queue and playback state afterwards. Anything that is not listed is left as it was.
- **Desktop** and **Android**: whether that app does this today.
  - **yes**: the app does this.
  - **not checked**: nobody has checked that app against this scenario yet.
  - **no**: the app does not do this. A tag such as `gap D2` names the entry in
    [Known gaps](#known-gaps) that describes what the app does instead. `no: #N` names the issue
    in that app's repository that adds a new behaviour.
  - **n/a**: by design only on the other platform (a phone's networks). There is no gap.

IDs are permanent (see the [top README](../README.md#rules-for-files)). A test that checks a
scenario names its ID in its name or in a comment.

## Terms

- **Queue**: the list of tracks the player walks through. It holds only available tracks
  ([SRC-05](sources.md)), and every index refers to that filtered list.
- **Head** and **continuation**: the two parts of a queue's source. The head is a fixed list of
  tracks, possibly empty; the continuation, which a source may have, is an endless supply of
  tracks after it ([continuation.md](continuation.md)). A queue with a continuation is
  **endless**, every other queue is **finite**.
- **Wave**: a continuation from a rotor session that grows by loading more. My Wave and a station
  are a wave with an empty head ([wave.md](wave.md)); a wave from history follows a head
  ([continuation.md](continuation.md)). A **wave track** is a track a wave gave.
- **Current track**: the track under the cursor, whether it is playing, paused or stopped.
- **Play order**: the order in which the queue plays: the head in its source order, or with
  shuffle in the order of the chosen algorithm ([shuffle.md](shuffle.md)), then the continuation
  in its own order.
- **Remaining**: the number of tracks from the current one to the end of the queue in play order,
  counting the current track. It is endless while repeat holds the head (CONT-10) and with Random
  track (SHUF-08).
- **Source request**: a request that replaces the queue. It covers likes, a playlist, an album, an
  artist, a station, a wave and a search.
- **Status**: a short message the user sees. On desktop this is the main window's status line,
  and on Android a toast or snackbar. The wording is up to each app; the spec only fixes when a
  status appears.

## Known gaps

Where an app does not follow a scenario. Each entry is closed by the issue it links. When an
issue is fixed, remove its tag from the scenario rows and delete the entry here.

| Gap | App | What the app does instead | Scenarios | Issue |
|---|---|---|---|---|
| A2 | Android | A next track that ExoPlayer has already begun to load keeps the quality it was fetched in; the tracks after it follow the new choice. Replacing the loaded item would reshuffle ExoPlayer's shuffle order | QUAL-05 | Kickoman/QiYaa-android#76 |
| A4 | Android | Removing tracks of the head keeps the pass: the removed tracks leave it and the others keep their order, so a track already heard does not come back in it | SHUF-04 | Kickoman/QiYaa-android#NNN |
| A5 | Android | Previous on the first track of a continuation goes to the last track of the head's pass, not to the head track that was current last. The two differ when the user picked a track of the continuation before the pass ended | SHUF-02 | Kickoman/QiYaa-android#NNN |
