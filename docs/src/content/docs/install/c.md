---
title: C
description: Link the C library into a project.
---

## Install

Each [release](https://github.com/maplibre/maplibre-native-ffi/releases/latest)
publishes the C library as `maplibre-native-c-<preset>.tar.gz`, one archive for
each preset. A preset names a platform, an architecture, and a
[backend](/maplibre-native-ffi/install/#choose-a-backend), such as
`macos-arm64-metal` or `linux-gnu-x64-vulkan`. OpenGL presets name their context
instead of the backend: `egl`, `wgl`, or `webgl`.

Download the archive for your preset and unpack it. The directory inside,
`maplibre-native-c-<preset>`, is the install prefix.

With CMake, point `maplibre-native-c_DIR` at the prefix's package config, and
link the imported target:

```cmake title="CMakeLists.txt"
find_package(maplibre-native-c CONFIG REQUIRED)
target_link_libraries(app PRIVATE maplibre-native-c::maplibre-native-c)
```

```bash frame="terminal"
cmake -S . -B build -Dmaplibre-native-c_DIR=<prefix>/lib/cmake/maplibre-native-c
cmake --build build
```

To link the static library instead, use
`maplibre-native-c::maplibre-native-c-static`.

With another build system, use pkg-config:

```bash frame="terminal"
export PKG_CONFIG_PATH=<prefix>/share/pkgconfig
cc -std=c23 main.c $(pkg-config --cflags --libs maplibre-native-c) -o main
```

On Windows, put `<prefix>/bin` on `PATH`, or copy `maplibre-native-c.dll` next
to the executable. On other platforms, the executable finds the library through
the rpath that pkg-config adds.

## Package managers

### xmake

With xmake 3.1.2 or later, add the distribution repository at a release's tag:

```lua title="xmake.lua"
add_repositories("maplibre https://github.com/maplibre/maplibre-native-ffi-distribution.git v{{version}}")
add_requires("maplibre-native-c {{version}}", {configs = {backend = "vulkan"}})

target("app")
    set_kind("binary")
    add_files("main.c")
    add_packages("maplibre-native-c")
```

Leave out `backend` to use the platform's default. For musl on Linux, add
`libc = "musl"` to `configs`.

### vcpkg

Register the distribution repository as a Git registry. vcpkg takes a commit
hash as the baseline, not a tag, so look up the commit of the release's tag:

```bash frame="terminal"
git ls-remote https://github.com/maplibre/maplibre-native-ffi-distribution 'refs/tags/v{{version}}^{}'
```

```json title="vcpkg-configuration.json"
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/maplibre/maplibre-native-ffi-distribution",
      "baseline": "<commit>",
      "packages": ["maplibre-native-c"]
    }
  ]
}
```

Choose one backend feature, `opengl`, `metal`, or `vulkan`, and link the CMake
target as above:

```json title="vcpkg.json"
{
  "dependencies": [{ "name": "maplibre-native-c", "features": ["vulkan"] }]
}
```

vcpkg also needs a baseline for its default registry. Keep your manifest's
`builtin-baseline`, or set `"default-registry": null` if you take no other vcpkg
packages.

### Conan

With Conan 2, clone the distribution repository and add it as a local recipe
index. Pull the clone to see newer releases.

```bash frame="terminal"
git clone https://github.com/maplibre/maplibre-native-ffi-distribution.git
conan remote add maplibre-native-ffi ./maplibre-native-ffi-distribution --type=local-recipes-index
conan install --requires="maplibre-native-c/{{version}}" --build=missing \
  -o "maplibre-native-c/*:backend=vulkan" -g CMakeDeps -g CMakeToolchain
```

`backend` takes `metal`, `opengl`, `vulkan`, `webgpu`, or `default`. For musl on
Linux, add `-o "maplibre-native-c/*:libc=musl"`. On Windows, pass
`-s compiler.runtime=static`.

### Swift Package Manager

For Apple platforms, `MaplibreNativeC.xcframework.zip` holds the library as the
framework `MaplibreNativeC`. Add it to a Swift package as a binary target, with
the zip's checksum from the release's `SHA256SUMS`:

```swift title="Package.swift"
.binaryTarget(
  name: "MaplibreNativeC",
  url: "https://github.com/maplibre/maplibre-native-ffi/releases/download/v{{version}}/MaplibreNativeC.xcframework.zip",
  checksum: "<checksum>"
)
```

C code includes `<MaplibreNativeC/maplibre_native_c.h>`.

### Gradle

For Android, native code can link the library from a Kotlin runtime AAR, through
[Prefab](https://developer.android.com/build/native-dependencies):

```kotlin title="build.gradle.kts"
android {
  buildFeatures {
    prefab = true
  }
}

dependencies {
  implementation("org.maplibre.nativeffi:maplibre-native-ffi-runtime-opengl-android:{{version}}")
}
```

```cmake title="CMakeLists.txt"
find_package(maplibre-native-c REQUIRED CONFIG)
target_link_libraries(app PRIVATE maplibre-native-c::maplibre-native-c)
```

## Android

Initialize the library once per process, before you create a runtime, with the
calling thread's JNI environment and an Android `Context`:

```c title="android.c"
mln_android_init(jni_env, context, NULL);
```

## Snapshots

The
[snapshot release](https://github.com/maplibre/maplibre-native-ffi/releases/tag/unstable-native-snapshot)
holds the same archives, built from `main` daily. The prefix's
`share/maplibre-native-c/artifact.json` records the commit that it was built
from.
