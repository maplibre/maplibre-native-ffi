# The C API on Orbis (exploration)

`MLN_FFI_IMPLEMENTATION=orbis` builds `maplibre_native_c` from this crate. The
crate implements the headers in `include/` on
[Orbis](https://github.com/sargunv/orbis)'s embedding API, `orb-api`, instead of
the C++ core. The design follows maplibre-native-ffi PR 1589's switch for
maplibre-native-next:

- `types.rs` includes the Rust `-sys` crate's generated declarations, so the
  crate keeps no copy of the headers.
- `tools/gen_stubs.py` generates `stubs.rs`. Every function the crate doesn't
  implement returns `MLN_STATUS_UNSUPPORTED` there, so any caller of the full
  API links. The stubs file also checks the implemented functions against the
  headers' signatures.
- Each runtime is a thread that owns an `orb_api::Runtime` and that runtime's
  maps and sessions (all `!Send`). Calls check their arguments on the caller's
  thread, post their work to the runtime thread, and complete through their
  `mln_completion`.

## Build and run

```bash
cmake --preset macos-arm64-metal-orbis \
  -DMLN_FFI_ORBIS_SOURCE_DIR=/path/to/orbis \
  -DMLN_FFI_SOURCE_DIR=/path/to/maplibre-native   # the test harness's headers only
cmake --build build/macos-arm64-metal-orbis
ctest --test-dir build/macos-arm64-metal-orbis -R orbis/render
```

`tests/render_test.c` follows the "Render images" guide for a STATIC map and a
TILE map, then drives a CONTINUOUS map through frame demands. It reads all three
back as PNGs. The ABI suite and the exit programs also build and link.

## Status

51 of 227 functions are implemented:

- runtimes and the event handler;
- maps;
- style by URL and by JSON;
- camera jumps, eases, flights, deltas, fits and tile cameras;
- map snapshots;
- offscreen and CAMetalLayer sessions;
- frame demands, frame results and an Apple frame pacer on CVDisplayLink;
- still images and readback;
- delivery queues and logging.

## Open blockers

These are the blockers for the SwiftUI example's CAMetalLayer session on the
core-worker driver:

- wgpu-hal 30's macOS occlusion check (gfx-rs/wgpu#8309) sends AppKit messages
  from the thread that acquires the drawable, and blocks off the main thread.
- With that check patched out locally, `nextDrawable` still returns nil on every
  frame.
