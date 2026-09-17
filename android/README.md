# Android engine packaging (optional)

The 0.1.0 reference APK used Chess Engine style packaging:

- Native library: `lib/arm64-v8a/liblughnasadh.so` (UCI binary built as a shared object / PIE executable consumed by the host app)
- Resource: `res/xml/enginelist.xml` with name **Lughnasadh 0.1.0 First Harvest**

For **0.2.0 Second Harvest**, update the display name and ship an arm64 build of this engine.

## Build arm64 with NDK (outline)

```bash
# Example — adjust NDK path / API level
export ANDROID_NDK=$HOME/Android/Sdk/ndk/<version>
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-android -j
```

Rename/copy the binary to `liblughnasadh.so` under `lib/arm64-v8a/` inside an APK that declares the engine in `enginelist` (name: `Lughnasadh 0.2.0 Second Harvest`, filename: `liblughnasadh.so`, target: `arm64-v8a`).

## enginelist (conceptual)

```xml
<enginelist>
  <engine
    name="Lughnasadh 0.2.0 Second Harvest"
    filename="liblughnasadh.so"
    target="arm64-v8a" />
</enginelist>
```

A full signed APK is nice-to-have; the Linux UCI binary is the required deliverable for 0.2.0.
