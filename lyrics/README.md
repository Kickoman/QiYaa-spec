# Lyrics reference vectors

How a lyrics text becomes the lines the lyrics view shows ([player/lyrics.md](../player/lyrics.md)).
Both apps read the same texts: the LRC of an `.lrc` file or of a service, the lyrics in a file's
tags, Yandex Music's plain text. Each case holds its input and its result, so a test parses the
input and compares.

## `lrc.json`

- `limits`: `maxBytes` (a text over it, in UTF-8, is not taken), `maxLines` (the lines kept, after
  the time tags are expanded), `maxLineCharacters` (a line is cut after this many characters,
  counted as UTF-16 code units, as both apps' strings count them; a surrogate pair that the cut
  would split is left out whole).
- `cases[]`: `{name, text, kind, lines}`.
  - `kind` is `synced`, `plain`, or `null` when the text gives nothing to show.
  - `lines[]` is `{ms, text}`, with `ms` `null` in a plain text.

## The rules

**Lines.** The text is split at `\r\n`, `\r` and `\n`, after a leading byte order mark is
dropped. Each line is trimmed of white space at both ends. What a line shows, after its time tags
and word tags are taken off and it is trimmed again, is cut to `maxLineCharacters`.

**Time tags.** A time tag is `[m:ss]`, `[m:ss.f]` or `[m:ss:f]`.
- `m` is one or more digits.
- `ss` is two digits, 00 to 59.
- `f` is 1 to 3 digits of a decimal fraction of a second (`.5` is 500 ms, `.25` 250 ms, `:50`
  500 ms). Further digits are dropped.

Only tags at the start of a line count. Several tags in a row give the same text at each time.

**ID tags.** A line that is only `[key:value]`, with `key` one of `ar`, `al`, `ti`, `au`, `by`,
`length`, `offset`, `re`, `tool`, `ve`, `la`, `id`, `#` in any case, is an ID tag and is never
shown. `[offset:±N]` shifts every time by −N ms, so a positive offset shows lines earlier. A time
below 0 becomes 0.

**Synced.** A text with at least one line that starts with a time tag is synced.
- Its lines are those with time tags; any other line is dropped.
- Word tags `<m:ss.f>` inside a line are removed, and the line is trimmed again.
- The lines are sorted by time, keeping the order of the text for equal times.
- A line with a time and no text is a pause; it stays, with empty text.

**Plain.** Any other text is plain.
- ID tag lines are dropped.
- Runs of empty lines become one empty line.
- Empty lines at the start and the end are dropped.
- Brackets that are not ID tags (`[Chorus]`) stay as they are.

**Nothing.** A text that leaves no line is `null`.

**Limits.** A text over `maxBytes` is `null` before it is parsed. After parsing, the first
`maxLines` lines are kept.
