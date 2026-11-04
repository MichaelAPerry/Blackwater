# Third-party software

BLACKWATER application files are original project code, marked GPL-3.0-or-later.
The repository's existing GPL version 3 license text is retained.

| Component | License and notices | Use |
| --- | --- | --- |
| fm1-base, pinned submodule | MPL-2.0; retain `platform/fm1-base/LICENSE`, `LICENSING.md`, `CREDITS.md` and `LICENSES/` | Platform API, graphics, audio/control/storage implementations, host/browser simulator, build tools, single-precision math |
| felucca-bsp, pinned nested submodule | MPL-2.0 and component notices in its repository | FM-1 board/chip support and update infrastructure |
| Barlow fonts from fm1-base | SIL OFL 1.1, `platform/fm1-base/assets/fonts/Barlow-OFL.txt` | Generated device and simulator text rendering |
| Pillow | HPND, installed development tool | Upstream font generation |

See upstream `LICENSING.md` for the complete component inventory and optional
features. BLACKWATER does not currently enable Bluetooth, WiFi, SDK radio libraries,
or the optional eSpeak voice. Firmware packaging resources are not included here.
No Ravel or official fm1_open application/engine code was imported.

No Jeremy Wade recording has been included. Its exact clip, source and distribution
permission remain asset prerequisites before bundling it in a public build.
