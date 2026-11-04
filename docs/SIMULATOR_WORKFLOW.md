# Firmware and browser-emulator development loop

The user requests firmware that can be loaded on simulator sites like
https://michaelaperry.github.io/MIKU-FM1/ before physical testing.

## Verified source findings

Audited MIKU-FM1 at `e4f904e1eacb09b5c2f7d901cd8576d781c623ff`.
The live GitHub Pages URL returned HTTP 403 in this environment; findings below
come from its repository's page source and build scripts, not a browser session.

- [`site/index.html`](https://github.com/MichaelAPerry/MIKU-FM1/blob/e4f904e1eacb09b5c2f7d901cd8576d781c623ff/site/index.html)
  credits Dinghy, Felucca/Hügelton Instruments, and Joakim Eriksson's fm1-emu.
- [`tools/site_emu.sh`](https://github.com/MichaelAPerry/MIKU-FM1/blob/e4f904e1eacb09b5c2f7d901cd8576d781c623ff/tools/site_emu.sh)
  builds the existing fm1-emu browser client, supplies a one-firmware catalog, and
  points it at the actual Miku `.fwsc` package. It does not replace firmware DSP
  with browser-side application code.
- [`tools/emu_smoke.sh`](https://github.com/MichaelAPerry/MIKU-FM1/blob/e4f904e1eacb09b5c2f7d901cd8576d781c623ff/tools/emu_smoke.sh)
  boots the package, presses PLAY, and checks rendered audio and screen output.
- [Pages workflow](https://github.com/MichaelAPerry/MIKU-FM1/blob/e4f904e1eacb09b5c2f7d901cd8576d781c623ff/.github/workflows/pages.yml)
  builds/downloads the package, runs the emulator smoke check, assembles the
  emulator/site, then deploys. Useful infrastructure pattern, not BLACKWATER's
  current build or proof that BLACKWATER boots.

Additional source checkouts inspected:

- [fm1-emu](https://github.com/joakimeriksson/fm1-emu/tree/9de1ef053a3c4bf56f16f05b5f0de193f4608a1d),
  MIT. Existing PI32 CPU/peripheral emulator with desktop/headless and Rust-to-Wasm
  browser targets. Loads real `.fwsc` packages, exposes physical controls, and
  handles screen/audio/flash state. Browser builds need Rust and the
  `wasm32-unknown-unknown` target, not Emscripten. Hardware timing remains an
  emulation; passing it is not physical-FM-1 verification.
- [Dinghy](https://github.com/zvenson/dinghy/tree/f77bf03283bbb2c05294b01cdad68a30b410f93d),
  GPL-3.0-only. Minimal application framework descended from Felucca through
  SLOOP; provides device drivers, storage, update/recovery, host tests and packaging.
  MIKU uses it, but this does not require migrating BLACKWATER from approved fm1-base.

## Recommended loop

1. Keep BLACKWATER's application in the approved fm1-base architecture. Use the
   current desktop tests for fast internal feedback while developing.
2. Establish the actual device cross-build: verified JieLi compiler, matching SDK
   resources, an unused BLACKWATER package identity, and linked memory checks.
3. Load the resulting BLACKWATER `.fwsc` into existing fm1-emu. Verify boot, display,
   buttons/encoders, genuine audio, saves/reboot and fishing behavior. Investigate
   firmware faults or emulator incompatibilities; do not swap in a demo package.
4. Build/embed that same emulator with a BLACKWATER-only catalog and the same package
   in a static site. Verify browser interaction and audio, not just headless checks.
5. Make it available for user evaluation through the agreed publication process.
   Repeat the package-build/emulator/test/site loop for each approved feature.
6. Move to physical hardware testing only after simulator evaluation, while retaining
   the distinction between emulated behavior and measured hardware performance.

There is no separate website instrument to build first and then reverse-engineer
into firmware. A minimal booting application should reach the browser early; the
complete product need not be finished before that milestone.

No platform migration, new emulator, deployment or extra product features are
authorized by this research. MIKU's character artwork is not a bootstrap asset for
BLACKWATER. Preserve component licenses when reusing scripts or emulator assets.

## Current status

BLACKWATER has only been validated through fm1-base's desktop application simulator.
No BLACKWATER `.fwsc` has been cross-built or booted in fm1-emu, and no browser site
has been deployed. The research repositories were read outside the project checkout;
fm1-emu and Dinghy were not added as dependencies or built during this audit.
