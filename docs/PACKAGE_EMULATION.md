# Real-package emulator integration

## Current verified state

- Pinned existing fm1-emu at `9de1ef053a3c4bf56f16f05b5f0de193f4608a1d`
  in `platform/fm1-emu`; upstream sources remain unchanged.
- Built native CLI and Rust/Wasm browser emulator with stable Rust **1.99.1**.
  Rust 1.91.1 failed on upstream `if let` match guards; use the tested newer compiler.
  Upstream builds emit unused-assignment warnings; they are not warning-free.
- Emulator browser output is generated at `build/emulator-web`. This is emulator
  tooling, **not a working BLACKWATER demonstration**.
- Upstream Wasm smoke check with official stock V15 package: 132,352 audio frames
  over three simulated seconds, 55,823 lit pixels, halted=0. MIDI note RMS rose
  from 0.0000 to 0.0474. This checks the emulator baseline only.
- Native CLI baseline with stock V15 also completed: 1,200 simulated ms, 240x240
  screenshot with 57 colors, 45,760 audio frames. That short native run was near-silent
  (RMS 0.05), so it is a boot/display check, not a native sound success.
- BLACKWATER's desktop application/engine tests still pass. Five package guard tests
  pass using explicitly synthetic parser fixtures (not executable firmware).

## Blocking prerequisites

The official JieLi compiler download at `pkgman.jieliapp.com` returned HTTP 403;
read-only SDK Git access at `gitee.com` also returned proxy HTTP 403. The platform's
GitHub SDK URL returned repository-not-found; that is distinct from a missing user
GitHub token. No extra credential was requested.

Environment network draft additions for those two domains have been saved, preserving
`storage.googleapis.com`. They have **not** established runtime access. Review/save
in environment settings and publish the environment as instructed by the configuration
workflow; then retry the affected downloads. Preserve TLS, package hashes and the
SDK's pinned resource checks. Do not disable verification or fabricate SDK resources.

The real firmware cross-build, BLACKWATER package identity assignment/check,
BLACKWATER package boot, end-to-end site assembly, browser interaction, and physical
hardware remain outstanding. `build.sh` still guards the device target rather than
packaging with the placeholder identity. No BLACKWATER site is deployed.

## Reproduce emulator builds

The cloud instance retains Rust under `/workspace/.blackwater-tools`; the build
helper selects those paths if present. On another machine install current stable
Rust (tested with 1.99.1), including `wasm32-unknown-unknown`, before running:

```sh
./tools/build_emulator.sh
```

This builds only the existing upstream native and browser emulator targets, with
four compiler jobs by default. It does not initialize the optional unrelated SID
firmware submodule. `tools/emulator-toolchain.txt` selects stable Rust; the version
above records what was actually tested, not a promise that every future stable
compiler is validated.

## Package-driven checks and site assembly

Once a **real cross-build** has produced both files:

```sh
node tools/package_metadata.mjs firmware/build/blackwater.fwsc firmware/build/blackwater.bin
python3 tools/emulator_smoke.py firmware/build/blackwater.fwsc firmware/build/blackwater.bin
node tools/emulator_wasm_smoke.mjs build/emulator-web firmware/build/blackwater.fwsc firmware/build/blackwater.bin
python3 tools/build_simulator_site.py firmware/build/blackwater.fwsc firmware/build/blackwater.bin
```

These BLACKWATER-specific steps are prepared but **not executed successfully yet**,
so do not describe them as verified integration. Missing package inputs fail before
site output is produced.

- Package checking reuses established upstream parsers, validates UFW checksums,
  rejects template identities, requires a BLACKWATER identification string and exact
  application-byte equality with the intended build binary. A name alone is not
  treated as application verification; the emulator steps must execute it too.
- Native smoke uses a fresh evidence directory, boots the package, casts, starts
  playback, opens fishing and statistics, and checks actual audio and screen changes.
  It is a smoke check, not complete feature validation.
- Wasm smoke executes the actual device package using fm1-emu's existing exports and
  verifies audio/control/display responses. A Node Wasm run is not browser testing.
- Site assembly has no skip-test flag: both checks must pass. It creates a static
  wrapper around the existing emulator with a BLACKWATER-only firmware catalog,
  copies that same package, preserves relevant notices, and records hashes, source
  revision/dirty state and validation limits in `build.json`. Existing nonempty output
  directories are refused; choose another generated output directory when rebuilding.
- The resulting page still needs real browser interaction/audio/persistence testing
  before being called browser-verified. It must not be published as tested hardware.
- Deployment is a separate action. No automatic deployment workflow was added.

The browser emulator uses Rust/Wasm, so its build does not require the previously
blocked Emscripten installation for fm1-base's separate application simulator.

## Local checks of integration tooling

```sh
python3 tests/package_metadata_test.py
```

Tests reject a mismatched app binary, unrelated application, template identity and
corrupt package; one valid synthetic format fixture checks matching-binary parsing.
No synthetic package is used as the site demonstration or represented as firmware.
