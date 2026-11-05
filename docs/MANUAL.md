# BLACKWATER manual

Four fish, four FM voices, one river. Cast a line and that fish's part plays; catch it and its theme develops. The
bigger the fish, the further the theme moves. Rare fish start a new section, and personal bests change the key.

## The fish

| Slot | Fish (its rare cousin) | Role in the music | Register | Steps |
| --- | --- | --- | --- | --- |
| 1 | Bluegill (Crappie) | the lead: busy, plucked, pentatonic-leaning | C5 | sixteenths |
| 2 | Largemouth Bass (Peacock Bass) | the bass line: root, fifths, syncopation | C3 | sixteenths |
| 3 | Brown Trout (Brook Trout) | flowing triplet arpeggios over the chord | C4 | eighth triplets |
| 4 | Channel Catfish (Flathead Catfish) | the drone underneath: long held notes | C2 | eighths |

Each fish is a six-operator FM voice with four factory patches (EDIT page 4) and four of your own. All four follow
one key, scale and chord progression (SCALE), so their themes stay in harmony as they change.

## The screen

- **Top:** the twelve pages as icons, this one lit (SELECT turns them); the page's name and PLAYING / STOPPED
  with the weather; then the chosen fish (or the latest catch, or a message) and, on the right, **what PRESET does
  on this page**.
- **Bottom:** the knob bar: what **K1-K4** do here, their values and a line for each. On the river they are the
  four fish's levels.
- The version (0.2 and on) shows on the boot screen, GLOBAL and HELP.

## Fishing

1. All four lines start in the water: press **PLAY** and the whole river plays. **REC** on a fish whose line is in
   the water pulls it in (that fish goes quiet); REC again casts it.
2. The bobbers bob; a nibble shakes one; a **bite** pulls it under, the lane flashes and REC blinks. Press **REC**
   within three seconds: it answers whichever fish is biting and that fish becomes the chosen one.
3. The fight: **reel with SELECT, K1 or REC** (the arrow keys or R in the browser); you can change pages meanwhile,
   the fight goes on. Keep the line tension in the
   green and ease off while the fish runs: too tight snaps the line, too slack throws the hook. The rod bends with
   the tension.
4. A catch shows in the header for a few seconds (species, length, a new best) and its lane flashes; nothing covers
   the screen. Its theme has changed: hear it on the next pass.
5. On the **FISHING** page (SEQ): K1/K2 the shortest and longest wait (0–600 s), K3 TIMED or AUTO (AUTO lands fish by
   itself), K4 the bait; PRESET the depth. The fish's own depth, its bait and the weather shorten the wait; the rare
   fish have their own (a flathead likes the bottom and a jig).

A rare fish taking the line is the FISH ON! strike (1.6 s): the bobber yanked under, a flash and a shake, the
portrait slamming in, FISH and ON!, spray, then a wave washing it away; with the clip (GLOBAL page 2: clip on/off,
level).

## The theme (SEQ again)

A piano roll of the fish's current theme. Page 1: K1 length (4–32 steps), K2 density (notes skipped at random), K3
octave, K4 mute. Page 2 (PRESETS), turn right to act: K1 **release** the last catch (undo its change), K2 back to the
**home** theme, K3 make the current theme the home, K4 **record** from the white keys for one pass (left: stop).

What a catch does depends on the fish (the bluegill ornaments and leaps, the bass shifts rhythm and leaves space,
the trout inverts and reverses, the catfish moves slowly and deepens) and on the bait (worm melodic, crankbait
rhythmic, spinner pitch, jig structure). Small fish change one thing, big ones three, trophies four.

## Controls

| Control | Does |
| --- | --- |
| HOME | RIVER: K1-K4 each fish's level, PRESET the weather; again: CATCH LOG (all eight species; PRESET: misses, snaps, releases) |
| SEQ | FISHING; again: THEME |
| REC | set the hook on a biting fish (any lane); reel in a fight; otherwise pull the chosen fish's line in, or cast it |
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
| PRESETS | what the header says, top right: RIVER the weather, FISHING the depth, SCALE the key, EDIT and ENVELOPE the operator, LFO the target, ARP the mode; elsewhere the page's sub-page (the page's button again does that too) |
| K1–K4 | the page's four values: the bar along the bottom of every screen names them and shows them |
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
In: notes on channels 1–4 play those fish; with GLOBAL clock = MIDI, the river follows MIDI clock, start and stop.

## Saving

Four session slots (song, the four fish, their themes and patches), your patches, the catch log and settings, each
in the FM-1's own storage. SAVE saves the slot; autosave does it ten seconds after the last change while stopped (a
flash write pauses the sound for a moment, so never while playing). The catch log is shared by all slots. Backup and
restore of all of it over USB is built in (fm1-base's backup tool). Sessions from the first prototype (format 1) are
not read: the river starts fresh.

## Limits

Up to eight voices at once (the four parts, their release tails, live keys and the arpeggiator share them). With all
four fish playing and the effects on, the audio takes about 56% of the chip's time at 240 MHz (measured by the
firmware in fm1-emu; not yet on a real FM-1). Screens show graphs where they can: level meters on the river, the FM
routing diagram on EDIT, the envelope and LFO shapes, the theme's piano roll, a piano octave on SCALE, best-size
bars in the catch log.

## Notice

BLACKWATER is an unofficial fan project, not affiliated with, sponsored or endorsed by Jeremy Wade, Fish On!, River
Monsters or Oxford Scientific Films. The FISH ON! portrait is an original pixel-art homage; the one-second sample is
a sound in an original musical work. Requests to change or remove either: an issue on the project's GitHub.
