#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests
${CC:-cc} -std=c99 -O2 -g -Wall -Wextra -Werror -Wno-unused-function \
  -ffp-contract=off -fsanitize=undefined -fno-sanitize-recover=all \
  -Ifirmware -Iplatform/fm1-base/include \
  firmware/bw_engine.c tests/engine_test.c -o build/tests/engine_test
build/tests/engine_test
./build.sh --host
firmware/build/host/blackwater_host tests/simulator.txt build/tests
