"""All eight field-guide drawings must be present and geometrically distinct, excluding labels/colour."""
import sys
from pathlib import Path
from PIL import Image

image=Image.open(Path(sys.argv[1])/'fish-guide.png').convert('RGB')
background=image.getpixel((0,0))
shapes=[]
for cell in range(8):
    x=8+cell%2*116; y=52+cell//2*42
    drawing=image.crop((x+16,y,x+82,y+32))
    mask=bytes(int(pixel!=background) for pixel in drawing.get_flattened_data())
    assert sum(mask)>60, (cell,'missing fish')
    shapes.append(mask)
assert len(set(shapes))==8, 'a rare species still shares its cousin drawing'
print('fish guide: eight distinct line drawings passed')
