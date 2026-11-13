# BLACKWATER project record

## Website, release 0.4 and main/Pages publication, approved 2026-11-12

The user explicitly authorized updating the website and README, then merging to main so Pages publishes the
installer for their own FM-1 test. This supersedes the earlier publication hold for these changes. The website
now presents the current flowing line-art river, nature/lighting, eight fish and full-key/Free performance, using
actual native-host screenshots and recording. `site/media.json` records their provenance; no physical-device
footage or replacement emulator is represented. The README reflects current controls and installation.

Pages release version is 0.4; the v0.3 session format remains unchanged. Review-branch/PR builds validate but do
not deploy; pushes to main build and publish automatically after all checks. The installer uses the exact build's
content-hashed package URL, retaining the platform's identity/CRC and transfer logic. Native/Wasm/Chromium gates
remain mandatory. Browser checks now exercise actual world/FM controls, an unattended encounter while off-page,
saved catch records and tempo, Free operator selection and a chromatic black key. Landing checks cover desktop/
mobile layout, real video playback, media and installer/package consistency. Physical FM-1 testing remains the
owner's next step; no hardware claim is made from host/browser checks.

2026-11-13: the first codex/** Pages run failed its Chromium gate because the browser check zeroed the waits
before selecting LISTEN; a zero wait starts an encounter at once and the firmware (correctly) locks the mode
mid-encounter, so the line stayed on FISH and only missed. The check now selects LISTEN first, matching
tests/simulator.txt. Firmware unchanged; native, Node-Wasm and Chromium site gates pass locally.

## Notes drive downstream flow, approved 2026-11-12

The user corrected the anchored water rows: notes should push flow down the river. This supersedes the older
fixed sixteen-curve/four-at-rest presentation. Each audible FM note start now launches a coloured crest upstream
and briefly strengthens the current carrying existing crests. The projected crests travel towards the viewer,
widen with the shore perspective, leave wakes and dissolve in the foreground. Pitch shapes their curvature/span;
velocity and layer level shape strength. A close chord reinforces one crest. Faint current moves between notes.
No surface row stays anchored; banks and ground remain stable. Musical bloom opens colour, bending and wakes,
then encounter/recovery settle those while notes keep moving. Keyboard, generative and shooting-star notes use
the same actual FM trigger; silent layers and FREE do not emit river crests. PLAY freezes all river movement.

The fixed pool of 24 runtime impulses and five-band projected geometry cache introduce no framebuffer, allocation,
audio change, encounter RNG or saved-layout change. The engine advances flow independently of the displayed page.
Tests cover downstream/widening motion, current acceleration, finite pool/expiry, chord reinforcement, mute/FREE,
and pause. Host image checks track one real keyboard crest from upstream to foreground and verify frozen pixels
on pause; visual growth/recovery checks now assess colour rather than a fixed count of water rows.
Review: `tools/render_flow_review.py` records isolated-note, keyboard/current and encounter clips through the actual
firmware and host audio. Device/browser/hardware verification retains the documented toolchain limitations.

## Riverbanks, reactive colour and nature sound, approved 2026-11-12

The user requested more wildlife detail and passing sounds, splashes/wind, more reactive psychedelic colour,
defined banks/ground/grass, crickets/cicadas and a daylight scene. Added these within the existing line-art
firmware and host simulator. The established AUTO weather order remains day → dusk → night → storm, three
minutes per stage; explicit weather is still selected on GLOBAL page 1 / K3.

Stable curved banks, a second water-edge stroke, land contours and wind-bent grass define the river. All sixteen
water curves end inside the shared shore geometry. More saturated flowing colours now follow actual played
pitches (including live keys and star motifs), note attacks and measured layer energy with faster attack/slower
release. Musical bloom still adds the detail; strike/fight/recovery reduce saturation and complexity for clarity.

The light state crossfades the background, sun/clouds, crescent/stars and insects. Stopped weather edits preview
immediately; changing weather during play fades smoothly. Cicadas rattle/swell in daylight, crickets call in
spaced groups at dusk/night, and both retreat in storms and during encounters. Procedural nature audio adds
stereo gust/rustle, takeoff/landing splashes, bee buzz, UFO hum, star air and firefly tones synchronized to its glow.
These small sources accompany the existing texture without using FM voices or encounter randomness. FREE has
no autonomous nature bed. Actor drawings add droplets/ripples, leaf veins, bee anatomy/wings and UFO detailing.

UBSan engine/host checks cover sound sources, day/night selection, fades, two splash timestamps, pause/FREE
silence, live-key preservation and unchanged fish outcomes. The existing save format, platform and emulator
remain unchanged. `tools/render_living_review.py --nature` records the actual firmware for review, including
day/night sound-only WAVs with the ordinary mixer turned down. Device/browser/hardware checks retain the
documented toolchain limitations; nothing is published or deployed.

## Living river and keyboard performance, approved 2026-11-12

The user requested shooting stars with their own motif, rare UFOs, jumping fish, windblown leaves, a bee and
a firefly; an unlockable full keyboard playing one fishing-mode voice; and free play with SELECT choosing an
operator. Implemented in the existing firmware/host, retaining the six-operator core and line-drawing style.

One occasional actor animates at a time, with space between events. Stars play a generated four-note motif
twice through an available FM layer, in the current harmony. Motifs share the actual voice pool and yield to
held keys. Wildlife uses an independent random stream, pauses with PLAY, and clears for approach/encounter/
recovery. Bees prefer daylight/dusk; fireflies prefer dusk/night (or the FIREFLIES world). The first UFO waits
15–20 minutes; later visits wait another 20–35 minutes. Jumping fish are visual wildlife, not catches.

GLOBAL page 4 selects RIVER/FREE, unlocks all 27 chromatic keys in RIVER, chooses the keyboard layer, and sets
octave. ALGO also chooses that layer. The unlocked layer yields its ambient part to the player; other layers
and fishing continue. FREE always uses all 27 keys and has no autonomous river music or fishing. On its home
screen, SELECT chooses one of six FM operators, ALGO chooses the complete voice, PRESET loads a factory/user
patch, and K1–K4 edit ratio/level/feedback/algorithm. Keys play the full voice; operator selection chooses the
operator being edited. Keys remember their original pitch/layer through octave and layer changes.

Runtime state and spare preference bits preserve v0.3 session/patch/log byte layouts and version. Mode, river
key unlock and keyboard layer survive SAVE/reboot. Host panel scenarios and UBSan engine tests cover these
behaviours, all six natural event types, star audio, independent fish outcomes and the full-key arpeggiator.
Review generation: `tools/render_living_review.py`; device/Wasm/browser/hardware checks retain the toolchain
limitations documented below. Local changes are not a publication or deployment.

## Atmosphere, typography, species and FM controls, 2026-11-12

The user liked the line-art direction and requested wind, stars, more shapes, music-linked colour movement and
replacement typography. They also asked whether all fish and the FM editing buttons/screens were present.

Added a sparse star field, constellation strokes, wind traces and reeds. Shared wind state opens the actual air
texture's filter; actual played pitches, modulation, feedback and measured layer energy steer a smoothed colour
field travelling along the water. It settles with the existing musical bloom and pauses with the music. Visual
randomness does not alter fish selection. The old Barlow face is replaced by an original monoline instrument
alphabet in all six font roles; lowercase notation remains distinct for minor/major chord labels.

All eight species retain their encounter data and now have individually distinct geometry/markings. The field
guide is CATCH LOG / PRESET page 3, common species beside rare cousins. FM screens use the same typeface and
number the six operators. Their original hardware mapping and shared six-operator core remain in place.
The audit found ratio/fixed-Hz changes did not update held voices; they now do without retriggering the envelope.
Existing v0.3 saved layouts remain unchanged. Tests exercise actual hardware inputs across operator, frequency,
routing, envelope, LFO, effects and patch pages, plus held-note changes and all eight guide drawings.

## Line-art direction and flow revision, approved 2026-11-12

The user requested a complete firmware art change: OP-1-like line drawing for the river, water, rod, fish,
Jeremy-inspired portrait and all other surfaces, replacing sprites. Subsequent feedback asked for a slicker,
more coherent result with better flow between elements. This supersedes the earlier pixel-art presentation.

The firmware now draws original geometry directly into the platform canvas. Raster art generation and sprite
rendering have been removed from the application; original source images remain archived. A fixed sixteen-curve
water family fades detail in without repositioning existing curves. Subpixel antialiasing, eased ink/trace,
consistent 24-pixel scene margins, a shared focal point and a 25 fps target support continuous transitions.
The rod/portrait draw in; the fish rises from the line while the rod recedes; labels arrive afterward and fade
back to water, without a cutout catch card. The recovery no longer grows a second visual/musical bloom.

The portrait remains an original line interpretation, displayed on rare strikes alongside the supplied speech.
Tests cover actual panel-driven rare encounters, visual density/return, host controls, audio and v0.3 persistence.
Device/browser/hardware performance remains unverified under the existing toolchain download blocker. No
publication or deployment has been performed. Listening and visual approval remain the owner's decision.

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
