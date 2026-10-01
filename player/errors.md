# Errors during playback

How the player reacts when a track cannot be played. Terms and the status columns are explained in
the [README](README.md). This comes from the decision in Kickoman/QiYaa-android#3.

A failure is classified by its type and code, never by the text of its message.

- **Network failure**: the device has no connection, or the connection failed, broke off or
  timed out (DNS, refused, reset or closed midway, unreachable, timeout, a failed TLS handshake as
  behind a captive portal). This covers the link request (`download-info`) and the stream. A
  broken connection counts as a network failure even when a status line had arrived.
- **Account failure**: HTTP 401 or 403 on `download-info` or on the stream (ERR-08).
- **Track failure**: everything else. That is any other HTTP error on `download-info` or on the
  stream, no usable download variant, or audio the decoder cannot read.

Failures of source requests are covered in [sources.md](sources.md) (SRC-04), and failures of
feedback in [tracking.md](tracking.md) (TRK-11).

## Network

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| ERR-01 | A track is starting or playing | A network failure hits its link or its stream | The player pauses on this track and keeps its position. A status says it is waiting for the network. The queue does not move. | yes | yes |
| ERR-02 | Paused by ERR-01 | The network comes back | The track loads again. If it was playing before the failure and the user has not paused since, playback continues from the same position. If the network was up all along, the app retries after 2 s, then 4 s, 8 s and so on up to 60 s between attempts. | yes | yes |
| ERR-03 | A track is playing | Its stream breaks off midway through a network failure | When the buffered audio runs out, the same as ERR-01 and ERR-02. It does not move on to the next track. | yes | yes |
| ERR-09 | A started track waits by ERR-01 to ERR-03 | The wait begins, and later the track continues | It is the same play: the track stays open. No `skip` when the wait begins, and no `trackStarted` or `/play-audio` when it continues ([tracking.md](tracking.md)). A track whose link failed before it ever started starts normally once it plays. | yes | not checked |

## Broken tracks

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| ERR-04 | A track is starting, a next track exists ([TR-03](transport.md)) | It hits a track failure, and it is the first or second track failure in a row | A status names the failure. The player goes to the next track and plays it. | yes | yes |
| ERR-05 | Two track failures in a row | A third track in a row hits a track failure | The player stops on that track. A status says that playback stopped after 3 failed tracks. | yes | yes |
| ERR-06 | Some track failures in a row | A track actually starts playing audio, or a new queue is set | The count of failures in a row goes back to 0. | yes | yes |
| ERR-07 | A track is starting, and there is no next track (end of a finite queue without repeat) | It hits a track failure | The player stops on it with the status of ERR-04. In a wave it waits for more instead ([WAVE-08](wave.md)). | yes | yes |

## Account

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| ERR-08 | A track is starting or playing | Its link or stream answers HTTP 401 or 403 | A status says access was refused. Playback stops on this track: no next track, and it does not count as a track failure (ERR-05). | yes | yes |
