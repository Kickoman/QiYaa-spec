# Expected parse results

For each fixture in [fixtures/yandex/](../../fixtures/yandex/README.md), the models that parsing
it must produce (`Track`, `PlaylistReference`, `WaveBatch`, …), written as neutral JSON: field
names from this spec, not from either app's classes. Each app's test converts its own models to
this form and compares.

Filled by Kickoman/QiYaa#3.
