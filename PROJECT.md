# BLACKWATER project record

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
- User prefers a Jeremy Wade sample. Exact clip and integration behavior are not yet
  specified and no audio asset has been supplied.
- Simulator is the primary target. Publish only after separate approval.

## Implemented prototype functionality — not a completed product

- Audited the original license-only repository and four requested reference projects;
  upstream template/Ravel desktop baselines recorded in `docs/TECHNICAL_AUDIT.md`.
- Pinned fm1-base and felucca-bsp as submodules; original upstream sources unchanged.
- Original BLACKWATER C application: four mono six-operator voices, eight routing
  presets, editable forward routes and carrier levels, ratios/fixed frequencies,
  per-operator feedback, velocity sensitivity and ADSR.
- Prototype hook toggle, transport, per-track automatic/timed timers, missed bites,
  zero-delay boundary safety, and catch counters/synthetic sizes.
- Generation is currently narrow: initial sixteen-step patterns use fixed arithmetic
  formulas; catches randomly replace pitches/gates in selected steps. This is not yet
  species-specific composition, phrase development, or a finished fishing minigame.
- Functional FM/envelope/fishing/global/statistics screens and a first river view.
- Versioned single-object session save/load using platform storage; error feedback.
- Pinned host setup, reproducible tests, UBSan engine tests and actual application
  simulator scenario. Real audio captured and selected screenshots inspected.
- No imported Ravel/official application engines, replacement simulator, or sample DSP.

## Provisional engineering defaults, not final product decisions

- 100 BPM, sixteen-step motifs with a minor-flavored scale, four contrasting starting
  patches/registers; one voice per track. No DX7 compatibility claim.
- Two-second manual landing window; waits rounded to sixteenth-note boundaries.
- Size display in millimetres; generic 50-1000 mm synthetic catches until fish identities
  and species-specific size distributions are chosen.
- Functional first control layout documented in `docs/DEVELOPMENT.md`; generic pixel
  silhouettes with the approved species labels. Species-specific artwork is not done.
- Saved sessions restart stopped, with hooks retrieved; explicit SAVE rather than
  autosave. Hardware flash-write interruption is not hidden.

## Proposed, awaiting approval or assets

- Musical character and generative behavior per species remain to be designed with
  the user. The current four generic patches are not approved species sound identities.
- Jeremy Wade clip: needs the actual selected asset, provenance/distribution clearance,
  and trigger behavior before a public bundle. No substitute voice has been added.
- Track locking, catch keep/release/undo, additional scale controls, effects, LFO
  modulation, and other performance features remain proposals. FX/LFO/ARP explicitly
  report that they are unassigned rather than pretending to work.
- Final minigame presentation, fishing animation, fish-size effects on music, exact
  species patches/art and final UI behavior still require user direction/evaluation.

## Current tasks and known limitations

- Browser toolchain blocked: official Emscripten download at `storage.googleapis.com`
  returns proxy HTTP 403. Domain addition saved in environment configuration draft;
  current-instance access still denied on retry. Browser build/interaction unverified.
- Desktop simulator tests pass. No device cross-build, linked memory report, device
  CPU deadline validation, physical FM-1 test, or deployment has been performed.
- Device packaging guarded pending a unique firmware identity and verified toolchain.
- Upstream platform suite audit had reproducible `mirror`/`mirror-viewer` failures
  (56 other named groups passed). They remain upstream issues of unconfirmed cause.
- See `docs/DEVELOPMENT.md` for exact current controls, tests, behavior and limitations.
- Next: resolve browser environment prerequisite, obtain the clip and musical-behavior decisions,
  evaluate the first playable behavior with the user, and implement approved changes.

## Communication correction

- User challenged the completeness claim. Earlier reporting overstated the prototype.
  Distinguish basic tested random motif mutation and timer/landing mechanics from the
  intended generative fish-music experience. Neither musical quality nor finished
  gameplay has been approved by the user.
