#!/usr/bin/env python3
"""Record wildlife and free playing from the real fm1-base host; no event injection or substitute audio.

Wildlife clips skip the waits between events. Each clip itself runs at normal speed with its actual FM audio.
The UFO is captured after nineteen minutes of ordinary, line-lifted play.
"""
import argparse
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--out', type=Path, default=ROOT / 'build/living-review')
p.add_argument('--nature', action='store_true', help='also record day/night light, insects and musical growth')
a = p.parse_args()
out = a.out.resolve()
out.mkdir(parents=True, exist_ok=True)
host = ROOT / 'firmware/build/host/blackwater_host'


def capture(name, prefix, seconds, suffix=''):
    target = out / name
    target.mkdir(exist_ok=True)
    script = prefix.splitlines() + ['wav audio.wav']
    for frame in range(seconds * 25):
        # A short actual keyboard performance, including SELECT moving among FM operators.
        if name == 'free':
            actions = {0: ['key 0 down'], 40: ['key 7 down'], 80: ['key 14 down'],
                       120: ['turn SELECT 1', 'turn K1 2'], 160: ['key 0 up', 'key 7 up'],
                       200: ['key 14 up', 'key 26 down'], 250: ['key 26 up']}
            script += actions.get(frame, [])
        script += [f'shot frame-{frame:04d}.png', 'wait 40']
    script += ['wavstop'] + suffix.splitlines()
    source = target / 'scenario.txt'
    source.write_text('\n'.join(script) + '\n')
    with (target / 'host.log').open('w') as log:
        subprocess.run([str(host), str(source), str(target)], cwd=ROOT,
                       stdout=log, stderr=subprocess.STDOUT, check=True)
    result = out / f'{name}.mp4'
    subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-framerate', '25',
                    '-i', str(target / 'frame-%04d.png'), '-i', str(target / 'audio.wav'),
                    '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '18', '-c:a', 'aac',
                    '-b:a', '160k', '-shortest', '-movflags', '+faststart', str(result)], check=True)
    shutil.copyfile(target / 'frame-0050.png', out / f'{name}.png')
    print(result, flush=True)
    return result


night = 'wait 600\ntap GLO\nturn K3 2\ntap HOME\ntap REC\ntap PLAY\n'
clips = []
for name, start, seconds in [('leaf', 13000, 4), ('shooting-star', 28200, 8),
                             ('jump', 43000, 3), ('firefly', 56500, 5)]:
    clips.append(capture(name, night + f'wait {start}\n', seconds, 'expect catches 0'))
clips.append(capture('bee', night + 'wait 59000\ntap GLO\nturn K3 2\ntap HOME\nwait 21000',
                     4, 'expect bees >0\nexpect catches 0'))
clips.append(capture('ufo', night + 'wait 1138200', 13, 'expect ufos 1\nexpect catches 0'))
listing = out / 'clips.txt'
listing.write_text(''.join(f"file '{clip.name}'\n" for clip in clips))
subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-f', 'concat', '-safe', '0',
                '-i', str(listing), '-c', 'copy', '-movflags', '+faststart',
                str(out / 'living-river.mp4')], check=True)

capture('free', 'wait 600\ntap GLO\nturn PRESET 3\nturn K1 1\nturn K3 3\ntap HOME',
        12, 'expect mode 1\nexpect catches 0\nexpect held 0\nexpect peak_db_min -45')
print(out / 'living-river.mp4', flush=True)

if a.nature:
    day = 'wait 600\ntap GLO\nturn K3 4\ntap HOME\ntap REC\ntap PLAY\n'
    capture('daylight', day + 'wait 8000', 8, 'expect daylight 100')
    capture('night', night + 'wait 8000', 8, 'expect night_light 100')
    capture('day-to-night', day + 'wait 5000\ntap GLO\nturn K3 -2\ntap HOME',
            40, 'expect night_light >98')
    capture('musical-growth', 'wait 600\ntap SEQ\nturn K1 -14\nturn K2 -42\nturn K3 -1\n'
            'tap HOME\ntap PLAY\nwait 26000', 22, 'expect river_phase 4')
    # Only the ordinary mixer is turned down, so the actual procedural soundscape can be heard alone.
    for name, weather in [('cicadas', 4), ('crickets', 2)]:
        source = out / f'{name}.txt'
        source.write_text('wait 600\ntap GLO\n' + f'turn K3 {weather}\n' +
                          'tap HOME\nturn K1 -50\nturn K2 -50\nturn K3 -50\nturn K4 -50\n'
                          f'tap REC\ntap PLAY\nwav {name}.wav\nwait 12000\nwavstop\n')
        with (out / f'{name}.log').open('w') as log:
            subprocess.run([str(host), str(source), str(out)], cwd=ROOT,
                           stdout=log, stderr=subprocess.STDOUT, check=True)
