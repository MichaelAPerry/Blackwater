# FM-1 technical audit

Audit date: 2026-11-03. Recommendation only; no framework adoption is approved.

## Local repository

Initial HEAD: `55ced919765acf904523f2527eb2b31a686cf09a` (`Initial commit`).
The checkout contained only `LICENSE` (GPL version 3 text), with no application,
dependencies, build scripts, simulator, tests, deployment configuration, or agent
instructions. The initial working tree was clean. No functioning firmware was replaced.

## Sources inspected

| Repository | Audited revision | Relevant source |
| --- | --- | --- |
| [fm1-base](https://github.com/charlesvestal/fm1-base/tree/2d8ed748cdade582c65d9fc8d8cf17da4ddadec8) | `2d8ed748cdade582c65d9fc8d8cf17da4ddadec8` | `include/plat.h`, `template/app.h`, `template/app.c`, `template/fm1.toml`, `host/plat_host.c`, `web/emu/emu_web.c`, `tools/build.py`, `device/app.ld`, `tools/fm1pkg_make.py`, `tests/run_tests.sh`, `LICENSING.md` |
| [Ravel_FM1](https://github.com/De-Pattern/Ravel_FM1/tree/16522bd3098e82a94440e8463535da7d62eb6127) | `16522bd3098e82a94440e8463535da7d62eb6127` | `firmware/app`, `firmware/fm1`, `host/build_host.sh`, `tests/scenarios/tour.rvl`, `web/emu/build.sh`, `README.md`, `THIRD_PARTY.md` |
| [fm1_open](https://github.com/m-vave/fm1_open/tree/4c7496daac04a95ff10d1afdcc8f9ead2dbabc41) | `4c7496daac04a95ff10d1afdcc8f9ead2dbabc41` | `app/instrument.h`, `sdk/include`, `sdk/manifest.json`, `sdk/Simulator.js`, `tools/Build-Web.ps1`, `README.md`, `LICENSE.md`, `THIRD_PARTY.md` |
| [awesome-fm-1](https://github.com/cicloid/awesome-fm-1/tree/c7cc615b9624f01387ab18ac018c67e66f32c9b1) | `c7cc615b9624f01387ab18ac018c67e66f32c9b1` | Ecosystem index; useful discovery material, not an executable platform or proof of capabilities |

fm1-base's pinned BSP is `hugelton/felucca-bsp` at
`773b0b99f5ade1a30d1879b2618c6c3c3e6ba031`. References were cloned under
`/tmp/blackwater-research`, outside the BLACKWATER checkout. Temporary checkouts are
research artifacts and must not be assumed to survive future tasks.

## Foundation comparison

| Option | Advantages | Costs and limits |
| --- | --- | --- |
| fm1-base + felucca-bsp | Purpose-built application/platform separation; existing scripted host and WebAssembly simulator execute the application's C code; device audio, panel, graphics, MIDI, storage, packaging, accessibility and installer infrastructure | Beta API/configuration may change: pin revisions. Emscripten needed for browser build. Device build needs JieLi PI32v2 compiler and verified SDK package resources. Upstream mirroring tests currently fail in this environment. |
| Ravel's Felucca-derived platform | Portable C application, scripted screenshots/WAVs, existing browser emulator, device build and packaging | A complete instrument with substantial synthesis/sequencing/UI behavior, not a neutral starting point. Extracting just the platform creates maintenance work; adopting the instrument needs product approval. GPL-3.0-only plus component notices. |
| Official fm1_open V14 | Vendor application, documented SDK APIs, sample/project tooling and existing C-to-Wasm preview | Device engine is a binary SDK with separate FM1-only terms. Documented builds require Windows x64/PowerShell and pinned JieLi Windows 2.5.3; web uses WASI SDK 27. No official build was executed in this Linux audit. |

Recommendation: adopt a pinned fm1-base submodule and its pinned BSP, with BLACKWATER's
own C application separated from platform files. Reuse its existing desktop simulator
and browser emulator unchanged where possible. Importing template behavior is not a
decision to include that behavior in BLACKWATER. Do not choose radio features or a DSP
engine until required by approved scope.

The existing BLACKWATER GPL text is unchanged. fm1-base/BSP use MPL-2.0 file-level
licensing, with third-party notices detailed in `LICENSING.md`. Its documentation
describes MPL/GPL compatibility. Proprietary radio libraries require a separate
licensing check before linking them into a GPL application; optional SDK-based radio
support is not automatically cleared for BLACKWATER. Ravel carries GPL-3.0-only and
third-party components. Official APP uses GPLv3 with an SDK linking exception; its
proprietary device engine has separate terms. Do not copy an upstream license over
BLACKWATER's license or assume all upstream files have one license.

## Actual application/platform contract

Verified in fm1-base sources:

- `app_init`, `app_ui_init`, `app_frame`, `app_input_only`, `app_render`, `app_panic`,
  and `app_say` form the template's application callbacks.
- `app_render(int32_t *lr, uint32_t n, uint32_t master_q12)` produces interleaved
  stereo 24-bit samples in 32-bit words through the device's real audio callback.
- Inputs use `plat_buttons`, `plat_keys`, `plat_enc`, `plat_master`; panel enums
  contain 14 buttons, seven encoder roles, and 27 keys. LEDs use platform functions.
- Drawing uses the existing graphics/canvas and LCD implementation; host output is
  a framebuffer, while the hardware implementation drives the device display.
- `plat_store_load/save` use platform storage objects; raw flash access is a separate
  bounded interface. Application storage allocation must preserve calibration and
  other reserved objects.
- `host/plat_host.c` compiles the application into the desktop simulator.
  `web/emu/emu_web.c` includes that host implementation. The browser AudioWorklet
  requests samples generated by the Wasm application and advances the simulated
  clock; JavaScript handles host audio/output and controls, not replacement DSP.
- Radio, USB, flash failure modes and device timing are partly simulated. This is
  shared application execution, not cycle-accurate hardware emulation.

## Hardware constraints relevant before feature selection

- JieLi AC791N / WL82, PI32v2 `r3` target; upstream device flags use single-precision
  FPU operations and disable fused multiply-add. Host builds also disable contraction.
- Display: 240 x 240. RGB565 framebuffer-equivalent storage is 115,200 bytes;
  platform strip/canvas drawing avoids requiring a full framebuffer on device.
- Default audio: 44,100 Hz, 256 frames per app block, about 5.80 ms. DSP must finish
  inside the hardware deadline; host CPU percentages cannot predict that deadline.
- Package app slot: `0x8DFBC` = 581,564 bytes (about 568 KiB). The linker's larger
  prepacking XIP allowance is not permission to exceed the final package slot.
- Default linker regions: 96 KiB ordinary RAM, 336 KiB `.pool`, separate RAM code,
  retained state and stack regions. These are not all freely interchangeable RAM.
  Platform, display buffers and optional features consume those budgets too.
- Upstream WiFi configuration changes the split to 128 KiB RAM / 304 KiB pool;
  WiFi and Bluetooth are mutually exclusive in this platform and carry large costs.
  No wireless capability is currently required by BLACKWATER.
- Storage objects have a 3,840-byte payload limit (`PLAT_STORE_MAX`); larger data
  requires an intentional layout or resource approach. Flash erase/write can stop
  audio briefly; do not put persistence writes in the real-time render callback.
- Device polyphony, DSP cost, sequencing resolution and sample capacity are not
  BLACKWATER requirements yet. Stock/Ravel voice counts are not hardware promises.

## Validation performed

Environment: Linux x86-64, GCC 14.2.0, Python 3.12.14, Pillow 13.0.0, Node 24.22.1.
No reference source files were edited; both reference working trees remained clean.

| Check | Observed outcome |
| --- | --- |
| fm1-base template host build | Passed, with two signedness warnings in `lib/sense.c` |
| Template `tests/basic.txt` | Passed; 13,186 simulated ms, 225 display blits; key/MIDI note handling, encoder/button state, save/reboot, simulated pressure/velocity and Bluetooth states, screen-reader text |
| Actual host-rendered audio | `note.wav`: 8,960 stereo frames, 44.1 kHz, 16-bit WAV, peak -26.3 dBFS; nonzero RMS. `master.wav`: 9,216 stereo frames with nonzero audio. WAV capture converts firmware output to the file format. |
| Display | Generated 240 x 240 screenshots; `controls.png` visually inspected and showed FX input, octave, waveform and encoder state |
| fm1-base full `tests/run_tests.sh` | Exit 1: 56 passing named test groups, 2 failing groups (`mirror`, `mirror-viewer`). This is not an all-pass platform certification. |
| Mirroring failure reproduction | Unmodified `mirror_test --out ...` independently returned 1: 200 screen rows differ. Viewer consumes that stream and reports display/control discrepancies. Root cause not established; do not label this a BLACKWATER bug. |
| Sanitizers | Runner's ASan/LeakSanitizer probe failed in sandbox; upstream runner fell back to UBSan. Bluetooth test groups passed under that fallback; ASan coverage is not claimed. |
| Update path | Skipped by upstream runner: no built template `.fwsc` package |
| `.rfm1` | Skipped: Emscripten missing (Node is present despite the runner's combined message) |
| Browser toolchain/build | Emscripten SDK clone succeeded; `emsdk install 4.0.19` failed downloading from `storage.googleapis.com` with proxy HTTP 403. No browser Wasm build or browser interaction test completed. Verification was not disabled. |
| Ravel desktop build/tour | Passed after creating the output directory; 52 generated PNGs, dimensions checked as 240 x 240. Initial tour invocation lacked an output directory and could not write screenshots; corrected run produced them. |
| Physical firmware cross-build | Not executed: JieLi toolchain and verified AC79 package resources not installed |
| Physical FM-1, BLACKWATER app, deployment | Not tested/not implemented/not deployed |

The baseline proves that established FM-1 application code can compile and execute in
the existing desktop simulator here. It does not establish that BLACKWATER exists or
that all upstream platform features pass.

## Reproduce the desktop research baseline

Use an existing research checkout when available. From a fresh temporary directory:

```sh
git clone https://github.com/charlesvestal/fm1-base.git fm1-base
cd fm1-base
git checkout 2d8ed748cdade582c65d9fc8d8cf17da4ddadec8
git submodule update --init --recursive
# Prerequisites: C compiler, Python 3 with Pillow. Node runs web-tool unit tests.
python3 tools/build.py template --host
template/build/host/template_host template/tests/basic.txt template/build/host
# Broader platform suite; known failures/skips are described above:
sh tests/run_tests.sh
```

Verified Ravel comparison, from its checkout at the audited revision:

```sh
sh host/build_host.sh
mkdir -p build/audit-tour
build/host/ravel_host tests/scenarios/tour.rvl build/audit-tour
```

Source-verified but **not executed successfully here**:

```sh
# fm1-base browser target, once Emscripten is installed and activated:
python3 tools/build.py template --web
# Output: template/build/emu/{index.html,app.wasm,worklet.js}
# Physical target, once JIELI_TOOLCHAIN and AC79_SDK are configured:
python3 tools/build.py template
```

`tools/build.py` verifies SHA-256 hashes of the three SDK packaging resources.
Preserve those checks. The repository provides `tools/get_toolchain.sh`; compiler
download uses `pkgman.jieliapp.com`. Browser SDK downloads need
`storage.googleapis.com`, currently denied. Do not bypass the environment proxy or
silently broaden network settings. No environment configuration changes were saved
during this audit; installation/startup automation should follow foundation approval.

## Decisions for the user

1. Approve fm1-base as the technical foundation, or prefer another investigated option.
2. Define BLACKWATER's first functional experience: what happens at startup, what the
   player does, and what sound/display response constitutes the first useful version.
3. State any day-one requirements for MIDI, persistence, wireless operation, or physical
   device delivery. These affect engineering budgets and validation, not assumed scope.

After foundation and first feature scope approval, implement the real application,
build the existing simulator with that application, resolve browser prerequisites,
and add tests for approved behavior. Choose an unused firmware identity before making
installable packages; do not reuse the template's identity or publish its demo as
BLACKWATER.
