#!/usr/bin/env python3
"""Assemble a static site only after BLACKWATER's actual package passes emulation."""
import argparse
import hashlib
from site_content import copy_site_assets, installer_pages, render_landing
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
p.add_argument('--version', help='a release package\'s version (build.sh --release X.Y): adds the web installer')
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
# every build its own URLs: the emulator and the package (a browser's cache never serves an older firmware)
build_id = metadata['package_sha256'][:10]
emu_dir, pkg_name = f'emulator-{build_id}', f'blackwater-{build_id}.fwsc'
shutil.copytree(web, out/emu_dir)
(out/'firmware').mkdir()
shutil.copy2(package, out/'firmware'/pkg_name)
metadata['emulator_path'], metadata['package_path'] = emu_dir + '/', 'firmware/' + pkg_name
(out/'emulator').mkdir()  # the old address: on to this build's
(out/'emulator'/'index.html').write_text(f'<!doctype html><meta charset="utf-8"><title>BLACKWATER emulator</title>'
    f'<meta http-equiv="refresh" content="0; url=../{emu_dir}/?fw=blackwater&amp;mhz=240">'
    f'<script>location.replace("../{emu_dir}/" + (location.search || "?fw=blackwater&mhz=240"))</script>'
    f'<a href="../{emu_dir}/?fw=blackwater&amp;mhz=240">BLACKWATER in fm1-emu</a>')
# Remove unrelated firmware choices from the generated page's catalogs.
catalog = {'note':'BLACKWATER development firmware; not physically verified.', 'default':'blackwater',
    'firmwares':[{'id':'blackwater','name':'BLACKWATER','version':(a.version or 'dev') + ' ' + build_id[:7],
        'license':'GPL-3.0-or-later','repo':'https://github.com/MichaelAPerry/Blackwater',
        'about':'An ambient musical river: flowing notes, wildlife, fishing and free FM play.',
        'package':'../firmware/' + pkg_name}]}
(out/emu_dir/'firmwares.json').write_text(json.dumps(catalog, indent=2)+'\n')
# The optional SMK-37 page is unrelated to this FM-1 application.
shutil.rmtree(out/emu_dir/'smk37')
# (the FM-1 page still asks for the SMK-37's catalog, to pair firmwares: an empty one)
(out/emu_dir/'smk37').mkdir()
(out/emu_dir/'smk37/firmwares.json').write_text('{"firmwares": []}\n')
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
installed = installer_pages(root, out, package, a.version) if a.version else None
if installed:
    metadata['release_version'], metadata['release_identity'] = a.version, installed[1]
    (out/'build.json').write_text(json.dumps(metadata, indent=2)+'\n')
copy_site_assets(root, out)
(out/'index.html').write_text(render_landing(root, metadata, a.version, installed))
assert hashlib.sha256((out/'firmware'/pkg_name).read_bytes()).hexdigest() == metadata['package_sha256']
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
