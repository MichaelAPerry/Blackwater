# Development

Use the existing checkout. Do not create another emulator or a worktree for normal cloud work. `platform/fm1-base`
(and its nested `bsp`) and `platform/fm1-emu` are pinned submodules; update revisions deliberately. What the firmware
does for a player: [MANUAL.md](MANUAL.md). The device build and the emulator/browser evidence:
[PACKAGE_EMULATION.md](PACKAGE_EMULATION.md).

## Setup and tests

Requires a C99 compiler, Git, Python 3 with venv/pip. From the repository root:

```sh
./tools/setup.sh
. build/venv/bin/activate
./tests/run_tests.sh
python3 tests/package_metadata_test.py
```

`tests/run_tests.sh` runs, all under UBSan where it is C:

- `tests/engine_test.c`: the engine alone — every factory patch sounds and releases to silence; the scale mapping;
  all four fish play on their MIDI channels with the clock out at 24 a beat; silence without a line in the water;
  automatic catches across all species with sizes and weights in range and rare fish (each playing the clip hook);
  themes change on catches, release and home restore them; personal bests move the key a fourth; the fight
  (patient reeling lands, cranking snaps, idling throws the hook); missed bites; live keys, a macro, ARP on and
  latch; the LFO; the effects' tail; external MIDI clock; everything at full stays inside 24 bits; determinism.
- `tests/art_test.py`: the art importer on a synthetic blurred sheet (never shipped): every game pixel exact.
- `tests/clip_test.c` and `tests/clip_simulator.txt`: the clip player against the reference decoder, and the app
  playing it for a rare fish (a synthetic tone, never shipped).
- `tests/simulator.txt`: the app on fm1-base's host simulator — editing, audition, the patch library, effects,
  scale, LFO, ARP, a black-key macro, automatic catches, the theme page and release, the catch log, a timed fight,
  save and reboot (song, patch, catch log restored), slots, units, a failed save, and the help page. Screens and
  rendered WAVs land in `build/tests`.

## Source map

| Path | What |
| --- | --- |
| `firmware/bw_engine.h`, `.c` | the engine: voices (8, six-operator FM), sequencer (96 PPQN, per-fish step rates, swing, chords), themes and their development, fishing, the fight, LFOs, FM macros, ARP, live keys, MIDI out and clock |
| `firmware/bw_species.c` | the eight fish (sizes, weights, depth and bait), the sixteen factory patches |
| `firmware/bw_fx.c` | the effects bus: chorus, tempo delay, reverb, master tone and width (BLACKWATER's own code) |
| `firmware/bw_clip.c` | the "fish on" clip player |
| `firmware/app.c`, `app.h` | controls, screens, saves (four slots, patches, catch log, settings), backup hooks |
| `firmware/fm1.toml` | build settings: identity `FM-1_77`, generators, backup |
| `tools/make_clip.py`, `tools/adpcm.py` | WAV → ADPCM header (encoder from MPFM1, MIT) |
| `tools/import_art.py` | art sheets → sprite header |
| `tools/get_sdk_files.sh` | the three hash-checked AC79 SDK files |
| `tools/build_emulator.sh`, `emulator_smoke.py`, `emulator_wasm_smoke.mjs`, `browser_smoke.mjs`, `build_simulator_site.py`, `package_metadata.mjs` | fm1-emu builds, package checks, the gated site |

## Design notes

- The engine is float (the AC79's single-precision FPU), no doubles; fastmath's exp2 is only called with
  non-negative exponents (`bw_exp2`), as it shifts a negative one.
- Per-operator constants are refreshed every 32 frames, not per sample; operators at level 0 never start. With all
  four fish and the effects, the firmware reports about 56% audio load at the chip's 240 MHz in fm1-emu.
- The effects' lines (about 150 KB) are in the pool; a stage whose input has been silent past its tail sleeps.
- The fight runs in the main loop (it reads the encoders) under the platform lock; catches from AUTO fishing happen
  in the render, on sixteenth-note boundaries.
- Saved objects are versioned (format 2). Object numbers 0 and 1 are kept from the first build.

## Not done

Physical FM-1 testing; a person playing and listening; the art (placeholders are drawn until the sheets arrive);
the clip itself; key velocity/pressure, Bluetooth MIDI and the second core are not used.
