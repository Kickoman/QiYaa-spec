# Local files

Playing the user's own audio files. The desktop opens them one by one or as a folder
([Opening](#opening)); Android takes them from the phone's media library instead
([The phone's media library](#the-phones-media-library-android)). Terms and the status columns are
explained in the [README](README.md).

A local file is not from Yandex Music: nothing about it goes to the Yandex API, a wave from
history never takes it as a seed ([CONT-05](continuation.md#the-wave-from-history)), and in a jam
it is a track that only the host can play ([HOST-37](../jam/host.md#the-base)).

The formats are mp3, FLAC, WAV, AAC in MP4 (`.m4a`, `.mp4`) and Ogg Vorbis (`.ogg`, `.oga`,
LOCAL-09). On the desktop an MP4 file with another codec (ALAC, for one) is not taken, and neither
is an Ogg file with another codec (Opus, FLAC). On Android a codec the phone cannot decode fails
when the file plays, and FLAC without a decoder is not taken (LOCAL-13).

## Opening

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| LOCAL-01 | Any queue, possibly playing | The user opens one or more files | The files of the formats above, in the order of LOCAL-02, replace the queue, and the first one plays: a source request as in [SRC-06](sources.md#what-gets-queued). The status names a single file by its name without the extension, and several files by the name of the first one's folder. | yes | n/a |
| LOCAL-02 | Any queue, possibly playing | The user opens a folder | Its files of the formats above are found in it and in its subfolders at any depth, and ordered by their paths in natural order: component by component, ignoring case, with numbers compared by value ("2" before "10"). Hidden files and folders and links to folders are skipped. While the files are read, a status says so. At most 10 000 files from at most 10 000 folders are taken: the rest is left out, and the status says that it was. Then as LOCAL-01, the status naming the folder. | yes | n/a |
| LOCAL-03 | Any queue, possibly playing | The opened files or folder hold no file that LOCAL-01 or LOCAL-02 takes | Status "<name>: empty", and the queue and playback stay as they were, as in [SRC-07](sources.md#what-gets-queued). A source request picked while a folder is read wins over it ([SRC-01](sources.md#requests)). | yes | n/a |

## Tracks

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| LOCAL-04 | A local file | It is queued | The title, the artists, the album, the year and the genre come from its tags, and the duration from the file itself. A file without a title is titled by its name without the extension. Several artists are kept apart. On Android these are the tags the phone's media library keeps for the file: its artists are one line, as the library has it. A duration that cannot be read is left out, as for any track without one: the position cannot be dragged. | yes | not checked: Kickoman/QiYaa-android#118 |
| LOCAL-05 | A local file | Its cover is shown | The picture in its tags, the front cover first. Without one, an image named `cover`, `folder` or `front` (`.jpg`, `.jpeg` or `.png`, in any case) in the file's folder. Otherwise none. Only JPEG and PNG are taken, and a picture over 16 MiB is not: the rest of the file's tags are read as usual. On Android all of this is the media library's: the picture is one in the file's tags (any of them), else any `.jpg` or `.png` in its folder (`albumart` first), at most half the screen's shorter side and with no other limit; the current track shows the front cover in its tags. | yes | not checked: Kickoman/QiYaa-android#118 |
| LOCAL-06 | A local file is current | It plays, or the user seeks | It is read from the disk: nothing is downloaded, a seek plays from the target at once as in [TR-09](transport.md), and the file is never held whole in memory. A file that is cut short while it plays ends the track at the cut, with no status. On Android nothing already played is kept in memory either: a seek back reads the disk. | yes | not checked: Kickoman/QiYaa-android#118 |
| LOCAL-07 | A local file | It cannot be opened or decoded: removed before it plays, broken, or not of its format | A track failure: [ERR-04 to ERR-07](errors.md) apply. A network wait (ERR-01 to ERR-03) never does. | yes | not checked: Kickoman/QiYaa-android#118 |
| LOCAL-08 | A local file | It starts and closes | No `/play-audio` ([TRK-01](tracking.md#play-audio)) and no rotor feedback. Like, dislike and opening the track in the browser are not offered for it. | yes | not checked: Kickoman/QiYaa-android#118 |
| LOCAL-09 | An Ogg Vorbis file | It is opened, read or played | As the other formats: LOCAL-01 to LOCAL-08 apply (on Android LOCAL-04 to LOCAL-08 and LOCAL-11 to LOCAL-13). Its tags are its Vorbis comments, its cover their `METADATA_BLOCK_PICTURE`. Its duration is exact, from the last page's granule position: a seek lands where it should ([TR-08](transport.md)), and the next track follows without a gap. | yes | not checked: Kickoman/QiYaa-android#118 |

## The phone's media library (Android)

Android takes the user's own files from the phone's media library: the index of the phone's music
that the system keeps. Reading it needs the user's permission. A file that leaves the library fails
as a removed one does (LOCAL-07).

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| LOCAL-10 | The app may not read the phone's audio | The user opens the local files of the library | The first time, the app asks for the permission there. Granted: the library's picks show (LOCAL-11). Refused: the view says that the permission is needed and offers to ask again; refused for good, it offers the app's settings in the system instead. Nothing is read meanwhile, and the queue and playback stay as they were. | n/a | not checked: Kickoman/QiYaa-android#118 |
| LOCAL-11 | The app may read the phone's audio | The user picks all tracks, an album, an artist or a folder | The files of the formats above that the media library holds for the pick replace the queue (the library leaves out ringtones, notification and alarm sounds, podcasts and recordings), and the first one plays: a source request as in [SRC-06](sources.md#what-gets-queued). A folder is taken with its subfolders, as in LOCAL-02. An album is ordered by disc and track number, then by title; an artist by album, then as an album; a folder and all tracks by path. Names and paths are compared as in LOCAL-02. | n/a | not checked: Kickoman/QiYaa-android#118 |
| LOCAL-12 | As LOCAL-11 | The pick holds more than 10 000 files, or none the app takes | The first 10 000 in LOCAL-11's order are taken, and the status says that the rest was left out, as in LOCAL-02. A pick with no file the app takes: as LOCAL-03. | n/a | yes |
| LOCAL-13 | A local file whose container the app takes | Its codec is one the phone cannot decode | FLAC on a phone without a FLAC decoder is not taken at all. Any other is taken, and it is a track failure when it plays (LOCAL-07; ALAC in MP4 on most phones): playback never goes on in silence. A codec the phone decodes plays, even one the desktop does not take (Opus in Ogg). | n/a | not checked: Kickoman/QiYaa-android#118 |
| LOCAL-14 | A queue with local files was saved, and the system has since built its media library anew | The app starts | The queue is restored, and the library is asked about its local files in the background. When the library now gives one of them another title or duration, the queue is cleared, unless another source has replaced it by then. A file that is simply gone does not clear it: it fails when it plays (LOCAL-07). | n/a | not checked: Kickoman/QiYaa-android#118 |
