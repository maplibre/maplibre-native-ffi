# Worklog

- Started goal: bring up `android-arm64-vulkan` for `maplibre_native_c` using
  Android/default MapLibre platform sources, stubs for dependency-heavy
  components, then Rust image decoding.
- Planning note committed in `db25a7b5` as
  `docs/src/content/docs/development/android-rust-platform-spike.md`.
- Current intended first slice: create Android build variant and CMake platform
  selection before adding Rust.
- Added initial Android variant/CMake plumbing and Android platform source
  module with HTTP and image decode stubs.
- Host has Android SDK at `~/Library/Android/sdk`, but no NDK was installed at
  goal start; Rust Android target was also not installed.
- Installed Android NDK `29.0.14033849`, installed Rust target
  `aarch64-linux-android`, and initialized MapLibre Native nested vendor
  submodules.
- `android-arm64-vulkan` CMake configure succeeds with the Android NDK
  toolchain.
- First Android build reached the final wrapper link area; failures were default
  Linux thread-name API usage on Android and clang-tidy parameter naming in new
  stubs. Switched to a project-local no-JNI Android thread implementation for
  the spike.
- `android-arm64-vulkan` build succeeds and links `libmaplibre-native-c.so` with
  Android/default platform sources plus HTTP/image stubs.
- Added a Rust workspace crate linked as a static library from the Android CMake
  platform path. The Rust `image` crate stack cross-compiles for
  `aarch64-linux-android` and provides PNG/JPEG/WebP decoding without native
  libpng/jpeg/webp dependencies.
- Replaced the Android image stub with a C++ bridge to Rust decode output; Rust
  returns RGBA bytes, C++ copies them into `PremultipliedImage`.
