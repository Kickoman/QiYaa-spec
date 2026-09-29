# Yandex Music API fixtures

Responses of the Yandex Music API as the server sends them, one file per case:
`<endpoint>/<case>.json` (for example `account-status/ok.json`). Real responses have personal
data scrubbed; error cases (`{error:{message}}`, `{error:"…"}`, empty body, 401) are included.

Each case has its parsed counterpart in [expected/yandex/](../../expected/yandex/README.md) under
the same `<case>` name.

Filled by Kickoman/QiYaa#3.
