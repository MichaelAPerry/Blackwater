"""Verify the display's empty-space/growth/clarity contract on actual host captures."""
from collections import Counter
from pathlib import Path
import sys
from PIL import Image

root = Path(sys.argv[1])
ink = {}
for name in ('quiet', 'growth', 'strike', 'fight', 'reveal', 'return', 'portrait'):
    im = Image.open(root / f'vector-{name}.png').convert('RGB')
    assert im.size == (240, 240), (name, im.size)
    colors = Counter(im.get_flattened_data())
    ink[name] = 240 * 240 - colors.most_common(1)[0][1]
    # Even the portrait is drawn in open strokes, not a filled illustration.
    assert 500 < ink[name] < 6000, (name, ink[name])

assert ink['growth'] > ink['quiet'] * 1.5, ink
assert ink['return'] < ink['growth'] * 0.6, ink
portrait = Image.open(root / 'vector-portrait.png').convert('RGB').crop((20, 24, 95, 119))
colors = Counter(portrait.get_flattened_data())
face_ink = portrait.width * portrait.height - colors.most_common(1)[0][1]
assert 250 < face_ink < 2000, face_ink
print('vector display: quiet/growth/return, readable encounter, sparse portrait passed', ink)
