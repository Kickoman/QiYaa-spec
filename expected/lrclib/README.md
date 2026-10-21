# Expected LRCLIB results

For each fixture in [fixtures/lrclib/](../../fixtures/lrclib/README.md), what the app must make of
it, at the same path.

| Fixtures | Expected object |
|---|---|
| `search/*` with status 200 | `{"picks": [{"durationMs", "kind", "id"}]}`: for a track of `durationMs` (0 when unknown), the record the choice of [LYR-04](../../player/lyrics.md#lrclib) takes and what it gives. `kind` is `synced`, `plain`, `instrumental` or `missing`; `id` is the record's, `null` for `missing` |
| error cases (status ≥ 400) | `{"error": {"status", "message"}}`, as in [expected/yandex/](../yandex/README.md#shapes) |

A test runs the choice once per pick. For `synced` and `plain` the text is the record's
`syncedLyrics` or `plainLyrics`, parsed as in [lyrics/](../../lyrics/README.md).
