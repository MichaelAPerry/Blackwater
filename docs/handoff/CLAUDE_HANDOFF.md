# BLACKWATER technical handoff

Prepared 2026-11-04. This is a transfer of work, not a claim of completed firmware.
Use `CLAUDE_PROMPT.md` for the product/governance brief.

## GitHub handoff branch

Use branch **`codex/blackwater-claude-handoff`**, which the user authorized pushing
without modifying `main`:

```sh
git clone --branch codex/blackwater-claude-handoff https://github.com/MichaelAPerry/Blackwater.git
cd Blackwater
git submodule update --init platform/fm1-base platform/fm1-emu
git -C platform/fm1-base submodule update --init --recursive
```

The push result and remote commit are verified separately by the handing-off agent.
The following section records the earlier state and optional offline recovery path;
it is not a direction to ignore the published handoff branch.

## Original source-transfer state

Implementation HEAD before this documentation handoff:
`e15f61ece450bc1ffac52be3d6fc24f114bded90`, local branch **work**.

Read-only GitHub check at handoff:
`refs/heads/main = 55ced919765acf904523f2527eb2b31a686cf09a` (initial license only).
No remote `work` branch was returned at that original check. Implementation had not
been pushed then; use the dedicated handoff branch above after publication.
**Cloning GitHub main alone does not recover the implementation.**

The supplied `BLACKWATER.bundle` contains the local root repository history, source,
and these handoff documents. It intentionally excludes ignored build outputs, local
Python/Rust installations and submodule contents. Submodules are public pinned Git
references and can be fetched normally.

For a new checkout, with the bundle in the current directory:

```sh
git clone -b work BLACKWATER.bundle Blackwater
cd Blackwater
# A clone from a bundle initially sets origin to the local bundle path.
git remote set-url origin https://github.com/MichaelAPerry/Blackwater.git
git submodule update --init platform/fm1-base platform/fm1-emu
git -C platform/fm1-base submodule update --init --recursive
```

If a checkout already has local work, inspect/preserve it first. Fetch the bundle's
`work` branch into a separate local branch for review rather than resetting the
checkout. Do not blindly run hard resets or clean commands copied from other projects.

## Approved direction and remaining product decisions

Approved: four fish/tracks (Bluegill, Largemouth Bass, Brown Trout, Catfish), distinct
editable six-operator FM sounds, pixel art, hook placement, per-track SEQ fishing,
automatic and timed landing modes, waits from immediate to minutes, catch-driven
musical variation, and size statistics. Foundation approved: fm1-base. Browser-first
user evaluation via existing firmware emulation is the immediate priority.

Not settled: species-specific musical character, how catches should develop a theme,
final fishing interaction/animation, size-to-music mapping, units/realistic size ranges,
FM editing UX, polyphony, effects, modulation, locks, undo and detailed performance
controls. A Jeremy Wade clip is desired but missing. Do not invent its contents or
claim a sample player exists.

## Source map and dependencies

| Path | What actually exists |
| --- | --- |
| `firmware/app.c` | Application callback integration, control handling, strip-rendered UI, session storage and host assertions |
| `firmware/app.h` | fm1-base application contract, block size, canvas size and flash-object map |
| `firmware/bw_engine.c`, `.h` | Original six-operator DSP, four voices, motif mutation and fishing timers |
| `firmware/fm1.toml` | Build settings; **identity is still `FM-1_0` placeholder** |
| `build.sh` | Defaults to host; permits host/application-Wasm targets; deliberately refuses device packaging |
| `tools/setup.sh` | Pinned submodules, local Python venv/Pillow, host build |
| `tests/engine_test.c`, `tests/simulator.txt`, `tests/run_tests.sh` | DSP/generation UBSan checks and actual application host-simulator scenario |
| `tests/package_metadata_test.py` | Five format/identity/binary guard tests with explicitly synthetic parser fixtures |
| `tools/build_emulator.sh` | Builds existing fm1-emu native CLI and Rust/Wasm browser client |
| `tools/package_metadata.mjs` | Upstream CRC/parser reuse, identity checks and exact app-byte match |
| `tools/emulator_smoke.py` | Prepared native real-package smoke script; fresh evidence directories |
| `tools/emulator_wasm_smoke.mjs` | Prepared real-package Wasm controls/audio/display smoke script |
| `tools/build_simulator_site.py` | Prepared static wrapper/catalog around fm1-emu, gated on native/Wasm smoke checks |
| `PROJECT.md` | Approved decisions, provisional choices, proposals, status and caveats |
| `docs/DEVELOPMENT.md` | Existing controls and host workflow |
| `docs/TECHNICAL_AUDIT.md` | Original four-repository research and hardware constraints |
| `docs/SIMULATOR_WORKFLOW.md`, `docs/PACKAGE_EMULATION.md` | Browser/real-package workflow and exact evidence/blockers |
| `THIRD_PARTY.md` | Component notices; upstream license texts remain in submodules |

Pinned public dependencies:

- fm1-base: `2d8ed748cdade582c65d9fc8d8cf17da4ddadec8`
  — https://github.com/charlesvestal/fm1-base
- felucca-bsp (nested in fm1-base): `773b0b99f5ade1a30d1879b2618c6c3c3e6ba031`
  — https://github.com/hugelton/felucca-bsp
- fm1-emu: `9de1ef053a3c4bf56f16f05b5f0de193f4608a1d`
  — https://github.com/joakimeriksson/fm1-emu

Upstream source trees were unmodified at handoff. The optional SID submodule of
fm1-emu is not needed for this work; the setup script avoids fetching it.

## What the application currently does — narrowly

- Four monophonic voices, each with six sine operators; stereo output currently has
  the same signal on both channels. No DX7 SysEx/patch compatibility is implemented.
- Eight routing presets plus editable forward operator routes, carrier levels,
  operator ratio/fixed Hz, level, self-feedback, velocity sensitivity and ADSR.
  Arbitrary cyclic cross-operator routing is not supported. No effects or LFO engine.
- Initial sixteen-step motifs are constructed using fixed arithmetic formulas.
  On a catch, a PRNG chooses steps and replaces pitch/gate values. This is basic
  mutation, not developed species-specific composition or phrasing.
- Hook placement is currently a per-track on/off action, **not spatial river gameplay**.
  Timers choose whole seconds between editable bounds, currently 0–600. Encounters
  begin on sixteenth-note boundaries; zero cannot create an unbounded catch loop.
- AUTO counts a catch and changes the motif. TIMED opens a provisional two-second
  window; REC lands it. Missed bites do not mutate music or count as catches.
- Fish sizes are uniform synthetic 50–1000 mm values for every track. Count, best
  and last size persist. This is not species-specific fish biology or size design.
- Four generic pixel silhouettes, with approved fish labels. Species-specific art
  and sound design are not complete.
- SAVE writes a versioned session in one platform object, including parameters,
  motifs, PRNG state and catch records. Startup is stopped, hooks retrieved.
- FX/LFO/ARP explicitly report unassigned. Do not imply those controls have features.

The user challenged earlier overstatements. Treat all of the above as a functional
host prototype, not an approved final experience. Do not label the project complete.

## Verified results versus unrun work

**BLACKWATER host application:** engine tests pass under UBSan; existing simulator
scenario passes, with 16,724 simulated ms. It exercises real C audio, operator editing,
casting, auto catches, timed landing, missed bites, screens, failed saves and reboot.
Selected screenshots were visually inspected. WAV files contain actual rendered audio.
No aesthetic/musical acceptance or listening-quality result was established.

**Package guards:** five tests pass. Their fixtures are synthetic format test data,
not executable BLACKWATER firmware and not evidence of a device cross-build.

**fm1-emu builds:** native CLI and browser Wasm built with stable Rust **1.99.1** and
`wasm32-unknown-unknown`. Rust 1.91.1 failed on `if let` guards in pinned upstream
source. Upstream emits unused-assignment warnings. `tools/emulator-toolchain.txt`
currently says `stable`, not an exact compiler pin; improve reproducibility if needed.

**Official stock V15 emulator baseline — not BLACKWATER:** upstream Node Wasm smoke
produced 132,352 frames, 55,823 lit pixels, halted=0, MIDI RMS 0.0000 -> 0.0474.
Native CLI completed 1,200 simulated ms and a 240x240 screenshot with 57 colors;
its 45,760-frame audio was near-silent (RMS 0.05). Do not call that native audio a pass.

**Not done:** BLACKWATER device cross-build; valid project identity assignment/check;
linked memory report; BLACKWATER `.fwsc` boot in native fm1-emu; BLACKWATER Node-Wasm
package tests; successful end-to-end site assembly; real browser interaction/audio/
persistence tests; GitHub Pages deployment; physical FM-1 tests.

Earlier upstream fm1-base platform audit: 56 named test groups passed, `mirror` and
`mirror-viewer` failed reproducibly; cause unconfirmed. Update-path and `.rfm1` tests
were skipped for absent prerequisites. These are not BLACKWATER regressions proved
by the audit, nor a reason to discard the whole platform.

## Build/environment facts and blockers

Host test environment used GCC 14.2.0, Python 3.12.14, Pillow 13.0.0 and Node 24.22.1.
Rust tooling was installed outside Git under `/workspace/.blackwater-tools`. Ignored
`build/` and `firmware/build/` outputs, `/tmp/blackwater-research` checkouts/logs, and
that local tooling are not part of the transfer. Recreate them in your environment.

Codex could not access:

- `https://pkgman.jieliapp.com/s/linux-toolchain` — proxy HTTP 403.
- `https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git` — CONNECT/proxy HTTP 403.
- The platform's default GitHub SDK URL — repository-not-found, distinct from the
  working GitHub read proxy for the project/source repositories.
- `storage.googleapis.com` for Emscripten — previously denied, but Emscripten is
  **not required for the fm1-emu Rust/Wasm site path**.

The SDK revision expected by upstream is
`d179b4484759423312073f5fbb232501aa491047`, tag
`AC79NN_SDK_V1.2.1_2023-12-13`. Source build tooling contains expected SHA-256 hashes
for `uboot.boot`, `cfg_tool.bin`, and `cfg/eq_cfg_hw.bin`. Preserve them. No compiler
or verified SDK resources were installed. The official fm1_open repo's bundled
`uboot.boot` did not match the expected hash; it was not substituted.

The saved Codex environment draft contains `pkgman.jieliapp.com`, `gitee.com`, and
`storage.googleapis.com`, but a later retry still returned 403 and the running
policy lacked them. This is a Codex-instance configuration issue, not a portable
project dependency defect. Use compatible existing tools from your successful
MIKU/MPFM1 environment instead of repeating that setup conversation unnecessarily.
No new token or secret is known to be needed.

## Recommended next execution sequence

1. Recover the bundle and read current instructions. Inspect MIKU-FM1 and MPFM1's
   actual current code/build outputs rather than relying on memory. Codex inspected
   MIKU's site/build scripts; **MPFM1 was not audited by Codex**.
2. Reproduce host checks. Identify real cross-compiler and verified SDK availability
   in your environment, reusing known-good prerequisites where compatible.
3. Choose/check a BLACKWATER firmware identity. `FM-1_0` is a placeholder; do not reuse
   MIKU's `FM-1_39`, template identity, or Dinghy identity. Update/replace the guarded
   wrapper only after establishing the real device workflow.
4. Compile the actual BLACKWATER app with fm1-base. Investigate device-only failures,
   memory placement, ISR cost and imports. This code has never passed a cross-build;
   a working host build is not proof of PI32 compatibility.
5. Check package integrity and exact app contents; boot it in native fm1-emu. The
   native smoke script currently assumes startup/controls at fixed times and a
   300-second wall timeout. Inspect those assumptions against actual package behavior.
6. Boot that same package through browser emulator Wasm, then test in an actual
   browser. Node smoke is not real browser testing. Exercise save/reload as well
   as controls and sound; current package smoke scripts do not prove all persistence.
7. Assemble the site around the real package. Current assembler refuses a nonempty
   output directory. Its generated `build.json` records source dirty state; create
   reviewable reproducible artifacts rather than claiming an unrelated commit built
   them. Audit and fix the prepared scripts; none has passed with BLACKWATER yet.
8. Present the working prototype to the user before expanding composition/gameplay.
   Prepare deployment, but get publication authorization before making it public.

## Commands

Host workflow, tested in the previous environment:

```sh
./tools/setup.sh
. build/venv/bin/activate
./tests/run_tests.sh
python3 tests/package_metadata_test.py
```

Existing emulator build, after Rust setup:

```sh
./tools/build_emulator.sh
```

Prepared package integration commands — **not yet successful with BLACKWATER**:

```sh
node tools/package_metadata.mjs firmware/build/blackwater.fwsc firmware/build/blackwater.bin
python3 tools/emulator_smoke.py firmware/build/blackwater.fwsc firmware/build/blackwater.bin
node tools/emulator_wasm_smoke.mjs build/emulator-web firmware/build/blackwater.fwsc firmware/build/blackwater.bin
python3 tools/build_simulator_site.py firmware/build/blackwater.fwsc firmware/build/blackwater.bin
```

Useful MIKU reference files: `native/build.sh`, `tools/site_emu.sh`,
`tools/emu_smoke.sh`, `.github/workflows/pages.yml`, `docs/VALIDATION.md`.
Its platform is Dinghy and its site embeds existing fm1-emu with a real package.
Reuse compatible engineering infrastructure, not MIKU's product design/character art.

## Commit landmarks before handoff documentation

| Commit | Change |
| --- | --- |
| `55ced91` | Initial license-only repository; remote main at handoff |
| `dee9485` | Architecture audit/governance records |
| `ae17beb` | Approved generative-fishing direction |
| `ba8b5f9` | Initial host application and pinned fm1-base |
| `9dbc9a3` | Approved species names and correction of completeness claims |
| `d939039` | Actual-package/browser evaluation workflow research |
| `e15f61e` | Pinned fm1-emu builds and prepared package/site validation |

The bundle contains the original documentation handoff commit. A subsequent user
instruction authorized pushing the work to the dedicated GitHub handoff branch.
No public site deployment, hardware flashing or new product implementation is part
of that source transfer. Retain GPL/MPL/MIT/OFL notices; see `THIRD_PARTY.md` and upstream license files.
