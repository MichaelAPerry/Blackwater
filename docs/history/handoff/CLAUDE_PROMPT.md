# BLACKWATER — continuation prompt for Claude

You are taking over BLACKWATER, my original generative fish music maker for the M-VAVE
FM-1. You have previously helped me complete MIKU-FM1 and MPFM1. Use those projects'
working firmware/emulator/build infrastructure where appropriate:

- https://github.com/MichaelAPerry/Blackwater
- https://github.com/MichaelAPerry/MIKU-FM1
- https://michaelaperry.github.io/MIKU-FM1/
- https://github.com/MichaelAPerry/MPFM1

Start from the handoff branch `codex/blackwater-claude-handoff` in BLACKWATER's GitHub
repository, not `main`. Read `docs/handoff/CLAUDE_HANDOFF.md`, then `AGENTS.md`,
`PROJECT.md`, and recent Git history. At the original handoff, GitHub main was still
the license-only initial commit. The user has now authorized publishing the existing
implementation on this separate branch. A Git bundle is an optional offline backup.
Preserve my code and history; audit rather than blindly replace it.

## Product direction I have approved

- Four North American freshwater fish represent four tracks/instruments: **Bluegill,
  Largemouth Bass, Brown Trout, and Catfish**. Do not silently narrow Catfish to a
  particular species or replace Brown Trout with Brook Trout.
- Each species has a distinct sound, with fully editable six-operator FM synthesis.
- Pixel-art aesthetic inspired by freshwater fishing, aquatic mysteries and Jeremy
  Wade/River Monsters. Exact species sound identities and final art are not settled.
- The user places hooks. SEQ opens per-track fishing gameplay. Catches at intervals
  cause generative variation in that fish's musical theme.
- Both modes: automatic catches after variable waits, and a variable wait followed
  by a timed player action to land the fish.
- Wait timing should range from zero seconds to minutes. Codex's current 0–600-second
  implementation is provisional, not a final maximum I selected.
- Fish sizes are recorded in a stats screen.
- I want a Jeremy Wade clip considered/included, but the actual clip has not been
  supplied. Leave the asset pending rather than inventing a substitute recording.

## Non-negotiable architecture and evaluation workflow

Use existing FM-1 firmware infrastructure, actual hardware-facing audio/display/
control/storage interfaces, and an existing emulator. I approved **fm1-base** as the
foundation. Do not switch to Dinghy or another major foundation without explaining
why and getting my approval. Reusing MIKU/MPFM1's build/site workflow does not require
copying their instruments or replacing BLACKWATER's foundation.

The next milestone is **BLACKWATER's actual compiled `.fwsc` package running in the
existing fm1-emu browser emulator**, like my MIKU-FM1 site. Develop the firmware and
its emulator evaluation together. I need to hear and interact with the real application
before physical FM-1 testing. Do not build a disconnected website instrument, another
emulator, JavaScript replacement DSP, or a reskinned unrelated firmware demo.

## Your first assignment

1. Recover and inspect the existing work; reproduce the useful host tests.
2. Reuse your known-good compiler/SDK setup from MIKU-FM1 or MPFM1 where compatible.
   Codex's download failures were its cloud proxy policy, not proof that those tools
   cannot be obtained in your environment. Preserve checksums, signatures and TLS.
3. Verify a project-specific firmware identity, finish a real FM-1 cross-build, inspect
   linker/memory checks, and fix genuine port/build issues without removing features
   merely to get a green build.
4. Boot that BLACKWATER package in existing fm1-emu. Exercise startup, controls,
   display, genuine audio and persistence. The checked-in package-specific smoke/site
   scripts have NOT yet passed with BLACKWATER; inspect and fix them as needed.
5. Prepare the browser site using the same package and established emulator. Test in
   a real browser. Keep compilation, host tests, native binary emulation, Node Wasm
   tests, browser interaction and physical testing clearly separate.
6. Present the working result and actual limitations. Prepare deployment for review;
   do not publish, flash hardware, or introduce new product features without authority.

Do not expand the musical/gameplay design before this real-package/browser baseline
works. Existing generation is just random pitch/rhythm changes in short patterns;
existing fishing is a hook toggle, timers and a button window. I challenged Codex's
claims of completeness. Treat this as a prototype requiring audit, not a finished
composition engine or fishing game.

## How to collaborate with me

I own the vision, features, musical behavior, interface direction and priorities.
You own routine engineering implementation, debugging, optimization and tests within
approved scope. Do not ask permission for every engineering step; do not invent a
roadmap or silently turn suggestions into requirements. For significant changes or
unclear musical/game behavior, present a focused choice and obtain my decision.

Suggested extras (effects, modulation, track locks, catch keep/release/undo, etc.) remain
proposals. Current defaults such as a minor-flavored scale, 100 BPM, a two-second landing
window, millimetres and uniform synthetic sizes are not final user-approved design.

Keep `PROJECT.md` current, preserve upstream licenses, commit meaningful working
changes, and report only verified results. Start with a short factual state check,
then proceed with the firmware-package/emulator milestone rather than another broad
planning or brainstorming round.
