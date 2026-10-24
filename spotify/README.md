# Spotify protocol test vectors

The bytes that Spotify's client protocol ([player/spotify.md](../player/spotify.md)) must produce on
both apps. The desktop app speaks it through `src/spotify/protocol`, which has been checked against
the real servers. The Android app ports that code to Kotlin
([Kickoman/QiYaa-android#113](https://github.com/Kickoman/QiYaa-android/issues/113)), and both
apps' tests check themselves against these files. Each case holds its inputs and its results, so a
test computes from the inputs and compares.

## Format

- Each file is one JSON object, `{"description": …, "cases": […]}`, formatted as the
  [top README](../README.md#rules-for-files) requires.
- Bytes are lower-case hex strings, two digits per byte; `""` is no bytes. Multi-byte numbers
  inside bytes are big-endian unless a file says otherwise.
- Numbers are JSON integers, all below 2^53. The numbers of `protobuf.json` (`varint`, `zigzag`,
  `value`), which go up to 2^64 − 1, are decimal strings; `zigzag` is signed.
- Text is a JSON string, UTF-8.
- `null` is "nothing": an input that is refused, or a value that never comes to exist.
- A file with several kinds of case gives each one a `kind`.

## Files

### `shannon.json`: the packet cipher

- `cases[]`: `{name, key, steps[]}`. Make one Shannon cipher from `key` and make the `steps`
  calls in order, comparing every result:
  - `{call: "nonce", value}`;
  - `{call: "encrypt", input, output}` and `{call: "decrypt", input, output}`;
  - `{call: "finish", mac}`, a MAC of `mac`'s length (4 bytes).
- What an app must get right:
  - the key and the data are read as **little-endian** 32-bit words, but the nonce is loaded as
    its four **big-endian** bytes;
  - the MAC covers the plaintext, both when encrypting and when decrypting;
  - a word split over two or more calls gives the same bytes and MAC as one call;
  - a received packet is decrypted as its 3-byte header and then its payload, under one nonce;
  - each `nonce` call starts the cipher afresh.
- The known answers `b90d865f…95822a` with MAC `bea0ba6c` and `ab0010…` with MAC `8cc90655` are the
  desktop test's (after the `shannon` crate); the other cases are made from them.

### `crypto.json`: the primitives

| `kind` | Fields | What |
|---|---|---|
| `sha1` | `message`, `digest` | FIPS 180-2 A.1 and A.2, and the empty message |
| `hmac-sha1` | `key`, `message`, `digest` | RFC 2202's seven cases |
| `aes-128-ctr` | `key`, `iv`, `offset`, `input`, `output` | `input` is the stretch from byte `offset` of a stream whose block n is encrypted with counter `iv + n`: SP 800-38A F.5.1 whole and from inside it, a counter that carries past its low 32-bit word, a counter that wraps around 2^128 |
| `modpow` | `base`, `exponent`, `modulus`, `result` | `base^exponent mod modulus`, unsigned big-endian |
| `dh` | `prime`, `generator`, `privateA`, `privateB`, `publicA`, `publicB`, `shared` | Diffie-Hellman in Oakley group 1 (RFC 2409, 768 bits) with 95-byte private keys |
| `rsa-pkcs1-sha1` | `modulus`, `exponent` (a number), `message`, `signature`, `encoded`, `valid`, sometimes `privateExponent` | The check of an RSA PKCS#1 v1.5 signature over SHA-1(`message`) |

- **The CTR counter is the whole 128-bit IV**, a big-endian number: block n uses `(iv + n) mod
  2^128`, and a stretch from byte `offset` starts at block `offset / 16`, `offset % 16` bytes in.
  Library CTR modes may count in the low 32 or 64 bits only; building CTR from AES-ECB, one block
  per counter, avoids that.
- **`modpow`'s result has exactly the modulus's length**, leading zero bytes kept (`0002` for
  2 mod 0x0101). `result` is `null` where the desktop refuses: a base that is not below the modulus,
  an even modulus. It also refuses an empty modulus and one longer than 512 bytes; no case covers
  those. A library that reduces such a base silently must refuse it itself.
- **`dh`:** the public keys and the secret also keep the prime's length, 96 bytes. In the second case
  the secret starts with a zero byte.
- **`rsa-pkcs1-sha1`:** `valid` holds when the signature, read as a number s, is below the modulus
  and `s^exponent mod modulus`, written in the modulus's length (`encoded`), is
  `00 01 FF…FF 00 ‖ DigestInfo ‖ SHA-1(message)` with either DigestInfo:
  `3021300906052b0e03021a05000414` (with the `05 00` NULL parameters) or
  `301f300706052b0e03021a0414` (without), as BearSSL accepts both. A signature shorter than the
  modulus is read as a number, that is left-padded with zeros. A signature longer than the modulus is
  refused before any arithmetic, even when its extra leading bytes are zero. `encoded` is `null` when
  the signature is refused that way or s is not below the modulus. Two keys: the desktop test's 1024-bit key, whose private half is not known
  (its signature of `abc` comes from that test), and a 1024-bit key made for these vectors, given
  with its `privateExponent` so that a test can sign more.

### `audio.json`: decrypting a track's file

- `kind: "decrypt"`: `{key, iv, offset, encrypted, decrypted}`. AES-128-CTR as in `crypto.json`,
  with `iv` always Spotify's fixed `72e067fbddcbcf77ebe8bc643f630d93`, from byte `offset` of the
  file.
  - The file is 200 bytes whose plain bytes are 0, 1, …, 199 under the key `07…07`. It is decrypted
    whole, from byte 5 and byte 21 (as in the desktop test) and from every byte 160 to 170, around
    the header's end.
  - Two offsets far beyond any file test the counter: one where its low 32-bit word wraps
    (block 0xc09cf26d), one at block 2^32.
- `kind: "header"`: `{key, iv, encrypted, headerSize, audio}`. Decrypt the whole file and drop the
  first `headerSize` = 167 bytes, Spotify's header; `audio` is what the Ogg Vorbis decoder gets.
  The offset is always the file's: audio byte x is file byte 167 + x, for a seek and for an HTTP
  range alike.

### `handshake.json`: an access point session

Sessions of the access point client against a fake access point, the one the desktop's tests use.

- **Inputs:**
  - `identity`: `{platform, cpuFamily, os, protocolVersion, systemInformation, versionString,
    deviceId}` of the client hello and the login;
  - `credentials`: `{username, type, data}`, `type` 1 for stored credentials, 3 for an OAuth token;
  - `random`: what the client's random source returns, call by call: the 95-byte DH private key,
    then the 16-byte nonce. The first four cases use the desktop test's source: byte i of every
    call is (i × 13 + 5) mod 256, i counting from 0 in each call. The last two use i × 13 + 103 and
    i × 13 + 8, found so that the client's public key, or the shared secret, starts with a zero byte;
  - `trustedKey`: `{modulus, exponent}`, the key the client checks the server's signature with: the
    fake access point's, or Spotify's own in the case of an access point it did not sign;
  - `dhPrime`: Oakley group 1, generator 2.
- **`server`:** the fake access point. Its DH key is fixed: `dhPrivate` (byte i is i × 7 + 3) and
  `dhPublic`. So is `signature`, its PKCS#1 v1.5 SHA-1 signature of `dhPublic` by the RSA key
  `rsaModulus`, `rsaExponent`. The private half of that key is not published; the precomputed
  signature is all a fake access point needs.
- **`keys`:** what the handshake derives, `null` when the client never gets that far:
  - `clientPublic` and `shared`: the client's public key and the shared secret with their leading
    zero bytes dropped, as the client sends and uses them;
  - `material`: HMAC-SHA1(`shared`, transcript ‖ i) for i = 1 to 5, each i one byte (`01` to `05`),
    the five digests joined: 100 bytes. The transcript is the client hello and the server's response,
    both whole, with their length prefixes;
  - `challengeAnswer`: HMAC-SHA1(`material[0..20]`, transcript);
  - `clientToServer`, `serverToClient`: the Shannon keys, `material[20..52]` and `material[52..84]`.

  `[a..b]` here is bytes a to b − 1: the end is excluded, unlike Kotlin's `a..b`
  (`copyOfRange(a, b)` takes the same bytes).
- **`steps[]`:** the calls on the client in order, and what comes out of each.
  - `call` is `start`; `receive`, with `nowMs` and `incoming`, the messages the server sends (the
    client gets their `wire` bytes joined); `advance`, with `nowMs`; or `requestAudioKey`, with
    `track`, `file` and `returns`, the request's sequence or `null`.
  - `outgoing`: the messages the client writes during the call, in order.
  - `events`: `{event: "loggedIn", username, type, data}`, `{event: "loginFailed", code,
    description}`, `{event: "country", code}`, `{event: "product", type}`, `{event: "audioKey",
    sequence, key}`, `{event: "audioKeyRefused", sequence, code}` or `{event: "failure"}`.
  - `deadline` (when the keep-alive next needs `advance`, or `null`), `loggedIn` and `failed`, after
    the call.
  - A message is `{what, wire}`. A packet after the handshake also has `nonce`, `command` (one byte)
    and `payload`, decrypted.
- **Rules the steps follow:**
  - `deadline` is `null` whenever `loggedIn` is false;
  - a refused login is `loginFailed` alone, without `failure`; an `AuthFailure` the client cannot
    read is a refused login with code 0. `failure` ends the session for every other reason: a
    signature the trusted key does not verify, a bad MAC, a server response the client cannot read,
    a handshake message whose length is below 4 or above 64 KiB, a missed keep-alive. Packets of a
    known command but the wrong length (`AesKey`, `AesKeyError`) are ignored, not failures;
  - audio key requests are numbered from 0 on each connection, one up per request.

**The wire:**
- The client hello is `00 04`, a 32-bit length of the whole message (these six bytes included) and
  the protobuf. The server's response and the client's answer are a 32-bit length (itself included)
  and the protobuf.
- A packet is encrypted with the Shannon key of its direction under a nonce that counts that
  direction's packets from 0: `command`, a 16-bit payload length, the payload, then the 4-byte MAC
  of `finish`.
- `Ping` (0x04) is answered by `Pong` (0x49) with four zero bytes, not the ping's payload, through
  `advance` 60 s later. Commands the client does not know (0x76 in the first case) are ignored.
- `RequestKey` (0x0c) is the file id (20 bytes), the track's gid (16), the sequence (32 bits) and
  `00 00`. `AesKey` (0x0d) is the sequence and the 16-byte key; another length is ignored.
  `AesKeyError` (0x0e) is the sequence and a 16-bit code; a payload under 6 bytes is ignored, a
  longer one is read from its first 6.
- `CountryCode` (0x1b) is ASCII. `ProductInfo` (0x50) is XML, and the product is the text between
  its first `<type>` and `</type>`.
- The keep-alive: the first ping is due 20 s after `APWelcome`, the pong 60 s after a ping, the
  `PongAck` (0x4a) 20 s after the pong, the next ping 80 s after that. A missed one is `failure`.

**Protobuf field numbers** (`{…}` is a nested message):

| Message | Fields |
|---|---|
| `ClientHello` | 10 build info {10 product 0, 20 product flags 0, 30 platform, 40 version}, 30 crypto suite 0, 50 {10 Diffie-Hellman {10 public key, 20 server keys known 1}}, 60 nonce, 70 padding `1e` |
| `APResponseMessage` | 10 challenge {10 {10 Diffie-Hellman {10 gs, 20 server signature key 1, 30 gs signature}}}, 30 `APLoginFailed` |
| `ClientResponsePlaintext` | 10 {10 Diffie-Hellman {10 HMAC}}, 20 {} and 30 {}: two empty messages that are still written |
| `Login` (0xab) | 10 credentials {10 username, left out when empty, 20 type, 30 data}, 50 system info {10 CPU family, 60 OS, 90 system information, 100 device id}, 70 version string |
| `APWelcome` (0xac) | 10 username, 30 reusable credentials type, 40 reusable credentials; the rest is ignored |
| `APLoginFailed` (0xad, and in the response) | 10 error code, 40 description |

**What an app must check:**
- The client: replay the steps against its own access point code, feeding it `random`, and compare
  `outgoing`, `events`, `deadline`, `loggedIn` and `failed` after each call. Feeding the incoming
  bytes one at a time must give the same.
- A fake access point: answer a client hello with the response, which it can copy from the case or
  build from `server` by the table above (`APResponseMessage` {10 {10 {10 {10 `dhPublic`, 20 1,
  30 `signature`}}}}, after its length); derive `keys`; check the client's answer and packets; send
  the `incoming` packets.

### `protobuf.json`: the wire format

- `kind: "write"`: `{fields, bytes}`. Writing `fields` in order gives `bytes`. A field is
  `{field, varint}` (a decimal string), `{field, string}` (written as its UTF-8 bytes),
  `{field, bytes}` or `{field, message: [fields]}`, a nested message as a length-delimited field.
- `kind: "read"`: `{bytes, fields}`. What a reader returns, field by field: `{field, wire, varint,
  zigzag}` for wire type 0; `{field, wire, bytes, value}` for wire types 1 and 5, little-endian;
  `{field, wire, bytes}` for wire type 2. Spotify's signed fields (a track's duration, an image's
  width, a year, a list item's status) are zigzag.
- `kind: "reject"`: `{bytes, fieldsBefore}`. The reader must fail; `fieldsBefore` are the fields it
  returned first. A top-level message whose reader failed is dropped whole. The desktop does not
  apply this to nested messages in three places, where a failed nested reader keeps what it read:
  the AP's response, login5's `ok` and `challenges`, and client-token's hashcash parameters. Groups (wire types 3 and 4), wire
  types 6 and 7, field number 0 and numbers above 2^29 − 1 are refused, and so are a varint of more
  than ten bytes and anything cut short.
- A ten-byte varint whose last byte is above 1 does not fit in 64 bits. The desktop takes it and
  drops the high bits (`08ffffffffffffffffff02` reads as 2^63 − 1); no vector covers it, and an
  implementation may refuse it instead.

### `ids.json`: base62 ids

- `cases[]`: `{name, base62, gid, uri}`. `gid` is the 16-byte big-endian number that the 22
  `base62` digits write, and `uri` is `spotify:track:<base62>`. The digits are `0-9`, then `a-z`,
  then `A-Z`, lower case before upper case, unlike many base62 libraries.
- `gid` and `uri` are `null` for a refused id: not 22 digits, a character outside the digits, or a
  number of 2^128 or more.

### `messages.json`: request bodies

| `kind` | Fields | What |
|---|---|---|
| `hashcash` | `context`, `prefix`, `length`, `suffix` | The solution of a hashcash challenge |
| `login5` | `clientId`, `deviceId`, `username`, `data`, `context`, `hashcash`, `body` | `POST login5.spotify.com/v3/login` with stored credentials, alone or answering a challenge |
| `clientToken` | `clientVersion`, `clientId`, `deviceId`, `platform`, `body` | `POST clienttoken.spotify.com/v1/clienttoken` |
| `clientTokenAnswer` | `state`, `suffix`, `body` | The same, answering its hashcash challenge |
| `metadata` | `uris`, `extensionKind`, `body` | An `extended-metadata` request: 10 `TRACK_V4`, 9 `ALBUM_V4`, 8 `ARTIST_V4` |
| `autoplay` | `contextUri`, `recentTrackUris`, `body` | `POST /context-resolve/v1/autoplay` |
| `collection` | `username`, `set`, `pageToken`, `limit`, `body` | `POST /collection/v2/paging` |

- **Hashcash:** the target is the last 8 bytes of SHA-1(`context`), a big-endian 64-bit number. For
  n = 0, 1, …, the suffix is (target + n) mod 2^64 and n, both 8 bytes big-endian; the solution is
  the first suffix for which the last 8 bytes of SHA-1(`prefix` ‖ suffix) end in at least `length`
  zero bits. client-token's challenge has no context, login5's comes with one.
- **login5's answer carries how long solving took** (`hashcash.tookNanos`), as whole seconds and
  the remaining nanoseconds, each left out when 0. The protocol code does not read the clock: that
  time is an input.
- **client-token's answer carries the suffix as upper-case hex text.**
- `platform` is one of:
  - `{os: "linux", release, version, hardware}`: the kernel release, the distribution's version
    and the CPU. Android sends this form (`os.version`, `Build.VERSION.RELEASE`, `aarch64` or `arm`);
  - `{os: "windows", osVersion, osBuild, imageFileMachine, peMachine}`, the desktop's on Windows;
  - `{os: "macos", systemVersion, cpuType}`, the desktop's on macOS; the model is always `iMac21,1`.

| Body | Fields |
|---|---|
| login5 | 1 client info {1 client id, 2 device id}, 2 the challenge's context when there is one, 3 {1 {1 hashcash {1 suffix, 2 duration {1 seconds, 2 nanoseconds}}}}, 100 stored credential {1 username, 2 data} |
| client-token | 1 request type 1, 2 client data {1 client version, 2 client id, 3 {1 platform data, 2 device id}} |
| platform data, Linux | 5 {1 `Linux`, 2 release, 3 version, 4 hardware} |
| platform data, Windows | 4 {1 `osVersion`, 3 `osBuild`, 4 2, 6 9, 7 `imageFileMachine`, 8 `peMachine`, 10 1} |
| platform data, macOS | 3 {1 `systemVersion`, 2 `iMac21,1`, 3 `cpuType`} |
| client-token answer | 1 request type 2, 3 {1 state, 2 {1 challenge type 3, 4 {1 suffix as text}}} |
| extended-metadata | 2 for each URI {1 URI, 2 {1 extension kind}} |
| autoplay | 1 context URI, 2 each recent track URI |
| collection | 1 username, 2 set, 3 page token when there is one, 4 limit |

## Where the values come from

- **Standards:** FIPS 180-2 (SHA-1), RFC 2202 (HMAC-SHA1), NIST SP 800-38A F.5.1 (AES-CTR),
  RFC 2409 (the Oakley group 1 prime). The generator stops if it does not reproduce their results.
- **The desktop's tests** at the commit below: the Shannon known answers, the desktop RSA key and
  its signature of `abc`, the DH private keys, the hashcash challenge and the base62 id of
  `tests/spotify_protocol_test.cpp`; the fake access point's DH key, RSA key and signature of
  `tests/support/fake_access_point.cpp`; the session of `tests/spotify_access_point_test.cpp`
  (randomness, identity, credentials, welcome, key requests, keep-alive times).
- **Made for these vectors:** the second RSA test key, the leading-zero cases, the huge offsets and
  the other synthetic inputs. Nothing comes from a real account: usernames, credentials, tokens and
  device ids are made up. The client ID and Spotify's access point key are the public constants of
  [player/spotify.md](../player/spotify.md) and the desktop's code.
- **Every other result** is computed by the desktop's `src/spotify/protocol` and its BearSSL subset,
  through the generator in [scripts/spotify-vectors](../scripts/spotify-vectors). The files were last
  generated against Kickoman/QiYaa `b99c1dfb8a8eb90b49316b2e146a9c753448a0d3` (v0.9.0).

`npm run check` ([scripts/check-spotify-vectors.mjs](../scripts/check-spotify-vectors.mjs)) checks
the format, fails on a `kind` or a `call` this README does not describe, and recomputes every result
from its inputs, independently of the desktop's code:
- with `node:crypto` and `BigInt`: SHA-1, HMAC-SHA1, AES-128-CTR (through AES-ECB, one block per
  counter), the audio decryption, `modpow`, `dh`, the RSA checks in both DigestInfo forms, the
  hashcash solutions;
- with its own Shannon, protobuf, base62 and access point client, written from this README:
  `shannon.json`, `protobuf.json`, `ids.json`, the request bodies, and every handshake session. The
  client replays each session's steps, whole and byte by byte, and must give the same messages,
  events, deadlines and flags; the fake access point's side (the response, `keys`, the packets in
  both directions with their MACs) is rebuilt from `server`.

## Changing them

The generator is a C++20 program built against a checkout of the desktop app; it needs a C and a
C++ compiler, not Qt:

```bash
make -C scripts/spotify-vectors DESKTOP=/path/to/QiYaa BUILD=/tmp/spotify-vectors
```

- `DESKTOP`, `BUILD` and `OUT` are absolute or relative to `scripts/spotify-vectors`. `DESKTOP`
  defaults to `../../..`, the desktop checkout when this spec is its `spec/` submodule; `BUILD` to
  `scripts/spotify-vectors/build` (ignored by git); `OUT` to this folder.
- The generator is compiled from `DESKTOP` on every run (about half a minute), so it never writes
  the files from other or older sources. The output is deterministic: running it again on unchanged
  code gives the same files.
- Then run `npm run check`, update the desktop commit above and commit, following the spec workflow
  in the [top README](../README.md). A change in these bytes is a change in behaviour: it needs an
  issue for the other app.
