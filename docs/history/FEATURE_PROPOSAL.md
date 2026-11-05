# Expanded feature set — approved 2026-11-04, implemented in version 2

Written 2026-11-04 at the user's request; the user approved all of it the same day ("all"), plus effects, rare fish
in each slot and FM manipulation on the keys (see `PROJECT.md`). What was built: `docs/MANUAL.md`. Each item notes what the FM-1 and fm1-base already
give us, so the scope stays realistic for the hardware (one PI32 core at audio rate, 240×240 screen, 4 encoders +
4 knobs, 14 buttons, 27 keys, USB/MIDI).

## The gap today

The engine and the plumbing work on the real package; the *experience* is thin: catches swap random steps, the
fishing is a timer, the four fish sound like four generic patches, and there's nothing to steer. A fully fledged
firmware needs (1) fish that sound like characters, (2) catches that develop music instead of scrambling it,
(3) fishing you actually play, and (4) the usual instrument basics: key/scale, sync, MIDI, saves.

## A. Music

| # | Feature | Sketch |
| --- | --- | --- |
| A1 | **Species roles** | Each fish a role in a band. **Bluegill**: bright plucked lead, high register, short busy phrases, pentatonic. **Largemouth Bass**: punchy bass line, low, syncopated, root/fifth/octave. **Brown Trout**: flowing arpeggio/pad, mid register, legato, triplet feel. **Catfish**: deep drone/sub, slow long notes, rare moves, "the monster underneath". Each gets 3–4 designed FM patches (still fully editable). |
| A2 | **Theme development** | Replace random step swaps with species "moves" applied to a home theme: transpose to a chord tone, invert, retrograde, displace rhythm, ornament, extend/shorten the phrase (4–32 steps), add/remove rests. Keep the home theme and a short history, so the music evolves but can come back. |
| A3 | **Size drives the change** | Small fish = small move (one note, one ornament); big fish = bigger (new bar, octave leap); a personal best = an event (key change, new section, the clip). |
| A4 | **Key, scale, chords** | Global key + scale (minor, dorian, major, pentatonic, blues, harmonic minor) and a slow chord progression all fish follow, so four evolving parts stay in harmony. |
| A5 | **Per-track shape** | Pattern length, swing, octave, mute/solo, step probability. |
| A6 | **Polyphony** | 2–3 voices for Trout/Bluegill (chords, arpeggio overlap). Needs a CPU budget measurement first. |

## B. Fishing

| # | Feature | Sketch |
| --- | --- | --- |
| B1 | **Casting** | Pick a lane and depth with a knob before REC; depth biases which fish/size turns up. |
| B2 | **Bait/lure per track** | Worm / crankbait / spinner / jig choose the *kind* of musical move (melodic, rhythmic, pitch, sparse). |
| B3 | **The fight** | TIMED mode becomes a short minigame: bite → reel with an encoder while a tension bar moves; too hard snaps the line (no catch, no change), too slow and it throws the hook. Bigger fish fight longer. |
| B4 | **Weather & time of day** | Dusk / night / storm change bite rates and the background art; could follow tempo or play time. |
| B5 | **Real sizes** | Species-realistic length ranges and weights (inches/pounds or cm/kg — your choice), rare "monster" catches, records per species. |
| B6 | **Keep or release** | Keep = the variation stays; release = the theme reverts. (Earlier proposal, still open.) |
| B7 | **Animation** | Float bob, nibble tilt, plunge, splash, landed fish, "FISH ON!" banner with the clip. Art prompts: `docs/ART_PROMPTS.md`. |

## C. Sound and performance

| # | Feature | Sketch |
| --- | --- | --- |
| C1 | **FX** (button now unassigned) | A small "river" reverb/delay with per-track send. |
| C2 | **LFO** (unassigned) | Slow "current" modulation of FM index/pitch per track. |
| C3 | **ARP** (unassigned) | Arpeggiate held keys through the scale on the selected fish's patch. |
| C4 | **Live play & record** | Keys play the selected fish in key; record a new home theme. |
| C5 | **MIDI** | Four tracks out on channels 1–4; MIDI clock in/out sync (fm1-base has MIDI in/out and USB MIDI). |
| C6 | **Patch library** | Per-species presets, store/recall. |

## D. Platform and release

| # | Feature | Sketch |
| --- | --- | --- |
| D1 | **Saves** | Several session slots, autosave, backup/restore to a computer (fm1-base `backup`). |
| D2 | **Manual** | fm1-base's manual page for the site and an on-device help screen. |
| D3 | **Attested releases** | Tag-built releases through the shared FM-1 builder, as MIKU-FM1/MPFM1 do, for the Firmware Hub. |
| D4 | **Hardware pass** | First flash and test on a real FM-1 (needs your go-ahead). |

## E. The "fish on" clip

The player is built (see `docs/DEVELOPMENT.md`): put a WAV at `firmware/assets/clip/fish_on.wav` and it plays when
a fish takes the hook. Open: when it plays (every bite / big fish only / personal bests only), an on/off and level
control (GLO K4 is free), and the clip's rights: audio from a TV programme belongs to its producers, and a clip
committed here is published with the source and the public site. Options: permission, a local-only build
(`BW_CLIP_WAV=... ./build.sh`, nothing committed), or your own recorded "Fish on!".

## Recommendation

A first "fully playable" release in this order: **A1 + A2 + A3** (the music is the product), **B3 + B5** (fishing
you play, sizes that mean something), **B7 + art import** once the sheets exist, **A4**, **C5**, **D1**, then the
clip controls. FX/LFO/ARP (C1–C3) and polyphony (A6) after a CPU budget measurement in the emulator and on hardware.
