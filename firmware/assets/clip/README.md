# The "fish on" clip

Put the clip here as `fish_on.wav` (16-bit PCM WAV, mono or stereo, any sample rate, at most 12 seconds).
`tools/make_clip.py` converts it at build time (22,050 Hz mono, IMA ADPCM, about 11 KB per second) into the
firmware image. With no file here the firmware is built without a clip and plays nothing.

To build with a clip without committing it: `BW_CLIP_WAV=/path/to/clip.wav ./build.sh`.

Rights: anything committed here is published with the source code and the public site. A clip from a television
programme belongs to its producers; commit one only with permission. No substitute recording or synthetic voice
is supplied.
