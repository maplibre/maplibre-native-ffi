# Browser Map Example

This example runs the Rust map demo in a browser through Emscripten and WebGPU.
It uses the Rust binding for map and render-session access, and follows the map
example specification's desktop profile where the browser platform exposes
equivalent concepts.

## Running

```bash
mise -E browser-wasm64-webgpu run //examples/browser-map:run
```

The page renders through an owned texture session. A small page-owned WebGPU
compositor samples the map texture into the canvas. Optional `lat`, `lon`,
`zoom`, `bearing`, and `pitch` query parameters override the initial camera for
manual smoke testing. Add `benchmark=1` to run a 30-second city-jump camera
benchmark that reports frame timing to the console and
`window.maplibreBrowserMapBenchmark`.

## Desktop Profile Coverage

- The map uses the shared OpenFreeMap Bright style URL.
- The initial camera is San Francisco at zoom `13.0`, bearing `12.0`, and pitch
  `30.0`.
- The browser display refresh loop uses `requestAnimationFrame`, pumps the
  runtime once per frame, drains runtime events, and renders only when a render
  update is pending.
- The canvas is resizable through browser layout and device-pixel-ratio changes.
- Startup logs include the supported render backend mask, render-target status
  line, and viewport diagnostics.
- The page is a full-window map canvas.
- Desktop controls are implemented for pointer drag, wheel zoom, keyboard pan,
  zoom, rotate, pitch, and orientation reset.
- The browser example covers `owned-texture`.

## Browser-Specific Deviations

- Entry uses a fixed owned-texture render target instead of a required
  positional CLI argument. Browser pages do not have process arguments, and this
  first browser example focuses on the practical integration path.
- The browser WebGPU device is created by JavaScript through the browser
  `navigator.gpu` API and imported with Emscripten's `emdawnwebgpu` glue.
  Texture sessions require the renderer and compositor to use the same browser
  `GPUDevice`.
- `native-surface` is not covered in this browser example. The native WebGPU
  surface path needs synchronous adapter/device setup through Dawn `WaitAny`,
  which requires Asyncify or JSPI in browser WASM. This build uses
  `-fwasm-exceptions` and no Asyncify because Asyncify failed the Rust
  Emscripten link, so the example uses texture sessions as the practical browser
  integration path.
- `borrowed-texture` is not covered in this browser example. Browser host
  integrations that sample a texture are represented by the owned texture path
  here; broader browser interop can be added once there is a concrete host
  integration that needs borrowed texture ownership.
- The texture path models a host app that already owns a WebGPU canvas and
  samples the map texture with a fullscreen triangle shader.
- The texture path calls the render-update path once per browser display tick so
  the visual smoke test keeps the sampled texture current in browsers.
- Browser runtime work is single-threaded. Emscripten pthreads require
  cross-origin isolation and a pthread-enabled Rust standard library, which this
  browser example does not have. The browser build installs a single-threaded
  scheduler and uses browser `fetch` for network resources.
- Shutdown is handled by browser page teardown instead of an explicit window
  close event and process exit. The example closes and recreates map state when
  it is initialized again in the same page.
- Browser resource loading supports network requests for the live OpenFreeMap
  style. Cache-only, asset, filesystem, MBTiles, PMTiles, and resource-transform
  sources are not constructed in the browser build because the default sources
  own worker threads or platform storage that are not available in the current
  WASM environment.
- The browser texture compositor presents the sampled map color with alpha
  `1.0`. Root cause: the texture-session contract guarantees the rendered color
  target contents, but it does not define final presentation alpha for an opaque
  map viewport. In this browser fake-host path, forwarding the sampled alpha
  into the canvas compositor makes valid RGB output present as black. The demo
  clamps only the final presentation alpha because changing core rendering would
  broaden the texture-session ABI contract beyond what the other backends
  require.
- Control help is intentionally omitted from the page so the browser example is
  a visual map smoke test rather than an instructional UI.
