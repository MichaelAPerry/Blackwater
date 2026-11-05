#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""BLACKWATER's pixel art: the sheets of docs/ART_PROMPTS.md (firmware/assets/art/*.png) into a generated header.

  import_art.py OUT.h [--dir DIR]       (default DIR: $BW_ART_DIR, else firmware/assets/art)

Each sheet is a grid of equal cells; each cell holds one sprite drawn at some whole number of screen pixels per game
pixel (the prompts ask for 2x2 .. 8x8 blocks). For every game pixel the most common colour of the middle of its block
is taken (anti-aliased block edges are ignored), magenta (#FF00FF-ish) becomes transparent on keyed sheets, and the
whole sheet is reduced to 16 colours (15 and transparency), 4 bits a pixel, RGB565. A missing sheet is left empty and
the firmware draws its own placeholder. Nothing here invents art.
"""
import argparse
import collections
import os
import pathlib
import sys

from PIL import Image

HERE = pathlib.Path(__file__).resolve().parent
DEFAULT = HERE.parent / 'firmware/assets/art'
# name: (file, game width, game height, columns, rows, keyed)
FISH = ['bluegill', 'bass', 'trout', 'catfish', 'crappie', 'peacock', 'brook', 'flathead']
SHEETS = {f: (f + '.png', 48, 24, 4, 3, True) for f in FISH}
SHEETS.update({
    'portraits': ('portraits.png', 96, 64, 2, 2, False),
    'portraits_rare': ('portraits_rare.png', 96, 64, 2, 2, False),
    'river': ('river.png', 240, 240, 3, 1, False),
    'props': ('props.png', 24, 24, 6, 3, True),
    'banner': ('fish_on.png', 150, 50, 1, 1, True),
    'portrait': ('fish_on_portrait.png', 112, 112, 1, 1, False),  # the FISH ON! moment  # the splash scene, 3:1
})


def is_key(rgb):
    """magenta by hue, not by value: the key itself, pink grid lines, and the purple fringe where a generated image
    blends the subject into the key"""
    r, g, b = rgb[:3]
    return min(r, b) - g > 50 and abs(r - b) < 70


def window(img, cols, rows, gw, gh):
    """keyed sheets: the part of every cell that holds something (the union over the cells, so all frames share one
    window and an animation does not jitter), widened to the sprite's proportions -> (x0, y0, source px per game px)
    in cell coordinates"""
    W, H = img.size
    cw, ch = W // cols, H // rows
    px = img.load()
    x0 = y0 = 10 ** 9
    x1 = y1 = -1
    step = max(1, min(cw, ch) // 256)
    for row in range(rows):
        for col in range(cols):
            for y in range(0, ch, step):
                for x in range(0, cw, step):
                    if not is_key(px[col * cw + x, row * ch + y]):
                        x0, y0, x1, y1 = min(x0, x), min(y0, y), max(x1, x + step), max(y1, y + step)
    if x1 < 0:
        return 0.0, 0.0, cw / gw
    s = max((x1 - x0) / gw, (y1 - y0) / gh)
    return (x0 + x1) / 2 - gw * s / 2, (y0 + y1) / 2 - gh * s / 2, s


def sample(img, gw, gh, cols, rows, keyed):
    """the sheet as game pixels: a list of cells, each gh rows of gw (r, g, b) or None (transparent)"""
    W, H = img.size
    cw, ch = W / cols, H / rows
    if keyed:
        ox, oy, sx = window(img, cols, rows, gw, gh)
        sy = sx
    else:
        ox, oy, sx, sy = 0.0, 0.0, cw / gw, ch / gh
    px = img.load()
    cells = []
    for row in range(rows):
        for col in range(cols):
            cell = []
            for j in range(gh):
                line = []
                for i in range(gw):
                    bx, by = ox + i * sx, oy + j * sy
                    if keyed and (bx < 0 or by < 0 or bx + sx > cw or by + sy > ch):
                        line.append(None)
                        continue
                    x0, y0 = col * cw + bx, row * ch + by
                    # the middle of the block: edges blur in generated images
                    xs = range(int(x0 + sx * 0.375), max(int(x0 + sx * 0.625), int(x0 + sx * 0.375) + 1))
                    ys = range(int(y0 + sy * 0.375), max(int(y0 + sy * 0.625), int(y0 + sy * 0.375) + 1))
                    votes = collections.Counter(px[min(x, W - 1), min(y, H - 1)][:3] for y in ys for x in xs)
                    c = votes.most_common(1)[0][0]
                    if not keyed and is_key(c):  # a divider between panels: the neighbour's colour
                        c = line[-1] if line else (cell[-1][0] if cell else (0, 0, 0))
                    line.append(None if keyed and is_key(c) else c)
                cell.append(line)
            cells.append(cell)
    return cells


def grid_lines(img, step, mh, mw):
    """rows and columns that are a dark rule across nearly the whole sheet: a grid between the cells, not a fish
    (four fish side by side also fill a row, but not with one near-black line); a cell of slack either side"""
    px = img.load()
    dark = lambda c: max(c[:3]) < 80
    rows = {j for j in range(mh) if sum(dark(px[i * step, j * step]) for i in range(mw)) > 0.9 * mw}
    cols = {i for i in range(mw) if sum(dark(px[i * step, j * step]) for j in range(mh)) > 0.9 * mh}
    return ({y + d for y in rows for d in (-1, 0, 1) if 0 <= y + d < mh},
            {x + d for x in cols for d in (-1, 0, 1) if 0 <= x + d < mw})


def find_sprites(img, frames):
    """a fish sheet by its drawings, not its grid: rows of content, then the drawings in each row (gaps of key
    between them); specks (a stray splash, a line's broken end) join the drawing nearest them. 13 drawings (an
    extra fight frame) keep the first seven, three fights spread over the rest and the last two -> boxes
    (x0, y0, x1, y1) in sheet pixels, in reading order"""
    W, H = img.size
    step = max(1, W // 600)
    px = img.load()
    mask = [[not is_key(px[x, y]) for x in range(0, W, step)] for y in range(0, H, step)]
    mh, mw = len(mask), len(mask[0])
    lines_r, lines_c = grid_lines(img, step, mh, mw)
    for y in lines_r:
        mask[y] = [False] * mw
    for x in lines_c:
        for y in range(mh):
            mask[y][x] = False

    def runs(flags, gap):
        out, start, last = [], None, None
        for i, f in enumerate(flags):
            if f:
                if start is None:
                    start = i
                elif i - last > gap:
                    out.append((start, last + 1))
                    start = i
                last = i
        if start is not None:
            out.append((start, last + 1))
        return out
    boxes, per_row = [], []
    for r0, r1 in runs([any(row) for row in mask], max(2, mh // 60)):
        cols = [any(mask[y][x] for y in range(r0, r1)) for x in range(mw)]
        found = []
        for c0, c1 in runs(cols, max(2, mw // 200)):
            ys = [y for y in range(r0, r1) if any(mask[y][c0:c1])]
            found.append([c0, ys[0], c1, ys[-1] + 1])
        if not found:
            continue
        big = max((b[2] - b[0]) * (b[3] - b[1]) for b in found)
        keep = [b for b in found if (b[2] - b[0]) * (b[3] - b[1]) > big * 0.05]
        for b in found:
            if b not in keep:  # a speck: into its neighbour
                n = min(keep, key=lambda k: abs((k[0] + k[2]) - (b[0] + b[2])))
                n[0], n[1], n[2], n[3] = min(n[0], b[0]), min(n[1], b[1]), max(n[2], b[2]), max(n[3], b[3])
        boxes += keep
        per_row.append(len(keep))
    boxes = [(b[0] * step, b[1] * step, b[2] * step, b[3] * step) for b in boxes]
    find_sprites.lines = ({y * step + d for y in lines_r for d in range(step)},
                          {x * step + d for x in lines_c for d in range(step)})
    if len(boxes) > frames:
        extra = boxes[7:len(boxes) - 2]
        fights = [extra[round(i * (len(extra) - 1) / 2)] for i in range(3)]
        boxes = boxes[:7] + fights + boxes[-2:]
    if len(boxes) != frames:
        raise ValueError(f'found {len(boxes)} drawings ({per_row} by row), expected {frames}')
    return boxes


def sample_sprites(img, gw, gh, frames):
    """each drawing centred in one window shared by all (the largest drawing, at the sprite's proportions)"""
    boxes = find_sprites(img, frames)
    lr, lc = find_sprites.lines
    s = max(max((b[2] - b[0]) / gw, (b[3] - b[1]) / gh) for b in boxes)
    W, H = img.size
    px = img.load()
    cells = []
    for x0, y0, x1, y1 in boxes:
        ox, oy = (x0 + x1) / 2 - gw * s / 2, (y0 + y1) / 2 - gh * s / 2
        cell = []
        for j in range(gh):
            line = []
            for i in range(gw):
                bx, by = ox + i * s, oy + j * s
                xs = range(int(bx + s * 0.375), max(int(bx + s * 0.625), int(bx + s * 0.375) + 1))
                ys = range(int(by + s * 0.375), max(int(by + s * 0.625), int(by + s * 0.375) + 1))
                # only this drawing: its own box (a neighbour's fin past the edge is not part of it)
                votes = collections.Counter(px[x, y][:3] if x0 <= x < x1 and y0 <= y < y1 and y not in lr and x not in lc
                                            else (255, 0, 255) for y in ys for x in xs)
                c = votes.most_common(1)[0][0]
                line.append(None if is_key(c) else c)
            cell.append(line)
        cells.append(cell)
    return cells


def quantize(cells, keyed):
    """-> palette (RGB565 list), cells as indices (0 = transparent on keyed sheets). The palette is the sheet's own
    most frequent colours, a near-duplicate (a blurred edge, a JPEG-ish wobble) folded into the one it is close to;
    each pixel then takes its nearest entry"""
    n = 15 if keyed else 16
    counts = collections.Counter(c for cell in cells for line in cell for c in line if c is not None)
    pal = []
    for c, _ in counts.most_common():
        if all(sum((a - b) ** 2 for a, b in zip(c, p)) > 30 ** 2 for p in pal):
            pal.append(c)
            if len(pal) == n:
                break
    if not pal:
        pal = [(0, 0, 0)]

    def nearest(c):
        return min(range(len(pal)), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, pal[i])))
    memo = {}
    off = 1 if keyed else 0
    out = [[[0 if c is None else off + memo.setdefault(c, nearest(c)) for c in line] for line in cell] for cell in cells]
    rgb565 = ([0] if keyed else []) + [((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3) for r, g, b in pal]
    return (rgb565 + [0] * 16)[:16], out


def pack(cells):
    data = bytearray()
    for cell in cells:
        flat = [c for line in cell for c in line]
        for i in range(0, len(flat), 2):
            data.append(flat[i] | (flat[i + 1] << 4))
    return data


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out', type=pathlib.Path)
    ap.add_argument('--dir', type=pathlib.Path)
    a = ap.parse_args()
    d = a.dir or (pathlib.Path(os.environ['BW_ART_DIR']) if os.environ.get('BW_ART_DIR') else DEFAULT)
    lines = ['/* Generated by tools/import_art.py from %s. */' % d.name, '#pragma once', '#include <stdint.h>',
             'typedef struct', '{', '    uint16_t w, h;', '    uint8_t cols, rows, key;', '    const uint16_t *pal;',
             '    const uint8_t *px; /* 4 bits a pixel, cell after cell; 0 = no art */', '} bw_sheet;']
    have = {}
    for name, (file, gw, gh, cols, rows, keyed) in SHEETS.items():
        path = d / file
        if not path.is_file():
            continue
        img = Image.open(path).convert('RGBA')
        bg = Image.new('RGBA', img.size, (255, 0, 255, 255) if keyed else (0, 0, 0, 255))
        img = Image.alpha_composite(bg, img).convert('RGB')  # transparent PNG pixels count as the key
        W, H = img.size
        if W < gw * cols or H < gh * rows:
            raise SystemExit(f'import_art: {file}: {W}x{H} is smaller than {cols}x{rows} cells of {gw}x{gh}')
        try:
            cells = sample_sprites(img, gw, gh, cols * rows) if name in FISH else sample(img, gw, gh, cols, rows, keyed)
        except ValueError as e:
            print(f'art: {file}: {e}; read as a {cols}x{rows} grid instead')
            cells = sample(img, gw, gh, cols, rows, keyed)
        pal, cells = quantize(cells, keyed)
        data = pack(cells)
        lines.append('static const uint16_t bw_pal_%s[16] = {%s};' % (name, ','.join(map(str, pal))))
        lines.append('static const uint8_t bw_px_%s[%d] = {' % (name, len(data)))
        lines += [','.join(map(str, data[i:i + 40])) + ',' for i in range(0, len(data), 40)]
        lines.append('};')
        have[name] = (gw, gh, cols, rows, keyed)
        print(f'art: {file}: {cols}x{rows} cells of {gw}x{gh}, {len(data)} B')

    def entry(name):
        gw, gh, cols, rows, keyed = SHEETS[name][1:]
        if name in have:
            return '{%d, %d, %d, %d, %d, bw_pal_%s, bw_px_%s}' % (gw, gh, cols, rows, int(keyed), name, name)
        return '{%d, %d, %d, %d, %d, 0, 0}' % (gw, gh, cols, rows, int(keyed))
    lines.append('static const bw_sheet bw_art_fish[8] = {%s};' % ', '.join(entry(f) for f in FISH))
    lines.append('static const bw_sheet bw_art_portraits[8] = {%s};' %
                 ', '.join([entry('portraits')] * 4 + [entry('portraits_rare')] * 4))
    lines.append('static const bw_sheet bw_art_river[1] = {%s};' % entry('river'))
    lines.append('static const bw_sheet bw_art_props[1] = {%s};' % entry('props'))
    lines.append('static const bw_sheet bw_art_banner[1] = {%s};' % entry('banner'))
    lines.append('static const bw_sheet bw_art_portrait[1] = {%s};' % entry('portrait'))
    a.out.write_text('\n'.join(lines) + '\n')
    if not have:
        print('art: no sheets yet, the firmware draws placeholders')


if __name__ == '__main__':
    sys.exit(main())
