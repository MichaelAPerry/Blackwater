#!/bin/sh
set -eu
cd "$(dirname "$0")"
[ "$#" -gt 0 ] || set -- --host
case "$1" in
  --host|--web) exec python3 platform/fm1-base/tools/build.py firmware "$@" ;;
  *) echo 'Device packaging is not enabled: assign a unique firmware identity and validate the hardware toolchain first.' >&2; exit 2 ;;
esac
