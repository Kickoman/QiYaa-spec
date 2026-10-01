# Wave

A wave is an endless queue from a rotor session. Terms and the status columns are explained in
the [README](README.md). The feedback events a wave sends are in [tracking.md](tracking.md).

## Start

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| WAVE-01 | Any queue | The user starts a wave | A status "<wave>: loading…" appears at once. The app sends `POST /rotor/session/new` with `{"seeds":[…],"includeTracksInResponse":true,"includeWaveModel":true,"interactive":true}`. On success the queue is replaced by the available tracks of `sequence[].track` ([SRC-05](sources.md)) and the first track plays. The app keeps `radioSessionId`, the first seed as the station id, and each track's `batchId`. `radioStarted` is sent ([TRK-03](tracking.md)). | yes | yes |
| WAVE-02 | — | The user picks My Wave or a station | My Wave's seeds are `["user:onyourwave"]`. A station's seeds are `["<type>:<tag>"]`, its id. | yes | yes |
| WAVE-03 | Any queue, possibly playing | `rotor/session/new` answers with no tracks, or only unavailable ones | Status "<wave>: empty". The queue and playback stay as they were, and no feedback is sent for this session. | yes | yes |
| WAVE-04 | Any queue | `rotor/session/new` fails, or its reply has no `radioSessionId` | Error status. The queue and playback stay as they were. | yes | yes |

## Loading more

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| WAVE-05 | A wave, no load-more in flight | A track becomes current (play, next, previous, jump, or the automatic advance) and remaining ≤ 2 | `POST /rotor/session/<radioSessionId>/tracks` with `{"queue":[ids]}`. The ids are the plain track ids (no album) of the last 5 tracks of the queue: the end of the list, not the tracks around the cursor. The available tracks of the reply are appended ([SRC-05](sources.md)). If the reply carries no `radioSessionId`, the old one stays. | yes | yes |
| WAVE-06 | A load-more request is in flight | Another track becomes current with remaining ≤ 2 | No second request. At most one load-more is in flight per queue. | yes | yes |
| WAVE-07 | A load-more request is in flight for wave W | The queue is replaced before the reply arrives, and the reply then succeeds or fails | The reply is dropped silently: nothing is appended, no status appears, and the new queue's load-more state is untouched. | yes | yes |
| WAVE-08 | A wave; the last track ends while load-more is in flight | The reply arrives with new tracks | Playback continues with the first new track. With shuffle it is still the first new track ([WAVE-10](#shuffle-and-repeat)). | yes | yes |
| WAVE-09 | A wave, stopped at its end | Load-more fails, or brings no available track | Playback stays stopped at the end and no request is retried on its own. The next Next sends the load-more request again. | yes | yes |

## Shuffle and repeat

The decision from Kickoman/QiYaa-android#4: in a wave, shuffle and repeat do not apply, as in the
Yandex Music app. The server already picks the tracks, and the wave never ends.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| WAVE-10 | Shuffle is on | A wave starts, or the user turns shuffle on during a wave | The wave plays in queue order. The shuffle control is disabled or ignored during the wave, and the status says why. | yes | yes |
| WAVE-11 | The user turned shuffle on before a wave | An ordinary queue replaces the wave | Shuffle is on again. The wave does not change the user's choice. | yes | yes |
| WAVE-12 | Repeat is on, a wave is at its last track | The track ends, or the user presses Next | The wave does not wrap to its first track. It waits for more, as in WAVE-08 and WAVE-09. | yes | yes |
