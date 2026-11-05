# Third-party software

BLACKWATER application files are original project code, marked GPL-3.0-or-later.
The repository's existing GPL version 3 license text is retained.

| Component | License and notices | Use |
| --- | --- | --- |
| fm1-base, pinned submodule | MPL-2.0; retain `platform/fm1-base/LICENSE`, `LICENSING.md`, `CREDITS.md` and `LICENSES/` | Platform API, graphics, audio/control/storage implementations, host/browser simulator, build tools, single-precision math |
| felucca-bsp, pinned nested submodule | MPL-2.0 and component notices in its repository | FM-1 board/chip support and update infrastructure |
| Barlow fonts from fm1-base | SIL OFL 1.1, `platform/fm1-base/assets/fonts/Barlow-OFL.txt` | Generated device and simulator text rendering |
| fm1-emu, pinned submodule | MIT, `platform/fm1-emu/LICENSE` | Existing native/browser CPU and peripheral emulator; not replacement application DSP |
| Pillow | HPND, installed development tool | Upstream font generation, the art importer |
| `tools/adpcm.py` (from MPFM1) | MIT, Copyright (c) 2026 Michael Perry | The clip's IMA ADPCM encoder |
| Pixel art (`firmware/assets/art/`, `site/`) | Made by the project's owner with ChatGPT image generation; see `firmware/assets/art/NOTICE.md` | Sprites, backgrounds, the UI kit, the site's images |
| The "Fish on!" sample (`firmware/assets/clip/`) | Not covered by the GPL; see `firmware/assets/clip/README.md` and `docs/RIGHTS.md` | One second, played for rare fish |

See upstream `LICENSING.md` for the complete component inventory and optional features. BLACKWATER does not enable
Bluetooth, WiFi, SDK radio libraries or the optional eSpeak voice. The device package also carries three files of
JieLi's AC79 SDK that fm1-base's packager adds (fetched at build time, hash-checked; not in this repository).
No Ravel, official fm1_open or MIKU-FM application code was imported; MIKU-FM served only as a reference.

BLACKWATER is an unofficial fan project, not affiliated with, sponsored or endorsed by Jeremy Wade, Fish On!,
River Monsters, Oxford Scientific Films or M-VAVE.
