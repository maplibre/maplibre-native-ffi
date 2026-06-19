---
title: Android Rust Platform Spike
description: Planned Android platform bring-up path using native Android sources, default MapLibre sources, and Rust platform components.
sidebar:
  order: 6
---

## Goal

Bring up an Android native build of `maplibre_native_c` without waiting on a
desktop-style native dependency provider. The spike treats Rust platform
components as the primary path for dependency-heavy Android platform behavior,
then leaves room to compare that path against a later vcpkg-based build.

The first milestone is a linked Android `maplibre_native_c` library for one
target, preferably `android-arm64-vulkan`. Rendering a map is a later milestone;
the first build may use Rust replacements for features that would otherwise
require curl, image codec libraries, or full ICU.

## Build Shape

- Add an Android mise environment for one ABI and render backend.
- Configure CMake with the Android NDK toolchain file and API 24.
- Add `cmake/platform/android.cmake` and select it from
  `cmake/mln_platform.cmake` when `CMAKE_SYSTEM_NAME` is `Android`.
- Keep the existing `MLN_WITH_CORE_ONLY` wrapper flow and attach platform
  sources to `maplibre_native_c`.
- Add a Rust static library target only after the C++ Android platform source
  list configures and links.

Rust integrates through a narrow C ABI. C++ platform shims continue to own
MapLibre types, exceptions, callbacks, and lifecycle. Rust functions receive
plain buffers and return plain structs or status codes.

## Component Plan

| Component                    | Required behavior                                                               | First Android implementation                                    | Rust candidate                            |
| ---------------------------- | ------------------------------------------------------------------------------- | --------------------------------------------------------------- | ----------------------------------------- |
| Platform source registration | Select Android source list and libraries from CMake                             | Custom `cmake/platform/android.cmake`                           | None                                      |
| Run loop                     | Drive MapLibre scheduled work on Android threads                                | `platform/android/src/run_loop.cpp`                             | None                                      |
| Async task                   | Post work into the Android run loop                                             | `platform/android/src/async_task.cpp`                           | None                                      |
| Timer                        | Schedule delayed and repeating tasks                                            | `platform/android/src/timer.cpp`                                | None                                      |
| Threading                    | Worker threads and thread local state                                           | Default `thread.cpp` and `thread_local.cpp`                     | None                                      |
| Monotonic time and wall time | Clock and timestamp support                                                     | Default `monotonic_timer.cpp` and `platform/time.cpp`           | None                                      |
| Logging                      | Emit MapLibre logs                                                              | Default `logging_stderr.cpp` first                              | None                                      |
| Filesystem                   | Read and write app-private paths                                                | Default filesystem and local file sources                       | None                                      |
| Asset source                 | Load non-network resources                                                      | Default filesystem-backed asset source first                    | Later Android `AAssetManager`, likely C++ |
| Database and offline cache   | Cache DB and offline region operations                                          | Default sources plus vendored SQLite                            | None                                      |
| Resource loader routing      | Route asset, local, database, network, PMTiles, and MBTiles requests            | Default sources plus existing custom manager                    | None                                      |
| C API resource provider      | Let bindings intercept network requests                                         | Existing `src/resources/resource_loader.cpp`                    | None                                      |
| Compression                  | Inflate and deflate compressed resource payloads                                | Default compression with NDK `libz`                             | `flate2` or `miniz_oxide`, low priority   |
| HTTP                         | Async request, cancellation, headers, ranges, cache metadata, and error mapping | Rust replacement                                                | `minreq` + `rustls`                       |
| PNG decode                   | Decode encoded bytes to premultiplied RGBA8                                     | Rust replacement                                                | `image`                                   |
| JPEG decode                  | Decode encoded bytes to RGBA8                                                   | Rust replacement                                                | `image`                                   |
| WebP decode                  | Decode encoded bytes to RGBA8                                                   | Rust replacement                                                | `image`                                   |
| PNG write                    | Encode readback or snapshot pixels to PNG                                       | Stub unless a test or API path needs it                         | `png` or `image`                          |
| Bidi and Unicode core        | Unicode text processing used by layout                                          | Default sources plus vendored `mbgl-vendor-icu` and `nunicode`  | None                                      |
| Collation                    | Compare strings with case and diacritic options                                 | Default `collator.cpp`                                          | Later ICU4X collator                      |
| Number formatting            | Implement style `number-format` expression                                      | `number_format.cpp` with `MBGL_USE_BUILTIN_ICU` degraded mode   | Later ICU4X decimal or number formatting  |
| Local glyph rasterizer       | Generate local CJK glyph bitmaps                                                | Default stub                                                    | No first-pass Rust target                 |
| Layer manager and factories  | Register MapLibre style layer factories                                         | Existing wrapper source list                                    | None                                      |
| Vulkan backend               | Render through Android Vulkan                                                   | Existing FFI Vulkan backend plus default Vulkan headless source | None                                      |
| OpenGL ES backend            | Render through Android EGL/GLES                                                 | Follow-up after Vulkan unless required first                    | None                                      |
| JNI bootstrap                | Initialize JVM-backed Android platform behavior                                 | Excluded from first pass                                        | None                                      |

## Rust Image Boundary

Start with image decoding because it is isolated and testable. A minimal C ABI
can look like this:

```c
typedef struct mln_rust_image {
  uint32_t width;
  uint32_t height;
  uint8_t* rgba;
  size_t rgba_len;
} mln_rust_image;

int32_t mln_rust_decode_image(
  const uint8_t* data,
  size_t data_len,
  mln_rust_image* out_image
);

void mln_rust_image_free(mln_rust_image* image);
```

The C++ Android image shim converts `mln_rust_image` into
`mbgl::PremultipliedImage`. The shim owns validation of null pointers, dimension
overflow, alpha premultiplication, and conversion errors into MapLibre
exceptions or diagnostics.

## Milestones

1. Android CMake configure succeeds for one ABI/backend.
2. `maplibre_native_c` links with Android platform source selection.
3. A minimal native smoke path calls a basic exported C API function.
4. Rust static library links into `maplibre_native_c`.
5. Rust PNG/JPEG/WebP decoding replaces native image codec dependencies.
6. HTTP is replaced by a Rust implementation behind the same MapLibre
   `FileSource` behavior.
7. ICU4X collation and number formatting are evaluated separately after image
   and HTTP behavior are stable.

## Open Decisions

- Whether the first backend is Vulkan only or also includes OpenGL ES.
- Whether Rust image decoding should use the unified `image` crate or
  lower-level per-format crates.
- Whether Rust HTTP should use blocking `ureq` on a Rust-owned thread pool or
  async `reqwest` with a runtime.
- How Android TLS trust should work if Rust HTTP uses `rustls`; static web PKI
  roots are portable, while Android's Network Security Config requires platform
  integration.
- How much locale data ICU4X should embed if it replaces degraded number
  formatting and default collation.
