# One river, one stroke system

User direction: OP-1-like line drawing throughout, with slicker composition and better flow. These are original
drawings and interface layouts, using sparse geometry, instrument accents and negative space.

## Visual rules

- A dark neutral canvas, chalk foreground, muted structural strokes and one restrained accent per sound world.
- River scenes use consistent 24-pixel margins, aligned labels and a quiet two-line footer that fades away.
- All illustrations are lines, curves and paths: river, rod, reel, fish, the Jeremy-inspired portrait, weather,
  navigation icons, operator diagrams, meters, keyboard and patch/session controls. No sprite fallback exists.
- Fixed-point antialiasing blends strokes into the current 240×48 LCD band. Water vertices retain subpixel
  precision, avoiding the jitter of whole-pixel curve motion. No extra full-screen framebuffer is allocated.
- Old PNGs and importer tests remain historical material. `fm1.toml` generates the clip header only; the app
  does not include sprite tables or sample an image to draw the landscape.

## Choreography

The sixteen water curves have fixed positions in a shared family. Four remain at rest; detail fades in between
them as the musical bloom opens. Crossing a complexity threshold does not redistribute the existing curves.

An approach gathers at the same central ripple that the line will meet. At the strike, the existing musical bloom
eases down, the background yields, and the rod traces in. The rare-strike portrait traces into open space; it has
no rectangular backing. On entering the fight the portrait fades and the title moves into the common heading
position. Tension and reeling cues ease in below it.

A catch rises from the last line position while the rod recedes. Species and measurements arrive after the fish
settles. The drawing and labels dissolve during the final 1.2 seconds of the reveal as the same water returns.
A loss leaves an expanding ripple. Recovery stays minimal instead of introducing another bloom.

The app targets 25 display updates per second. Recorded review clips use 25 fps by default; timing remains the
firmware's. This is host-verified choreography, not a measured device frame-rate or audio-load guarantee.

## Review and checks

Run the commands in [River review](RIVER_REVIEW.md). `vector_simulator.txt` captures actual quiet, growth,
strike, fight, reveal and return states. `vector_portrait.txt` reaches a naturally occurring rare strike through
ordinary controls. `vector_visual_test.py` checks sparse composition, visible growth, return to empty space and
the portrait region. The recording is the evidence for motion; screenshot tests alone do not certify its feel.

The sound engines, species, saves, platform and existing emulator remain in place. Current device/browser
verification is blocked by unavailable compiler/SDK downloads; no deployment is part of this visual change.
