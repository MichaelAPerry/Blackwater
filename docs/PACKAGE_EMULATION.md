# Real-package build and emulator integration

Recorded 2026-11-04 (Claude Code cloud session). Each level of evidence below is kept
separate: compilation, host simulation, native binary emulation, Node-Wasm, automated
Chromium, a person using the page, and physical hardware.

## Version 2 (all approved features, the user's art), 2026-11-04

- Device package: every fm1-base check passes; image 253,388 B (art included) of the 581,564 B slot; RAM
  .data+.bss 48,044 of 98,304 B; pool 169,304 of 344,064 B (the effects' lines); backup and restore built in.
- fm1-emu native, the chip at 240 MHz, all four fish cast and playing with the effects: the firmware's own readout
  shows **audio load 56%, xruns 0**.
- The site emulates 240 MHz (the page's own default, 100 MHz, is too slow for four fish). Headless Chromium in this
  cloud machine kept 1.0–1.4x real time at 240 MHz; one fish showed 12% audio load and 1 xrun in a minute. A slow
  computer or a phone may stutter: the page's clock menu trades that off.
- The gated site build passes: native and Node-Wasm smoke, Chromium (audio after REC + PLAY, screens change for
  REC/PLAY, SEQ, the catch log, ALGORITHM and SELECT; a catch saved survives a reload).
- The browser test no longer compares screenshots that the animated screens make differ on their own, and
  compares buffers without printing them (a failed comparison of two PNGs had run the machine out of memory).

## Summary (first package, version 1)

| Level | State |
| --- | --- |
| Device cross-build (PI32v2, fm1-base) | **Done.** `firmware/build/blackwater.fwsc`, identity `FM-1_7700` |
| fm1-base device checks (memory, ISR, FPU, registers) | **Pass** |
| Rebuild reproducibility | **Byte-identical** package on a clean rebuild |
| Host simulator + engine tests | **Pass** (unchanged from the handoff) |
| Package guard tests (synthetic fixtures) | **Pass** (5) |
| Package integrity vs. build binary | **Pass** (`tools/package_metadata.mjs`) |
| Native fm1-emu boot of the package | **Pass** (`tools/emulator_smoke.py`) |
| Node-Wasm fm1-emu boot of the package | **Pass** (`tools/emulator_wasm_smoke.mjs`) |
| Automated Chromium on the assembled site | **Pass** (`tools/browser_smoke.mjs`), incl. SAVE → reload |
| A person playing/listening in a browser | **Not done** |
| Public deployment | **Not done** (needs the user's authorization) |
| Physical FM-1 | **Not done** |

## Toolchain and SDK

- JieLi toolchain: `https://pkgman.jieliapp.com/s/linux-toolchain`, which redirected to
  `jieli-linux-toolchains-20260730.1.tar.xz`, SHA-256
  `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`
  (fm1-base's `tools/get_toolchain.sh` fetches the same URL). Used natively on Linux
  x86-64 via `JIELI_TOOLCHAIN`; no container.
- AC79 SDK: a non-Bluetooth, non-WiFi fm1-base build reads only three SDK files
  (`tools/fm1pkg_make.py`). Whole-repository git access to gitee reset the connection
  from this environment, but gitee's raw-file endpoint worked intermittently.
  `tools/get_sdk_files.sh` fetches those three files at the pinned SDK commit
  `d179b4484759423312073f5fbb232501aa491047` (tag `AC79NN_SDK_V1.2.1_2023-12-13`),
  with retries, and accepts each only if it matches fm1-base's `SDK_SHA256`:

  | File | SHA-256 (matches fm1-base) |
  | --- | --- |
  | `cpu/wl82/tools/uboot.boot` | `4e3b4c22…76ded4` |
  | `cpu/wl82/tools/cfg_tool.bin` | `27657995…7b010d1b` |
  | `cpu/wl82/tools/cfg/eq_cfg_hw.bin` | `41167491…ed80ea7d` |

  build.py printed no SDK-mismatch warning. Enabling Bluetooth or WiFi later would need
  a full SDK checkout (libraries and headers), as in MPFM1/MIKU's GitHub Actions builds.
- MIKU-FM1 and MPFM1 build their device packages in GitHub Actions (the same toolchain
  URL and a gitee clone), not in a cloud sandbox; that is the "known-good" setup.

## Identity

`firmware/fm1.toml` now has `id_prefix = "FM-1_77"`: dev packages `FM-1_7700`, releases
`FM-1_77XXYYZZ`. There is no central registry; the prefix was chosen not to collide with
those seen in fm1-base, fm1-emu and the sibling projects (0 placeholder, 11 template,
12, 39 MIKU-FM1, 56 MPFM1, 99, stock 015/016). It is a routine choice and can change.

## Build

```sh
platform/fm1-base/tools/get_toolchain.sh /opt/jieli    # or any directory
tools/get_sdk_files.sh build/ac79-sdk
. build/venv/bin/activate                              # Pillow with raqm (tools/setup.sh)
JIELI_TOOLCHAIN=/opt/jieli/toolchain AC79_SDK=build/ac79-sdk ./build.sh
```

`./build.sh` now builds the device package by default (it needs both variables);
`--host`, `--web` and `--release X.Y` pass through to fm1-base's `tools/build.py`.

fm1-base's report for the dev package:

```
ok    .ram_text: 975 insns, no calls
ok    no 64-bit multiply inlined beside a load
ok    no soft-double routines (single-precision FPU only)
ok    image 83816 B; RAM .data+.bss 31028 B of 98304; pool 23304 B of 344064
ok    register access: felucca-bsp only (device/, include/ and the app clean)
parts    USB audio in, USB audio out
app      firmware/build/blackwater.bin  83816 B
loader   firmware/build/loader/ota.bin  8340 B
package  firmware/build/blackwater.fwsc  611496 B, identity FM-1_7700
```

Package SHA-256 `30f3da8c6efb4e7cab6d641b268fccf84b70385bc0ebbff490772354ec6662ac`;
app (as carried in the package) SHA-256 `36a8b0f4…350d2be3d`, byte-equal to
`blackwater.bin` without its trailing 0xFF padding.

## Emulator

fm1-emu `9de1ef053a3c4bf56f16f05b5f0de193f4608a1d`, unmodified. Native CLI and the
browser Wasm both built with Rust **1.97.0** (the environment's stable) plus the
`wasm32-unknown-unknown` target. (The handoff recorded 1.99.1 working and 1.91.1
failing.)

### Native CLI (`tools/emulator_smoke.py`)

6 s emulated, keys REC 1.5 s, PLAY 1.8 s, SEQ 4.0 s: silent before PLAY (RMS 0),
then RMS ≈ 530–570, peak 2060 (16-bit) once a hook is cast and transport runs. 240×240
screens of 48–93 colours; river → hook placed → fishing screen as expected. 59,461
interrupts, 1,012 audio halves, no halt; cpu0 12.8 % in ISRs. Three unknown MMIO
addresses were reported (0x000100c0, 0x00040200) — an emulator bring-up list, not
investigated further. The native CLI runs about 25× slower than real time here
(≈145 s for 6 s), so the smoke script's timeout was raised from 300 to 900 s.

### Node-Wasm (`tools/emulator_wasm_smoke.mjs`)

Display drawn at boot; REC + PLAY → RMS 0.017 over 1 s (44,160 frames); SEQ and SEL
each change the screen; no halt. About 4 emulated seconds in 1.5 s wall time (JIT on).

### Automated Chromium (`tools/browser_smoke.mjs`)

Playwright's Chromium (headless) opens fm1-emu's own page from the assembled site,
presses its Start button and drives the firmware with the page's keyboard mapping.
Audio is measured with an AnalyserNode attached beside the page's own AudioWorklet →
speakers connection (it only listens). Screens are cut from the page's panel canvas.

- Status line: `BLACKWATER · 100 MHz · 2.3–3.1x real time · load 32–43 %`.
- Stopped: output RMS 0. REC (R) + PLAY (Space): RMS ≈ 0.026.
- SEQ (0) → fishing screen with the wait timer; SEL (2) → catch log.
- After ~40 s an automatic catch registered (Bluegill, count 1, 376 mm in one run).
- SAVE (8), the page's "save now", reload, Start: the catch log shows the same catch.
  A separate browser profile that never saved shows all zeros. Persistence passes.
- Encoders (fresh profile): ALGORITHM (.) selects track 2; PRESETS (↑) leaves the river
  screen unchanged (it acts only in EDIT, as designed); SELECT (→) moves to the next
  screen. Every screen above was also inspected by eye.
- One early run timed out waiting for the second browser's Start; the test now
  retries Start (reload, up to three tries). Later runs passed every time (4 of 4).
- Knob (pot) drags, the piano keys, EDIT/ENV/GLO editing and timed landing were not
  driven in the browser; the host scenario covers them in the desktop simulator only.
- Headless Chromium plays to no speakers: levels are measured, nobody listened.

## Site

`tools/build_simulator_site.py PACKAGE APP` refuses a nonempty output, runs the
package check, the native and Node-Wasm smoke tests, assembles `build/site/` (front
page, `emulator/` = fm1-emu's page with a BLACKWATER-only catalog, `firmware/`,
`licenses/`, `build.json`), and finally runs `tools/browser_smoke.mjs` on the assembled
site; on failure it deletes the site. `build.json` records hashes, source and emulator
commits, `source_dirty`, and which test levels passed (`browser_tested_by_a_person`
and `physical_hardware_tested` stay false). The front page with the embedded emulator
also loaded in Chromium without errors.

The site is static and needs only a web server (`python3 -m http.server -d build/site`).
`.github/workflows/pages.yml` (after MIKU-FM1's, authorized by the user) runs the same steps on GitHub:
host tests, toolchain, SDK (a full gitee clone as MIKU/MPFM1 do, else `tools/get_sdk_files.sh`), device
package, fm1-emu, then this builder with all its gates, and deploys `build/site`. It runs on pushes to `main`
and needs Settings → Pages → Source: GitHub Actions. Its steps were run locally; the workflow itself has not
run on GitHub yet.

## Fixes to the prepared tools

- `package_metadata.mjs`: upstream parsers decode the buffer they receive in place, so
  the script hashed modified bytes (`381daef…` instead of the file's `30f3da8…`), which
  made the site builder's final hash check fail. It now hashes the file as read and
  gives each parser its own copy.
- `emulator_smoke.py`: timeout 300 → 900 s (the native emulator's real speed here).
- `build_simulator_site.py`: browser gate added; page text matches what was verified;
  an empty SMK-37 catalog instead of a 404 on the page's optional twin lookup.
- `build.sh`: device target enabled now that the identity and toolchain are in place.

## Local checks of integration tooling

```sh
python3 tests/package_metadata_test.py
```

Tests reject a mismatched app binary, unrelated application, template identity and
corrupt package; one valid synthetic format fixture checks matching-binary parsing.
No synthetic package is used as the site demonstration or represented as firmware.
