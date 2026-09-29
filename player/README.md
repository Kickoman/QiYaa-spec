# Player scenarios

Markdown tables, one file per area (sources, wave, play tracking, errors, transport). Each row is
one scenario:

| ID | Initial state | Action | Expected: API requests, events, queue state |
|---|---|---|---|

IDs are `<AREA>-<NN>` (`WAVE-03`) and are permanent (see the [top README](../README.md#rules-for-files)).
Where the current apps differ from a scenario, the row says so and links the issue that fixes it.

Filled by Kickoman/QiYaa#2.
