#!/bin/sh
# BLACKWATER build: fm1-base's tools/build.py on firmware/.
#   ./build.sh                 device package firmware/build/blackwater.fwsc (needs JIELI_TOOLCHAIN and AC79_SDK)
#   ./build.sh --release X.Y   release package firmware/build/blackwater-X.Y.fwsc (identity FM-1_77XXYYZZ)
#   ./build.sh --host          desktop simulator firmware/build/host/blackwater_host
#   ./build.sh --web           fm1-base's WASM app preview
set -eu
cd "$(dirname "$0")"
case "${1:-}" in
  --host|--web) ;;
  *)
    : "${JIELI_TOOLCHAIN:?set JIELI_TOOLCHAIN to the JieLi Linux toolchain (platform/fm1-base/tools/get_toolchain.sh)}"
    : "${AC79_SDK:?set AC79_SDK to an AC79 SDK (AC79NN_SDK_V1.2.1_2023-12-13) holding cpu/wl82/tools/}" ;;
esac
exec python3 platform/fm1-base/tools/build.py firmware "$@"
