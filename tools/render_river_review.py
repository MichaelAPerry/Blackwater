#!/usr/bin/env python3
"""Capture the real host firmware's three sound studies and optional audiovisual encounter review.

Run after ./build.sh --host. No substitute audio engine or browser demo. Outputs are local review artifacts.
The video deliberately sets the wait to 30 s through normal controls; approach/fight/recovery use real time.
"""
import argparse
from pathlib import Path
import subprocess
import shutil

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--out', type=Path, default=ROOT / 'build/river-review')
p.add_argument('--video', action='store_true', help='also make a 110-second MP4; requires ffmpeg')
p.add_argument('--portrait', action='store_true', help='capture a naturally occurring rare strike and portrait')
p.add_argument('--fps', type=int, choices=(10, 20, 25), default=25)
p.add_argument('--controls', action='store_true', help='capture verified FM editing screens and all eight fish')
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

if a.controls:
    target=capture('controls',(ROOT/'tests/fm_controls.txt').read_text())
    print(target/'fish-guide.png',flush=True)
    print(target/'fm-operator.png',flush=True)

if a.video:
    script = ['wait 600', 'tap SEQ', 'turn K1 -14', 'turn K2 -42', 'turn K3 -1',
              'expect wait_min 30', 'expect wait_max 30', 'tap HOME', 'tap PLAY', 'wav encounter.wav']
    for frame in range(110 * a.fps):
        script += [f'shot frame-{frame:04d}.png', f'wait {1000 // a.fps}']
    script += ['wavstop', 'expect catches >0']
    target = capture('encounter', '\n'.join(script) + '\n')
    subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-framerate', str(a.fps),
                    '-i', str(target / 'frame-%04d.png'), '-i', str(target / 'encounter.wav'),
                    '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '18', '-c:a', 'aac',
                    '-b:a', '160k', '-shortest', '-movflags', '+faststart', str(out / 'river-encounter.mp4')],
                   check=True)
    print(out / 'river-encounter.mp4', flush=True)

if a.portrait:
    # Same normal controls/seed as vector_portrait.txt; the wait is real simulation, not a forced fish.
    script = ['wait 600', 'tap SEQ', 'turn K1 -14', 'turn K2 -42', 'turn K3 -1',
              'tap HOME', 'tap PLAY', 'wait 627200', 'wav portrait.wav']
    for frame in range(5 * a.fps):
        script += [f'shot portrait-{frame:04d}.png', f'wait {1000 // a.fps}']
    script += ['wavstop', 'expect clip_plays 1']
    target = capture('portrait', '\n'.join(script) + '\n')
    shutil.copyfile(target / f'portrait-{a.fps * 6 // 5:04d}.png', out / 'portrait.png')
    subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-framerate', str(a.fps),
                    '-i', str(target / 'portrait-%04d.png'), '-i', str(target / 'portrait.wav'),
                    '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '18', '-c:a', 'aac',
                    '-b:a', '160k', '-shortest', '-movflags', '+faststart', str(out / 'rare-strike.mp4')],
                   check=True)
    print(out / 'rare-strike.mp4', flush=True)
