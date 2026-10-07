#!/usr/bin/env bash
# Quick pointers for Android arm64 packaging (0.4.0 Fourth Harvest)
set -euo pipefail
cat <<'TXT'
1) Build Linux UCI:  cmake -B build && cmake --build build -j
2) Build arm64 .so:  ANDROID_NDK=/path/to/ndk ./android/build_arm64.sh
3) Package unsigned: python3 android/package_apk.py \
     --skeleton /path/to/extracted-0.1.0-apk \
     --so android/out/liblughnasadh.so \
     --ref-apk /path/to/Lughnasadh-0.1.0.apk \
     --out-dir build/apk-stage \
     --apk build/Lughnasadh-0.4.0-arm64-unsigned.apk
4) Sign:             ./android/sign_apk.sh unsigned.apk signed.apk /path/to.keystore lughnasadh

Display name in enginelist: Lughnasadh 0.4.0 Fourth Harvest
Native lib: lib/arm64-v8a/liblughnasadh.so
TXT
