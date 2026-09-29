# DSP reference vectors

The numbers the equalizer and the spectrum must produce on both apps. Each file holds its
inputs as well as its results, so a test reads the inputs, computes, and compares within the
file's `tolerance`. The desktop app generates the files; the Android tests only read them.

## Files

### `eq-response.json`: equalizer frequency response

- `bandsHz` are the 10 band centres. `q` is the Q of every band, an RBJ-cookbook peaking
  biquad: `A = 10^(dB/40)`, `w0 = 2π·f/fs`, `alpha = sin(w0)/(2Q)`, and the coefficients are
  normalised by `a0` and **stored as 32-bit floats**.
- A band is left out (identity) when `|dB| < 0.05` or its centre is ≥ 0.49·fs. Band gains and
  preamp are clamped to ±12 dB. The preamp is a float gain `10^(preampDb/20)`. `enabled: false`
  gives 0 dB everywhere, preamp included.
- `frequenciesHz`: 31 points log-spaced from 20 Hz to 20 kHz (rounded to 0.01 Hz), plus the band
  centres, sorted.
- `cases[]`: `{name, sampleRate, settings{enabled, preampDb, bandsDb[10]}, responseDb[]}`, one
  `responseDb` per frequency. `responseDb` = `20·log10 |preamp · Π H_band(e^{jω})|`, evaluated in
  double from the float coefficients.
- The cases are: flat; the 17 built-in presets; all +12; all −12; alternating +12/−12; +12 at
  1 kHz only; preamp +6; preamp −12; disabled with all bands at +12. Each case is given at
  44 100 Hz and 48 000 Hz.
- `tolerance.db`: 1e-4.

### `eq-presets.json`: the built-in presets

- The 17 Winamp presets in menu order: `{name, levels[10], preampLevel, bandsDb[10], preampDb}`.
- `levels` are on the `.eqf` 1..64 scale. `bandsDb` = `levelToDb(level)`, unrounded (see below).
- `tolerance.db`: 1e-9.

### `eqf.json`: Winamp `.eqf` levels and bytes

- **Layout:** the file starts with `header` (31 bytes: `Winamp EQ library file v1.1`, 0x1A,
  `!--`). Then one record of `recordBytes` = 268 bytes per preset: a `nameBytes` = 257-byte
  NUL-padded name, 10 band bytes, and 1 preamp byte. A stored byte is `64 − level`.
- **`levelToDb`** (levels 0..65): the level is clamped to 1..64. Level 33 is exactly 0 dB;
  otherwise `(level − 1)/63·24 − 12`.
- **`dbToLevel`**: `level = clamp(round((dB + 12)/24·63 + 1), 1, 64)`, rounding halves away from
  zero; 0 dB is the tie 32.5 → 33. `byte = 64 − level`.
- **`parsedBytes`**: what reading a file gives for a stored byte. That is `levelToDb(64 − byte)`
  (so bytes above 63 clamp to −12 dB), rounded to 0.1 dB.
- `tolerance.db`: 1e-9.

### `spectrum.json`: the 19-bar spectrum

- **Input:** `fftSize` = 1024 mono samples, `x[i] = float(amplitude · sin(2π·hz·i/sampleRate))`,
  computed in double. `hz = 0` with amplitude 0 is silence.
- **Analyzer:** a symmetric Hann window `0.5 − 0.5·cos(2πi/(N−1))`, then a radix-2 FFT. Each bin
  `k = 0..N/2` is `20·log10(max(|X_k|·4/N, 1e-9))` dB, so a full-scale sine peaks near 0 dB.
- **`bands[sampleRate]`:** 19 log-spaced bands from 60 Hz to `min(16 000, fs/2)`: band b spans
  `60·r^(b/19)` to `60·r^((b+1)/19)`, with `r = high/60`. The bins are
  `firstBin = clamp(floor(lowHz/binHz), 1, N/2)` and
  `endBin = clamp(ceil(highHz/binHz), firstBin + 1, N/2 + 1)`, where `binHz = fs/N`.
- **`levels`:** for each band, the loudest bin in `[firstBin, endBin)`, at least −72 dB, mapped
  as `clamp((dB + 72)/66, 0, 1)`. This is one frame from a fresh visualizer. The bars' fall-off
  (0.07 per frame) and the peak markers come after this and are not part of the vectors.
- `tolerance.level`: 1e-3 on the 0..1 scale (0.066 dB). The two apps' FFTs differ in float
  rounding.

## Changing them

The desktop test `dsp_test` writes the files:

```bash
QIYAA_WRITE_DSP_VECTORS=1 QT_QPA_PLATFORM=offscreen build/tests/dsp_test
```

Then commit the files here, following the spec workflow in the [top README](../README.md). The
output is deterministic: running the command again on unchanged code gives the same files. A
change to the DSP that moves these numbers is a behaviour change. It needs an issue for the
Android app, which must reproduce the new values.
