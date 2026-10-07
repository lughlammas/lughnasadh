#!/usr/bin/env bash
# Sign a locally generated APK. Credentials remain local environment variables.
set -euo pipefail
IN="${1:?unsigned apk}"
OUT="${2:?signed apk}"
: "${KEYSTORE:?Set KEYSTORE to a local keystore}"
: "${KEY_ALIAS:?Set KEY_ALIAS}"
: "${KEYSTORE_PASS:?Set KEYSTORE_PASS}"
: "${KEY_PASS:?Set KEY_PASS}"
ALIGNED="$(mktemp --suffix=.apk)"
trap 'rm -f "$ALIGNED"' EXIT
zipalign -f -p 4 "$IN" "$ALIGNED"
apksigner sign --ks "$KEYSTORE" --ks-pass env:KEYSTORE_PASS --key-pass env:KEY_PASS \
  --ks-key-alias "$KEY_ALIAS" --v1-signing-enabled true --v2-signing-enabled true \
  --v3-signing-enabled true --out "$OUT" "$ALIGNED"
apksigner verify --verbose "$OUT"
