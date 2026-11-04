# Art prompts (ChatGPT image generation)

Prompts for BLACKWATER's device pixel art and its website art. The FM-1 screen is 240 × 240 pixels (RGB565);
the river screen gives each fish a row about 48 pixels tall, so device sprites are small, with few colours.
Website art can be larger but keeps the same pixel style and palette.

**Mood:** North American freshwater fishing at dusk and night: murky green-brown rivers, mist, a lantern, a
field-journal / expedition feel — investigative, a little ominous, never cartoon-cute. It is *inspired by* the
atmosphere of river-monster documentaries, so the prompts never name a real person or show, and never ask for a
likeness, logo or title card. Keep the line "no people, no logos, no text" in every prompt (the site adds its own
text). The Jeremy Wade clip is audio only (see `docs/DEVELOPMENT.md`); no artwork depicts him.

**How to send the results back:** the PNGs exactly as they come out (don't resize or recompress them), into
`firmware/assets/art/<name>.png` with the names below. They become an art pack with a NOTICE that they were made
with ChatGPT. An import step (planned: nearest-neighbour downscale to the game-pixel grid, magenta → transparent,
palette → RGB565 tables in a generated header) turns each sheet into firmware data, so the grid rules matter more
than the picture: equal cells, no gaps, one game pixel = one crisp block.

Do prompt 1 first and attach its image to every later chat ("use the palette and style of this image") so all
sheets match.

---

## 1. Style and palette reference (first)

```
Make a STYLE REFERENCE SHEET for a pixel-art music game about freshwater fishing in North America, shown on a
tiny 240x240 colour screen.
Mood: a dark river at dusk and night; murky olive-green and peat-brown water, fog, a warm lantern glow, moonlight
on ripples; the feel of a fishing expedition's field notebook - investigative, quiet, a little ominous.
Show on one sheet: a 16-colour palette as large swatches with hex codes (deep water darks, two mid greens,
mud brown, reed yellow-green, mist grey, moonlight pale blue, lantern amber, bobber red, bone white, and
per-fish accents: bluegill sunfish orange/blue, bass olive-green, trout golden-brown with red, catfish slate grey);
a small sample scene: a riverbank with reeds, a float bobbing on dark water, ripples, and a fish shadow below.
Style: true pixel art, hard pixel edges, no anti-aliasing, no blur, no gradients except stepped dithering.
No people, no logos, no text other than the hex codes.
```

## 2–5. Species sprite sheets (one chat per fish)

Use the same prompt four times, changing only the FISH line. Name the files `bluegill.png`, `bass.png`,
`trout.png`, `catfish.png`.

```
Using the palette and style of the attached image, make a PIXEL ART sprite sheet of one fish for a 240x240 LCD.
FISH: <one of the four lines below>
Strict rules:
- Side view, facing RIGHT, the whole fish inside each cell. Each sprite is exactly 48x24 game pixels, drawn so
  every game pixel is a crisp 8x8 block of screen pixels (each cell is 384x192 px). No anti-aliasing, no blur.
- At most 12 colours in the whole sheet, the same palette in every cell.
- Background of every cell: solid magenta #FF00FF (it becomes transparent). Never use magenta in the fish.
- A 4 x 3 grid of equal cells, no gaps, no borders, no labels, no text.
- The fish keeps the same size, outline and markings in every cell; only the pose changes.
Cells, left to right, top to bottom:
1-4 swimming loop (tail and fins sweep; 4 frames that cycle smoothly)
5-6 idle hover (gentle fin movement)
7   taking the bait (mouth open, lunging forward)
8-10 hooked and fighting (body curved, thrashing; a short line from the mouth to the top-right corner of the cell)
11  landed (lying flat, slightly turned, a glint on the scales)
12  dark silhouette of the same fish (one deep-water colour only) for fish not yet caught
No people, no logos, no text.
```

FISH lines (accurate field marks matter more than detail):

- **Bluegill:** a bluegill sunfish: deep, round, flat body; dark "ear flap" spot at the back of the gill cover;
  faint dark vertical bars on olive-blue sides; orange-yellow breast; small mouth.
- **Largemouth Bass:** a largemouth bass: long, heavy body; very large mouth whose jaw reaches past the eye; a
  dark ragged stripe along the side; olive-green back, pale belly; spiny front dorsal fin.
- **Brown Trout:** a brown trout: streamlined body, golden-brown sides, black spots and a few red spots with pale
  halos; square-ish tail with few spots; slightly hooked jaw.
- **Catfish:** a channel catfish: long smooth scaleless slate-grey body with a few dark spots, pale belly, long
  whisker barbels around the mouth, deeply forked tail.

If frames don't line up, ask: "Redo it with the fish in exactly the same pixels in every cell; only the fins, tail
and body curve may change."

## 2b–5b. Rare fish sprite sheets (one chat each)

Each fish slot has a rare cousin that turns up now and then (and plays the "fish on" clip). Same prompt as 2–5,
same rules and 12 cells, with these FISH lines. Files: `crappie.png`, `peacock.png`, `brook.png`, `flathead.png`.
Make them a touch more striking than the common fish: they are the trophies.

- **Crappie** (rare in the bluegill slot): a black crappie: deep, flattened, silvery-olive body covered in dark
  irregular speckles; large dorsal and anal fins with spotted rows; upturned mouth; big eye.
- **Peacock Bass** (rare in the bass slot): a peacock bass: golden-yellow to olive body with three bold dark vertical
  bars, a black "eye spot" ringed in gold on the tail, a red-orange eye and belly blush; humped forehead.
- **Brook Trout** (rare in the trout slot): a brook trout: dark olive back with pale worm-like "vermiculation",
  red spots with blue halos on the sides, orange-red belly, and white leading edges on the lower fins.
- **Flathead Catfish** (rare in the catfish slot, the river monster): a huge flathead catfish: broad flat head,
  wide mouth with the lower jaw jutting out, small eyes, mottled yellow-brown and olive skin, long barbels,
  a square (not forked) tail, heavy and menacing.

## 6. Catch portraits (the catch card)

`portraits.png` (the four common fish), then the same prompt again for `portraits_rare.png` with cells 1 crappie,
2 peacock bass, 3 brook trout, 4 flathead catfish (the field marks above).

```
Using the palette and style of the attached image, make a PIXEL ART sheet of four freshwater fish portraits.
Strict rules:
- Each portrait exactly 96x64 game pixels, every game pixel a crisp 4x4 block (each cell 384x256 px).
- A 2 x 2 grid of equal cells, no gaps, no borders, no text. At most 16 colours in the whole sheet.
- Each cell: one fish, side view facing right, filling most of the cell, over a dark water background with a
  few stepped-dither light rays and bubbles (no magenta here: these are full rectangles).
Cells: 1 bluegill sunfish (round, ear-flap spot, orange breast); 2 largemouth bass (huge jaw, dark side stripe);
3 brown trout (golden-brown, black and red haloed spots); 4 channel catfish (grey, long barbels, forked tail).
Lighting: a single lantern from the upper left, like an expedition photo taken at night.
No people, no logos, no text.
```

## 7. River backgrounds

`river.png`

```
Using the palette and style of the attached image, make three PIXEL ART backgrounds for a 240x240 screen.
Strict rules:
- Each exactly 240x240 game pixels, every game pixel a crisp 2x2 block (each panel 480x480 px), side by side
  in one row with no gaps and no borders. At most 16 colours each. No anti-aliasing.
- Composition for all three: the top 40 pixels are sky and far bank (they sit behind a title bar, keep them
  dark and calm); below, dark water seen from the bank, divided by subtle current lines into four horizontal
  lanes of about 48 pixels each (one fish per lane will be drawn on top, so keep the lanes uncluttered).
Panels: 1 dusk (orange-violet sky, reeds on the left), 2 night (moon, fog on the water, lantern glow at the
bottom-right corner), 3 storm (rain streaks, dark green water, distant lightning).
No people, no boats with people, no logos, no text.
```

## 8. Fishing props and effects

`props.png`

```
Using the palette and style of the attached image, make a PIXEL ART sheet of fishing props and effects.
Strict rules:
- A 6 x 3 grid of equal cells, each 24x24 game pixels, every game pixel a crisp 8x8 block (each cell 192x192 px).
- Background of every cell: solid magenta #FF00FF. No gaps, no borders, no text. At most 12 colours.
Cells, left to right, top to bottom:
1 fish hook (silver, barbed)        2 red-and-white float/bobber upright   3 bobber tilted (a nibble)
4 bobber pulled under (half sunk)   5-8 water splash, 4 frames from small to large
9-11 expanding ripple rings, 3 frames   12 baited hook with a worm   13 crankbait lure   14 spinner lure
15 tape-measure ruler segment       16 trophy (small silver cup)     17 lantern           18 a single bubble
No people, no logos, no text.
```

## 9. "FISH ON!" moment banner (shown with the clip)

`fish_on.png`

```
Using the palette and style of the attached image, make a PIXEL ART banner graphic WITHOUT letters: a burst of
water spray and a taut, bending fishing line across the frame, as if a big fish just struck. Exactly 200x48 game
pixels, every game pixel a crisp 4x4 block (800x192 px). Background solid magenta #FF00FF. At most 12 colours.
Leave a clear dark band across the middle third for the firmware to print text on. No people, no logos, no text.
```

---

## Website art

The site (`tools/build_simulator_site.py`) wraps the emulator. These use the same palette; text is added by the
page, so none of them contain words.

### W1. Hero banner — `site_hero.png`

```
Using the palette and style of the attached image, make a wide PIXEL ART banner, 1600x600 px, drawn at 4x4 screen
pixels per game pixel (400x150 game pixels). A dark North American river at night: tall reeds and a fallen log
on the left, mist over still water, a red-and-white float in the centre with rings spreading from it, and beneath
the surface four faint fish shapes (a round sunfish, a big-mouthed bass, a spotted trout, a whiskered catfish)
circling the line. A warm lantern glow at the bottom-right edge. Keep the right third darker and calmer
(page text goes there). Stepped dithering only, no smooth gradients. No people, no logos, no text.
```

### W2. Social preview — `site_og.png` (1280 × 640, what links show when shared)

```
Same scene and style as the attached hero banner, recomposed for 1280x640 px at 4x4 screen pixels per game pixel:
the float and its ripples centred, the four fish shapes below it, and a dark empty band across the top quarter
for a title the site adds. No people, no devices, no logos, no text.
```

### W3. Firmware card — `card.png` (240 × 240, like MPFM1's `docs/card.png` for fmhub.json)

```
Using the palette and style of the attached image, make a 240x240 px PIXEL ART tile (2x2 screen pixels per game
pixel): a largemouth bass leaping from dark water at night, a fishing line taut from its mouth, spray frozen in
the air, a crescent moon. Strong silhouette that still reads at 64x64. No people, no logos, no text.
```

### W4. Favicon / tab icon — `icon.png`

```
A 32x32 game-pixel PIXEL ART icon, drawn at 16x16 screen pixels per game pixel (512x512 px): a red-and-white
fishing float on a dark green-black circle of water with one ripple ring. At most 6 colours, crisp pixels,
transparent background outside the circle. No text.
```

---

## Open choices for you

- Settled: the catfish slot is the **channel catfish**; the **flathead** is its rare cousin (prompt 5b).
- Background count: one river or the three moods in prompt 7 (they could follow tempo, time played, or catches).
- Whether a fish's sprite scales with its recorded size on the river screen.
