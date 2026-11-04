#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
git submodule update --init --recursive
python3 -m venv build/venv
build/venv/bin/python -m pip install --disable-pip-version-check -r requirements-dev.txt
PATH="$PWD/build/venv/bin:$PATH" ./build.sh --host
