# BLACKWATER project record

## River redesign approved 2026-11-12

The user approved the salvage/redesign plan and requested deeper, psychedelic art: a minimal river grows with
the sound, comes together in sharp clarity when the fish strikes, then dissolves back to the river. Ambient music
and tense fishing encounters replace the fixed four-fish band as the primary experience. Casting remains open;
the first implementation uses direct line placement. Retain the real firmware, platform and emulator.

Implemented for review: three sound worlds; shared elapsed-time encounter phases; one active line with weighted
selection across all eight species; slow FM gestures and filtered noise; a musical/visual growth envelope;
strike ducking and rare-only speech; clear fight/reveal screens; a thirty-second return before the next wait.
Existing v0.3 binary save layouts are unchanged, including restoration of saved user patches and catch records.
Spare preference bytes hold world and harmonic memory. The old motif/patch editor remains available; motifs
unfold slowly instead of as the old fish-specific rhythmic parts. The new composition and sound await the user's
listening/play judgment. No deployment is authorized by this implementation request.

Validation and review commands: `docs/RIVER_REVIEW.md`. Prior sections below record the earlier product and
historical build evidence; they do not certify the redesigned package. In this cloud instance the device toolchain
and SDK download domains still return proxy 403, and Emscripten is absent. Host validation is separate from actual
package/native/Wasm/browser/hardware validation.

## Approved requirements and user decisions

- The user is product owner, creative director and final decision-maker. Research
  does not authorize a roadmap. Use established FM-1 firmware/simulator infrastructure,
  genuine application audio/control/display/storage paths, and practical hardware
  compatibility. No replacement emulator or disconnected browser demo.
- Generative fish music maker: four North American freshwater species represent four
  tracks/instruments, each with a distinct sound and fully editable FM synthesis.
- Approved species: Bluegill, Largemouth Bass, Brown Trout and Catfish. The user
  selected Catfish without specifying a narrower species.
- Pixel art; user places hooks; SEQ opens per-track fishing; catches at intervals
  generate variation in each fish's musical theme. Record sizes in a stats screen.
- User approved the recommended fm1-base foundation and six-operator FM direction.
- Catch waiting times can range from zero seconds to minutes; implementation currently
  supports 0-600 seconds per track.
- User explicitly chose BOTH fishing modes: automatic catches and variable wait
  followed by a timed player action.
- Jeremy Wade "fish on" clip: the user will supply it (under 12 seconds, possibly 1 s) and takes responsibility
  for its use; it plays for **rare fish only**. The supplied clip is now in the repository (older note corrected 2026-11-12).
- 2026-11-04, the user approved **the whole expanded feature set** in `docs/history/FEATURE_PROPOSAL.md` (A1–A6, B1–B7,
  C1–C6, D1–D4, E), plus **effects**, an audit of the keys with FM manipulation on keys left blank, and **rare fish
  in each slot**: Crappie (bluegill slot), Peacock Bass (bass), Brook Trout (brown trout), Flathead Catfish
  (catfish; the common catfish is therefore the channel catfish). Art is being made by the user from
  `docs/ART_PROMPTS.md`. Effects should take MIKU-FM's screens as a reference for how they look, without copying
  MIKU's style: BLACKWATER has its own palette and bobber-on-a-line bars.
- A pull request to main waits until the firmware is "in a better state" (the user's call).
- The user said "Go" to a public GitHub Pages deployment (the workflow runs once the branch is merged).
- Simulator is the primary target. Publish only after separate approval.
- 2026-11-05, after playing 0.2 in the browser ("looks a lot better and sounds a lot better"): bites came too often
  to do anything and catch rates were far too high. 0.3: default waits about 1-5 minutes per line (bluegill
  45-150 s, bass 60-200, trout 50-170, catfish 90-300), wait knobs 5 s a click, strikes that spit the hook (20%
  for a small fish up to 65% for a big rare one), and reeling during a run doubling the strain.
- User-facing firmware evaluation should be possible through an existing browser
  firmware emulator site like MIKU-FM1, before physical-device testing. The next
  development milestone is a real BLACKWATER `.fwsc` loading in that emulator,
  not further expansion based only on desktop screenshots/WAVs.

## Implemented (version 2, 2026-11-04) — awaiting the user's evaluation

See `docs/MANUAL.md` for the player's view and `docs/DEVELOPMENT.md` for the source map and tests.

- Fish as band roles (lead, bass line, triplet arpeggios, drone) with per-fish step rates, registers, gate lengths,
  theme generators, and sixteen designed factory patches (four per fish) plus user patches.
- Key, eight scales, six chord progressions, swing; themes of 4–32 steps; catches develop the theme with
  species- and bait-weighted moves sized by the catch; rare fish start a new section; personal bests change key.
- Fishing: depth, bait, weather (auto cycle), species-realistic sizes and weights with trophies, eight species
  with rare chances, AUTO catches, TIMED bite → hook set → reeling fight with tension, runs, snaps and thrown hooks;
  keep/release, home theme, record from keys; a catch card; an eight-species catch log; cm/kg or in/lb.
- Eight-voice pool, pan, drive, sends to chorus / tempo delay / reverb, master tone and width; per-fish LFOs; eleven
  black-key FM macros; white keys play in the scale; arpeggiator with latch; MIDI out on channels 1–4 with clock,
  MIDI clock in; four save slots, autosave while stopped, backup/restore; help page; audio-load readout.
- Pixel-art importer for the user's sheets with drawn placeholders meanwhile; the clip player for rare fish.

## Provisional engineering choices (the user may change any)

- Tempo 96, A minor, i–VI–III–VII, swing 12 %; the two-second bite window; fight tuning; rare chance about 6 %
  (more at the rare fish's depth and bait, at night, in storms); weather three minutes each; patch designs and
  effect defaults; the help text; the reel on SELECT or K1.
- A fish is heard while its line is in the water (kept from the prototype).

## Current tasks and known limitations

- BLACKWATER's actual device package now cross-compiles with fm1-base (JieLi toolchain,
  the three pinned AC79 SDK files, hash-checked): `firmware/build/blackwater.fwsc`,
  identity `FM-1_7700` (prefix `FM-1_77`, a routine engineering choice). fm1-base's
  memory/ISR/FPU/register checks pass; a clean rebuild is byte-identical.
- That same package boots in the existing fm1-emu: native CLI, Node-Wasm, and automated
  headless Chromium on the assembled static site, including audio after cast + PLAY,
  screen changes for controls, and SAVE surviving a page reload. Evidence and limits:
  `docs/PACKAGE_EMULATION.md`.
- Version 2 (all approved features): engine, host-simulator and art/clip tests pass; the device package builds
  with every fm1-base check passing; in fm1-emu at the chip's 240 MHz, four fish playing with effects show 56%
  audio load and no xruns (the firmware's own readout). Evidence: `docs/PACKAGE_EMULATION.md`.
- The user's art is in (common fish, props, river, banner, site art); rare-fish sheets and portraits still wanted.
- Not yet done: a person playing and listening, physical FM-1 testing, the clip, the Pages
  deployment (merge to main + Pages source "GitHub Actions"; the workflow has not run on GitHub yet).
- The site emulates the chip at 240 MHz (the browser page's default is 100 MHz, too slow for four fish).
- Upstream platform suite audit had reproducible `mirror`/`mirror-viewer` failures
  (56 other named groups passed). They remain upstream issues of unconfirmed cause.
- See `docs/DEVELOPMENT.md` for exact current controls, tests, behavior and limitations.
- Next: the user plays version 2 and tunes it; the art sheets and the clip go in; then the pull request.
- MIKU-FM1 site-source audit identifies `joakimeriksson/fm1-emu` as its real-package
  emulator and Dinghy/Felucca as its firmware ancestry. fm1-emu's Rust/Wasm browser
  build is distinct from fm1-base's Emscripten application simulator. Retain the
  approved fm1-base foundation while verifying package compatibility; do not migrate
  to Dinghy just to copy the example site's appearance. See `docs/history/SIMULATOR_WORKFLOW.md`.

## Communication correction

- User challenged the completeness claim. Earlier reporting overstated the prototype.
  Distinguish basic tested random motif mutation and timer/landing mechanics from the
  intended generative fish-music experience. Neither musical quality nor finished
  gameplay has been approved by the user.
