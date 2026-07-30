# Compose Web map texture-sharing prototype

This prototype renders MapLibre Native **inside** a normal Compose Multiplatform
Web scene. MapLibre and Compose share one browser `WebGLTexture`: MapLibre
renders into it and Skia samples it in the same GPU command stream. Pixels stay
on the GPU, and the map participates in Compose drawing, hit testing, clipping,
transforms, alpha, shadows, borders, layout, and pointer input.

The example intentionally reaches through generated Skiko module glue. It is a
proof of the integration seam needed from Skiko, rather than production API.

## Result

The working pipeline is:

```text
Compose Canvas modifier node
        |
        v
Compose/Skiko WebGL2RenderingContext
        |
        +-- WebGLTexture object -- registered in --> MapLibre Emscripten GL table
        |                                |
        |                                v
        |                    MapLibre Native OpenGL borrowed-texture session
        |                                |
        |                                v
        |                         renders into texture
        |
        +-- same object -------- registered in --> Skiko Emscripten GL table
                                         |
                                         v
                              Skia BackendTexture / Image
                                         |
                                         v
                              Compose drawIntoCanvas()
```

There is one browser WebGL context and one browser texture object. The integer
texture names passed to C++ and Skia are local handles in two separate
Emscripten object tables; they resolve to that same JavaScript `WebGLTexture`.

The component demonstrates ordinary Compose modifiers:

- responsive Compose layout and GPU texture reallocation;
- `graphicsLayer` rotation and alpha;
- rounded `shadow`, `clip`, and `border`;
- pointer input after transformed Compose hit testing;
- drag-to-pan and wheel-to-zoom.

## Why WebGL, despite the WebGPU prototype

[PR 285](https://github.com/maplibre/maplibre-native-ffi/pull/285) proves that
MapLibre Native can produce a `GPUTexture` in a browser. The standard Compose
Web renderer used here is Skiko's WebGL/Ganesh renderer, however. Browser APIs
do not provide an identity-preserving import from a WebGPU `GPUTexture` to a
`WebGLTexture`. A WebGPU MapLibre target therefore cannot become a Skia WebGL
image without copying or changing Compose's renderer.

Building MapLibre's OpenGL backend as WebGL2 aligns both renderers on the one
graphics API that current standard Compose Web already owns. Sharing the host
context also gives ordering for free on the browser thread: MapLibre finishes
its draw calls, Skia resets its cached GL state, and Compose samples the result.

## Approaches evaluated

| Approach                                               | GPU-only                      | Inside Compose draw tree | Standard libraries | Finding                                                                                                |
| ------------------------------------------------------ | ----------------------------- | ------------------------ | ------------------ | ------------------------------------------------------------------------------------------------------ |
| DOM canvas over/under Compose                          | Yes                           | No                       | Yes                | Useful for platform views, but CSS composition cannot provide Compose modifier or draw-order fidelity. |
| CPU readback into a Skia image                         | No                            | Yes                      | Yes                | Violates the GPU-resident requirement and adds a frame-sized transfer.                                 |
| `GPUTexture` from MapLibre WebGPU into Compose WebGL   | No portable path              | Potentially              | Yes                | WebGPU and WebGL have no standard shared-texture import API.                                           |
| `ImageBitmap`, `VideoFrame`, or canvas transfer bridge | Implementation-dependent copy | Potentially              | Yes                | Does not preserve texture identity; synchronization and copies are browser-controlled.                 |
| CanvasKit/Skia WebGPU or a custom Compose renderer     | Potentially                   | Yes                      | No                 | Current Skiko Compose Web uses WebGL; switching its backend requires upstream work or a fork.          |
| Separate WebGL contexts                                | No portable zero-copy path    | Potentially              | Yes                | WebGL object sharing is scoped to one context in the browser API.                                      |
| Shared Compose WebGL context and texture               | **Yes**                       | **Yes**                  | **Yes**            | Implemented by this prototype.                                                                         |

## Integration details

The native side adds an Emscripten WebGL context provider to the C render-target
descriptor and builds the existing MapLibre OpenGL borrowed-texture session for
WebAssembly. The host imports Compose's `WebGL2RenderingContext` with
`GL.registerContext` and imports the shared texture into the native Emscripten
module's `GL.textures` table.

The Compose side discovers the canvas in Compose's shadow DOM, obtains its
existing WebGL2 context, allocates the texture, and adopts it as a Skia
`BackendTexture`. Calling `DirectContext.resetGLAll()` after MapLibre rendering
invalidates Skia's cached GL state before the image draw. In the other
direction, the borrowed MapLibre backend uses `ContextMode::Shared`, which
invalidates MapLibre's cached state before every command encoder. It also drains
pending WebGL errors left by host extension probing so MapLibre allocation
checks observe only errors from their own GL calls.

Compose 1.11.1 uses Skiko 0.144.6. That version does not expose its
`DirectContext` or texture table, so the Gradle build makes a narrow, checked
edit to the generated `skiko.mjs` file. It captures the context pointer and
publishes Skiko's existing Emscripten GL table. The installed Compose and Skiko
artifacts remain unchanged. This is the Web counterpart of the desktop
prototype's reflection onto Skiko internals.

Skiko upstream added public Wasm texture adoption hooks in
[JetBrains/skiko#1219](https://github.com/JetBrains/skiko/pull/1219). Once a
Compose release carries a mutually compatible Skiko runtime with those hooks,
the generated-glue edit and internal `DirectContext` wrapper can become public
API calls; the MapLibre WebGL and texture-sharing architecture remains the same.

## Build and run

From the repository root:

```bash
mise run //examples/compose-web-map:build
mise run //examples/compose-web-map:run
```

Open <http://localhost:8080>. The server supplies cross-origin isolation headers
needed by the WebAssembly runtime.

The two halves can also be built directly:

```bash
cmake --workflow --preset browser-wasm32-webgl
./gradlew :examples:compose-web-map:wasmJsBrowserDevelopmentExecutableDistribution
```

## Prototype boundaries and production follow-up

- The example runs MapLibre and Compose on the browser main thread and renders
  continuously with Compose's frame clock. A production component can connect
  MapLibre render-request events to Compose invalidation for idle efficiency.
- The generated Skiko glue edit is pinned by exact-symbol checks, so a changed
  Skiko module fails the build visibly. Public Skiko texture-adoption APIs are
  the intended replacement.
- Reflected `DirectContext` wrappers are retained for the page lifetime so they
  cannot dispose Compose's contexts. The prototype follows pointer changes when
  Compose recreates its context after a browser backing-canvas resize. A public
  borrowed-context accessor would express this ownership directly.
- Resize destroys the MapLibre borrowed session before Skia closes the image
  that owns the old texture. Old Skia images remain alive for eight Compose
  frames because recorded graphics layers can retain them. This ordering keeps
  the texture valid for both native session access and deferred Compose draws.
- Map destruction and final page teardown follow the source shared with the
  browser-map prototype. A reusable component should expose explicit disposal
  through `DisposableEffect` and add a native shutdown export.
- Browsers without WebGL2 cannot run this path. WebGPU becomes attractive when
  standard Compose Web itself gains a WebGPU-backed Skia renderer and a public
  texture-adoption API for that backend.
