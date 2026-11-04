#!/bin/sh
# Build the existing emulator, not a BLACKWATER-specific emulator implementation.
set -eu
cd "$(dirname "$0")/.."
ROOT="$PWD"
TOOLS="${BLACKWATER_TOOLS_DIR:-/workspace/.blackwater-tools}"
if [ -x "$TOOLS/cargo/bin/cargo" ]; then
    export CARGO_HOME="$TOOLS/cargo" RUSTUP_HOME="$TOOLS/rustup"
    export PATH="$CARGO_HOME/bin:$PATH"
fi
command -v cargo >/dev/null || { echo 'Install Rust and the wasm32-unknown-unknown target first.' >&2; exit 1; }
export CARGO_BUILD_JOBS="${CARGO_BUILD_JOBS:-4}"
# The project's tested toolchain is recorded in emulator-toolchain.txt.
export RUSTUP_TOOLCHAIN="$(cat tools/emulator-toolchain.txt)"
cargo build --locked --release --manifest-path platform/fm1-emu/Cargo.toml -p fm1-emu
# Upstream build writes only this generated directory; never pass it a source directory.
bash platform/fm1-emu/web/build.sh "$ROOT/build/emulator-web"
