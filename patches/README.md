# Upstream patches

## `maplibre-native-emdawnwebgpu.patch`

Browser WebGPU support for
[maplibre-native](https://github.com/maplibre/maplibre-native) using
Emscripten's `emdawnwebgpu` port.

Base commit when generated: `f6d70e954b07fdadf6a5adda8da49e73178298c6`.

The patch currently carries these logical changes:

- `MLN_WEBGPU_EMDAWN` CMake/vendor wiring. This lets a Dawn WebGPU build use
  Emscripten's port-supplied WebGPU implementation instead of linking native
  Dawn or wgpu-native.
- Omit the native headless WebGPU backend for emdawn builds. That backend
  bootstraps native WebGPU and is not the browser device/surface model.
- Single-threaded browser scheduler support. The browser build runs without
  pthreads, so background/sequenced work is queued and drained on the browser
  thread.
- Deferred WebGPU dynamic texture creation. The WebGPU dynamic texture path
  needs to retain context, size, and pixel format until upload time, because the
  browser path cannot eagerly create every texture in the same way as native.
- Skip Dawn-native `wgpuDeviceTick` for emdawn readback. The browser WebGPU path
  does not use Dawn native device ticking.
- Initialize WebGPU uniform buffers with `wgpuQueueWriteBuffer` instead of
  `mappedAtCreation` plus `wgpuBufferUnmap`. This is a real bug fix found while
  stress-testing browser texture rendering: repeated zoom/render work trapped in
  Emscripten's `_emwgpuBufferUnmap` during uniform-buffer initialization. The
  rest of MapLibre's WebGPU buffer path already uploads initial buffer data with
  queue writes when a queue is available, and uniform-buffer updates already use
  queue writes. Render backends that have a WebGPU device are expected to set a
  queue on `RendererBackend`; if a queue is missing, that is a backend
  initialization bug rather than a separate upload mode.

The uniform-buffer change should be proposed upstream separately from the
emdawn/browser bootstrap because it is backend-agnostic WebGPU cleanup. The
emdawn-specific pieces can be split into smaller PRs around build wiring,
scheduler/runtime behavior, dynamic texture lifetime, and native-Dawn API
guards.

### Apply

From a `maplibre-native` checkout:

```bash
git apply /path/to/maplibre-native-emdawnwebgpu.patch
# or, from this repo:
git apply "$MLN_FFI_REPO_ROOT/patches/maplibre-native-emdawnwebgpu.patch"
```

Configure a browser build:

```bash
cmake -DMLN_WITH_WEBGPU=ON -DMLN_WEBGPU_IMPL_DAWN=ON -DMLN_WEBGPU_EMDAWN=ON ...
# Emscripten link flags must include --use-port=emdawnwebgpu
```

Same commit on fork branch `cursor/webgpu-emdawn-minimal-bcb7`
(`sargunv/maplibre-native`).
