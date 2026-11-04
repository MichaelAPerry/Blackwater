#!/usr/bin/env python3
"""Boot BLACKWATER's real package using the existing fm1-emu executable."""
import argparse
import array
import json
from pathlib import Path
import subprocess
import tempfile
import wave
from PIL import Image

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('package', type=Path)
p.add_argument('app', type=Path)
p.add_argument('--emulator', type=Path, default=root/'platform/fm1-emu/target/release/fm1-emu')
p.add_argument('--out', type=Path, default=root/'build/package-tests')
p.add_argument('--timeout', type=int, default=300)
a = p.parse_args()
metadata = json.loads(subprocess.check_output(
    ['node', str(root/'tools/package_metadata.mjs'), str(a.package.resolve()), str(a.app.resolve())], text=True))
a.out.mkdir(parents=True, exist_ok=True)
# A fresh directory prevents a previous passing run's screenshots/audio masking failure.
run = Path(tempfile.mkdtemp(prefix='native-', dir=a.out.resolve()))
(run/'work').mkdir()
command = [str(a.emulator.resolve()), '--fwsc', str(a.package.resolve()), '--ms', '5500',
           '--keys', '1200:REC,1500:PLAY,3000:SEQ,3500:SCL',
           '--snap', '1000,2500,3200,4000', '--wav', 'audio.wav']
with (run/'boot.log').open('w') as log:
    subprocess.run(command, cwd=run, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=a.timeout)
with wave.open(str(run/'audio.wav'), 'rb') as w:
    assert w.getnchannels() == 2 and w.getsampwidth() == 2, 'unexpected audio format'
    rate = w.getframerate()
    samples = array.array('h', w.readframes(w.getnframes()))
    # Sound must occur after casting and starting, not just at boot.
    active = samples[2*rate*2:2*rate*3]
    assert len(active) >= 2*rate//2, 'short audio run'
    rms = (sum(v*v for v in active)/len(active))**0.5
    assert rms > 10, f'silent/near-silent playback: {rms}'
images = [Image.open(run/f'work/lcd_{ms}.bmp').convert('RGB') for ms in (1000,2500,3200,4000)]
assert all(im.size == (240,240) for im in images)
assert all(len(im.getcolors(1 << 24)) > 2 for im in images), 'blank screen'
assert images[1].tobytes() != images[2].tobytes(), 'SEQ did not change the display'
assert images[2].tobytes() != images[3].tobytes(), 'stats control did not change the display'
result = dict(metadata, native_audio_rms=rms, evidence=str(run))
(run/'result.json').write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result, indent=2))
