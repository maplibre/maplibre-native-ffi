---
title: Android Rust Platform Spike
description: Android platform bring-up path using native Android sources, default MapLibre sources, and Rust platform components.
sidebar:
  order: 6
---

## Goal

Bring up an Android native build of `maplibre_native_c` without waiting on a
desktop-style native dependency provider. This spike treats Rust platform
components as the primary path for dependency-heavy Android platform behavior,
then leaves room to compare that path against a later vcpkg-based build.

The current target is a linked Android `maplibre_native_c` library for
`android-arm64-vulkan`. Rendering a map is a later milestone.

## Build Shape

- An Android mise environment selects one ABI and render backend.
- CMake configures with the Android NDK toolchain file and API 24.
- `cmake/platform/android.cmake` is selected from `cmake/mln_platform.cmake`
  when `CMAKE_SYSTEM_NAME` is `Android`.
- The existing `MLN_WITH_CORE_ONLY` wrapper flow attaches platform sources to
  `maplibre_native_c`.
- A Rust static library links into `maplibre_native_c` through a CMake custom
  command.

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
| Threading                    | Worker threads and thread local state                                           | Project-local Android thread implementation                     | None                                      |
| Monotonic time and wall time | Clock and timestamp support                                                     | Default `monotonic_timer.cpp` and `platform/time.cpp`           | None                                      |
| Logging                      | Emit MapLibre logs                                                              | Default `logging_stderr.cpp` first                              | None                                      |
| Filesystem                   | Read and write app-private paths                                                | Default filesystem and local file sources                       | None                                      |
| Asset source                 | Load non-network resources                                                      | Default filesystem-backed asset source first                    | Later Android `AAssetManager`, likely C++ |
| Database and offline cache   | Cache DB and offline region operations                                          | Default sources plus vendored SQLite                            | None                                      |
| Resource loader routing      | Route asset, local, database, network, PMTiles, and MBTiles requests            | Default sources plus existing custom manager                    | None                                      |
| C API resource provider      | Let bindings intercept network requests                                         | Existing `src/resources/resource_loader.cpp`                    | None                                      |
| Compression                  | Inflate and deflate compressed resource payloads                                | Default compression with NDK `libz`                             | `flate2` or `miniz_oxide`, low priority   |
| HTTP                         | Async request, cancellation, headers, ranges, cache metadata, and error mapping | Rust replacement                                                | `minreq` + Rustls                         |
| PNG decode                   | Decode encoded bytes to premultiplied RGBA8                                     | Rust replacement                                                | `image`                                   |
| JPEG decode                  | Decode encoded bytes to RGBA8                                                   | Rust replacement                                                | `image`                                   |
| WebP decode                  | Decode encoded bytes to RGBA8                                                   | Rust replacement                                                | `image`                                   |
| PNG write                    | Encode readback or snapshot pixels to PNG                                       | Default `png_writer.cpp`                                        | None                                      |
| Bidi and Unicode core        | Unicode text processing used by layout                                          | Default sources plus vendored `mbgl-vendor-icu` and `nunicode`  | None                                      |
| Collation                    | Compare strings with case and diacritic options                                 | Default `collator.cpp`                                          | Later ICU4X collator                      |
| Number formatting            | Implement style `number-format` expression                                      | `number_format.cpp` with `MBGL_USE_BUILTIN_ICU` degraded mode   | Later ICU4X decimal or number formatting  |
| Local glyph rasterizer       | Generate local CJK glyph bitmaps                                                | Default implementation                                          | No first-pass Rust target                 |
| Layer manager and factories  | Register MapLibre style layer factories                                         | Existing wrapper source list                                    | None                                      |
| Vulkan backend               | Render through Android Vulkan                                                   | Existing FFI Vulkan backend plus default Vulkan headless source | None                                      |
| OpenGL ES backend            | Render through Android EGL/GLES                                                 | Follow-up after Vulkan unless required first                    | None                                      |
| JNI bootstrap                | Initialize JVM-backed Android platform behavior                                 | Excluded from first pass                                        | None                                      |

## Rust Image Boundary

Image decoding is isolated behind a C ABI:

```c
typedef struct mln_rust_decoded_image {
  uint32_t width;
  uint32_t height;
  uint8_t* data;
  size_t data_len;
  char* error;
} mln_rust_decoded_image;

mln_rust_decoded_image mln_rust_decode_image(
  const uint8_t* data,
  size_t data_len
);

void mln_rust_decoded_image_free(mln_rust_decoded_image image);
```

The C++ Android image shim converts the Rust response into
`mbgl::PremultipliedImage`. Rust returns premultiplied RGBA8 bytes and owns
decoder errors until C++ copies them into MapLibre exceptions.

## Rust HTTP Boundary

HTTP uses a Rust-owned background thread per request. C++ owns MapLibre
`Resource`, `Response`, cancellation, and callback delivery; Rust owns
URL/header copying, the blocking HTTP request, TLS, and response buffers. The C
ABI passes plain request headers and returns status, body bytes, selected
response headers, and an error string.

## Milestones

1. Android CMake configure succeeds for one ABI/backend.
2. `maplibre_native_c` links with Android platform source selection.
3. Rust static library links into `maplibre_native_c`.
4. Rust PNG/JPEG/WebP decoding replaces native image codec dependencies.
5. HTTP is replaced by a Rust implementation behind the same MapLibre
   `FileSource` behavior.

## Open Decisions

- Whether to add OpenGL ES alongside Vulkan.
- How Android TLS trust should work with Rustls; static web PKI roots are
  portable, while Android's Network Security Config requires platform
  integration.
- How much locale data ICU4X should embed if it replaces degraded number
  formatting and default collation.
- Whether the Rust HTTP implementation should move from one thread per request
  to a shared worker pool after functional testing.
