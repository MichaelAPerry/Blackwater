# Rights: the fish-on clip and the portrait

Not legal advice. Two pieces of BLACKWATER belong to other people:

- **The clip** (`firmware/assets/clip/fish_on.wav`): one second of Jeremy Wade saying "Fish on!", from a recording
  whose copyright belongs to its producer.
- **The portrait** (`firmware/assets/art/fish_on_portrait.png`): an image made to look like Jeremy Wade. Using a
  real person's likeness, above all in something given away publicly, is his to allow (publicity / personality
  rights), whoever made the picture.

The Fish On! team's message to the project's owner (about his buffalo photograph, featured in their vodcast) was a
request to use *his* picture; it gives nothing in return. Permission for the clip and the likeness needs its own
yes, in writing.

## Until then

- The public site (`.github/workflows/pages.yml`) builds with `BW_PUBLIC_SAFE=1`: no clip, no portrait. The
  FISH ON! strike still plays, with the rare fish itself in the portrait's place. Set it to `"0"` once permission
  is given.
- The owner's own builds (`./build.sh`) keep both.
- The two files are in this Git repository. If the repository is public, they are already published with it;
  keeping them out of Git until permission (local files, `BW_CLIP_WAV` and the art folder) is the cautious choice.

## A request to send

To the Fish On! team (Oxford Scientific Films), for Jeremy Wade:

> Hello Anna and the Fish On! team,
>
> I'm a founding Patreon member (and the buffalo photo you asked about last week is mine: happy for you to use it).
>
> I make free, open-source firmware for a small music synthesizer, the M-VAVE FM-1. My latest, BLACKWATER, is a
> fishing game that makes music: four North American freshwater fish each play a part, and catching them changes
> the music. When a rare fish takes the line, I'd love the moment to be a one-second clip of Jeremy saying
> "Fish on!", with a pixel-art portrait of him.
>
> May I use (1) a one-second "Fish on!" clip from Fish On! and (2) a pixel-art likeness of Jeremy, inside the
> firmware and on its free web page, where anyone can play it in their browser? It is non-commercial: no sales,
> no ads, no payment. I would credit Jeremy Wade and Fish On! on the page and in the project, link to the
> podcast, and take either down at once if you ask.
>
> A short reply saying yes (or what you'd prefer) is all I need. Thank you!
>
> Michael Perry

Keep the reply with the project (a note here is enough), with the date and what it covers.
