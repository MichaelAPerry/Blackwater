#!/usr/bin/env python3
"""Assemble a static site only after BLACKWATER's actual package passes emulation."""
import argparse
import hashlib
import html
import json
from pathlib import Path
import shutil
import subprocess

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
metadata['browser_interaction_tested'] = False
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
page = '''<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>BLACKWATER · development emulator</title>
<style>body{background:#051017;color:#d9e9df;font:16px system-ui;margin:24px auto;max-width:1100px;padding:0 16px}
a{color:#93d6b3}iframe{width:100%;height:850px;border:1px solid #34534b}code{overflow-wrap:anywhere}</style>
<h1>BLACKWATER</h1><p>Four fish. Four FM tracks. Development prototype.</p>
<p>This is the actual FM-1 firmware package running in the existing fm1-emu emulator.
It has passed native and Wasm smoke checks. Browser interaction and physical hardware still need validation.</p>
<p>Start the emulator, press REC to cast a hook, then PLAY. ALGORITHM selects a track;
SEQ opens fishing, EDIT opens FM editing, ENV opens envelopes, and SEL shows catches.</p>
<p>Generative behavior is currently basic pitch/rhythm mutation. Artwork, musical identities and the Jeremy Wade clip remain unfinished.</p>
<iframe src="emulator/?fw=blackwater&amp;embed=1" title="BLACKWATER firmware in fm1-emu" allow="autoplay"></iframe>
<p><a href="emulator/?fw=blackwater">Open the full emulator</a> · <a href="firmware/blackwater.fwsc">Download development package</a>
 · <a href="build.json">Build and validation details</a></p>
<p>Package SHA-256: <code>PACKAGE_HASH</code></p>
<footer>Firmware platform: <a href="https://github.com/charlesvestal/fm1-base">fm1-base</a> and
<a href="https://github.com/hugelton/felucca-bsp">felucca-bsp</a>.
Emulator: <a href="https://github.com/joakimeriksson/fm1-emu">fm1-emu by Joakim Eriksson</a>.
<a href="https://github.com/MichaelAPerry/Blackwater">BLACKWATER source</a> ·
<a href="licenses/THIRD_PARTY.md">Credits and licenses</a>.</footer></html>'''
(out/'index.html').write_text(page.replace('PACKAGE_HASH', html.escape(metadata['package_sha256'])))
assert hashlib.sha256((out/'firmware/blackwater.fwsc').read_bytes()).hexdigest() == metadata['package_sha256']
print(out)
