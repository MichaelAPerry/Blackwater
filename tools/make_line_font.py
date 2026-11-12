#!/usr/bin/env python3
"""Original monoline instrument alphabet, rendered into fm1-base's supported font-header format."""
import sys
from pathlib import Path
from PIL import Image, ImageDraw

# Paths on a 4 by 6 grid. Preserve case for musical notation (i / iv versus I / IV).
GLYPHS = {
    'A':'0,6 0,2 2,0 4,2 4,6;0,3 4,3', 'B':'0,0 0,6 3,6 4,5 4,4 3,3 0,3;0,0 3,0 4,1 4,2 3,3',
    'C':'4,1 3,0 1,0 0,1 0,5 1,6 3,6 4,5', 'D':'0,0 0,6 2,6 4,4 4,2 2,0 0,0',
    'E':'4,0 0,0 0,6 4,6;0,3 3,3', 'F':'4,0 0,0 0,6;0,3 3,3',
    'G':'4,1 3,0 1,0 0,1 0,5 1,6 3,6 4,5 4,3 2,3', 'H':'0,0 0,6;4,0 4,6;0,3 4,3',
    'I':'0,0 4,0;2,0 2,6;0,6 4,6', 'J':'0,0 4,0 4,5 3,6 1,6 0,5',
    'K':'0,0 0,6;4,0 0,4;1,3 4,6', 'L':'0,0 0,6 4,6',
    'M':'0,6 0,0 2,3 4,0 4,6', 'N':'0,6 0,0 4,6 4,0', 'O':'1,0 3,0 4,1 4,5 3,6 1,6 0,5 0,1 1,0',
    'P':'0,6 0,0 3,0 4,1 4,2 3,3 0,3', 'Q':'1,0 3,0 4,1 4,5 3,6 1,6 0,5 0,1 1,0;2,4 4,6',
    'R':'0,6 0,0 3,0 4,1 4,2 3,3 0,3;2,3 4,6', 'S':'4,1 3,0 1,0 0,1 0,2 1,3 3,3 4,4 4,5 3,6 1,6 0,5',
    'T':'0,0 4,0;2,0 2,6', 'U':'0,0 0,5 1,6 3,6 4,5 4,0', 'V':'0,0 2,6 4,0',
    'W':'0,0 0,6 2,3 4,6 4,0', 'X':'0,0 4,6;4,0 0,6', 'Y':'0,0 2,3 4,0;2,3 2,6', 'Z':'0,0 4,0 0,6 4,6',
    '0':'1,0 3,0 4,1 4,5 3,6 1,6 0,5 0,1 1,0;1,4 3,2', '1':'1,1 2,0 2,6;0,6 4,6',
    '2':'0,1 1,0 3,0 4,1 4,2 0,6 4,6', '3':'0,0 4,0 2,3 4,4 4,5 3,6 0,6',
    '4':'3,6 3,0 0,4 4,4', '5':'4,0 0,0 0,3 3,3 4,4 4,5 3,6 0,6',
    '6':'4,0 1,0 0,2 0,5 1,6 3,6 4,5 4,4 3,3 0,3', '7':'0,0 4,0 1,6',
    '8':'1,0 3,0 4,1 4,2 3,3 1,3 0,2 0,1 1,0;1,3 0,4 0,5 1,6 3,6 4,5 4,4 3,3',
    '9':'4,3 1,3 0,2 0,1 1,0 3,0 4,1 4,5 3,6 0,6',
    '.':'2,5.7 2,6', ',':'2,5 1,6', ':':'2,1 2,1.4;2,4.6 2,5', ';':'2,1 2,1.4;2,5 1,6',
    '-':'0,3 4,3', '_':'0,6 4,6', '+':'0,3 4,3;2,1 2,5', '=':'0,2 4,2;0,4 4,4',
    '/':'0,6 4,0', '\\':'0,0 4,6', '|':'2,0 2,6', '!':'2,0 2,4;2,5.7 2,6',
    '?':'0,1 1,0 3,0 4,1 4,2 2,3 2,4;2,5.7 2,6', '(': '3,0 1,2 1,4 3,6', ')':'1,0 3,2 3,4 1,6',
    '[':'3,0 1,0 1,6 3,6', ']':'1,0 3,0 3,6 1,6', '{':'3,0 2,1 2,2 1,3 2,4 2,5 3,6',
    '}':'1,0 2,1 2,2 3,3 2,4 2,5 1,6', '<':'4,1 0,3 4,5', '>':'0,1 4,3 0,5',
    '*':'2,1 2,5;0,2 4,4;0,4 4,2', '^':'0,2 2,0 4,2', "'":'2,0 2,2', '"':'1,0 1,2;3,0 3,2',
    '`':'1,0 2,1', '~':'0,3 1,2 3,4 4,3', '#':'1,0 1,6;3,0 3,6;0,2 4,2;0,4 4,4',
    '$':'4,1 1,1 0,2 1,3 3,3 4,4 3,5 0,5;2,0 2,6',
    '%':'0,6 4,0;0,0 1,0 1,1 0,1 0,0;3,5 4,5 4,6 3,6 3,5',
    '&':'4,6 0,2 0,1 1,0 2,0 3,1 3,2 0,4 0,5 1,6 2,6 4,3',
    '@':'4,5 1,5 0,4 0,1 1,0 3,0 4,1 4,4 2,4 2,2 4,2', ' ':'',
}
SIZES = [('T',9,5,.65),('XS',12,6,.75),('S',15,8,.8),('B',15,8,1.0),('M',22,11,1.1),('L',29,15,1.3)]
for code in range(ord('a'),ord('z')+1):
    letter=chr(code)
    small=[]
    for path in GLYPHS[letter.upper()].split(';'):
        points=[tuple(map(float,p.split(','))) for p in path.split()]
        small.append(' '.join(f'{x:g},{2+y*2/3:g}' for x,y in points))
    GLYPHS[letter]=';'.join(small)
GLYPHS['i']='2,0 2,0.3;2,2 2,6;1,6 3,6'
GLYPHS['v']='0,2 2,6 4,2'

def generate(dest):
    lines=['/* Generated original BLACKWATER monoline font. SPDX-License-Identifier: GPL-3.0-or-later */',
           '#pragma once', '#include <stdint.h>', '#define FONT_PAD 1']
    for name,height,advance,stroke in SIZES:
        width=advance+1
        data=[]; offsets=[]
        for code in range(32,127):
            offsets.append(len(data))
            image=Image.new('L',(width*4,height*4)); draw=ImageDraw.Draw(image)
            for path in GLYPHS[chr(code)].split(';'):
                if not path: continue
                pts=[tuple(map(float,p.split(','))) for p in path.split()]
                pts=[((1+x*(advance-2)/4)*4,(1+y*(height-3)/6)*4) for x,y in pts]
                draw.line(pts,fill=255,width=max(2,round(stroke*4)),joint='curve')
            image=image.resize((width,height),Image.Resampling.LANCZOS)
            values=[(v*15+127)//255 for v in image.get_flattened_data()]
            for row in range(height):
                pixels=values[row*width:(row+1)*width]+[0]
                data.extend((pixels[i]<<4)|pixels[i+1] for i in range(0,width,2))
        lines.append(f'static const uint8_t FONT_{name}_DATA[]={{'+','.join(map(str,data))+'};')
        lines.append(f'static const uint16_t FONT_{name}_OFF[]={{'+','.join(map(str,offsets))+'};')
        for suffix,val in [('ADV',advance),('BW',width)]:
            lines.append(f'static const uint8_t FONT_{name}_{suffix}[]={{'+','.join([str(val)]*95)+'};')
        lines.append(f'static const felucca_font_t FONT_{name}={{ {height},1,32,126,FONT_{name}_ADV,FONT_{name}_BW,FONT_{name}_OFF,FONT_{name}_DATA }};')
    Path(dest).write_text('\n'.join(lines)+'\n')
    print('font: original monoline / six roles / printable ASCII')

if __name__=='__main__': generate(sys.argv[1])
