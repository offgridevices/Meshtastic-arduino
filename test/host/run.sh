#!/bin/sh
# Compile and run the host-side tests.
#
# These build the library against a stub Arduino layer on a development
# machine, so behaviour that does not depend on a radio can be checked
# without one. Nothing here proves anything about real hardware.
#
# Usage:  sh test/host/run.sh

set -e

here=$(dirname "$0")
src="$here/../../src"
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

echo "building..."
g++ -std=c++17 -w \
  -I "$here" -I "$src" \
  -o "$out/test_packet_meta" \
  "$here/test_packet_meta.cpp" \
  "$src/mt_protocol.cpp" \
  "$src/pb_common.c" "$src/pb_decode.c" "$src/pb_encode.c" \
  "$src"/meshtastic/*.c

echo "running..."
"$out/test_packet_meta"
