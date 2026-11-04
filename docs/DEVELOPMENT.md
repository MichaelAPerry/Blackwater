# Development and current behavior

Use the existing checkout. Do not create another emulator or a worktree for normal
cloud work. `platform/fm1-base` and its nested `bsp` are pinned submodules; update
revisions deliberately, not as a side effect of a build.

## Setup and tests

Requires a C99 compiler, Git, Python 3 with venv/pip, and package/Git network access.
From this repository root:

```sh
./tools/setup.sh
. build/venv/bin/activate
./tests/run_tests.sh
```

Setup initializes pinned submodules, installs Pillow 13.0.0 into `build/venv`, and
builds `firmware/build/host/blackwater_host` through fm1-base. Tests compile the
original DSP/generation engine with UBSan, then execute the same application's UI,
controls, render callback and storage through fm1-base's host simulator. They do not
substitute an independent sound generator. Outputs are under ignored `build/tests`:
PNG screens and actual rendered `audition.wav` / `fishing.wav`.

`./build.sh --host` builds the desktop simulator; plain `./build.sh` builds the device
package (see `docs/PACKAGE_EMULATION.md`). The upstream host takes a scenario and an
**existing** output directory:

```sh
mkdir -p build/manual
firmware/build/host/blackwater_host tests/simulator.txt build/manual
```

## Browser target

The existing fm1-base browser emulator compiles `firmware/app.c` through the same
host platform and runs its real DSP in a Wasm AudioWorklet. With Emscripten activated:

```sh
./build.sh --web
```

Output is `firmware/build/emu`. No replacement HTML/audio implementation is supplied.
This target is **not yet verified**: installation of official Emscripten 4.0.19 failed
at `storage.googleapis.com` with proxy HTTP 403. An additive environment network
configuration draft was saved, but did not change the live instance's access.
After the environment settings are applied, finish installing/activating Emscripten,
compile the target, then test its controls, display, audio and persistence in a browser.
Do not present these host screenshots as browser verification or a deployed demo.

## First application implementation

- Four tracks, one six-operator FM voice per track. Keys and incoming MIDI channels
  1-4 audition voices while stopped. Playing transport reserves voices for generation.
- Eight routing presets, individually editable operator ratio, level, feedback,
  fixed/ratio mode, fixed frequency, velocity sensitivity and ADSR. Custom forward
  modulation routes and carrier output levels are editable too. Self-feedback is
  per operator; arbitrary cyclic cross-operator routing is not supported.
- This is original FM synthesis, **not DX7 patch compatibility or the DX7's 32
  algorithms**. Sound parameters take effect fully on the next note; output/routing
  and some envelope settings can affect an already sounding voice.
- Initial sixteen-step patterns are constructed by fixed arithmetic formulas. Catches
  randomly mutate their pitches and gates using a repeatable PRNG. This is a basic
  generative prototype, not species-specific composition or developed musical phrasing. Current provisional defaults: a minor-flavored scale, 100 BPM, four
  contrasting patches/registers. These are initial implementation values, not an
  approved final musical identity or species assignment.
- Per-track hook placement/removal; automatic or timed catches. Editable wait bounds
  are 0-600 seconds, inclusive. Encoders use one-second increments. Equal bounds give
  a fixed wait; different bounds choose a random integer-second wait after each catch
  or missed bite. Timing-setting changes restart that track's wait.
- Catches/bites begin on sixteenth-note boundaries. Zero wait means the next boundary,
  and at most one encounter per boundary. Transport stop pauses fishing time.
- Timed mode currently offers a two-second landing window; REC lands a bite. A miss
  does not mutate the theme or count as a catch. The window is a provisional tuning
  value for user evaluation, not a final minigame design.
- Variation changes 1-16 motif steps per catch; selection may revisit a step. Changes
  affect pitches/rhythm, preserving track identity through its editable patch/register.
- Catch totals, last size and personal best per track. Current sizes are synthetic
  50-1000 mm values, awaiting species-specific ranges and preferred units.
- SAVE writes parameters, themes, RNG state and catch records in one versioned platform
  storage object. Paused/running state, pending bites and placed hooks are not resumed
  on load: startup is stopped with hooks retrieved. No autosave or slot browser yet.
- Flash writes remain outside the audio callback. The FM-1's flash-write audio pause
  still applies. A failed save leaves the session marked dirty and displays a message.

## Current controls

| Control | Behavior |
| --- | --- |
| ALGORITHM encoder | Select track 1-4 on every screen |
| HOME | River overview; REC places/retrieves selected track's hook |
| PLAY | Start/stop all generative tracks; start stopped, cast a hook to hear it |
| REC | Cast/retrieve selected hook; while a timed bite is active, land it |
| SEQ | Fishing screen: K1 minimum wait, K2 maximum wait, K3 AUTO/TIMED, K4 variation |
| EDIT | FM editor; SELECT chooses operator; PRESETS chooses one of three editor pages |
| EDIT page 1 | K1 ratio in hundredths (5-unit detents), K2 level, K3 feedback, K4 routing preset |
| EDIT page 2 | K1 fixed mode, K2 fixed Hz (10 Hz detents), K3 velocity sensitivity, K4 operator output |
| EDIT page 3 | K1 source operator, K2 lower-numbered destination, K3 modulation amount, K4 source output |
| ENV | SELECT chooses operator; K1 attack, K2 decay, K3 sustain, K4 release |
| GLO | K1 tempo 30-240, K2 selected track root MIDI note, K3 track volume |
| SEL button | Catch statistics for all four tracks |
| SELECT encoder outside EDIT/ENV | Navigate the six screens |
| HOME knobs | K1 selected track volume, K2 root MIDI note |
| OCT-/OCT+ | Audition keyboard octave, while stopped |
| SAVE | Save session and catch log; star indicates unsaved changes |
| FX / LFO / ARP | Explicitly unassigned pending approved features |

Choosing a routing preset replaces custom routes/output assignments; it does not
replace operator envelopes or frequency parameters. The current editor is functional
but provisional. Generic original pixel silhouettes now carry the approved Bluegill, Largemouth Bass,
Brown Trout and Catfish labels. Species-specific artwork has not been implemented.

## Validation and limits

The host scenario passes: 16,724 simulated milliseconds, real audio capture, operator
editing, automatic catches, timed landing and misses, stats/envelope page navigation,
save/reboot and failed-save handling. Engine tests pass with undefined-behavior
sanitization: all eight presets emit audio, release reaches silence, operator editing
changes samples, four-voice output stays in the 24-bit range, zero/600-second timers,
clock rollover, track isolation, and deterministic generation. Selected simulator
screenshots were visually inspected.

Engine state is 1,580 bytes; each track record is 192 bytes. The sine/note tables add
about 4.6 KiB; the 48-row RGB565 canvas is 23,040 bytes. These are source/host-derived
figures. The linked device build reports an 83,816-byte app image, RAM .data+.bss
31,028 of 98,304 bytes and a 23,304-byte pool (see `docs/PACKAGE_EMULATION.md`, which
also records the native, Node-Wasm and automated Chromium runs of that package). No
physical FM-1 verification, DSP deadline measurement on hardware, or deployment is
claimed, and nobody has yet played the browser build by hand.

Known upstream USB-mirroring failures from the audit remain unresolved; upstream
sources are unchanged. Species-specific art/sounds, species size distributions, the
Jeremy Wade audio asset, effects/modulation features, and polished game behavior
remain outstanding. No sample-player implementation is claimed.
