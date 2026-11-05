# The "fish on" clip

Put the clip here as `fish_on.wav` (16-bit PCM WAV, mono or stereo, any sample rate, at most 12 seconds).
`tools/make_clip.py` converts it at build time (22,050 Hz mono, IMA ADPCM, about 11 KB per second) into the
firmware image. With no file here the firmware is built without a clip and plays nothing.

To build with a clip without committing it: `BW_CLIP_WAV=/path/to/clip.wav ./build.sh`.

Rights: anything committed here is published with the source code and the public site. A clip from a television
programme belongs to its producers; commit one only with permission. No substitute recording or synthetic voice
is supplied.

## This clip

`fish_on.wav` is the user's upload `Fish_On.wav` (2.0 s, 48 kHz stereo), trimmed at the user's request: a word
before the phrase removed (cut at 0.575 s, inside the silence after it: the leading word ends at 0.55 s, the "f" of
"fish" starts at about 0.585 s, found from the level and the hiss of each 10 ms), ended at 1.580 s before the next
sound, 5 ms fade in and 15 ms fade out, mixed to mono and raised to a 28,000 peak (x2.59). 1.0 s. The user supplied
it and decided to use it; it plays for rare fish only.

## Notice

The clip is used as a one-second sample in an original musical work. BLACKWATER is an unofficial fan project, not
affiliated with, sponsored or endorsed by Jeremy Wade, Fish On!, River Monsters or Oxford Scientific Films. A rights
holder's request to change or remove it is acted on at once (an issue on the project's GitHub).
