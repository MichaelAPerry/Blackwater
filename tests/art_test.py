#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""tools/import_art.py on a synthetic sheet drawn the way the prompts ask (never shipped): 8x8 blocks with blurred
edges, a magenta key, 4 x 3 cells. Every game pixel must come back as drawn, transparent where magenta."""
import pathlib
import random
import re
import subprocess
import sys
import tempfile

from PIL import Image, ImageFilter

ROOT = pathlib.Path(__file__).resolve().parents[1]
GW, GH, S = 48, 24, 8
COLOURS = [(30, 60, 40), (200, 120, 40), (240, 230, 200), (20, 20, 20), (90, 160, 110)]


def main():
    rng = random.Random(7)
    truth = []
    sheet = Image.new('RGB', (4 * GW * S, 3 * GH * S), (255, 0, 255))
    for cell in range(12):
        cx, cy = cell % 4 * GW * S, cell // 4 * GH * S
        grid = [[None if rng.random() < 0.4 else rng.randrange(len(COLOURS)) for _ in range(GW)] for _ in range(GH)]
        truth.append(grid)
        for j in range(GH):
            for i in range(GW):
                if grid[j][i] is not None:
                    sheet.paste(COLOURS[grid[j][i]], (cx + i * S, cy + j * S, cx + (i + 1) * S, cy + (j + 1) * S))
    sheet = sheet.filter(ImageFilter.GaussianBlur(1.2))  # the soft edges of a generated image
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        sheet.save(d / 'bluegill.png')
        out = d / 'bw_art.h'
        subprocess.run([sys.executable, '-I', str(ROOT / 'tools/import_art.py'), str(out), '--dir', str(d)], check=True)
        text = out.read_text()
    pal = [int(v) for v in re.search(r'bw_pal_bluegill\[16\] = \{([^}]*)\}', text)[1].split(',')]
    px = [int(v) for v in re.search(r'bw_px_bluegill\[\d+\] = \{([^}]*)\}', text, re.S)[1].replace('\n', '').split(',') if v]
    assert len(px) == 12 * GW * GH // 2, len(px)
    assert pal[0] == 0
    want = [((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3) for r, g, b in COLOURS]
    bad = 0
    for cell in range(12):
        for j in range(GH):
            for i in range(GW):
                n = cell * GW * GH + j * GW + i
                c = px[n // 2] >> 4 if n & 1 else px[n // 2] & 15
                t = truth[cell][j][i]
                ok = (c == 0) if t is None else (c != 0 and pal[c] == want[t])
                bad += not ok
    assert bad == 0, f'{bad} game pixels wrong'
    assert 'bw_art_fish[8]' in text and 'bw_art_river[1] = {{240, 240, 3, 1, 0, 0, 0}}' in text
    print('art: synthetic sheet imported exactly (12 cells, 48x24, magenta keyed)')


if __name__ == '__main__':
    main()
