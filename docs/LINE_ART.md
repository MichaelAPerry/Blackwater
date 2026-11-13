# One river, one stroke system

User direction: OP-1-like line drawing throughout, with slicker composition and better flow. These are original
drawings and interface layouts, using sparse geometry, instrument accents and negative space.

## Visual rules

- A dark canvas with day/dusk/night lighting, chalk foreground, muted structural strokes and a flowing palette
  per sound world. Saturation grows with music and recedes during the encounter/recovery.
- River scenes use consistent 24-pixel margins, aligned labels and a quiet two-line footer that fades away.
- All illustrations are lines, curves and paths: river, rod, reel, fish, the Jeremy-inspired portrait, weather,
  navigation icons, operator diagrams, meters, keyboard and patch/session controls. No sprite fallback exists.
- Fixed-point antialiasing blends strokes into the current 240×48 LCD band. Water vertices retain subpixel
  precision, avoiding the jitter of whole-pixel curve motion. No extra full-screen framebuffer is allocated.
- Old PNGs and importer tests remain historical material. `fm1.toml` generates the clip and monoline font headers; the app
  does not include sprite tables or sample an image to draw the landscape.

## Atmosphere and colour

Curved banks, a second edge, land contours and grass make the ground legible. Shared shore geometry places all
water endpoints inside the banks; grass bends with the same wind state as the audio. The sun/clouds crossfade
to a crescent/stars as the scene's lighting and insect bed change. Wind responds to the selected weather. These
details yield to the strike/fight focus and do not draw random values from the encounter generator.

The flowing palette reads played pitch, modulation, FM feedback and layer energy from the real engine. A smooth
phase carries colour along each curve, with per-layer pitch offsets. Actual note attacks (including live keys)
and a faster energy response make the saturated colours move and open with the music. The musical bloom grows
geometry/saturation; strike/fight/recovery quieten the palette. Pausing freezes the field.

Wildlife geometry includes meteor trails, UFO dome/panels/lights, leaf veins and wind traces, bee head/antennae/
legs/wings and firefly wings/glow. Fish takeoff and landing ripples/droplets share the audio's splash timestamp.

Typography is an original monoline alphabet from `tools/make_line_font.py`, generated through the repository's
normal app-generator hook into the native font-header format. It uses no downloaded fonts. All six roles share
the same strokes, with tabular values and distinct lowercase notation for minor chords. The platform source and
font generator are untouched.

The fish guide pairs common and rare species. All eight have distinct outlines/markings, including peacock bars,
crappie dorsal spines, brook-trout vermiculation, and the flathead's square tail and broad head.

## Choreography

Actual FM note starts launch coloured crests upstream and briefly push the current carrying earlier crests.
They travel downstream towards the viewer, widen with the shore perspective, trail wakes and dissolve into the
foreground. Pitch shapes the bend/span; velocity and layer level shape strength. A close chord reinforces one
crest. Faint current moves between notes. The former anchored sixteen-curve/four-at-rest system is removed.
Bloom opens colour, curvature and wakes without moving the banks. PLAY freezes the field. The fixed pool holds
up to 24 crests; FREE and silent layers do not launch them.

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
the portrait region. `flow_simulator.txt` isolates one real keyboard note; `flow_visual_test.py` tracks its crest
downstream in actual LCD captures and checks pixel-identical paused water. `tools/render_flow_review.py` records
the motion with its actual FM audio. Recordings remain the evidence for feel.

The sound engines, species, saves, platform and existing emulator remain in place. Current device/browser
verification is blocked by unavailable compiler/SDK downloads; no deployment is part of this visual change.
