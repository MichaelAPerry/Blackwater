#!/usr/bin/env python3
"""Record actual FM notes pushing water downstream in the fm1-base host at 25 fps, with real audio."""
import argparse
from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--out',type=Path,default=ROOT/'build/flow-review')
a=p.parse_args()
out=a.out.resolve()
out.mkdir(parents=True,exist_ok=True)
host=ROOT/'firmware/build/host/blackwater_host'

def capture(name,prefix,seconds,actions,suffix):
    target=out/name
    target.mkdir(exist_ok=True)
    script=prefix.splitlines()+['wav audio.wav']
    for frame in range(seconds*25):
        keys=actions.get(frame,[])
        script+=keys+[f'shot frame-{frame:04d}.png',f'wait {40-2*len(keys)}']
    script+=['wavstop']+suffix.splitlines()
    source=target/'scenario.txt'
    source.write_text('\n'.join(script)+'\n')
    with (target/'host.log').open('w') as log:
        subprocess.run([str(host),str(source),str(target)],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
    result=out/f'{name}.mp4'
    subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-framerate','25',
                    '-i',str(target/'frame-%04d.png'),'-i',str(target/'audio.wav'),
                    '-c:v','libx264','-pix_fmt','yuv420p','-crf','18','-c:a','aac','-b:a','160k',
                    '-shortest','-movflags','+faststart',str(result)],check=True)
    shutil.copyfile(target/'frame-0100.png',out/f'{name}.png')
    print(result,flush=True)

keyboard='tap GLO\nturn PRESET 3\nturn K2 1\nturn K3 2\ntap HOME\ntap PLAY\nwait 5000'
capture('one-note','wait 600\ntap REC\nturn K1 -50\nturn K2 -50\nturn K4 -50\n'+keyboard,
        20,{0:['key 0 down'],90:['key 0 up']},
        'expect flow_notes 1\nexpect flow_waves 0\nexpect held 0\nexpect peak_db_min -45')
capture('keyboard-current','wait 600\ntap REC\n'+keyboard,24,
        {0:['key 4 down'],50:['key 11 down'],100:['key 16 down'],125:['key 4 up'],
         150:['key 19 down'],175:['key 11 up'],200:['key 4 down'],225:['key 16 up'],
         250:['key 11 down'],275:['key 19 up'],300:['key 16 down'],325:['key 4 up'],
         350:['key 11 up'],375:['key 16 up']},
        'expect flow_notes >7\nexpect held 0\nexpect peak_db_min -45')
capture('encounter-current','wait 600\ntap SEQ\nturn K1 -14\nturn K2 -42\nturn K3 -1\n'
        'tap HOME\ntap PLAY\nwait 26000',22,{},'expect river_phase 4')
