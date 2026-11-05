#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests/clip
CFLAGS="-std=c99 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -ffp-contract=off -fsanitize=undefined -fno-sanitize-recover=all"
# the engine: sound, music, fishing, performance, effects, MIDI
${CC:-cc} $CFLAGS -Ifirmware -Iplatform/fm1-base/include firmware/bw_engine.c tests/engine_test.c -lm \
  -o build/tests/engine_test
build/tests/engine_test
# the art importer, on a synthetic sheet (never shipped)
python3 -I tests/art_test.py
# the clip player, on a synthetic tone (never shipped), against tools/adpcm.py's decoder
python3 tools/make_clip.py --test-tone build/tests/clip/tone.wav
python3 tools/make_clip.py build/tests/clip/bw_clip.h --wav build/tests/clip/tone.wav
python3 - build/tests/clip/tone.wav build/tests/clip/reference.raw <<'PY'
import struct, sys
sys.path.insert(0, 'tools')
import adpcm, make_clip
pcm, rate = make_clip.read(sys.argv[1])
pcm = [max(-32768, min(32767, v)) for v in make_clip.resample(pcm, rate)]
data, _ = adpcm.encode(pcm)
open(sys.argv[2], 'wb').write(struct.pack('<%dh' % len(pcm), *adpcm.decode(data, 0, len(pcm))))
PY
${CC:-cc} $CFLAGS -Ibuild/tests/clip tests/clip_test.c -o build/tests/clip_test
build/tests/clip_test build/tests/clip/reference.raw
# the app with that clip built in, on the host simulator
BW_CLIP_WAV="$PWD/build/tests/clip/tone.wav" ./build.sh --host
firmware/build/host/blackwater_host tests/clip_simulator.txt build/tests/clip
# the app as it ships (no clip unless one is supplied)
./build.sh --host
firmware/build/host/blackwater_host tests/simulator.txt build/tests
