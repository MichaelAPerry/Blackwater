#!/usr/bin/env python3
"""Capture the real host firmware's three sound studies and optional audiovisual encounter review.

Run after ./build.sh --host. No substitute audio engine or browser demo. Outputs are local review artifacts.
The video deliberately sets the wait to 30 s through normal controls; approach/fight/recovery use real time.
"""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--out', type=Path, default=ROOT / 'build/river-review')
p.add_argument('--video', action='store_true', help='also make a 110-second MP4; requires ffmpeg')
a = p.parse_args()
out = a.out.resolve()
out.mkdir(parents=True, exist_ok=True)
host = ROOT / 'firmware/build/host/blackwater_host'


def capture(name, script):
    target = out / name
    target.mkdir(exist_ok=True)
    source = target / 'scenario.txt'
    source.write_text(script)
    with (target / 'host.log').open('w') as log:
        subprocess.run([str(host), str(source), str(target)], cwd=ROOT,
                       stdout=log, stderr=subprocess.STDOUT, check=True)
    return target


for world, name in enumerate(('dusk', 'undertow', 'fireflies')):
    select = f'turn PRESET {world}\n' if world else ''
    capture(name, 'wait 600\n' + select + 'tap REC\ntap PLAY\nwait 5000\n'
            f'wav {name}.wav\nwait 30000\nwavstop\nexpect peak_db_min -45\nshot river.png\n')
    print(out / name / f'{name}.wav', flush=True)

if a.video:
    script = ['wait 600', 'tap SEQ', 'turn K1 -14', 'turn K2 -42', 'turn K3 -1',
              'expect wait_min 30', 'expect wait_max 30', 'tap HOME', 'tap PLAY', 'wav encounter.wav']
    for frame in range(110 * 8):
        script += [f'shot frame-{frame:04d}.png', 'wait 125']
    script += ['wavstop', 'expect catches >0']
    target = capture('encounter', '\n'.join(script) + '\n')
    subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-framerate', '8',
                    '-i', str(target / 'frame-%04d.png'), '-i', str(target / 'encounter.wav'),
                    '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '18', '-c:a', 'aac',
                    '-b:a', '160k', '-shortest', '-movflags', '+faststart', str(out / 'river-encounter.mp4')],
                   check=True)
    print(out / 'river-encounter.mp4', flush=True)
