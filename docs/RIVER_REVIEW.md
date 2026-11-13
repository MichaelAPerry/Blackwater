# River experience: first review build

Approved direction: minimal river → sound and psychedelic depth grow together → sharp clarity at the strike →
tense encounter → reveal → dissolve back to just the river. This is an implementation for listening/play review,
not a claim of finished musical or visual quality. All firmware artwork is now procedural line drawing. The
water, rod, reel, fish, original Jeremy-inspired portrait, icons and editor graphics share the same stroke
system. See [Line art](LINE_ART.md) for the design and transition rules. Raster sources remain archived but are
not generated or linked into the firmware.

## Listen and watch

```sh
cd /workspace/Blackwater
. build/venv/bin/activate
./build.sh --host
python3 tools/render_river_review.py --video --portrait --controls --out build/atmosphere-review
```

The tool drives fm1-base's actual host application at a 25 fps capture rate and produces:

- `build/atmosphere-review/{dusk,undertow,fireflies}/{name}.wav`: three thirty-second sound studies, with the line lifted.
- `build/atmosphere-review/river-encounter.mp4`: 110 seconds of actual LCD frames and rendered audio. Only the wait is
  shortened to thirty seconds via the ordinary line controls. Approach (12 s), strike (1.6 s), unattended fight
  (16 s), reveal (8 s) and return (30 s) retain their normal timing. A rare fish is not forced for this capture.

- `build/atmosphere-review/rare-strike.mp4` and `portrait.png`: a naturally occurring rare encounter, recorded after
  627.2 seconds of simulated play with normal thirty-second waits. This short clip includes the speech, portrait
  trace and handoff into the fight; no fish is forced by a test-only command.

- `build/atmosphere-review/controls/`: verified operator/frequency/routing/envelope/LFO/effects/patch screenshots,
  the actual FM control audio render, and the eight-species field guide. The control scenario is also in the test
  runner; the guide test compares the drawings without labels or colour to detect duplicated fish artwork.

The video is a local recording, not an interactive replacement emulator. Actual-package browser evaluation uses
the existing fm1-emu workflow once the required device compiler/SDK can be downloaded. No site has been deployed.

For the living river and keyboard revision:

```sh
python3 tools/render_living_review.py --out build/living-review
```

`living-river.mp4` joins normal-speed clips of a leaf, shooting star (with its actual FM motif), jumping fish,
firefly, bee and UFO. Waits between events are omitted from the recording; each event arises naturally through
normal panel settings. The UFO capture runs nineteen minutes of simulation before recording. `free.mp4` records
chromatic keys, held chords, SELECT choosing an operator and a live ratio edit. Individual clips, stills, WAVs,
input scenarios and logs are alongside them. Controls are documented in [the manual](MANUAL.md#full-keyboard-and-free-play).

The nature/detail revision adds banks/grass, stronger musical colour, event sound, crickets/cicadas and day/night
lighting. Record it with:

```sh
python3 tools/render_living_review.py --nature --out build/nature-review
```

Alongside the wildlife/free-play clips, this produces `daylight.mp4`, `night.mp4`, a forty-second
`day-to-night.mp4` (weather changed during actual play), and `musical-growth.mp4` (ordinary thirty-second line
wait, growth → strike → fight). `cicadas.wav` and `crickets.wav` isolate the nature bed using the ordinary mixer;
no substitute sound engine or post-render sound layers are used. AUTO still cycles day/dusk/night/storm every
three minutes; it is not sped up for the transition recording. Explicit WEATHER changes trigger that fade.

The downstream-flow revision replaces the anchored rows with actual note-triggered crests and moving current:

```sh
python3 tools/render_flow_review.py --out build/flow-review
```

`one-note.mp4` isolates one actual keyboard note using the normal mixer and follows its crest downstream until
it dissolves. `keyboard-current.mp4` plays keys over the other ambient layers; successive notes push the current
and leave overlapping wakes. `encounter-current.mp4` shows growth → strike → fight at the ordinary thirty-second
review wait. Each recording includes the actual rendered audio, input scenario and host log. No motion is added
after capture. The latest flow behaviour supersedes older clips with the fixed four-line minimal river.

## What changed

- One line, independent of four musical layers. All eight fish can approach; bait/depth influence the selection.
- Three FM sound worlds with slow envelopes, long gestures, drifting stereo, and a small filtered-noise source.
  Layer activity, FM depth and visuals share the same smooth bloom; fight pressure shapes the whole mix.
- Waiting uses elapsed seconds rather than tempo or MIDI-clock ticks. Defaults are 100–240 s. Matching bait
  does not secretly shorten the wait. Missing MIDI pulses do not freeze an encounter.
- A twelve-second approach and eight-second bite window precede the fight. Strike gets a grace period and
  smooth mix ducking; the supplied speech remains rare-only. Fight simulation advances on the render clock,
  independent of the selected UI page. PLAY pauses it.
- A catch influences subsequent harmony across layers and retains species/size/weight records. The reveal
  lasts eight seconds. Thirty seconds of recovery finish before the next wait begins. Zero wait still has the arc.
- Existing session, user-patch and catch-log layouts are preserved. The compatibility test restores a backup
  generated by unmodified v0.3, loads its saved patch, saves, and reboots. New-session defaults are not imposed
  on restored patches. World loading explicitly replaces current session patches, not the saved user library.

## Verification

```sh
PATH="$PWD/build/venv/bin:$PATH" sh tests/run_tests.sh
build/venv/bin/python tests/package_metadata_test.py
```

The runner includes original engine tests under UBSan, the new river suite, art import, reference clip decode,
synthetic speech playback, normal host controls/storage, manual encounter/off-page progression, and v0.3 restore.
It also exercises full-key performance, free play, operator selection, mode/key-map persistence, independently
scheduled wildlife, natural UFO rarity and actual star motifs. Live held keys take priority over background music.
The nature suite checks actual insect/event audio, day/night fades, both splashes, and pause/FREE silence;
host controls also check weather preview and restoration of saved lighting. Vector checks cover the banked scene.
The river suite renders a full ten minutes and checks encounter counts, no overlap, finite/unclipped audio,
elapsed-time independence, strike grace, pause, recovery, rare speech and harmonic memory. The long synthetic
clip scenario exercises unattended encounters and rare clip triggers rather than instant automatic catches.

## Outstanding evaluation

Device compilation, new image/RAM/pool budgets, worst-case load/xruns at 240 MHz, actual-package native and
Node-Wasm tests, Chromium interaction, and physical-device checks remain outstanding. In this instance,
`pkgman.jieliapp.com` and `gitee.com` return proxy 403; the alternative fm1-base Web build also fails because
`emcc` is absent. Earlier package metrics and browser evidence apply to the old build only.

The latest pass adds wind, stars, constellation strokes and reeds, travelling musical colour, and the original
monoline font. The first decision after listening is which sound world and degree of visual growth feel right. The next visual review should judge the movement and transitions in the recording, as well as still frames. Casting, additional locations, a larger synthesis
replacement and broad content expansion remain subsequent review decisions.
