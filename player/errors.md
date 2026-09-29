# Errors during playback

How the player reacts when a track cannot be played. Terms and the status columns are explained in
the [README](README.md). This comes from the decision in Kickoman/QiYaa-android#3.

A failure is classified by its type and code, never by the text of its message.

- **Network failure**: the device has no connection, or the connection failed or timed out (DNS,
  refused, unreachable, timeout). This covers the link request (`download-info`) and the stream.
- **Track failure**: everything else. That is an HTTP error other than 401/403 on
  `download-info` or on the stream, no usable download variant, or audio the decoder cannot
  read.

Failures of source requests are covered in [sources.md](sources.md) (SRC-04), and failures of
feedback in [tracking.md](tracking.md) (TRK-11).

## Network

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| ERR-01 | A track is starting or playing | A network failure hits its link or its stream | The player pauses on this track and keeps its position. A status says it is waiting for the network. The queue does not move. | no: stops (gap D1) | yes |
| ERR-02 | Paused by ERR-01 | The network comes back | The track loads again. If it was playing before the failure and the user has not paused since, playback continues from the same position. If the network was up all along, the app retries after 2 s, then 4 s, 8 s and so on up to 60 s between attempts. | no (gap D1) | yes |
| ERR-03 | A track is playing | Its stream breaks off midway through a network failure | When the buffered audio runs out, the same as ERR-01 and ERR-02. It does not move on to the next track. | no: plays what arrived, then goes to the next track (gap D1) | yes |

## Broken tracks

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| ERR-04 | A track is starting, a next track exists ([TR-03](transport.md)) | It hits a track failure, and it is the first or second track failure in a row | A status names the failure. The player goes to the next track and plays it. | no: stops (gap D2) | yes |
| ERR-05 | Two track failures in a row | A third track in a row hits a track failure | The player stops on that track. A status says that playback stopped after 3 failed tracks. | no (gap D2) | yes |
| ERR-06 | Some track failures in a row | A track actually starts playing audio, or a new queue is set | The count of failures in a row goes back to 0. | no (gap D2) | yes |
| ERR-07 | A track is starting, and there is no next track (end of a finite queue without repeat) | It hits a track failure | The player stops on it with the status of ERR-04. In a wave it waits for more instead ([WAVE-08](wave.md)). | no (gap D2) | in a finite queue yes; in a wave it stops (gap A4) |
