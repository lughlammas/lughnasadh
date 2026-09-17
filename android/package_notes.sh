#!/usr/bin/env bash
# Helper notes — does not produce a full Play Store APK by itself.
set -euo pipefail
echo "Build Linux first: cmake -B build && cmake --build build -j"
echo "For arm64-v8a, use Android NDK toolchain (see android/README.md)."
echo "Place liblughnasadh.so in lib/arm64-v8a/ and set enginelist name to"
echo "  Lughnasadh 0.2.0 Second Harvest"
