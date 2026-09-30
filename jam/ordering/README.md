# Queue order

The order of `state.room.queue`, as reference cases that the server's tests read. The server
computes the order from the items on every change and never stores it.

## The rule

```
queue = pinned items, by pinnedAt, then by item number
      + the other items, by the room's mode
```

The **item number** is the number in `itemId` (`i10` → 10), compared as a number.

- **fifo**: by `addedAt`, then by item number.
- **round-robin**:
  1. Each person's items go by `addedAt`, then by item number.
  2. The people with waiting (unpinned) items go in this order: first those who have never been
     played, then by `lastServedAt`, oldest first. On a tie: the person whose first waiting item
     has the earliest `addedAt`, then the lower item number.
  3. Round 1 takes each person's first item in that order, round 2 their second, and so on. A
     person whose items have run out drops out of the later rounds.

`lastServedAt` of a person is the server time when an item they added last started
([ROOM-37](../room.md#what-plays)), pinned items included. Wave tracks do not change it. The host
is a person like any other. The current item is never in the queue.

Counting from who was played longest ago, not from who added first, puts a guest who arrives late
in the nearest round instead of behind an hour of other people's tracks.

## Files

One case per file, `<name>.json`:

```json
{
  "name": "A guest who arrives late plays in the nearest round …",
  "mode": "round-robin",
  "items": [{"itemId": "i1", "addedBy": "A", "addedAt": 1000, "pinnedAt": null}],
  "lastServedAt": {"A": 9000},
  "expected": ["i1"]
}
```

- `addedBy` is any label for a person. A person missing from `lastServedAt` has never been played.
- `pinnedAt` is `null` for an item that is not pinned.
- `expected` is the queue: every item exactly once, in play order.
- A test reads every `*.json` here, computes the order of `items`, and compares it with `expected`.
  It names the file in its name (`ordering/latecomer-joins-next-round`).

`mode-switch-fifo` and `mode-switch-round-robin` hold the same items: switching the mode reorders
the queue at once ([ROOM-47](../room.md#settings)).
