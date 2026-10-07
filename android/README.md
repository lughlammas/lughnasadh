# Android packaging — Lughnasadh 0.4.0 Fourth Harvest

Chess-for-Android / DroidFish style UCI engine APK:

- Native binary: `lib/arm64-v8a/liblughnasadh.so` (PIE executable named `.so`)
- Resource: `res/xml/enginelist.xml` with name **Lughnasadh 0.4.0 Fourth Harvest**
- Package: `com.lughnasadh.engine` (same skeleton as 0.1.0)

## Cross-compile arm64-v8a with Android NDK

```bash
export ANDROID_NDK=/path/to/android-ndk-r27c   # or later

cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-android -j

# Strip and rename for the APK layout
$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip \
  --strip-unneeded build-android/lughnasadh -o liblughnasadh.so
```

Verify:

```bash
readelf -h liblughnasadh.so   # ELF64, DYN (PIE), AArch64
strings liblughnasadh.so | grep 'Lughnasadh 0.4.0'
```

## Package APK

Prefer cloning the 0.1.0 APK skeleton (icons, `classes.dex`, provider) and replacing:

1. `lib/arm64-v8a/liblughnasadh.so`
2. `res/xml/enginelist.xml` (binary XML — see `package_apk.py`)
3. `AndroidManifest.xml` `versionName` → `0.4.0`

Then `zipalign` + `apksigner` (v1+v2+v3). Helper scripts:

- `build_arm64.sh` — NDK cmake build + strip
- `package_apk.py` — rebuild enginelist binary XML, stage tree, zip APK
- `sign_apk.sh` — zipalign + signing with explicitly configured local credentials

## enginelist (conceptual source)

```xml
<enginelist>
  <engine
    name="Lughnasadh 0.4.0 Fourth Harvest"
    filename="liblughnasadh.so"
    target="arm64-v8a" />
</enginelist>
```

## Notes

- Training / preparation framing only — classical UCI engine for analysis and practice.
- On Android, do **not** link `-lpthread` (already in Bionic); `CMakeLists.txt` guards this with `if(NOT ANDROID)`.

## Status

These are legacy source helpers updated to the 0.4.0 identity. APK packaging and signing have not been verified for this source snapshot. No APK, skeleton, or signing key is committed. Set `KEYSTORE`, `KEY_ALIAS`, `KEYSTORE_PASS`, and `KEY_PASS` locally before signing.
