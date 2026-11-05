#!/usr/bin/env python3
"""Assemble a static site only after BLACKWATER's actual package passes emulation."""
import argparse
import hashlib
import html
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('package', type=Path)
p.add_argument('app', type=Path)
p.add_argument('--out', type=Path, default=root/'build/site')
a = p.parse_args()
package, app, out = a.package.resolve(), a.app.resolve(), a.out.resolve()
if out.exists() and any(out.iterdir()):
    p.error('output directory must be empty; choose a new generated directory')
metadata = json.loads(subprocess.check_output(
    ['node', str(root/'tools/package_metadata.mjs'), str(package), str(app)], text=True))
web = root/'build/emulator-web'
if not (web/'fm1_web.wasm').is_file():
    p.error('build the existing emulator first: ./tools/build_emulator.sh')
# No bypass flag: a rendered template or unrelated firmware cannot stand in for BLACKWATER.
subprocess.run(['python3', str(root/'tools/emulator_smoke.py'), str(package), str(app)], check=True)
wasm = json.loads(subprocess.check_output(
    ['node', str(root/'tools/emulator_wasm_smoke.mjs'), str(web), str(package), str(app)], text=True, timeout=300))
metadata.update(wasm)
metadata['source_commit'] = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
metadata['source_dirty'] = bool(subprocess.check_output(['git', '-C', str(root), 'status', '--porcelain'], text=True).strip())
metadata['emulator_commit'] = subprocess.check_output(
    ['git', '-C', str(root/'platform/fm1-emu'), 'rev-parse', 'HEAD'], text=True).strip()
metadata['browser_automated_chromium'] = False  # set below, once tools/browser_smoke.mjs passes on this site
metadata['browser_tested_by_a_person'] = False
metadata['physical_hardware_tested'] = False
out.mkdir(parents=True, exist_ok=True)
shutil.copytree(web, out/'emulator')
(out/'firmware').mkdir()
shutil.copy2(package, out/'firmware/blackwater.fwsc')
# Remove unrelated firmware choices from the generated page's catalogs.
catalog = {'note':'BLACKWATER development firmware; not physically verified.', 'default':'blackwater',
    'firmwares':[{'id':'blackwater','name':'BLACKWATER','version':metadata['source_commit'][:7],
        'license':'GPL-3.0-or-later','repo':'https://github.com/MichaelAPerry/Blackwater',
        'about':'Development prototype: four-track generative fishing FM.',
        'package':'../firmware/blackwater.fwsc'}]}
(out/'emulator/firmwares.json').write_text(json.dumps(catalog, indent=2)+'\n')
# The optional SMK-37 page is unrelated to this FM-1 application.
shutil.rmtree(out/'emulator/smk37')
# (the FM-1 page still asks for the SMK-37's catalog, to pair firmwares: an empty one)
(out/'emulator/smk37').mkdir()
(out/'emulator/smk37/firmwares.json').write_text('{"firmwares": []}\n')
(out/'licenses').mkdir()
for src,name in [('LICENSE','BLACKWATER-GPL.txt'),('platform/fm1-emu/LICENSE','fm1-emu-MIT.txt'),
                 ('platform/fm1-base/LICENSE','fm1-base-MPL.txt')]:
    shutil.copy2(root/src,out/'licenses'/name)
shutil.copy2(root/'THIRD_PARTY.md',out/'licenses/THIRD_PARTY.md')
for src,name in [('platform/fm1-base/LICENSING.md','fm1-base-LICENSING.md'),
                 ('platform/fm1-base/CREDITS.md','fm1-base-CREDITS.md'),
                 ('platform/fm1-base/bsp/LICENSE','felucca-bsp-MPL.txt'),
                 ('platform/fm1-base/bsp/NOTICE','felucca-bsp-NOTICE.txt'),
                 ('platform/fm1-base/assets/fonts/Barlow-OFL.txt','Barlow-OFL.txt')]:
    shutil.copy2(root/src,out/'licenses'/name)
shutil.copytree(root/'platform/fm1-base/LICENSES',out/'licenses/fm1-base-components')
shutil.copytree(root/'platform/fm1-base/bsp/LICENSES',out/'licenses/felucca-bsp-components')
(out/'build.json').write_text(json.dumps(metadata,indent=2)+'\n')
# the user's website art (site/): hero, link preview, tab icon, card, resized for the web
from PIL import Image
for src, name, size in [('hero.png', 'hero.png', (1600, 600)), ('og.png', 'og.png', (1280, 640)),
                        ('icon.png', 'icon.png', (64, 64)), ('card.png', 'card.png', (240, 240))]:
    if (root/'site'/src).is_file():
        Image.open(root/'site'/src).convert('RGBA').resize(size, Image.LANCZOS).save(out/name, optimize=True)
page = '''<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>BLACKWATER · development emulator</title>
<link rel="icon" href="icon.png"><meta property="og:title" content="BLACKWATER"><meta property="og:image" content="og.png">
<meta property="og:description" content="Generative fish music for the M-VAVE FM-1, running in your browser.">
<style>body{background:#060e0c;color:#ded6c2;font:16px system-ui;margin:24px auto;max-width:1100px;padding:0 16px}
a{color:#e8aa46}img.hero{width:100%;height:auto;image-rendering:pixelated;border:1px solid #1e322c}iframe{width:100%;height:850px;border:1px solid #34534b}code{overflow-wrap:anywhere}.notice{font-size:13px;color:#8a9a88}</style>
<img class="hero" src="hero.png" alt="A pixel-art river at night, a float on the water">
<h1>BLACKWATER</h1><p>Four fish. Four FM voices. One river. Development build.</p>
<p>This is the actual FM-1 firmware package running in the existing fm1-emu emulator.
It passed native, Node-Wasm and automated Chromium checks before this page was built.
It has not been tested on a physical FM-1.</p>
<p>Press Start, then R (REC) to cast a line and Space (PLAY): a fish is heard while its line is in the water.
, and . choose the fish (ALGORITHM); the arrow keys turn SELECT (pages, and reeling in a fight) and PRESETS.
The piano keys play the chosen fish in the scale; the upper (black) keys bend its FM while held.
7 HOME river / catch log, 0 SEQ fishing / theme, 2 SCALE, 5 EDIT, 3 ENV, 4 LFO, 1 FX, 9 ARP, 6 GLOBAL, 8 SAVE.
K1-K4 are the panel's knobs (drag them).</p>
<p>The emulator runs the FM-1's chip at its own 240 MHz: a slower computer may stutter (the page's clock menu).</p>
<p>The rare fish (crappie, peacock bass, brook trout, flathead) borrow their cousins' sprites until their own art is in; the fish-on clip plays only in builds that include one.</p>
<iframe src="emulator/?fw=blackwater&amp;mhz=240&amp;embed=1" title="BLACKWATER firmware in fm1-emu" allow="autoplay"></iframe>
<p><a href="emulator/?fw=blackwater&amp;mhz=240">Open the full emulator</a> · <a href="firmware/blackwater.fwsc">Download development package</a>
 · <a href="build.json">Build and validation details</a></p>
<p>Package SHA-256: <code>PACKAGE_HASH</code></p>
<p class="notice">BLACKWATER is an unofficial fan project, not affiliated with, sponsored or endorsed by Jeremy Wade,
Fish On!, River Monsters or Oxford Scientific Films. The FISH ON! portrait is an original pixel-art homage, not a
photograph or an official image of anyone, and the one-second "Fish on!" sample is used briefly, as a sound in an
original musical work. Rights holders who would like either changed or removed can open an issue on
<a href="https://github.com/MichaelAPerry/Blackwater/issues">the project's GitHub</a>, and it will be done.</p>
<footer>Firmware platform: <a href="https://github.com/charlesvestal/fm1-base">fm1-base</a> and
<a href="https://github.com/hugelton/felucca-bsp">felucca-bsp</a>.
Emulator: <a href="https://github.com/joakimeriksson/fm1-emu">fm1-emu by Joakim Eriksson</a>.
<a href="https://github.com/MichaelAPerry/Blackwater">BLACKWATER source</a> ·
<a href="licenses/THIRD_PARTY.md">Credits and licenses</a>.</footer></html>'''
(out/'index.html').write_text(page.replace('PACKAGE_HASH', html.escape(metadata['package_sha256'])))
assert hashlib.sha256((out/'firmware/blackwater.fwsc').read_bytes()).hexdigest() == metadata['package_sha256']
# Last gate: the assembled page itself, in Chromium. A failure leaves no site behind.
evidence = Path(tempfile.mkdtemp(prefix='browser-', dir=root/'build'))
try:
    subprocess.run(['node', str(root/'tools/browser_smoke.mjs'), str(out), str(evidence)], check=True, timeout=600)
except BaseException:
    shutil.rmtree(out)
    raise
metadata['browser_automated_chromium'] = True
metadata['browser_evidence'] = str(evidence.relative_to(root))
(out/'build.json').write_text(json.dumps(metadata,indent=2)+'\n')
print(out)
