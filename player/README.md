# Player scenarios

How the player behaves, as scenarios that the tests of both apps check. One file per area:

| File | Area | IDs |
|---|---|---|
| [sources.md](sources.md) | Loading a source into the queue: requests, empty sources, unavailable tracks, search | `SRC-` |
| [wave.md](wave.md) | Waves (rotor): start, loading more, shuffle and repeat in a wave | `WAVE-` |
| [tracking.md](tracking.md) | Play reports: `/play-audio` and the rotor feedback events | `TRK-` |
| [errors.md](errors.md) | Network failures and broken tracks during playback | `ERR-` |
| [transport.md](transport.md) | Previous, next, repeat and shuffle in an ordinary queue | `TR-` |

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
  - **no**: the app does not do this. A tag such as `gap D2` names the entry in
    [Known gaps](#known-gaps) that describes what the app does instead.

IDs are permanent (see the [top README](../README.md#rules-for-files)). A test that checks a
scenario names its ID in its name or in a comment.

## Terms

- **Queue**: the list of tracks the player walks through. It holds only available tracks
  ([SRC-05](sources.md)), and every index refers to that filtered list.
- **Wave**: a queue that is endless. It comes from a rotor session and grows by loading more.
  Every other queue is **finite**.
- **Current track**: the track under the cursor, whether it is playing, paused or stopped.
- **Remaining**: the number of tracks from the current one to the end of the queue in play order,
  counting the current track.
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
| D1 | Desktop | On a network failure it stops. A link failure stops playback. A stream that drops plays what arrived, sends `skip` and moves on. | ERR-01, ERR-02, ERR-03 | Kickoman/QiYaa#8 |
| D2 | Desktop | A broken track (no usable link, or the decoder fails) stops playback. There is no skip, no limit and no counter. | ERR-04 to ERR-07 | Kickoman/QiYaa#9 |
| D3 | Desktop | Previous always goes to the previous track. There is no 3-second rule. | TR-01, TR-02 | Kickoman/QiYaa#10 |
| D4 | Desktop | Empty likes, and any source whose tracks are all unavailable, clear the queue. An empty first wave batch clears the queue and leaves a dead wave. | SRC-07, SRC-08, WAVE-03 | Kickoman/QiYaa#11 |
| A1 | Android | Sends no rotor feedback: no `radioStarted`, `trackStarted`, `trackFinished` or `skip`, and no station fallback. | WAVE-01, TRK-03 to TRK-11 | Kickoman/QiYaa-android#26 |
| A2 | Android | Sends `/play-audio` when the player switches to an item, including the first item of a queue loaded without autoplay. It does not repeat the report when the same track plays again. | TRK-01, TRK-02 | Kickoman/QiYaa-android#27 |
| A4 | Android | If loading more fails at the end of a wave, the wave stays stopped and Next only shows a message. A load-more failure for a queue that was already replaced still shows an error. A broken last track of a wave stops the wave instead of waiting for more. | WAVE-07, WAVE-09, ERR-07 | Kickoman/QiYaa-android#29 |
