# Guest

What the QiYaa app does as a guest in someone else's jam, the same on desktop and Android unless a
row says otherwise. The server keeps the room ([room.md](room.md)), and the host plays and checks
the tracks ([host.md](host.md)). The guest adds tracks to the host's queue from its own account:
its search, its likes and its playlists. To the server it is a QiYaa guest
([protocol](protocol/README.md#who-may-send-what)): after `join` it sends only `add` with `track`,
`remove` and `skip`. It plays nothing, and the user's own queue and player stay as they are.

The web guest lives in [Kickoman/QiYaa-jam](https://github.com/Kickoman/QiYaa-jam). Both guests
follow the **Client** scenarios of [recovery.md](recovery.md#guests-reconnect), but only the web
guest listens along ([listen.md](listen.md)). Once Android has verified jam links (GUEST-21), a jam link opens the app
on a phone that has it. The app's user then reaches the web guest's listening along only by
opening the link in a browser.

## Reading a scenario

The table is the one from [player/README.md](../player/README.md#reading-a-scenario):

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|

`no: #N` names the issue in that app's repository that adds the behaviour.

## Terms

- **Link**: the join link `https://<server>/j/<roomId>#<joinSecret>`
  ([protocol](protocol/README.md#identifiers-and-secrets)). Its **server** is its origin: the
  scheme, the host, the port and any path before `/j/`.
- **Membership**: what the app keeps for one room of one server: the `participantId`, the name,
  the join secret, the time of the first join, whether the server ever answered `joined`
  (a **joined** membership), and whether the host ended the jam (an **ended** one). It lives
  until sign-out, or for the membership life of [limits.md](limits.md#connections) from the
  first join (GUEST-03).
- **Guest session**: the room the app shows as a guest, from opening a link until the user
  leaves, a `join` is refused for good, or `ended` or `kicked` arrives. The app has at most one.
  A link that has no secret and no membership (GUEST-05) opens none.
- **In the room**: the guest session got `joined`. It stays in the room while its connection
  drops and comes back (GUEST-15) and while it waits for the room (GUEST-16).
- **Connected**: in the room, with an open connection whose `join` got `joined`.

## The link

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| GUEST-01 | — | The user pastes text into the app's "Join a jam", shares text to the app, or the system hands the app a link (GUEST-21) | The app takes the first link in the text, so the host's share text ("Join my jam: https://…") works as well. The guest connects to `wss://<server>/ws` of the link's server, not to the jam server set in the app. The scheme and the host are taken in lower case, without the default port and without a trailing `/`, so one room has one membership. A secret that is not 22 characters of base64url counts as missing (GUEST-05). Text without a link, or an `http://` link, is refused with a status, and nothing connects. Before sign-in, the app keeps the link, opens its sign-in with a note that joining needs it, and opens the link after sign-in; leaving the sign-in forgets the link. A link to the room of the current guest session shows that session. A link to another room leaves the current one as in GUEST-18 and opens the new one; it leaves it also when the new link turns out to open no session (GUEST-05). | no: #16 | not checked: #118 |

## Joining

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| GUEST-02 | No membership for the link's room, and the link has a secret | The link is opened | The app asks for the guest's name. It is prefilled with the last name used as a guest, or else with the account's display name or login cut to 24 characters ([limits](limits.md#fields)). On "Join", the app trims the name and turns each run of spaces into one space. A space is any character of JavaScript's `\s`, U+00A0, U+2000 to U+200A and U+FEFF included: the server checks names with that set. A name that still breaks the name rule of [limits.md](limits.md#fields) is not sent, and the app says so. | no: #16 | yes |
| GUEST-03 | As GUEST-02 | The user joins | The app makes a `participantId`, a lower-case UUID v4. It stores the membership before it sends the first `join`, so that a crash before `joined` leaves no second participant behind. `joined` makes the membership joined, and the app shows the room. A membership that was never joined is dropped when its `join` is refused, whatever the reason, and the reason shows at once. A membership outlives leaving (GUEST-18), `kicked`, `ended` and a wait for the room that gave up (GUEST-16, GUEST-17). Only sign-out and the membership life of [limits.md](limits.md#connections) forget it. A rejected token does not: it is the same user on the same device. | no: #16 | yes |
| GUEST-04 | A membership for the link's room | The link is opened, with or without a secret | The app joins at once with the stored `participantId` and name, and with the link's secret, or the stored one when the link has none. There is no name step: the server admits a known participant whatever the secret (ROOM-05, ROOM-13). | no: #16 | yes |
| GUEST-05 | No membership for the link's room | The link has no secret (a messenger dropped the part after `#`), or a broken one | The app says that the link lacks its secret and asks for the whole link the host sent. It does not connect. | no: #16 | yes |

## Adding

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| GUEST-06 | In the room | The user searches, or opens its likes or its playlists | The requests go to Yandex Music with the guest's own account, as in the app's own library and search. The lists offer the available tracks of the Yandex catalog. The app never sends `search` to the jam server (ROOM-27), so the host's account searches nothing for the guest. The lists work while the host is offline too (GUEST-09). | no: #16 | yes |
| GUEST-07 | Connected, the host online | The user adds a track from those lists | `add{track}`, never `add{trackId}` (ROOM-65). The track is built as in [Search and check for guests](host.md#search-and-check-for-guests). Before the message goes out, the app checks it against its schema and its size against the guest frame size ([limits](limits.md#connections)), and it does not send one that fails. A close with 1008 or 1009 counts against the IP (ROOM-56, ROOM-57), and the guests at a party share one. The host checks the track (ROOM-16, ROOM-17). The item reaches the queue with the next state, with the host's metadata. | no: #16 | yes |
| GUEST-08 | In the room | A list shows a track | Its add action shows the first case that applies. The track is waiting in the queue: that it is the guest's own, or whose it is. An `add` of it waits for its reply: that it is being sent. The host is offline: adding is disabled (GUEST-09). The guest has `maxPendingPerGuest` waiting items: the limit, with a hint to wait until one plays. The last `add` of it was refused: the reason, except for `duplicate` and `queue-limit`, which the queue already shows. Otherwise: an action to add it. | no: #16 | yes |
| GUEST-09 | In the room, `hostOnline: false` | — | A status says that the host is offline. The lists of GUEST-06 still work, but adding and skipping are disabled: the server would refuse them with `host-offline` (ROOM-18, ROOM-46, REC-01). When the host is back, they work again. | no: #16 | yes |
| GUEST-10 | A waiting item that the guest added, pinned or not | The user removes it | `remove{itemId}` (ROOM-28). The item leaves the queue with the next state. Other participants' items, the current item and recent items offer no remove. | no: #16 | yes |
| GUEST-11 | `guestsCanSkip: true`, an item is current (`nowPlaying.source: item`), the host is online | The user skips it | `skip{itemId}` (ROOM-43). The host moves on, and the next state shows it. In every other case there is no skip: the setting is off (ROOM-44), a wave track or nothing plays (ROOM-45), or the host is offline (ROOM-46). | no: #16 | yes |

## The guest's own account

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| GUEST-12 | In the room, the current track is a track of the Yandex catalog | The user likes or unlikes it | The like goes to the guest's own account, as a like in its own player, and the button shows whether that account likes the track. Nothing goes to the jam. There is no like while nothing plays, and none for a track that only the host can play (`track.source: host`, [protocol](protocol/README.md#tracks)). | no: #16 | yes |
| GUEST-13 | A guest session | Anything happens in the room | The guest plays nothing of the jam. The user's own queue and player stay as they were, and whatever plays goes on. No `/play-audio` and no rotor feedback for jam tracks: TRK-01 to TRK-11 do not apply to them. The QiYaa guest does not listen along ([listen.md](listen.md)). The user's library and player work as usual during the session. | no: #16 | yes |

## What the guest shows

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| GUEST-14 | In the room | Each `state`, and every second | Now playing: the cover, the title, the artists, and who added it: the guest itself, another participant by name, or, for a wave track, the host's own queue. A track that only the host can play shows by its title and artists, with no cover and no link ([protocol](protocol/README.md#tracks)). Its progress follows the room ([protocol](protocol/README.md#state)) and stands still while `paused`. Idle: nothing plays yet. The queue, in play order: the guest's own items marked and removable (GUEST-10), the others with their adder's name, pinned items marked, and the guest's count of waiting items against `maxPendingPerGuest`. The participants: name, kind and online state, the guest itself marked. A status says whether the guest is connected and whether the host is online. | no: #16 | yes |

## Connection and the end

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| GUEST-15 | Connected | The connection closes or dies | A status says that there is no connection. The guest reconnects with the reconnect backoff of [limits.md](limits.md#connections) and joins with the same `participantId`: it is the same participant, with its items (REC-13). On one connection, a `state` whose `version` is not greater than the last one is ignored; the first `state` after a reconnect is taken as it is. Adds that waited for a reply when the connection closed are forgotten: their replies never come, and the next state shows whether they landed. An add while not connected is not sent, and a status says so. | no: #16 | yes |
| GUEST-16 | A joined membership that is not ended | Its `join` gets `rejected{room-not-found}` or `rejected{rate-limited}` | The guest waits for the room (REC-14), since the host may raise it (REC-06). The room stays on screen with a status that the server lost the jam. The guest sends `join` again on the open connection with the guest backoff of [limits.md](limits.md#connections), for the room wait from the first `room-not-found`. Then it shows that the jam is not there and stops; the membership stays. The waiting `join`s count toward the IP's failed joins ([ROOM-58](room.md)), which party guests share; that is the price of a room the host may still raise. A connection that opens sends its own `join`, and `joined` ends the wait. A timed `join` is never sent while another `join` waits for its reply. A `rejected{not-allowed}` for a `join` means that the connection has already joined, and it ends nothing. A membership that was never joined (GUEST-03), or an ended one (GUEST-17), gets no wait: the guest shows at once that the jam is not there, or that it is over. | no: #16 | yes |
| GUEST-17 | A guest session | `ended` or `kicked` arrives, `join` gets `rejected{kicked}`, or `rejected{update-required}` arrives | The guest shows why: the host ended the jam, the host was away too long (`ended{expired}`), the host removed the guest, or the app needs an update. It does not reconnect (REC-15), and the session ends. The membership stays (GUEST-03). After a kick, opening the link again sends the same `participantId` and gets `rejected{kicked}` (ROOM-08), shown the same way. After `ended{expired}`, the host may raise the room from its snapshot (REC-06), and the guest comes back as the same participant, who may still remove its items. `ended{host-ended}` makes the membership ended: that room cannot come back (ROOM-52), so a later `room-not-found` for it is final at once (GUEST-16). | no: #16 | yes |
| GUEST-18 | A guest session, joining or in the room | The user leaves | The connection closes with 1000. The guest goes offline, and its items stay in the queue (ROOM-11). The membership stays: opening the link again joins at once as the same participant (GUEST-04, ROOM-05). | no: #16 | yes |
| GUEST-19 | A guest session was open when the app's process ended, without leaving and without an end | The app starts again, signed in | When the session's membership is younger than the membership life of [limits.md](limits.md#connections), the app opens the guest session and joins at once (GUEST-04). An older session is dropped. A room never outlives its membership: the maximum room age ([limits](limits.md#rooms)) is no longer than the membership life, and the room existed before the first join. | no: #16 | yes |

## Hosting

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| GUEST-20 | The app hosts a jam, or a stored host session waits for "Continue the jam?" (HOST-23); or the app has a guest session | A jam link is opened; or the user starts a jam | One app is never host and guest at once. A link to the room the app hosts opens the host's jam screen. Any other link is not joined: a status says that the user must end the jam first, and the host's jam screen opens. Starting a jam during a guest session is refused, with a status that the user must leave first. | no: #16 | yes |

## App Links

| ID | Given | When | Then | Desktop | Android |
|---|---|---|---|---|---|
| GUEST-21 | The jam server the app is built for serves `/.well-known/assetlinks.json` with the fingerprint of the app's release key (ROOM-62) | The user taps a link to that server in another app: a messenger, or the camera's QR code | The app declares `https://<that server>/j/*` as an App Link with automatic verification. Verified, the link opens the app at once, as in GUEST-01. Not verified, Android 12 and later open the browser, which is the web guest, and Android 8 to 11 offer a choice; the user may also allow the link in the system's "Open by default". Links to other servers are pasted (GUEST-01). An intent that Android replays when the app is reopened from Recents is not handled again, so a guest that left does not join again by itself. | n/a | yes |
