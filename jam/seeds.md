# Wave seeds

When the jam queue is empty, the host plays the **jam wave**: a rotor wave started from seeds the
server picks from the jam's own tracks. The server keeps them in `state.room.fallback`. What the
host does with them is in [host.md](host.md#jam-wave). Format and terms are those of
[room.md](room.md).

## The rule

- **Candidates**: the jam's items that are waiting, current or recent. Tracks of the wave are
  never candidates.
- Sort the candidates by item number, highest first: the latest added first.
- Walk that list and take each track id not taken yet, until there are 5.
- `seeds` = `track:<id>` for each, in that order. No album id and no artists: the rotor ignores
  `track:<id>:<albumId>` and silently starts My Wave instead, and artists add nothing that 5 tracks
  do not already give (measured in Kickoman/QiYaa-jam#1).
- `seedsVersion` starts at 0 and grows by 1 when the **set** of seeds changes. The rotor sorts the
  seeds itself, so their order does not matter and a new order alone does not count as a change.
  `seedsVersion` is in the snapshot.

## Scenarios

| ID | Given | When | Then | Server |
|---|---|---|---|---|
| SEED-01 | — | A room is created | `seeds: []`, `seedsVersion: 0`. | no: #7 |
| SEED-02 | A new room | The first item, track A, is added | `seeds: ["track:A"]`, `seedsVersion: 1`. | no: #7 |
| SEED-03 | Items i1 … i7 with seven different tracks, some waiting, one current, some recent | — | The tracks of i7, i6, i5, i4, i3, in that order. | no: #7 |
| SEED-04 | Items i5 (track B), i4 (track A), i3 (track B), i2 (track C) | — | `track:B`, `track:A`, `track:C`: a track seen again is skipped, and the next distinct one fills the place. | no: #7 |
| SEED-05 | A waiting item is among the seeds | It starts (it becomes current), or it becomes recent | The set is the same, and `seedsVersion` does not change. | no: #7 |
| SEED-06 | A waiting item is among the seeds | It is removed | It is no longer a candidate. The next distinct track fills its place, and `seedsVersion` + 1. | no: #7 |
| SEED-07 | 5 seeds | An item with a new track is added | The new track comes first, the oldest one drops out, and `seedsVersion` + 1. | no: #7 |
| SEED-08 | 5 seeds | An item is added whose track is already a seed | The seeds are the same set, and `seedsVersion` does not change. | no: #7 |
| SEED-09 | A wave track plays | — | It is not a candidate, and the seeds do not change. | no: #7 |
| SEED-10 | A room raised from a snapshot | — | The seeds are computed from the snapshot's items. `seedsVersion` is the snapshot's, and grows only if that set differs from the snapshot's seeds. | no: #10 |

## Known limits of the jam wave

Measured in Kickoman/QiYaa-jam#1, with one request per case:

- Loading more (`/rotor/session/<id>/tracks`) drifts away from the seeds towards the host's
  account taste: the second batch had none of the seed artists. The host starts a new session
  whenever `seedsVersion` changes ([HOST-11](host.md#jam-wave)), which at a party happens often.
  Restarting the session every N wave tracks is not done in protocol 1.
- Seeds far from the host's taste pull less, and with mixed seeds the wave leans towards the
  majority. That is accepted for a party.
