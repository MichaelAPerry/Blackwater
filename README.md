<p align="center"><img src="site/hero.png" alt="A pixel-art river at night, a float on the water" width="100%"></p>

# BLACKWATER

**Generative fish music for the M-VAVE FM-1.** Four North American fish, four six-operator FM voices, one river.
Cast a line and that fish's part joins the music; catch it and its theme develops. The bigger the fish, the further
the music moves. Rare fish start a new section, and a personal best changes the key.

**Play it in your browser:** <https://michaelaperry.github.io/Blackwater/>. That page runs the actual firmware
package in [fm1-emu](https://github.com/joakimeriksson/fm1-emu), an emulator of the FM-1's chip, at its own 240 MHz.
**Install it on your FM-1** from the same page: fm1-base's web installer, in Chrome or Edge over USB
(<https://michaelaperry.github.io/Blackwater/install/>), with backup and restore of your sessions beside it.

| Slot | Fish (rare cousin) | Plays |
| --- | --- | --- |
| 1 | Bluegill (Crappie) | the lead |
| 2 | Largemouth Bass (Peacock Bass) | the bass line |
| 3 | Brown Trout (Brook Trout) | triplet arpeggios |
| 4 | Channel Catfish (Flathead Catfish) | the drone underneath |

- **Music:** key, eight scales, chord progressions and swing for all four fish; themes of 4 to 32 steps that
  catches develop according to the species and the bait; release a catch to undo it, keep a home theme, record
  from the keys.
- **Fishing:** depth, bait and weather (day, dusk, night, storm); species-true sizes and weights, with trophies;
  automatic catches, or a timed bite, a hook set and a reeling fight with line tension; a catch log covering all
  eight species, in cm/kg or in/lb.
- **Sound:** sixteen designed patches (four per fish) plus your own; per-fish drive, LFO, and sends to chorus, a
  tempo-synced echo and reverb; eight voices.
- **Play:** the white keys play the chosen fish in the scale; the black keys are held FM macros (bright, dark,
  growl, metal, dive, rise, stutter, half speed, cut, octave, throw); an arpeggiator with latch; MIDI out on
  channels 1 to 4 and clock in and out.
- **Keep:** four save slots, autosave, USB backup and restore.
- **FISH ON!:** when a rare fish takes the line.

Full controls: **[the manual](docs/MANUAL.md)**.

## Status

A development build. It runs in fm1-emu (natively, as WebAssembly, and in Chromium) and its tests pass; it has
**not yet been tested on a physical FM-1**. Package identity `FM-1_77xx`.

## Build

```sh
./tools/setup.sh                       # pinned submodules, a Python venv with Pillow, the desktop simulator
. build/venv/bin/activate
./tests/run_tests.sh                   # engine, art importer, clip, and the app on fm1-base's host simulator
platform/fm1-base/tools/get_toolchain.sh /opt/jieli
tools/get_sdk_files.sh build/ac79-sdk  # the three AC79 SDK files the package needs, checked against their hashes
JIELI_TOOLCHAIN=/opt/jieli/toolchain AC79_SDK=build/ac79-sdk ./build.sh   # firmware/build/blackwater.fwsc
```

Pushes to `main` build the package, check it in the emulator (native, WebAssembly, Chromium) and publish the
site (`.github/workflows/pages.yml`).

## Documents

- [Manual](docs/MANUAL.md): playing BLACKWATER
- [Development](docs/DEVELOPMENT.md): source map, tests, design notes
- [Package and emulator evidence](docs/PACKAGE_EMULATION.md): the device build and what was verified, and how
- [Art prompts](docs/ART_PROMPTS.md): how the pixel art was made, and what to make next
- [Rights](docs/RIGHTS.md): the clip and the portrait
- [Project record](PROJECT.md): decisions and status
- [History](docs/history/): the original audit, research and handoff notes

## Credits and licences

BLACKWATER's code is GPL-3.0-or-later ([LICENSE](LICENSE)). It is built on
[fm1-base](https://github.com/charlesvestal/fm1-base) and [felucca-bsp](https://github.com/hugelton/felucca-bsp)
(MPL-2.0) and runs in the browser through [fm1-emu](https://github.com/joakimeriksson/fm1-emu) (MIT) by
Joakim Eriksson. Pixel art was made by the project's owner with ChatGPT image generation. Details:
[THIRD_PARTY.md](THIRD_PARTY.md).

BLACKWATER is an unofficial fan project. It is not affiliated with, sponsored or endorsed by Jeremy Wade, Fish On!,
River Monsters or Oxford Scientific Films, and not by M-VAVE. The FISH ON! portrait is an original pixel-art homage;
the one-second "Fish on!" sample is a sound in an original musical work. Rights holders who would like either
changed or removed can [open an issue](https://github.com/MichaelAPerry/Blackwater/issues).
