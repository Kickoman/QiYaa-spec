# Local files

Playing the user's own audio files, opened one by one or as a folder. Terms and the status columns
are explained in the [README](README.md).

A local file is not from Yandex Music: nothing about it goes to the Yandex API, a wave from
history never takes it as a seed ([CONT-05](continuation.md#the-wave-from-history)), and in a jam
it is a track that only the host can play ([HOST-37](../jam/host.md#the-base)).

The formats are mp3, FLAC, WAV and AAC in MP4 (`.m4a`, `.mp4`). An MP4 file with another codec
(ALAC, for one) is not taken.

## Opening

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| LOCAL-01 | Any queue, possibly playing | The user opens one or more files | The files of the formats above, in the order of LOCAL-02, replace the queue, and the first one plays: a source request as in [SRC-06](sources.md#what-gets-queued). The status names a single file by its name without the extension, and several files by the name of the first one's folder. | no: #30 | no: #111 |
| LOCAL-02 | Any queue, possibly playing | The user opens a folder | Its files of the formats above are found in it and in its subfolders at any depth, and ordered by their paths in natural order: component by component, ignoring case, with numbers compared by value ("2" before "10"). Hidden files and folders and links to folders are skipped. While the files are read, a status says so. At most 10 000 files from at most 10 000 folders are taken: the rest is left out, and the status says that it was. Then as LOCAL-01, the status naming the folder. | no: #30 | no: #111 |
| LOCAL-03 | Any queue, possibly playing | The opened files or folder hold no file that LOCAL-01 or LOCAL-02 takes | Status "<name>: empty", and the queue and playback stay as they were, as in [SRC-07](sources.md#what-gets-queued). A source request picked while a folder is read wins over it ([SRC-01](sources.md#requests)). | no: #30 | no: #111 |

## Tracks

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| LOCAL-04 | A local file | It is queued | The title, the artists, the album, the year and the genre come from its tags, and the duration from the file itself. A file without a title is titled by its name without the extension. Several artists are kept apart. A duration that cannot be read is left out, as for any track without one: the position cannot be dragged. | no: #30 | no: #111 |
| LOCAL-05 | A local file | Its cover is shown | The picture in its tags, the front cover first. Without one, an image named `cover`, `folder` or `front` (`.jpg`, `.jpeg` or `.png`, in any case) in the file's folder. Otherwise none. Only JPEG and PNG are taken, and a picture over 16 MiB is not: the rest of the file's tags are read as usual. | no: #30 | no: #111 |
| LOCAL-06 | A local file is current | It plays, or the user seeks | It is read from the disk: nothing is downloaded, a seek plays from the target at once as in [TR-09](transport.md), and the file is never held whole in memory. A file that is cut short while it plays ends the track at the cut, with no status. | no: #30 | no: #111 |
| LOCAL-07 | A local file | It cannot be opened or decoded: removed before it plays, broken, or not of its format | A track failure: [ERR-04 to ERR-07](errors.md) apply. A network wait (ERR-01 to ERR-03) never does. | no: #30 | no: #111 |
| LOCAL-08 | A local file | It starts and closes | No `/play-audio` ([TRK-01](tracking.md#play-audio)) and no rotor feedback. Like, dislike and opening the track in the browser are not offered for it. | no: #30 | no: #111 |
