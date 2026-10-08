# QiYaa specification

The behaviour that the QiYaa apps share, written down once. The desktop app
([Kickoman/QiYaa](https://github.com/Kickoman/QiYaa), C++/Qt) and the Android app
([Kickoman/QiYaa-android](https://github.com/Kickoman/QiYaa-android), Kotlin) share no code. They
share this repository: descriptions of how the player behaves and data files that the tests of
both apps read.

## Layout

| Path | What it holds | Filled by |
|---|---|---|
| [player/](player/README.md) | Player behaviour scenarios: state → action → expected result, each with an ID | QiYaa#2 |
| [fixtures/yandex/](fixtures/yandex/README.md) | Yandex Music API responses as the server sends them | QiYaa#3 |
| [expected/yandex/](expected/yandex/README.md) | What parsing each fixture must produce, in neutral JSON | QiYaa#3 |
| [dsp/](dsp/README.md) | Reference vectors for the equalizer and the spectrum, with tolerances | QiYaa#4 |
| [parity.md](parity.md) | Feature × platform table | QiYaa#7 |
| [jam/](jam/README.md) | The jam (a shared queue): protocol, limits, room and host scenarios | QiYaa-jam#3 to #5 |
| [telemetry/](telemetry/README.md) | Usage statistics and crash reports: the batch the apps post, the events, what is never sent | — |

## Where it lives

This repository is a git submodule mounted at `spec/` in both app repositories. Their CI checks
it out with `submodules: true`, and their tests read the files by path from `spec/`. Each app
pins the spec commit it implements, so one platform can move ahead of the other without breaking
the other's build.

We did not choose a monorepo (`desktop/`, `android/`, `spec/`) because it would merge two
histories, two CI setups and two release cycles that have nothing else in common.

## Changing the behaviour

1. Change the spec first: edit the scenario or data here and commit it to this repository.
2. In the app you are working on, bump the `spec/` submodule to that commit in the same change
   as the code and tests that implement it.
3. Open an issue in the other app's repository that links the spec commit. Record the difference
   in [parity.md](parity.md) until that issue is closed.

A test that relies on the spec names the scenario ID it checks (`WAVE-03`). When you change a
scenario, search both apps for its ID.

## Rules for files

- Scenario IDs are permanent. A scenario that is dropped keeps its ID, marked as removed, and the
  ID is never reused.
- Fixtures are real responses with personal data scrubbed (tokens, uids, logins, e-mails), or
  minimal hand-written ones where no real response exists.
- JSON is UTF-8 with 2-space indentation, one object per file.
- Text is in English.

`npm ci && npm run check` checks the spec's own files (the jam protocol examples against their
schemas). CI runs it on every push.
