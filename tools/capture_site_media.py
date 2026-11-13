#!/usr/bin/env python3
"""Refresh website media from the real host firmware. No fabricated screens or substitute audio.

Run after ./build.sh --host. ffmpeg is needed for the actual current recording. Generated evidence stays in
build/site-media; only selected frames, the recording and capture provenance are copied to site/.
"""
import json
from pathlib import Path
import shutil
import subprocess
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
evidence=ROOT/'build/site-media'
evidence.mkdir(parents=True,exist_ok=True)
host=ROOT/'firmware/build/host/blackwater_host'
for name in ('vector_simulator','vector_portrait','nature_simulator','fm_controls'):
    with (evidence/f'{name}.log').open('w') as log:
        subprocess.run([str(host),str(ROOT/'tests'/f'{name}.txt'),str(evidence)],cwd=ROOT,
                       stdout=log,stderr=subprocess.STDOUT,check=True)
free=evidence/'free.txt'
free.write_text('wait 600\ntap GLO\nturn PRESET 3\nturn K1 1\nturn K3 3\ntap HOME\n'
                'key 4 down\nturn K2 -5\nwait 3000\nshot free-play.png\nkey 4 up\n')
with (evidence/'free.log').open('w') as log:
    subprocess.run([str(host),str(free),str(evidence)],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
with (evidence/'flow.log').open('w') as log:
    subprocess.run([str(ROOT/'build/venv/bin/python'),str(ROOT/'tools/render_flow_review.py'),
                    '--out',str(evidence/'flow')],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
screens=ROOT/'site/screens'
screens.mkdir(exist_ok=True)
photos={
    'river.png':evidence/'flow/keyboard-current/frame-0550.png',
    'growth.png':evidence/'flow/encounter-current/frame-0250.png',
    'night.png':evidence/'nature-night.png',
    'strike.png':evidence/'vector-portrait.png',
    'free-play.png':evidence/'free-play.png',
    'fish-guide.png':evidence/'fish-guide.png',
}
for name,source in photos.items(): Image.open(source).save(screens/name,optimize=True)
shutil.copy2(evidence/'flow/keyboard-current.mp4',ROOT/'site/current.mp4')
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
(ROOT/'site/media.json').write_text(json.dumps({
    'firmware_source_commit':commit,
    'capture':'fm1-base host application: native C display and audio; not physical-device footage',
    'refresh':'PATH="$PWD/build/venv/bin:$PATH" ./build.sh --host && build/venv/bin/python tools/capture_site_media.py',
    'recording':'flow/keyboard-current/scenario.txt (ordinary controls and chromatic keyboard)',
    'screens':{name:str(source.relative_to(evidence)) for name,source in photos.items()},
},indent=2)+'\n')
print('Website media refreshed from actual firmware:',screens)
