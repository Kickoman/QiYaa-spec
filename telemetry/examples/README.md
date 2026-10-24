# Examples

One batch per file, exactly as the app posts it. `check` validates every file against
`schemas/batch.schema.json`: the others must pass, `invalid-*` must fail. Each invalid example is
listed here with the reason.

| File | Why it is invalid |
|---|---|
| `invalid-raw-machine-id.json` | `machine` is the raw OS machine id, not the app's 32-hex hash of it. |
| `invalid-track-title.json` | A `feature` value is a code or a built-in skin's name in Latin letters; a track's title does not fit. |
| `invalid-error-text.json` | An `error` has no free text: a message could carry a path or a name. |
| `invalid-skin-path.json` | `skin` is a path; a skin from a file is sent as `custom`. |
| `invalid-too-many-events.json` | 51 events: a batch holds at most 50. |
| `invalid-empty.json` | No events: a batch carries at least one. |
| `invalid-crash-frame.json` | A frame names the module's file name and an offset, `QiYaa+0x4f2a10`, not a path. |
| `invalid-android-raw-id.json` | `machine` is the raw `ANDROID_ID`, not the app's 32-hex hash of it. |
| `invalid-android-module-frame.json` | An Android frame is `<class>.<method>:<line>`; a desktop frame, `QiYaa+0x4f2a10`, does not fit. |
| `invalid-android-exception-message.json` | `exceptionType` is a class name only: the exception's message could carry a path or a title. |
| `invalid-lyrics-not-count.json` | The lyrics counts are counts; a track's title in one of them does not fit. |
| `invalid-android-desktop-start.json` | An Android batch carries the Android `start`; the desktop's fields (`qt`, `skin`, `milkdropBuilt`…) do not fit. |
| `invalid-android-lyrics-partial.json` | An Android `exit` with `lyricsLookups` carries all the lyrics counts, as the desktop `exit` does; one alone does not fit. |
