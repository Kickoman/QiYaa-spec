# Player scenarios

How the player behaves, as scenarios that the tests of both apps check. One file per area:

| File | Area | IDs |
|---|---|---|
| [sources.md](sources.md) | Loading a source into the queue: requests, empty sources, unavailable tracks, search | `SRC-` |
| [queue.md](queue.md) | Adding to the queue without replacing it: "Play next" and "Add to the end" | `ADD-` |
| [wave.md](wave.md) | Waves (rotor): start, loading more, shuffle and repeat in a wave | `WAVE-` |
| [continuation.md](continuation.md) | What follows a queue's head: the flag "continue with a wave", the wave from history, shuffle and repeat with a continuation | `CONT-` |
| [tracking.md](tracking.md) | Play reports: `/play-audio` and the rotor feedback events | `TRK-` |
| [errors.md](errors.md) | Network failures and broken tracks during playback | `ERR-` |
| [transport.md](transport.md) | Previous, next, repeat, shuffle and seeking | `TR-` |
| [shuffle.md](shuffle.md) | Shuffle: the pass without repeats, the random track, Previous with shuffle, the choice of algorithm | `SHUF-` |
| [quality.md](quality.md) | The audio quality the user chooses, and what is known about lossless | `QUAL-` |
| [local.md](local.md) | Local files: opening files and a folder, or the phone's media library; their tags and covers, reading from the disk | `LOCAL-` |
| [spotify.md](spotify.md) | Spotify: the login, the picks, playing a track, the keys Spotify refuses, autoplay after a queue | `SPOT-` |
| [lyrics.md](lyrics.md) | Lyrics: the text of the current track from its source or a lyrics service, synced lines and seeking by them | `LYR-` |

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
- **Play order**: the order in which the queue plays: the waiting tracks first ([queue.md](queue.md)),
  then the head in its source order, or with shuffle in the order of the chosen algorithm
  ([shuffle.md](shuffle.md)), then the continuation in its own order.
- **Remaining**: the number of tracks from the current one to the end of the queue in play order,
  counting the current track. It is endless while repeat holds the head (CONT-10) and with Random
  track (SHUF-08).
- **Source request**: a request that replaces the queue. It covers likes, a playlist, an album, an
  artist, a station, a wave, a track played from a search or from an artist or album opened in it
  ([SRC-16, SRC-18](sources.md#search)), local files or a folder, a pick of the phone's media library
  ([local.md](local.md)), and the picks of Spotify ([spotify.md](spotify.md)).
- **Waiting track**: a track that plays once, right after the current one and before the queue goes
  on: added with "Play next" ([ADD-01](queue.md)), or a jam item
  ([jam/host.md](../jam/host.md#the-base)).
- **Status**: a short message the user sees. On desktop this is the main window's status line,
  and on Android a toast or snackbar. The wording is up to each app; the spec only fixes when a
  status appears.

## Known gaps

Where an app does not follow a scenario. Each entry is closed by the issue it links. When an
issue is fixed, remove its tag from the scenario rows and delete the entry here.

None at the moment: both apps follow every scenario that is checked for them.
