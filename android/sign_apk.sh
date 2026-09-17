#!/usr/bin/env bash
# zipalign + apksigner (v1+v2+v3). Requires build-tools on PATH.
set -euo pipefail
IN="${1:?unsigned apk}"
OUT="${2:?signed apk}"
KS="${KEYSTORE:-$HOME/.android/debug.keystore}"
ALIAS="${KEY_ALIAS:-androiddebugkey}"
STOREPASS="${KEYSTORE_PASS:-android}"
KEYPASS="${KEY_PASS:-android}"

if [[ ! -f "$KS" ]]; then
  KS="${3:-}"
  ALIAS="${4:-lughnasadh}"
fi
if [[ ! -f "$KS" ]]; then
  echo "Keystore not found. Pass path as \$3 or set KEYSTORE." >&2
  exit 1
fi

ALIGNED="$(mktemp --suffix=.apk)"
zipalign -f -p 4 "$IN" "$ALIGNED"
apksigner sign \
  --ks "$KS" --ks-pass "pass:$STOREPASS" --key-pass "pass:$KEYPASS" \
  --ks-key-alias "$ALIAS" \
  --v1-signing-enabled true --v2-signing-enabled true --v3-signing-enabled true \
  --out "$OUT" "$ALIGNED"
rm -f "$ALIGNED"
apksigner verify --verbose "$OUT"
echo "Signed $OUT"
