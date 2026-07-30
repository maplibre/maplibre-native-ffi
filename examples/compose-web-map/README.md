# Compose/Wasm WebGPU compositing prototype

This prototype renders MapLibre Native with its WebGPU backend and draws the
result as a Skia `Image` inside an ordinary Compose Multiplatform/Wasm `Canvas`.
The page has one DOM canvas: Compose's. Map pixels participate in Compose
drawing instead of using a positioned map canvas.

The proof works in Chromium with Experimental Web Platform Features enabled. It
uses the proposed Canvas2D/WebGPU transfer API, which is not a stable web
standard yet. There is currently no portable stable-browser solution that
satisfies all of this prototype's constraints.

## Result

`MapLibreWebGpuMap` is a regular composable. The demo deliberately applies:

- Compose layout and pointer hit testing
- rotation, scaling, and alpha
- a rounded clip, border, and shadow
- Compose content drawn above the map
- Compose stripes and text drawn below the translucent map layer

The demo button removes and reapplies the map's alpha, rotation, scale, rounded
clip, border, and shadow at runtime. With modifiers enabled, the purple
`BELOW MAP TEXTURE` label is visible through the map while the magenta
`ABOVE MAP TEXTURE` label remains in front. This makes the scene ordering
visible without inspecting browser internals.

Drag and wheel input arrives through Compose and uses coordinates transformed
into the component's local space. MapLibre never owns a visible DOM element. The
implementation uses the Compose 1.11.1 dependency graph unchanged, including its
stock Skiko 0.144.6 artifacts.

## One-copy GPU data path

```text
Canvas2D transferToGPUTexture(requireZeroCopy = true)
        |
        | import the frame-scoped GPUTexture and view through emdawnwebgpu
        v
MapLibre Native WebGPU renders directly into the borrowed target
        |
        | clear native target, then transferBackFromGPUTexture
        v
accelerated Chromium Canvas2D SharedImage
        |
        | one WebGL2 texSubImage2D(canvas) GPU image copy
        v
Compose-context WebGLTexture
        |
        | Skia BackendTexture / Image (no pixel copy)
        v
Compose Canvas draw + normal modifiers
```

The C API can now replace or clear the target of an attached WebGPU
borrowed-texture session. Each browser frame borrows Chromium's GPU-backed
Canvas2D texture, imports it through emdawnwebgpu, gives it to MapLibre as the
color target, renders, clears the native reference, and returns it to Canvas2D.
This removes the earlier fullscreen WebGPU copy from MapLibre's owned texture.

Chromium requires a Canvas2D WebGPU borrow to be completed in the same browser
task. The frame pump therefore borrows, attempts a native render, clears, and
transfers back synchronously. A render attempt with no new MapLibre frame
returns the texture without uploading it to Compose.

After `transferBackFromGPUTexture`, Chromium's accelerated Canvas2D backing is a
SharedImage. `texSubImage2D` copies it into a `WebGLTexture` owned by Compose's
WebGL context. That texture is inserted into Emscripten GL's texture table and
wrapped as a stock Skia `Image`.

The application does not call `copyTextureToBuffer`, `GPUBuffer.mapAsync`,
`WebGL.readPixels`, `CanvasRenderingContext2D.getImageData`, or another pixel
readback API. The browser test used during development instruments all four APIs
and observes zero calls while initial rendering, panning, and zooming.

This is one-copy, GPU-resident compositing rather than end-to-end zero-copy
compositing. Web APIs do not provide a cross-browser promise that every
CanvasImageSource upload stays on the GPU; the no-readback conclusion depends on
Chromium's accelerated SharedImage implementation. `requireZeroCopy = true` does
give an explicit zero-copy guarantee for the MapLibre target-to-Canvas2D
boundary.

## Hardware validation and performance

The final live validation used a Release native build in headless Chromium 140
on this host's Intel Core i5-10500T and integrated UHD Graphics 630. Chromium
reported a WebGPU adapter with `vendor=intel` and `architecture=gen-9`.
Compose's WebGL renderer used ANGLE over Vulkan on the same Intel device and
Mesa 26.0.3; the kernel driver was `i915`.

During a pan-and-zoom validation run:

- one DOM canvas was present
- 168 instrumented MapLibre frames produced 168 WebGL uploads, plus one frame
  rendered before instrumentation
- all CPU-readback API counters remained zero
- a settled two-second animation-frame sample produced 121 frames, with a 16.68
  ms median and 16.88 ms p95 interval

These measurements prove the path and show that it can sustain the display's 60
Hz cadence on this older integrated GPU. They are not a MapLibre backend
benchmark: the workload is one style and viewport, includes browser and Compose
work, and does not compare identical scenes against Metal or Vulkan.

## Run

Build the native WebGPU target and the Compose application, then start the
development server:

```bash
mise run //examples/compose-web-map:run
```

Open the printed URL in Chromium with Experimental Web Platform Features. The
hardware-accelerated Linux validation used:

```bash
chromium \
  --enable-unsafe-webgpu \
  --enable-experimental-web-platform-features \
  --enable-features=Vulkan \
  --use-gl=angle \
  --use-angle=vulkan \
  http://localhost:8080
```

Chrome's WebGPU availability and required flags vary by release and Linux GPU
allowlist. The Canvas2D transfer API remains the prototype's experimental
requirement. Without it, the component displays a capability error instead of
falling back to CPU pixels or a DOM overlay.

## MapLibre WebGPU backend status

MapLibre's WebGPU renderer is a full renderer rather than a thin experiment. Its
initial merge covered background, fill, fill extrusion, symbol, circle, line,
hillshade, raster, and heatmap layers, and reported all 1,266 macOS render tests
passing. Dawn and wgpu-native implementations share the renderer. The browser
build uses emdawnwebgpu, which maps the same `webgpu.h` API onto the browser's
WebGPU implementation.

The backend remains younger than Metal, Vulkan, and OpenGL. It has less
deployment history, location indicators and custom drawables were untested at
the initial merge, and a render-thread cleanup omission that could leak during
heavy tile churn was fixed in July 2026. Visual quality for ordinary vector and
raster styles is well covered; production maturity and backend-to-backend
performance equivalence still need broader measurement.

## Why end-to-end zero-copy is blocked

- Compose 1.11.1's browser `SkiaLayer` creates a WebGL context; it has no WebGPU
  renderer or `GPUDevice`.
- WebGPU defines no conversion or import from `GPUTexture` to `WebGLTexture`.
- `GPUTexture` is not a WebGL `TexImageSource`.
- Standard Compose exposes neither a GPU device-sharing hook nor a public
  browser external-texture composable.
- MapLibre must keep using WebGPU, so rendering it through WebGL is outside the
  experiment.

The only Skiko-runtime adaptation is the small `pushSkikoTexture` helper, which
reproduces Emscripten GL's texture-table insertion so the stock Skia WASM module
can see a `WebGLTexture` already owned by its context. This is analogous to the
Skiko internal reflection used by the desktop prototype; it changes no Compose
or Skiko library.

The custom scene host is also built entirely from shipped Compose and Skiko
classes. It uses `CanvasLayersComposeScene` and `SkiaLayer` so it can retain the
root Skia `DirectContext` needed by `Image.adoptTextureFrom`. Compose's public
browser viewport wrapper does not currently expose that context.

## Approaches explored

| Approach                                       | Finding                                                                                                                                                                                             |
| ---------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Import MapLibre's `GPUTexture` into Skia       | Blocked. Browser Skia is using WebGL and has no WebGPU texture-import API.                                                                                                                          |
| Give MapLibre Compose's device                 | Blocked. Compose owns a WebGL context, not a `GPUDevice`.                                                                                                                                           |
| Render MapLibre directly into Canvas2D texture | Implemented. A frame-scoped borrowed-target API removes the earlier WebGPU shader copy.                                                                                                             |
| Convert `GPUTexture` to `WebGLTexture`         | Blocked. No standard WebGPU/WebGL import extension exists.                                                                                                                                          |
| WebGPU canvas → `ImageBitmap` → WebGL          | Chromium can keep texture-backed bitmaps accelerated, and this rendered successfully, but detached-canvas presentation/snapshot timing was unreliable and the API cannot require a GPU-only result. |
| WebGPU canvas directly as a WebGL image source | Has the same presentation and implementation-guarantee problems as the `ImageBitmap` route.                                                                                                         |
| Canvas2D/WebGPU shared texture → WebGL         | Implemented. `requireZeroCopy` makes the first boundary explicit; the WebGL upload is the one remaining image copy.                                                                                 |
| `VideoFrame` or another external-image wrapper | Adds another lifetime/copy boundary and provides no stronger GPU-residency guarantee.                                                                                                               |
| WebGPU buffer mapping or WebGL readback        | Rejected: pixels cross through CPU memory.                                                                                                                                                          |
| Put a WebGPU canvas over Compose               | Rejected: modifiers, scene ordering, effects, and transformed hit testing would be split across DOM layers.                                                                                         |
| Use MapLibre's OpenGL backend                  | Could share the WebGL API but violates the WebGPU-backend requirement.                                                                                                                              |
| Fork Compose/Skiko for a WebGPU renderer       | Rejected by the standard-library requirement.                                                                                                                                                       |

## Production implications

The prototype establishes that full Compose scene fidelity is possible today on
Chromium, but the experimental API prevents shipping it as a general web
component. A productionized Chromium bridge would pool textures, recover from
device/context loss, stop the continuous frame pump when idle, and add
release-build GPU timestamp benchmarks.

A portable, potentially zero-copy implementation needs upstream support:

1. Compose/Skiko renders its browser scene with WebGPU and exposes or accepts a
   `GPUDevice`.
2. Skia/Skiko can borrow an application `GPUTexture` as an image with explicit
   synchronization and non-owning lifetime semantics.

Until those exist, stable web standards cannot satisfy the complete constraint
set. The Canvas2D shared-texture proposal is the most promising bridge because
it solves the otherwise unexpressible WebGPU-to-browser-shared-image step
without a library fork or CPU readback.

## Implementation references

- [MapLibre's WebGPU renderer merge](https://github.com/maplibre/maplibre-native/pull/3838)
- [MapLibre's wgpu implementation and test coverage](https://github.com/maplibre/maplibre-native/pull/3899)
- [MapLibre's browser emdawnwebgpu support](https://github.com/maplibre/maplibre-native/pull/4370)
- [MapLibre's render-thread cleanup fix](https://github.com/maplibre/maplibre-native/pull/4367)
- [Skiko's browser `SkiaLayer` selects WebGL](https://github.com/JetBrains/skiko/blob/master/skiko/src/webMain/kotlin/org/jetbrains/skiko/SkiaLayer.web.kt)
- [Chromium's Canvas2D WebGPU transfer implementation](https://chromium.googlesource.com/chromium/src/+/fe487bfab3b23b7a107987b0a2f7b65222ae7ae0/third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.cc)
  implements `requireZeroCopy`, SharedImage usage, and transfer-back.
- [Chromium's WebGL CanvasImageSource implementation](https://chromium.googlesource.com/chromium/src/+/master/third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.cc)
  contains the accelerated image upload path used at the second API boundary.
- [Chrome's WebGPU roadmap](https://developer.chrome.com/blog/next-for-webgpu)
  describes Canvas2D-to-WebGPU transfer as a proposal rather than a shipped web
  standard.
