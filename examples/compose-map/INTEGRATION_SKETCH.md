---
title: Compose map integration sketch
description: Working notes for a Compose Desktop map example built around Skiko texture sharing.
---

This document sketches a Compose Desktop map example that demonstrates a
general-purpose native rendering primitive for Compose Multiplatform. MapLibre
Native FFI is the first client, but the reusable part should also fit engines,
games, video renderers, and other native producers that need to present GPU
content inside Compose Desktop. It is a working note, not a complete
specification. The map example specification keeps the existing desktop and
mobile profiles; this example follows the broad map/runtime/input lifecycle
while using a renderer-integration architecture tailored to Skiko.

## Goal

The example should prove that a Compose Desktop application can host a native
GPU producer through texture sharing while keeping Compose on its natural Skiko
renderer for each operating system. The map is the concrete proof, not the
abstraction boundary.

MapLibre does not have a Direct3D backend, so the Windows path should render
with MapLibre's Vulkan backend and share the resulting image with Skiko's
Direct3D renderer instead of forcing Skiko onto OpenGL. The same principle
applies across platforms: the Compose renderer is the consumer, MapLibre is the
producer, and the bridge owns the shared storage and synchronization.

## Architecture

```text
examples/compose-map
  AppShell
    Compose window, lifecycle, frame pacing, input, and shutdown.

  ComposeNativeSurface
    Map-neutral Compose primitive.
    Owns the visual node, resize observation, invalidation, and drawing.

  NativeSurfaceSession
    Stable client-facing surface lifecycle.
    Exposes capabilities and leases producer render targets.

  SkikoHost
    Detects the active Skiko renderer.
    Exposes renderer handles needed by the selected bridge.
    Draws the shared texture into the Compose canvas.

  NativeSurfaceBridge
    One implementation per producer/consumer graphics API pair.
    Owns shared allocation, import/export handles, synchronization, and resize.

  MapLibreSurfaceRenderer
    Thin adapter from NativeSurfaceSession targets to MapLibre borrowed
    descriptors.
    Owns runtime, map, render session, events, and render_update.
```

The map-neutral surface layer is the main reusable surface. It creates or
imports a texture that both APIs can access, leases the producer-side handle to
client code, and exposes the consumer-side handle to Skiko drawing code.
MapLibre-specific code should start at the point where a producer target is
converted into a borrowed texture descriptor.

## Reusable Surface Abstraction

The reusable unit should feel closer to an Android `Surface` than to a MapLibre
integration helper: Compose provides a rectangle in the UI tree, and native code
gets a render target compatible with a selected producer API.

The mature-toolkit lessons to carry forward:

- The UI primitive should be small. Android `SurfaceView`, `TextureView`, and
  Flutter `Texture` do not ask the producer to understand the host compositor.
- The producer should receive short-lived render targets or buffers, not a
  permanent texture it can mutate whenever it wants.
- The surface layer should own resize, buffering, synchronization, and
  presentation. Producers should only render when given a frame lease.
- Backend details belong at the target edge. The common API should negotiate
  backends, report capabilities, and then hand the producer a typed target.
- Invalidation should be explicit. A video stream, game loop, and map renderer
  should all be able to ask for another frame without coupling to Compose's
  recomposition model.

Public names:

- `ComposeNativeSurface`: composable visual primitive.
- `NativeSurfaceController`: imperative invalidation and diagnostics.
- `NativeSurfaceRenderer`: client-owned producer implementation.
- `NativeSurfaceSession`: selected backend and surface capabilities.
- `NativeSurfaceFrame`: one render opportunity with a leased target.
- `NativeSurfaceTarget`: backend-native target for the active frame.

The public model should be map-neutral:

```kotlin
@Composable
fun ComposeNativeSurface(
  renderer: NativeSurfaceRenderer,
  modifier: Modifier = Modifier,
  controller: NativeSurfaceController? = null,
)

interface NativeSurfaceRenderer {
  val backend: ProducerBackend

  fun onSurfaceAvailable(session: NativeSurfaceSession) {}
  fun onSurfaceChanged(extent: SurfaceExtent) {}
  fun render(frame: NativeSurfaceFrame): NativeSurfaceRenderResult
  fun onSurfaceLost() {}
}

interface NativeSurfaceSession {
  val backend: ProducerBackend
  val capabilities: NativeSurfaceCapabilities
  fun requestFrame()
}

interface NativeSurfaceFrame {
  val frameId: Long
  val extent: SurfaceExtent
  val target: NativeSurfaceTarget
  val presentationTimeNanos: Long?
}

sealed interface NativeSurfaceRenderResult {
  data object Rendered : NativeSurfaceRenderResult
  data object Skipped : NativeSurfaceRenderResult
}
```

The default rendering contract is synchronous: when `render(frame)` returns
`Rendered`, producer writes are complete for that frame and the surface may hand
off to the Skiko host. Engines or decoders that submit work asynchronously can
be supported later with an explicit completion token, but the first public API
should stay synchronous until a concrete async producer forces that shape.

`NativeSurfaceController` should stay small:

```kotlin
@Composable
fun rememberNativeSurfaceController(): NativeSurfaceController

interface NativeSurfaceController {
  val state: StateFlow<NativeSurfaceState>
  fun requestFrame()
  fun dispose()
}
```

When `controller` is `null`, `ComposeNativeSurface` should create and remember
an internal controller. Applications that need an external render loop,
diagnostics, or explicit disposal can pass a remembered controller.

`NativeSurfaceState` should be diagnostic state, not rendering state:

- inactive: the composable is not attached or no bridge is selected.
- ready: a backend was selected and frames can be requested.
- unsupported: no bridge can satisfy the renderer's requested producer backends.
- failed: bridge setup or rendering failed with a diagnostic.

`NativeSurfaceTarget` is intentionally backend-specific at the edge. Targets are
valid only during the `render(frame)` call.

- `MetalTextureTarget`: `id<MTLTexture>` plus pixel format, origin, and extent.
- `VulkanImageTarget`: `VkImage`, `VkImageView`, `VkFormat`, layouts, and queue
  family requirements.
- `OpenGlTextureTarget`: texture name, target, context handles, context
  provider, and format.

Backend targets should use opaque native-handle wrappers instead of raw Kotlin
`Long` values wherever possible:

```kotlin
@JvmInline value class NativeHandle(val address: Long)

sealed interface NativeSurfaceTarget {
  val backend: ProducerBackend
  val extent: SurfaceExtent
  val generation: Long
}
```

The reusable layer owns Skiko reflection, consumer drawing, shared allocation,
producer target import/export, synchronization, resize, and teardown. Client
renderers own domain state and native rendering commands. A game should be able
to render a frame, a video app should be able to upload or decode into the
target, and the MapLibre example should be able to attach the target as a
borrowed render target without knowing how Skiko was reached.

The first implementation can be single-buffered because the current MapLibre
borrowed texture path completes rendering before `render_update()` returns. The
API should still use frame and target leases rather than a permanent texture
field so double or triple buffering can be added without changing the client
contract.

## Map Adapter Shape

MapLibre integration should be a small adapter over the reusable primitive:

```text
ComposeNativeSurface(renderer = MapLibreSurfaceRenderer(mapController))
  -> NativeSurfaceFrame.target
  -> MapLibre borrowed texture descriptor
  -> render_update()
  -> NativeSurfaceRenderResult.Rendered
```

`MapLibreSurfaceRenderer` should contain the only MapLibre-specific knowledge:

- runtime and map lifecycle
- render-pending state
- borrowed descriptor creation from `NativeSurfaceTarget`
- input and camera plumbing from Compose events

It should not know about Skiko reflection fields, D3D12 shared handles,
IOSurface, `dma_buf`, GL memory objects, or command queue synchronization.

## Bridge Matrix

The complete design matrix crosses the default Compose/Skiko renderer for each
desktop operating system with every MapLibre render backend supported on that
operating system.

| OS      | Compose/Skiko consumer | MapLibre producer | Bridge          | Notes                         |
| ------- | ---------------------- | ----------------- | --------------- | ----------------------------- |
| macOS   | Metal                  | Metal             | `metal-metal`   | Same-API control path.        |
| macOS   | Metal                  | Vulkan            | `vulkan-metal`  | First cross-API proof target. |
| macOS   | Metal                  | OpenGL/EGL        | `opengl-metal`  | Cross-API parity path.        |
| Windows | Direct3D 12            | Vulkan            | `vulkan-d3d12`  | Primary Windows path.         |
| Windows | Direct3D 12            | OpenGL/WGL        | `opengl-d3d12`  | Cross-API parity path.        |
| Linux   | OpenGL                 | Vulkan            | `vulkan-opengl` | Primary Linux path.           |
| Linux   | OpenGL                 | OpenGL/EGL        | `opengl-opengl` | Same-API control path.        |

The example may implement this matrix incrementally, but the architecture should
keep the bridge boundary broad enough for every row. A bridge row may fail fast
at runtime when the required Skiko renderer, native artifact backend, driver
extension, or shared-memory capability is unavailable.

## Frame Flow

The exact scheduling must respect Compose/Skiko repaint rules. The reusable
primitive's frame flow is:

```text
Compose frame
  -> surface.observeExtent()
  -> bridge.acquireFrame(extent)
  -> renderer.onSurfaceChanged(extent) if the target size changed
  -> result = renderer.render(frame)
  -> if result == Rendered:
       bridge.completeProducerAccess(frame)
       bridge.waitForConsumerAccess(frame)
       surface.draw(frame)
     else:
       surface.drawPreviousFrameIfAvailable()
  -> bridge.releaseFrame(frame)
```

MapLibre's runtime/event pumping fits inside `renderer.render(frame)`. Other
clients can use the same callback to drive an engine frame, copy a decoded video
frame, or submit native drawing work. `Skipped` means the producer did not
publish new content for this frame; the surface can keep presenting the most
recent completed frame if one exists.

## Shared Texture Direction

The default direction is consumer-compatible shared storage first, then a
producer-side imported view. Skiko is the less controllable participant because
Compose owns its renderer selection and most live renderer handles are internal.
MapLibre already accepts borrowed producer-native render targets, so the bridge
should create storage that Skiko can consume naturally and pass the imported
producer handle to MapLibre.

Per bridge row:

| Bridge          | Shared storage direction                                                                                                                                                                                                                                                           |
| --------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `metal-metal`   | Create an `MTLTexture` on the Skiko Metal device, pass the same texture to MapLibre's Metal borrowed texture descriptor, and draw it from Skiko's Metal path.                                                                                                                      |
| `vulkan-metal`  | Create an `MTLTexture` on the Skiko Metal device, import it into Vulkan with `VK_EXTERNAL_MEMORY_HANDLE_TYPE_MTLTEXTURE_BIT_EXT`, then pass the `VkImage` and `VkImageView` to MapLibre.                                                                                           |
| `opengl-metal`  | Create an `MTLTexture` on the Skiko Metal device, create an ANGLE Metal-backed EGL/GLES context, import the Metal texture with `EGL_ANGLE_metal_texture_client_buffer`, bind the `EGLImage` to `GL_TEXTURE_2D`, and pass the texture name plus `EglContextDescriptor` to MapLibre. |
| `vulkan-d3d12`  | Create a D3D12 committed resource or shared resource compatible with Skiko, export a shared handle, import it into Vulkan as a D3D12 resource-backed image, then pass the image and view to MapLibre.                                                                              |
| `opengl-d3d12`  | Create a D3D12 shared resource compatible with Skiko, import it into OpenGL with Win32 external memory objects, then pass the OpenGL texture name to MapLibre.                                                                                                                     |
| `vulkan-opengl` | Create Vulkan exportable image memory, import it into OpenGL with `GL_EXT_memory_object_fd`, then draw the OpenGL texture from Skiko.                                                                                                                                              |
| `opengl-opengl` | Create bridge-owned external-memory storage imported into both the producer EGL texture and the Skiko OpenGL texture, then synchronize producer-to-consumer access with GL semaphore/fence ownership.                                                                              |

Vulkan external memory supports the needed producer imports for Metal textures,
D3D12 resources, and Linux file-descriptor handles. OpenGL external memory
objects cover the Windows and Linux GL import side when the GL context and
Vulkan device report compatible driver/device UUIDs. Linux `dma_buf` import is
available through EGLImage, but it requires carrying DRM fourcc, offset, pitch,
and often modifier metadata across the bridge.

## Ownership Model

The UI side should own the shared texture whenever practical. Client renderers
should receive borrowed producer-side views of that storage through frame
leases.

The bridge owns:

- allocation and import/export handles
- producer-side texture/image/view handles
- consumer-side texture/resource handles
- external semaphores, fences, or command-queue waits
- resize recreation
- teardown ordering

The client renderer owns:

- domain runtime and scene state
- supported producer backend declaration
- render-pending or frame-needed state
- conversion from `NativeSurfaceTarget` to the producer's render target

The Skiko host owns:

- Compose window
- Skiko layer discovery
- draw callback integration

## Skiko Access Strategy

Skiko's AWT backend selection maps macOS to Metal, Windows to Direct3D by
default, and Linux to OpenGL by default. Skiko exposes public Skia wrapper APIs
for Metal, Direct3D, and OpenGL backend render targets, but the live Compose
renderer's native device/context handles are internal implementation details.

`SkikoHost` should isolate all Skiko coupling and expose a small capability
model to the rest of the example:

- renderer kind: Metal, Direct3D 12, or OpenGL
- enough native device/context identity to create or validate shared storage
- a wrapping or drawing path for the consumer-side shared texture
- a clear diagnostic when the expected Skiko internals are unavailable

Reflection is acceptable for the proof path if it is fully contained inside
`SkikoHost`. Platform graphics calls that Skiko does not expose through Kotlin
APIs should be isolated in the bridge layer; the current macOS proof uses
LWJGL's Objective-C runtime helpers rather than a Compose-specific native
library. The rest of the example should depend on capabilities rather than Skiko
field names.

The current Compose 1.11.1 / Skiko 0.144.6 macOS path stores the active
`SkiaLayer` as an anonymous `SkiaLayer` subclass inside
`ComposeWindowPanel -> ComposeContainer -> WindowSkiaLayerComponent`. The lookup
must use `SkiaLayer` assignability, not exact class-name equality.
`MetalRedrawer.contextHandler.device.ptr` points at Skiko's Metal device
wrapper; the Metal bridge uses that wrapper's `adapter` property to allocate an
`MTLTexture` on the same Metal device as Compose.

Skiko 0.144.6's `BackendRenderTarget.makeMetal(width, height, ptr)` expects
`ptr` to be a raw Objective-C `id<MTLTexture>` pointer encoded as a `Long`.
Skiko constructs the internal Metal backend-info structure and retains the
texture when it creates the backend render target. The bridge must keep its own
retained `MTLTexture` alive until resize or close, and it must keep the
`BackendRenderTarget` alive for the full lifetime of any `Surface` created from
it.

Linux needs special care. Skiko's default Compose Desktop path is OpenGL, but it
does not imply an EGL context that MapLibre can share directly. The current C
API supports WGL and EGL context providers for OpenGL, not GLX. The Linux OpenGL
bridge should use external-memory texture aliasing instead of direct context
sharing with Skiko's OpenGL context.

For Compose 1.11.1 / Skiko 0.144.6 on Linux, `LinuxOpenGLRedrawer` owns a
private native GL context handle and a private `OpenGLContextHandler`. Rendering
happens while Skiko holds the Linux drawing surface lock, makes that context
current, asks `OpenGLContextHandler` to draw, swaps buffers, and flushes GL.
`OpenGLContextHandler` wraps the current draw framebuffer with
`BackendRenderTarget.makeGL(...)`; it does not expose a public native context,
display, or framebuffer handle. The Linux bridge therefore needs a contained
`SkikoHost` reflection path that runs during the Compose draw callback, verifies
that the active redrawer is `LinuxOpenGLRedrawer`, and performs GL texture
creation/import/wrapping while Skiko's GL context is current.

Skiko's public wrappers are still useful once that context is current:
`BackendTexture.makeGL(...)` wraps a GL texture name, and
`Image.adoptTextureFrom` creates a GPU-backed `Image` from a `BackendTexture`
and a `DirectContext`. The missing public piece is access to the live
`DirectContext` used by `OpenGLContextHandler`. The Linux proof should retrieve
that context from the private handler and use Skia/Compose to composite the
imported texture as a normal GPU-backed UI element.

The first Linux spike should target the opaque-FD path:

1. Create the producer Vulkan image with `VK_KHR_external_memory_fd` /
   `VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT`.
2. Export the memory with `vkGetMemoryFdKHR`; close the FD in application code
   unless a successful GL import transfers ownership.
3. In Skiko's current GL context, require `GL_EXT_memory_object`,
   `GL_EXT_memory_object_fd`, and texture-storage support.
4. Import the FD with `ImportMemoryFdEXT(..., HANDLE_TYPE_OPAQUE_FD_EXT, fd)`,
   set `DEDICATED_MEMORY_OBJECT_EXT` when the Vulkan allocation is dedicated,
   and bind it to a `GL_TEXTURE_2D` with `TexStorageMem2DEXT` or
   `TextureStorageMem2DEXT`.
5. Wrap that texture with `BackendTexture.makeGL(...)` and
   `Image.adoptTextureFrom(...)`, then draw the resulting Skia image from the
   Compose draw callback.

This path avoids DRM modifier negotiation and is the most direct same-driver
Vulkan-to-OpenGL proof. It must fail fast when the GL context and Vulkan device
do not expose matching `DEVICE_UUID_EXT` / driver UUID data or when the chosen
Vulkan physical device cannot export an opaque FD image with the needed format,
usage, and tiling.

The `dma_buf` / EGLImage path remains a Linux portability probe:

- Vulkan must support `VK_EXT_external_memory_dma_buf` and, for images with DRM
  modifiers, `VK_EXT_image_drm_format_modifier`.
- EGL import requires `EGL_EXT_image_dma_buf_import`; modifier-aware imports and
  capability probes require `EGL_EXT_image_dma_buf_import_modifiers`.
- The bridge must pass logical width/height, DRM fourcc, per-plane FD, offset,
  pitch, and modifier metadata to `eglCreateImageKHR`.
- EGL does not take ownership of the dma-buf FDs; the bridge closes them after
  import succeeds or fails.
- Binding the imported `EGLImage` to a GL texture requires a GL extension such
  as `GL_EXT_EGL_image_storage` or an equivalent EGL-image texture target that
  works with Skiko's desktop GL context.

### Windows D3D12 Handoff Notes

The Windows rows should follow the Linux lesson: MapLibre receives only the
producer-native target, while the bridge owns Skiko reflection, shared-resource
handles, texture origin, synchronization, and owner-thread routing.

For Compose 1.11.1 / Skiko 0.144.6, the expected Direct3D consumer path is
`org.jetbrains.skiko.redrawer.Direct3DRedrawer`. The cached class shape shows
private `contextHandler` and `device` fields, public `makeContext()`,
`makeSurface(...)`, `changeSize(...)`, `getBufferIndex()`, and native swap-chain
helpers. `Direct3DContextHandler` owns Skiko's Direct3D `DirectContext` and
surfaces. The Windows bridge should keep those reflection details inside
`SkikoHost`, analogous to the Metal and Linux OpenGL reflection paths, and
return only a small Direct3D capability object to bridge code.

`vulkan-d3d12` should be the primary Windows implementation:

1. Discover Skiko's live Direct3D device/context through `SkikoHost`; fail fast
   when the active redrawer is not `Direct3DRedrawer`.
2. Allocate a D3D12 `ID3D12Resource` on Skiko's device with color-attachment and
   shader-resource usage that Skia can wrap. Prefer a shareable committed
   resource and use shared heaps only if the committed resource cannot be
   imported by Vulkan on target drivers.
3. Export a Win32 handle with `ID3D12Device::CreateSharedHandle`; duplicate or
   transfer handle ownership deliberately, and close each `HANDLE` exactly once.
4. Import the handle into Vulkan with `VK_KHR_external_memory_win32` and
   `VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT`, create a `VkImageView`,
   and pass only the resulting `VulkanImageTarget` to MapLibre.
5. Present the D3D12 resource through Skiko's Direct3D path. First try Skiko's
   `BackendRenderTarget.makeDirect3D(...)` wrapper; if the Kotlin wrapper cannot
   express the imported resource shape, isolate the missing Skia/D3D12 wrapping
   in a bridge-local native helper.
6. Start with `vkDeviceWaitIdle()` plus an explicit Direct3D queue/fence wait
   for proof-level ordering. Replace that with a shared D3D12 fence imported
   into Vulkan as `VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT` once the
   texture path is stable.

`opengl-d3d12` should reuse the same Skiko Direct3D presenter, but its producer
side is WGL:

1. Create a WGL share context for MapLibre using the existing
   `WglContextDescriptor` contract. Use `src/zig_test_support/wgl_context.zig`
   as the bootstrap reference for selecting a pixel format, creating a dummy
   window/device context, and creating a shareable WGL context.
2. Allocate the consumer-compatible D3D12 resource first, then expose that
   storage to WGL/OpenGL. Preferred path: `GL_EXT_memory_object` plus
   `GL_EXT_external_objects_win32` / `GL_EXT_memory_object_win32` from a D3D12
   resource handle. If driver support makes the GL external-memory path
   impractical, evaluate `WGL_NV_DX_interop2` or an ANGLE-backed D3D path as a
   separate bridge design.
3. Pass only `OpenGlTextureTarget` and `WglContextHandles` to MapLibre. Keep
   D3D12 resources, GL memory objects, duplicated handles, keyed mutexes, and
   Skiko Direct3D reflection inside the bridge.
4. Carry `TextureOrigin` explicitly and validate visual orientation before
   validating gestures or resize. Linux showed that a correct texture-sharing
   path can still be vertically flipped.
5. Route all MapLibre-owned calls through the context owner thread if WGL is
   made current away from the EDT. The Linux EGL bridge's `withRendererAccess`
   hook is the template for gestures, resize, render-session calls, and cleanup.
6. Use `glFinish()` plus a D3D12 fence wait for the first proof, then graduate
   to `GL_EXT_semaphore_win32`, keyed mutex, or shared-fence synchronization
   after import and presentation are stable.

Before implementing either Windows row, add the missing runtime dependencies or
native helpers for Win32/D3D12 calls. LWJGL covers Vulkan, OpenGL, and WGL-style
OpenGL APIs here, but this example currently has no D3D12/COM helper layer. Keep
that helper narrow: device/resource/handle/fence operations only, with
MapLibre-facing descriptors still built in Kotlin.

## Synchronization Strategy

External memory and synchronization are separate capabilities. Each bridge owns
the producer-to-consumer handoff for every frame.

The MapLibre Vulkan borrowed texture contract already gives the bridge a useful
baseline: before `render_update()`, the caller makes the image available on
MapLibre's graphics queue in the descriptor's initial layout; after
`render_update()` returns, MapLibre has waited for its submitted work and leaves
the image in the descriptor's final layout. That makes a conservative first
proof possible with CPU-visible ordering plus consumer-side waits/flushes, but
the final bridge should use GPU-native synchronization where the APIs support
it.

Per bridge family:

- D3D12 interop should use shared D3D12 fences or Vulkan external semaphores
  backed by D3D12 fence handles.
- OpenGL interop should use GL external semaphores on Linux and Windows so the
  consumer waits after producer rendering and observes the correct texture
  layout.
- Metal interop should start with conservative ordering for the proof and then
  graduate to `MTLSharedEvent` when Skiko exposes a practical command-buffer
  wait path. `VK_EXT_metal_objects` can expose the Metal shared event underlying
  a Vulkan semaphore or event; the remaining question is how to attach that
  event to Skiko's Compose command submission without taking over the renderer.

Bridge implementations should keep memory import/export, semaphore/fence
creation, and resize teardown in one place so the frame loop does not learn
backend-specific rules.

Linux synchronization should be treated as separate from memory import. Vulkan
`VK_KHR_external_semaphore_fd` exports and imports semaphore payloads through
POSIX FDs; GL `GL_EXT_semaphore_fd` imports semaphore FDs and `WaitSemaphoreEXT`
makes texture memory visible to GL after the external producer signals. The
initial Linux proof may use `vkQueueWaitIdle` plus GL flush/finish while the
interop path is validated, but the bridge design should keep room for a
producer-to-consumer semaphore per frame. FD ownership rules match external
memory: successful Vulkan export transfers the FD to application code, and
successful GL import transfers ownership to the GL implementation.

The public surface should expose synchronization as a lifecycle contract, not as
raw primitives by default. A renderer receives a frame whose target is ready for
producer writes. When `render(frame)` returns `Rendered`, the renderer has
completed its writes before returning. Advanced producer APIs can grow explicit
completion tokens later without changing the Compose-facing primitive.

## C API Coverage

The current borrowed texture descriptors are sufficient for MapLibre attachment
when the bridge creates the backend-native producer object before calling into
MapLibre.

The Vulkan borrowed descriptor already carries the context, image, image view,
format, initial layout, and final layout. External-memory details such as D3D12
handle type, Metal handle type, `dma_buf` file descriptor, dedicated allocation
status, DRM modifier, row pitch, and allocation ownership are bridge concerns as
long as the bridge constructs the `VkImage` and `VkImageView`.

The Metal borrowed descriptor only needs the `MTLTexture` pointer once the
bridge has created Skiko-compatible storage. The OpenGL borrowed descriptor only
needs the texture name, target, and a supported context provider once the bridge
has imported or created the texture in the producer context.

One C API gap is intentionally avoided by this architecture: direct Linux OpenGL
sharing with Skiko would require a GLX context provider or deeper Skiko context
ownership. The Linux OpenGL rows use external-memory texture aliasing so the
existing EGL provider remains enough for MapLibre's producer side.

## Build And Runtime Shape

The example should live under `examples/compose-map`, use the existing Kotlin
binding, and consume the native artifact metadata in the same spirit as
`examples/lwjgl-map`. The macOS proof should use LWJGL for Objective-C runtime
calls instead of adding a Compose-specific native component. Future bridge rows
can add native code only when the platform interop cannot be expressed cleanly
through the JVM toolchain.

The build should keep the Kotlin/Compose app separate from per-OS bridge code:

- Kotlin owns the Compose application, MapLibre binding calls, frame loop, and
  bridge selection.
- Gradle mirrors `examples/lwjgl-map` for the native C artifact metadata,
  `org.maplibre.nativeffi.library.path`, JVM target, and Compose Desktop JVM
  launch behavior. On the current macOS/JDK 25 toolchain, `-XstartOnFirstThread`
  hangs AWT while constructing the first `JFrame`; the example should let
  Compose's AWT launcher run without that flag unless a future pinned runtime
  requires it.
- LWJGL supplies the Objective-C runtime bridge used by the macOS same-API Metal
  implementation.
- Bridge construction uses the one row determined by the default Skiko renderer,
  the selected MapLibre backend artifact, and extension/capability probes.

The code should be laid out so extraction is mechanical:

```text
examples/compose-map/src/main/kotlin/.../surface
  ComposeNativeSurface
  NativeSurfaceController
  NativeSurfaceRenderer
  NativeSurfaceTarget
  SkikoHost
  NativeSurfaceBridge implementations

examples/compose-map/src/main/kotlin/.../map
  MapLibreSurfaceRenderer
  MapLibre input and camera adapters
  Example app wiring
```

The `surface` package should avoid MapLibre imports. Bridge helpers should also
use map-neutral names so they can move into a standalone library without
renaming exported symbols.

## Remaining Validation Targets

- Replace the first `opengl-metal` proof's `glFinish()` handoff with GPU-native
  synchronization once a practical Skiko Metal wait path is identified.
- Validate the best Metal-side synchronization path for `vulkan-metal` after the
  conservative proof, especially how to connect an `MTLSharedEvent` to Skiko
  command submission.
- Pin the Compose/Skiko version for the example and verify the reflection/native
  access points against that exact source.
- Validate same-device identity, context timing, and texture format/usage for
  the macOS same-API Metal draw path.
- Validate the proposed public Kotlin surface API against the MapLibre adapter
  and at least one non-map producer shape, such as a simple animated native
  renderer or decoded video frame source.
- Probe real driver support for GL external memory and semaphore extensions on
  the Linux and Windows machines we expect developers or CI to use.
- Prototype Linux `vulkan-opengl` with opaque-FD GL memory objects first, then
  evaluate `dma_buf`/EGLImage import only if opaque-FD support is missing or
  proves less portable across target drivers.
- Verify Skiko 0.144.6 Linux reflection points against the exact source before
  replacing the prototype bridge: `LinuxOpenGLRedrawer.context`,
  `LinuxOpenGLRedrawer.contextHandler`, and
  `OpenGLContextHandler.context`/surface lifecycle.
- Prioritize validating `vulkan-metal` because it exercises cross-API sharing
  against the current macOS development host while keeping the Windows D3D12
  bridge in the design from the start.

## Research Pointers

- [Vulkan Guide: External Memory and Synchronization](https://docs.vulkan.org/guide/latest/extensions/external.html)
- [VK_EXT_external_memory_metal](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_external_memory_metal.html)
- [VkExternalMemoryHandleTypeFlagBits](https://docs.vulkan.org/refpages/latest/refpages/source/VkExternalMemoryHandleTypeFlagBits.html)
- [VK_KHR_external_memory_win32](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_external_memory_win32.html)
- [VK_EXT_external_memory_dma_buf](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_external_memory_dma_buf.html)
- [VK_KHR_external_memory_fd](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_external_memory_fd.html)
- [VK_KHR_external_semaphore_fd](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_external_semaphore_fd.html)
- [VK_EXT_image_drm_format_modifier](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_image_drm_format_modifier.html)
- [D3D12 CreateSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createsharedhandle)
- [D3D12 Shared Heaps](https://learn.microsoft.com/en-us/windows/win32/direct3d12/shared-heaps)
- [EGL_EXT_image_dma_buf_import](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_image_dma_buf_import.txt)
- [EGL_EXT_image_dma_buf_import_modifiers](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_image_dma_buf_import_modifiers.txt)
- [GL_EXT_external_objects](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_external_objects.txt)
- [GL_EXT_external_objects_fd](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_external_objects_fd.txt)
- [GL_EXT_EGL_image_storage](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_EGL_image_storage.txt)
- [GL_EXT_external_objects_win32](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_external_objects_win32.txt)
- [ANGLE Metal texture client buffer extension](https://chromium.googlesource.com/angle/angle/+/refs/heads/main/extensions/EGL_ANGLE_metal_texture_client_buffer.txt)
- [VK_EXT_metal_objects](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_metal_objects.html)
- [VkExportMetalSharedEventInfoEXT](https://vulkan.lunarg.com/doc/view/1.4.341.0/mac/antora/refpages/latest/refpages/source/VkExportMetalSharedEventInfoEXT.html)
- [Skiko 0.144.6 LinuxOpenGLRedrawer](https://raw.githubusercontent.com/JetBrains/skiko/v0.144.6/skiko/src/awtMain/kotlin/org/jetbrains/skiko/redrawer/LinuxOpenGLRedrawer.kt)
- [Skiko 0.144.6 OpenGLContextHandler](https://raw.githubusercontent.com/JetBrains/skiko/v0.144.6/skiko/src/awtMain/kotlin/org/jetbrains/skiko/context/OpenGLContextHandler.kt)
- [Skiko 0.144.6 DirectContext wrappers](https://raw.githubusercontent.com/JetBrains/skiko/v0.144.6/skiko/src/commonMain/kotlin/org/jetbrains/skia/DirectContext.kt)
- [Skiko 0.144.6 BackendTexture wrappers](https://raw.githubusercontent.com/JetBrains/skiko/v0.144.6/skiko/src/commonMain/kotlin/org/jetbrains/skia/BackendTexture.kt)
- [Skiko 0.144.6 BackendRenderTarget wrappers](https://raw.githubusercontent.com/JetBrains/skiko/v0.144.6/skiko/src/commonMain/kotlin/org/jetbrains/skia/BackendRenderTarget.kt)
