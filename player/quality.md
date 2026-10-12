# Audio quality

The user chooses how much data a track costs. Terms and the status columns are explained in the
[README](README.md).

| Quality | What the app fetches | Typical file |
|---|---|---|
| `standard`, the default | The link of [SRC](sources.md) playback as it always was: `download-info`, the best mp3, a `/get-mp3/` link | mp3 192 kbps, about 1.4 MB a minute |
| `economy` | `get-file-info` at level `lq` | HE-AAC 64 kbps in MP4, about a third of standard |

The stored value is the name in the first column. Any other stored value reads as `standard`.
Android keeps two choices, one for Wi-Fi (an unmetered network) and one for mobile data (a
metered one), both `standard` until changed (QUAL-07).

## Fetching the economy file

```
GET /get-file-info?ts=<unix seconds>&trackId=<id>&quality=lq&codecs=he-aac-mp4,mp3&transports=raw&sign=<sign>
Authorization: OAuth <token>
X-Yandex-Music-Client: YandexMusicDesktopAppWindows/5.95.0
```

- **`sign`** is base64 of HMAC-SHA256 over `ts + trackId + quality + codecs + transports`, where
  `codecs` is joined without commas. The last character of the base64 text is dropped. The key is
  `kzqU4XhfCaY6B6JTHODeq5`, the key of Yandex Music's Windows app; the server checks it together
  with the `X-Yandex-Music-Client` header, so the two always go together. The key of Yandex's
  Android app (`p93jhgh689SBReK6ghtw62`) is refused with 403 `not-allowed` since at least
  2026-10-12.
- **The signature contains `+` and `/`.** It must be percent-encoded in the query (`%2B`, `%2F`):
  a bare `+` reads as a space, and the server answers 403 `not-allowed`, as it does for any
  wrong signature.
- **The reply** is `result.downloadInfo` (fixtures in
  [get-file-info/](../fixtures/yandex/get-file-info)): `quality`, `codec`, `bitrate` in kbps,
  `transport`, `url`, `urls` (mirrors of `url`), and `key` for `encraw`. The level is the one the
  server chose: when the track has no file of the asked level or codec, it answers with a higher
  one, down to `nq` mp3 192.
- **The file** at `url` is plain (`transports=raw`) and supports range requests. Its link carries
  its own expiry (`ts=<hex>` in the path, about 10 days ahead), unlike the hour of a `/get-mp3/`
  link.
- **Worked example** for tests: `ts` 1760000000, `trackId` 155647609, `lq`,
  `he-aac-mp4,mp3`, `raw` sign the text `1760000000155647609lqhe-aac-mp4mp3raw` as
  `keefcyb+nfzl1cRn8glR+ZyzpQf/h2gSrMF0fHIOHpQ`.

A reply is **usable** when its status is 200, `url` is an `https` link, `transport` is `raw` and
`codec` is one of the codecs asked for. Anything else falls back (QUAL-03).

## Scenarios

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| QUAL-01 | Quality `standard` | A track needs its link (to start or to preload) | Exactly as before quality existed: `download-info` and a `/get-mp3/` link. No `get-file-info`. | yes | yes |
| QUAL-02 | Quality `economy` | A track needs its link | `get-file-info` as above. The track plays the file at `url`, whichever of the asked codecs it is, and the bitrate shown is the reply's `bitrate`. | yes | yes |
| QUAL-03 | Quality `economy` | `get-file-info` answers with an HTTP error (401, 403, 429 and every other status) or a reply that is not usable | The same track takes the `standard` path (QUAL-01) at once. No status, no track failure ([errors.md](errors.md)): the user still hears the track. A 401 or 403 here is not ERR-08, and it does not send the user back to the login: only the `standard` path's own answers count. | yes | yes |
| QUAL-04 | Quality `economy` | `get-file-info` hits a network failure | A network failure of the link: ERR-01 and ERR-02. When the network is back, the link is asked again in the current quality. | yes | yes |
| QUAL-05 | A track plays, the next one may already be preloaded | The user changes the quality | The playing track goes on with its file. A preloaded next track fetched in the old quality is dropped; the next link is asked in the new quality. | yes | no (gap A2) |
| QUAL-06 | — | The app starts again | The quality is the one chosen last. | yes | yes |
| QUAL-07 | Separate choices for Wi-Fi and mobile data | A track needs its link | The choice of the network in use at that moment applies. A change of network changes the quality in effect as in QUAL-05. | n/a | yes |
| QUAL-08 | A track plays in one quality, and the choice in effect is another now | The app reads the playing file again (a seek beyond what is downloaded, a link that expired) | It reads the same file in the quality the track started with: a file of another codec cannot continue it. A track that loads from its start again (desktop: after a network wait) takes the choice in effect. | yes | yes |

Listening along in a jam takes only `/get-mp3/` links, so at `economy` the host shares nothing
([LISTEN-13](../jam/listen.md)).

## Lossless: not yet

Yandex Music has lossless files, and both apps could offer a third quality. It waits until
someone with access to lossless can test it: the account the apps were built with (Plus, region
Belarus) never gets one. What is known, from probes on 2026-10-12:

- **Asking.** `quality=lossless` with codecs `flac` and `flac-mp4`. Known levels are
  `lossless`, `hq`, `nq` and `lq`; known codecs `flac`, `flac-mp4`, `aac-mp4`, `he-aac-mp4`,
  `aac`, `he-aac` and `mp3`. `-mp4` means an MP4 container. For `flac-mp4` the reply's `bitrate`
  is 0. Without access the server answers `nq`: `aac-mp4` 192 when that codec is asked,
  otherwise mp3 192. `hq` was never returned either.
- **Who gets it.** Most likely only some regions, Russia among them. Neither the VPN exit
  (Frankfurt) nor a direct connection from Minsk got lossless.
- **Encryption.** Unknown whether lossless comes with `raw` or only with `encraw`. An `encraw`
  file is AES-128-CTR with the reply's `key` (hex) and an all-zero initial counter block; this
  was checked on an `lq` file, which decrypted to the same MP4 as its `raw` twin. A range read
  starting at byte `n` starts the counter at block `n / 16` and skips `n % 16` bytes of the
  keystream. The key must never reach a jam room (ROOM-50 in [../jam/room.md](../jam/room.md)).
- **Desktop.** miniaudio's FLAC decoder (dr_flac) plays `flac`. `flac-mp4` needs the MP4
  demuxer the economy quality brings (minimp4) and FLAC frames fed with a stream header built
  from the `dfLa` box. Tracks are held whole in memory: a lossless file is 5 to 10 times an
  mp3. The kbps display has three digits.
- **Android.** Media3 extracts FLAC and FLAC in MP4 (economy's HE-AAC in MP4 needed nothing);
  decoding goes through MediaCodec, which has
  FLAC from API 27 while `minSdk` is 26, so the app must check for a decoder. An `encraw` file
  needs a decrypting `DataSource` (Media3's `AesCipherDataSource` derives its own nonce and does
  not fit). The equalizer works on 16-bit PCM, so 24-bit FLAC loses its depth there.
- **Jam.** A lossless link is not a `/get-mp3/` link and is not shared (LISTEN-13).

Tracked in Kickoman/QiYaa-spec#1. **To start:** a tester with lossless access runs the same probe (one `get-file-info` per
level, `raw` and `encraw`, reading the first bytes of the file). The answers decide the codecs
and the transport; then lossless gets its scenarios here, after QUAL-06, and a row of its own in
[parity.md](../parity.md).
