"""Observe one real keyboard note moving downstream in host LCD captures, independently of the model test."""
from pathlib import Path
import sys
from PIL import Image, ImageChops

root=Path(sys.argv[1])

def crest(name):
    im=Image.open(root/f'flow-{name}.png').convert('RGB')
    # Quiet current is faint; the isolated played note is the only bright, saturated mark here.
    hits=[]
    for y in range(88,205):
        for x in range(110,131):
            p=im.getpixel((x,y))
            if max(p)-min(p)>65 and max(p)>100:
                hits.append(y)
    return hits

assert not crest('empty'), 'an unplayed note should not draw a bright crest'
positions=[]
for name in ('born','two-seconds','four-seconds'):
    hits=crest(name)
    assert len(hits)>=4, (name,hits)
    positions.append(sum(hits)/len(hits))
assert positions[1]>positions[0]+2 and positions[2]>positions[1]+4, positions
paused=Image.open(root/'flow-paused.png').crop((24,88,216,208))
later=Image.open(root/'flow-still-paused.png').crop((24,88,216,208))
assert ImageChops.difference(paused,later).getbbox() is None, 'paused water must not drift'
print('flow display: real note crest moves downstream, quiet field has no phantom note, pause freezes water:',positions)
