# BLACKWATER manual

An ambient musical river with occasional fishing encounters. This is the first river-experience review build;
its musical character and pacing are awaiting the owner's listening/play evaluation.

## Enter the river

Press **PLAY**. Four musical layers — current, bed, light, distance — make an evolving soundscape even with the
line lifted. One line begins in the water. **REC** lifts it; REC places it again. There is no casting minigame.
**K1–K4** adjust the layers' levels. **PRESET**, while stopped, explicitly loads **DUSK**, **UNDERTOW**, or
**FIREFLIES** into the current session's four patches; saved user patches are left alone. SAVE retains your work.

The river is drawn entirely with lines on an open canvas. New water detail fades between existing curves as
the sound grows. At the strike, the water settles and the rod draws in; rare fish also bring a traced, original
Jeremy-inspired portrait. The fight uses the same scene and focal point. A caught fish rises from that point
while the rod recedes, its measurements fade in, and the scene returns to just the river. No sprite cards appear.

## Fishing

- **SEQ** opens line settings. K1/K2 set minimum/maximum wait (0–600 seconds, five seconds per click), K3 selects
  FISH or LISTEN, K4 chooses bait; PRESET chooses depth. Wait settings are elapsed seconds, not secretly shortened
  by matching bait or weather. Defaults are 100–240 seconds, followed by a twelve-second approach.
- **ALGORITHM** selects a musical layer and its line-setting bank. Placing a line replaces any earlier waiting
  line. Every species can visit it; bait and depth affect which fish approaches, independently of the musical layer.
- In **FISH**, a bite gives eight seconds to press REC. A hook can fail. If it sticks, the 1.6-second strike gives
  an input grace period, then the fight begins. The speech clip still plays for rare fish only.
- On RIVER/FISHING, **SELECT or K1** reels; turn backwards to give line. REC reels a small amount on any page.
  Ease off when the fish runs. Too much tension snaps the line; excessive slack or lost distance loses the fish.
  The fight continues when you open an editor. PLAY pauses both the encounter and musical time.
- **LISTEN** uses an unattended encounter: approach, strike, a sixteen-second musical fight, then the catch reveal.
  It does not instantly award repeated catches. Both modes hold the reveal for eight seconds, return for thirty,
  and only then begin the next wait. The fastest settings still include that whole arc.
- A caught fish leaves a harmonic memory across the musical layers; catch records and the edited motif are saved.
  New sessions do not make abrupt personal-best key changes. Older v0.3 sessions, user patches and logs restore
  without a format reset. The new river scheduler interprets old motifs as slow gestures.

## The fish guide

HOME opens the river; HOME again opens CATCH LOG. Turn PRESET to page 3 for all eight fish: Bluegill / Crappie,
Largemouth Bass / Peacock Bass, Brown Trout / Brook Trout, Channel Catfish / Flathead Catfish. Each has its own
line drawing. Depth and bait influence encounters; musical layers do not restrict species.

## Sound editing

The FM patch editor, envelope, LFO, effects, live keys, MIDI, arpeggiator, and four user patch slots per
layer remain available. The old factory patches keep their names. Each layer is six-operator FM, sharing eight
voices with release tails and live playing. Filtered stereo noise adds water/air alongside the FM layers.

SEQ again opens the motif editor. Length, density, octave, mute, release/home and recording remain available;
these motifs unfold slowly in the river. GLOBAL controls musical tempo, MIDI sync, weather, clip, saves, and
output. Tempo affects delay and the arpeggiator; encounters and ambient gestures use elapsed time. Chord
progressions, if enabled, move once every 48 seconds.

Editor pages retain their navigation header and knob bar. SELECT navigates editor pages even during a fight;
reeling on those pages uses REC, so K1 keeps its displayed editor function.

Ratio and fixed-frequency edits now update held notes. EDIT cycles operator / frequency / routing / patches;
PRESET selects the operator on EDIT and ENV. The numbered routing diagram shows all six operators, with the
selected one outlined. ALGORITHM selects the musical layer. This remains the existing six-operator FM core,
with four editable layers and a shared eight-voice pool.

## Controls

| Control | Does |
| --- | --- |
| HOME | RIVER: K1-K4 layer levels, PRESET the sound world while stopped; again: CATCH LOG (all eight species; PRESET: misses, snaps, releases) |
| SEQ | FISHING; again: THEME |
| REC | answer a bite; reel in a fight; otherwise place or lift the selected line |
| PLAY | start / stop |
| SCL (SEL) | SCALE: K1 key, K2 scale (minor, dorian, major, mixolydian, harmonic minor, phrygian, minor pentatonic, blues), K3 chords (off, i–VI–III–VII, i–iv–v–i, i–VII–VI–VII, I–V–vi–IV, 12-bar), K4 swing |
| EDIT | FM EDIT, four pages (PRESETS): operator (ratio, level, feedback, algorithm), frequency (fixed, Hz, velocity, output), routing (from, into, amount, output), patch (choose, load, store) |
| ENV | ENVELOPE: attack, decay, sustain, release of the selected operator |
| LFO | LFO of this fish: rate (0.05–20 Hz), depth, target (pitch, FM index, volume, pan), shape (sine, triangle, square, random) |
| FX | EFFECTS, four pages: this fish's sends (reverb, echo, chorus, drive); reverb size, tone, level, echo time (1/16–1/4 dotted, at the tempo); echo feedback and level, chorus rate and depth; master tone and width |
| ARP | ARP page; again: off / on / latch. K1 mode (up, down, up-down, random, as played), K2 rate (1/4–1/32), K3 octaves, K4 gate |
| GLO | GLOBAL, three pages: tempo, clock (internal / MIDI), weather (auto: day, dusk, night, storm, three minutes each), best = key change; units (cm/kg or in/lb), fish-on clip and level, autosave; slots (load / save 1–4), MIDI out |
| SAVE | save this slot (with the catch log and settings) |
| ALGORITHM | choose the fish |
| SELECT | the pages (the icons along the top); in a fight: reel in |
| PRESETS | what the header says, top right: RIVER the sound world (stopped), FISHING the depth, SCALE the key, EDIT and ENVELOPE the operator, LFO the target, ARP the mode; elsewhere the page's sub-page (the page's button again does that too) |
| K1–K4 | the page's four values; editor pages and line settings show a knob bar |
| MASTER | volume |
| OCT- / OCT+ | the keyboard's octave |
| White keys | play the chosen fish in the scale (16 notes); with ARP on, arpeggiated |
| Black keys (hold) | FM macros on the chosen fish: BRIGHT, DARK, GROWL (feedback), METAL (inharmonic), DIVE (glide an octave down), RISE (up), STUTTER, HALF speed, CUT, OCTAVE up, THROW (into the reverb and echo) |

Lights: PLAY while playing, SAVE while unsaved, REC at a bite, ARP while on; the white keys walk the chosen fish's
steps.

**Key audit:** every button, encoder, knob and key now has a job; FX, LFO and ARP (unassigned before) are the effects,
the LFOs and the arpeggiator, and the eleven black keys are the FM macros. What the hardware offers that BLACKWATER
does not use yet: key velocity and pressure (fm1-base's key sensing), Bluetooth MIDI, and the second CPU core.

## MIDI

Out (USB): each fish on its channel (1–4), sequencer and keys; clock, start and stop while the clock is internal.
In: notes on channels 1–4 play those fish; with GLOBAL clock = MIDI, the arpeggiator follows clock pulses and transport follows start/stop.
The ambient gestures and fishing continue on elapsed time, including when no pulses arrive.

## Saving

Four session slots (song, the four fish, their themes and patches), your patches, the catch log and settings, each
in the FM-1's own storage. SAVE saves the slot; autosave does it ten seconds after the last change while stopped (a
flash write pauses the sound for a moment, so never while playing). The catch log is shared by all slots. Backup and
restore of all of it over USB is built in (fm1-base's backup tool). Sessions from the first prototype (format 1) are
not read: the river starts fresh.

## Limits

Up to eight voices at once (layers, release tails, live keys and the arpeggiator share them). This review build
has host-simulator and engine validation, including an uncompressed ten-minute listening-mode test. Device
cross-compilation and actual-package browser tests remain blocked by toolchain/SDK download access in this
cloud environment. Earlier 56% audio-load measurements belong to the previous build; they do not certify this one.
Physical FM-1 testing and human listening approval remain outstanding.

## Notice

BLACKWATER is an unofficial fan project, not affiliated with, sponsored or endorsed by Jeremy Wade, Fish On!, River
Monsters or Oxford Scientific Films. The FISH ON! portrait is an original pixel-art homage; the one-second sample is
a sound in an original musical work. Requests to change or remove either: an issue on the project's GitHub.
