#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# The three JieLi AC79 SDK files a BLACKWATER package needs (fm1-base's tools/fm1pkg_make.py; no Bluetooth or
# WiFi, so nothing else of the SDK is used), fetched one by one from the official repository at the pinned SDK
# commit (tag AC79NN_SDK_V1.2.1_2023-12-13) and checked against fm1-base's SDK_SHA256 (tools/build.py).
# For when a whole-repository clone of gitee is not available; a full checkout works the same way (AC79_SDK).
#   tools/get_sdk_files.sh [DEST]     (default: build/ac79-sdk) -> AC79_SDK=DEST
set -eu
DEST="${1:-build/ac79-sdk}"
REV=d179b4484759423312073f5fbb232501aa491047
BASE="https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/raw/$REV/cpu/wl82/tools"
mkdir -p "$DEST/cpu/wl82/tools/cfg"
while read -r sha rel; do
    out="$DEST/cpu/wl82/tools/$rel"
    i=0
    until [ -s "$out" ] && echo "$sha  $out" | sha256sum -c --quiet - 2>/dev/null; do
        i=$((i + 1))
        [ "$i" -le 8 ] || { echo "get_sdk_files: $rel: no verified copy after 8 tries" >&2; exit 1; }
        rm -f "$out"
        curl -fsSL -m 60 -o "$out" "$BASE/$rel" || sleep $((i * 2))
    done
    echo "ok $rel $sha"
done <<EOF
4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4 uboot.boot
276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b cfg_tool.bin
41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d cfg/eq_cfg_hw.bin
EOF
echo "AC79_SDK=$DEST"
