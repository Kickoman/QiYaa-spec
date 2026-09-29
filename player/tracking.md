# Play tracking

Reports the app sends to Yandex about what it plays. Terms and the status columns are explained in
the [README](README.md).

A track **starts** when its link is resolved and its audio begins to arrive. With a gapless
preload, it starts at the boundary where it takes over, or when Next jumps to it. A track that is
only queued has not started, and neither has a track whose link failed.

A track is **closed** when the app leaves it after it started. It closes **finished** if it played
to its end and its download completed. In every other case it closes as a **skip**.

## `/play-audio`

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| TRK-01 | Any queue | A track starts | `POST /play-audio` (form) with these fields: `track-id`; `album-id`; `from=web-own_tracks-track-track-main`; `play-id`, a new UUID; `uid`; `timestamp` and `client-now`, both the current UTC time as `yyyy-MM-ddTHH:mm:ss.SSSZ`; `track-length-seconds`; `total-played-seconds=0`; `end-position-seconds=0`. A failure is only logged and does not change playback. No report is sent when a track ends. | yes | no: reports when an item becomes current, even when it is not played (gap A2) |
| TRK-02 | A track is playing | The same track starts again: a restart, or repeat of a one-track queue | A new `/play-audio` with a new `play-id`. | yes | no (gap A2) |

## Rotor feedback

Only a wave sends these. The events of a track go to the session the track came from, even after
the queue has moved on to another wave.

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| TRK-03 | — | A wave starts ([WAVE-01](wave.md)) | Event `{"type":"radioStarted","timestamp":…,"from":"web-main-rup-radio-main"}` with the first `batchId`. It has no `trackId`. It is sent before the `trackStarted` of the first track. | yes | no (gap A1) |
| TRK-04 | A wave | A track starts | Event `trackStarted` with `trackId` = `"<id>:<albumId>"`, or `"<id>"` without an album, and the `batchId` of the batch the track came in. It has no `totalPlayedSeconds`. | yes | no (gap A1) |
| TRK-05 | A wave track has started | It closes finished | Event `trackFinished` with `trackId` and `totalPlayedSeconds` ([TRK-08](#played-seconds)). | yes | no (gap A1) |
| TRK-06 | A wave track has started | It closes as a skip: Next, Previous, a jump, a restart, Stop, a new queue, removal of the current track, quitting the app, or the end of a track whose download failed | Event `skip` with `trackId` and `totalPlayedSeconds`. It is sent before the next track's `trackStarted`. | yes | no (gap A1) |
| TRK-07 | A wave track is current | The user dislikes it | `POST /users/<uid>/dislikes/tracks/add-multiple` with `track-ids=<id>`. Then the app acts as for Next ([TR-03](transport.md)), so the track closes with `skip`. There is no separate dislike event, and the track stays in the queue. | yes | no: no `skip` (gap A1) |

## Played seconds

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| TRK-08 | A track has started | It closes | `totalPlayedSeconds` counts only the time that audio actually played. Seeks (jumps in either direction), pauses and buffering are not counted. It is rounded to 0.1 s, and each start resets it to 0. | yes | no (gap A1) |

## Delivery

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| TRK-09 | A wave session | A feedback event is sent | `POST /rotor/session/<radioSessionId>/feedback` with `{"event":{…},"batchId":"…"}`. `batchId` is left out when it is empty. | yes | no (gap A1) |
| TRK-10 | A wave session | The session endpoint answers 4xx | The same event goes to `POST /rotor/station/<stationId>/feedback?batch-id=<batchId>`, whose body is the bare event object. The session is remembered, and its later events go straight to the station endpoint. Without a station id nothing more is sent. | yes | no (gap A1) |
| TRK-11 | A wave session | The feedback request fails with 5xx or a timeout | It is logged and not retried. There is no fallback. | yes | no (gap A1) |
