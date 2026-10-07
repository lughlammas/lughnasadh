#!/usr/bin/env bash
# Cross-compile Lughnasadh for Android arm64-v8a (PIE → liblughnasadh.so)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NDK="${ANDROID_NDK:-${ANDROID_NDK_HOME:-}}"
if [[ -z "$NDK" || ! -f "$NDK/build/cmake/android.toolchain.cmake" ]]; then
  echo "Set ANDROID_NDK to an extracted NDK (e.g. r27c)." >&2
  exit 1
fi
API="${ANDROID_PLATFORM:-android-28}"
BUILD="$ROOT/build-android"
cmake -S "$ROOT" -B "$BUILD" \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM="$API" \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j"$(nproc 2>/dev/null || echo 4)"
STRIP="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
OUT="${1:-$ROOT/android/out/liblughnasadh.so}"
mkdir -p "$(dirname "$OUT")"
"$STRIP" --strip-unneeded "$BUILD/lughnasadh" -o "$OUT"
chmod 755 "$OUT"
echo "Wrote $OUT ($(wc -c < "$OUT") bytes)"
readelf -h "$OUT" | grep -E 'Class|Type|Machine' || true
strings "$OUT" | grep -F 'Lughnasadh 0.4.0' || true
