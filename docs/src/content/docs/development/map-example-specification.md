---
title: Map example specification
description: Specification for interactive *-map example programs.
sidebar:
  order: 4
---

Specification for interactive `*-map` example programs: small apps that exercise
language bindings and render-target integrations through a focused map demo.

The specification has three sections:

1. [Shared baseline](#shared-baseline) — map, render-session, frame-loop, and
   graphics contracts common to every profile.
2. [Desktop profile](#desktop-profile) — windowed desktop hosts with CLI entry
   and keyboard/mouse input.
3. [Mobile profile](#mobile-profile) — embedded view hosts with touch input.

Implement a desktop example by reading Shared baseline and Desktop profile.
Implement a mobile example by reading Shared baseline and Mobile profile.

---

## Implementations

| Example                | Profile | Binding    | Toolkit         | Platforms             | Backends              |
| ---------------------- | ------- | ---------- | --------------- | --------------------- | --------------------- |
| `examples/zig-map`     | Desktop | Zig        | SDL3            | Linux, macOS, Windows | Vulkan, Metal, OpenGL |
| `examples/rust-map`    | Desktop | Rust       | winit           | Linux, macOS, Windows | Vulkan, Metal, OpenGL |
| `examples/lwjgl-map`   | Desktop | Kotlin/JVM | GLFW, LWJGL     | Linux, macOS, Windows | Vulkan, Metal, OpenGL |
| `examples/android-map` | Mobile  | Kotlin     | Android view    | Android               | OpenGL/EGL            |
| `examples/dotnet-map`  | Desktop | C#         | GLFW            | Linux, macOS, Windows | Vulkan, Metal, OpenGL |
| `examples/swift-map`   | Desktop | Swift      | AppKit, SwiftUI | macOS                 | Metal                 |
| `examples/swift-map`   | Mobile  | Swift      | UIKit           | iOS                   | Metal                 |

The Compose map example follows the broad strokes of this specification, but
uses its own renderer-integration architecture for Skiko texture sharing.

For examples built by native render-backend variant, “Backends” is the union of
supported configured variants. Each native library artifact includes one render
backend. A single run uses one graphics API, selected at build time
(build-variant examples) or at startup from the loaded library (multi-context
examples).

---

## Shared baseline

### Scope

#### What every example provides

- All map, runtime, and render access from application code through the
  project’s language binding for that language.
- Continuous map mode: runtime pumping, event draining, and repaint driven by
  map render events and user input.
- Initial style URL and camera per [Shared defaults](#shared-defaults).
- Camera controls per the active profile ([Desktop profile → Input](#input) or
  [Mobile profile → Input](#input-1)).
- Every graphics API the host toolkit and target platform can support across
  configured variants (Vulkan, Metal, OpenGL/EGL as applicable).
- Render-target coverage per the active profile
  ([Render-target coverage](#render-target-coverage)).
- Startup logging that identifies the active render-target mode and which native
  render backends the loaded library supports.

#### What an example is not

A `*-map` program is a focused map demo. It MUST NOT include automated tests or
packaging/installer UX.

### Shared defaults

#### Style

- Style URL: `https://tiles.openfreemap.org/styles/bright`
- Load the style during map initialization, before the first render.

#### Initial camera

| Field   | Value                                                     |
| ------- | --------------------------------------------------------- |
| Center  | latitude `37.7749`, longitude `-122.4194` (San Francisco) |
| Zoom    | `13.0`                                                    |
| Bearing | `12.0` degrees                                            |
| Pitch   | `30.0` degrees                                            |

Apply with an immediate `jump_to` on startup.

#### Map and runtime

- Runtime cache path: `:memory:` (in-memory).
- Map mode: continuous (`MLN_MAP_MODE_CONTINUOUS`).

### Architecture

#### Overview

Every `*-map` example splits host responsibilities into the same logical
modules. Names differ by language; boundaries MUST NOT be collapsed into a
single monolithic type.

```mermaid
flowchart TB
  subgraph shell["App shell"]
    EL[Event loop]
    VP[Viewport]
    IN[Input]
    DG[Diagnostics]
  end
  subgraph mapstate["Map state"]
    RT[Runtime]
    MP[Map]
    RS[Render target]
  end
  subgraph gfx["Graphics host"]
    BE[Backend context]
    CP[Compositor]
    SC[Presentation]
  end
  Entry[Entry] --> shell
  shell --> mapstate
  mapstate --> gfx
  RS -->|texture modes| CP
  RS -->|native-surface| SC
```

#### Logical modules

| Module           | Responsibility                                                                                                          |
| ---------------- | ----------------------------------------------------------------------------------------------------------------------- |
| App shell        | Profile entry, toolkit lifecycle, main event loop, shutdown ordering.                                                   |
| Viewport         | Map logical size, physical drawable size, and `scale_factor` for `RenderTargetExtent`.                                  |
| Map state        | Owns runtime, map, and active render target; loads style and initial camera.                                            |
| Graphics context | Creates/configures the host presentation surface and owns host graphics API context and presentation resources.         |
| Render target    | Owns the render session and mode-specific resources such as compositors, borrowed textures/images, and acquired frames. |
| Compositor       | Host pass that draws a map-owned or borrowed texture into the swapchain.                                                |
| Input            | Pointer and/or touch → map camera APIs; profile-specific control help.                                                  |
| Diagnostics      | Optional log callback and consistent error messages on failed setup or camera commands.                                 |

Implementations SHOULD mirror this layout in the source tree (separate files or
packages per module).

#### Threads and loops

Examples run two loops on two native threads: a **render loop** and a **runtime
loop**.

- The render loop thread owns the host window and its input events, the
  viewport, the graphics API context and presentation resources, the compositor,
  and the render session for the session's whole lifetime. It attaches the
  session, renders through it, resizes it, and closes it.
- The runtime loop thread owns the runtime, the map, `run_once`, `poll_event`,
  and every map mutation. It never calls a render-session function.
- Each loop MUST run on a native thread whose identity is stable for the life of
  the loop. Host mechanisms that may move a logical task between native threads,
  such as thread pools, green threads, and coroutine dispatchers without thread
  confinement, are not usable for either loop.
- The render loop thread MUST be the thread on which the host toolkit delivers
  window, input, and display-refresh callbacks.
- Cross-thread state MUST be limited to three channels: a camera-command queue
  from the render loop to the runtime loop, a render request from the runtime
  loop to the render loop, and a one-time publication of the map from the
  runtime loop to the render loop so the render loop can attach against it. A
  shutdown signal and a first-failure record MAY accompany them.

This split exists because `run_once` drains the work it finds rather than a
fixed slice, so a single call can take as long as a style parse. Keeping it off
the display-paced loop is what lets presentation continue during heavy runtime
work.

##### Render loop thread by host toolkit

Where the host toolkit fixes which thread receives display-refresh and window
callbacks, that thread is the render loop thread. Where a graphics API context
is thread-current, such as OpenGL through EGL or WGL, the render loop thread is
the only thread that makes it current.

| Example       | Render loop thread                                 | Runtime loop thread |
| ------------- | -------------------------------------------------- | ------------------- |
| `zig-map`     | process main thread (SDL window, graphics context) | spawned thread      |
| `rust-map`    | winit event-loop thread                            | spawned thread      |
| `lwjgl-map`   | GLFW main thread                                   | spawned thread      |
| `dotnet-map`  | GLFW main thread                                   | dedicated `Thread`  |
| `swift-map`   | main run loop (`CADisplayLink`, AppKit/UIKit)      | dedicated `Thread`  |
| `android-map` | UI thread (`Choreographer`)                        | `HandlerThread`     |
| `compose-map` | native surface bridge's producer thread            | spawned thread      |

##### Attaching the render session

A render session's owner thread is the thread that attached it, and it does not
change for the session's lifetime. The render loop thread MUST therefore be the
thread that attaches the session, and the same thread MUST close it.

Attach requires only that the map be live, not that the caller own it, so the
render loop attaches against a map owned by the runtime loop. Attach follows
this operation:

1. The runtime loop creates the runtime and the map, then publishes the map to
   the render loop once. The publication provides the happens-before edge.
2. The render loop creates its graphics context and its mode-specific resources,
   then attaches, becoming the session's owner thread.
3. The render loop closes the session before the runtime loop destroys the map.
   Destroying a map with an attached session fails, so the render loop MUST
   signal that its session is closed and the runtime loop MUST wait for that
   signal before destroying the map.

Attach creates the session's graphics resources on the calling thread, so for
graphics APIs whose context is thread-current the host context MUST be current
on the render loop thread when it attaches. That thread is the only one that
makes it current, and it need never give it up.

The requirement is specific to WGL: the session resolves
`wglCreateContextAttribsARB` through the calling thread's current context, and
without one it falls back to a legacy context that cannot share with a context
current on another thread. Attaching where the host context is already current
avoids both failure modes. Vulkan and Metal have no thread-current context and
are unaffected.

Reattaching, which the borrowed-texture mode requires on resize, is entirely
local to the render loop thread: close the session, rebuild the mode-specific
resources, attach again.

##### Thread identity on Apple platforms and managed runtimes

Owner-thread checks are keyed on the native thread, so a host mechanism that
serializes work without pinning it to one native thread is not usable for either
loop. This rules out more than thread pools:

- A GCD serial `DispatchQueue` guarantees serialization but not thread affinity,
  so Swift examples MUST use a dedicated `Thread` for the runtime loop rather
  than a queue, an `actor`, or a `Task`.
- .NET examples MUST use a dedicated `System.Threading.Thread` rather than
  `Task.Run` or the thread pool.
- Kotlin examples MUST use a `Thread` or `HandlerThread` rather than a coroutine
  dispatcher.

#### Graphics API and mode matrix

The example architecture MUST model the active graphics API separately from the
active render-target mode. Graphics context code owns API-level resources
(Vulkan, Metal, OpenGL/EGL/WGL as applicable). Render target code owns the
attached `RenderSessionHandle`, mode-specific resources, resize behavior,
`render_update`, and close behavior.

The loaded native library reports one render backend per library artifact
through `mln_supported_render_backend_mask()`. Examples built across native
render-backend variants expose the union of those backends in the
[Implementations](#implementations) table.

Graphics API selection follows one of these patterns:

- **Build-variant examples** compile only the graphics API implementation that
  matches the active native build variant (for example `zig-map`, `rust-map`,
  `swift-map`).
- **Multi-context examples** ship a graphics context per targeted API and select
  the active API at startup from `supportedRenderBackends()` (for example
  `lwjgl-map`, `dotnet-map`).

OpenGL examples that can run with multiple context providers select EGL or WGL
from `supportedOpenGLContextProviders()`.

Each process run uses one graphics API. Render-target mode selection follows the
active profile ([Entry](#entry) or [Entry and shell](#entry-and-shell)).

Adding a graphics API or render-target mode MUST require localized changes. Keep
each graphics API and render-target mode in its own variant, class, or submodule
rather than branching ad hoc through shared draw code.

### Lifecycle

#### Startup

Order MUST be:

1. Parse profile entry configuration and validate the selected render mode.
2. Read and log the loaded library's supported native render backends from
   `mln_supported_render_backend_mask()`, then validate that the loaded native
   library supports the graphics API selected for this run; fail fast with a
   readable message if not.
3. Create the host presentation surface and initialize the graphics backend for
   the selected graphics API, on the render loop thread.
4. Start the runtime loop thread.
5. Create runtime (`:memory:` cache) on the runtime loop thread.
6. Create map with extent from the initial viewport and continuous mode.
7. Load style and apply initial camera.
8. Publish the map to the render loop thread.
9. Attach the render target for the selected mode on the render loop thread,
   using descriptors produced by the graphics context there.
10. Emit startup information:
    - active render-target mode identifier
    - active render-target status line

Steps 5 through 8 run on the runtime loop thread; steps 3 and 9 run on the
render loop thread. A host that cannot create its graphics context before the
window MUST still keep step 3 on the render loop thread.

On failure after partial setup, release already-created handles in reverse order
(render target → map → runtime → graphics), with the render target closed on the
render loop thread before the map is destroyed.

#### Shutdown

On host termination or fatal error, close resources in order:

1. Leave the render loop and finish or wait on in-flight GPU work if the backend
   requires it, on the render loop thread.
2. Render target (compositor and borrowed texture/image before or with the
   session, according to graphics API lifetime rules), on the render loop
   thread.
3. Signal the runtime loop that the session is closed, and stop it.
4. Map
5. Runtime, after which the runtime loop thread exits and the render loop thread
   joins it.
6. Graphics context and host presentation surface, on the render loop thread.

Steps 4 and 5 run on the runtime loop thread, which MUST wait for the step 3
signal before step 4: destroying a map that still has an attached render session
fails.

#### Handle ownership

- One runtime per process (the runtime loop thread drives `run_once` / pump).
- One map per runtime for the demo, sharing the runtime's owner thread.
- One live render target per map at a time.
- One render session owner thread, fixed for the session's lifetime: the thread
  that attached it, which is the render loop thread.
- Map configuration (style, camera) uses the map handle; render-target extent
  and present use the render target.

### Frame loop

The C API treats runtime pumping and presentation as separate concerns.
`run_once` advances native scheduler work and fills the event queue; it is not
display-driven. One call drains the work it finds instead of running a fixed
slice, so a single call can take as long as a style parse. `render_update` draws
only when the render request is set.

`*-map` examples split the two across the loops described in
[Threads and loops](#threads-and-loops): the render loop is display-paced and
draws, and the runtime loop pumps. `run_once` returns as soon as its iteration
finishes and never blocks waiting for more work, so a runtime loop paces itself
by waiting on a host condition between iterations.

#### Render loop iteration

1. Handle window, input, and resize events; translate camera input into commands
   and enqueue them for the runtime loop; set the render request.
2. Apply pending viewport changes to graphics resources and to the render
   session, or run the [reattach](#reattach).
3. Consume the render request; when it was set, call `render_update`.
4. Run `finishFrame()`.

```mermaid
sequenceDiagram
  participant RL as Render loop
  participant RQ as Render request
  participant RS as Render session
  participant BE as Backend

  RL->>RL: Input and resize
  RL->>RQ: enqueue camera commands, set request
  RL->>RQ: consume request
  RL->>RS: render_update() when it was set
  RL->>BE: finishFrame()
```

`finishFrame()` runs every iteration: swapchain or surface upkeep, resize
handling, and present hooks as required by the host graphics API.

A render loop iteration MUST NOT call `run_once` or `poll_event`.

#### Runtime loop iteration

1. Apply every queued camera command.
2. Call `run_once` exactly once.
3. Drain runtime events until the queue is empty, updating the render request.

```mermaid
sequenceDiagram
  participant RTL as Runtime loop
  participant CQ as Command queue
  participant RT as Runtime
  participant RQ as Render request

  RTL->>CQ: apply queued camera commands
  RTL->>RT: run_once()
  RTL->>RT: drain events
  RTL->>RQ: set request when a frame is needed
```

#### Cadence

While the map is visible and the example is active:

- The render loop MUST run at least one iteration per display refresh period,
  and MUST subscribe to the host toolkit's display refresh mechanism (for
  example swapchain frame callbacks, `CADisplayLink`, or `Choreographer`) to
  pace it.
- The runtime loop MUST run at least one iteration per display refresh period,
  and MUST wake immediately when the render loop enqueues a camera command or a
  viewport change. Between iterations it MUST wait on a host condition with a
  timeout no longer than one display refresh period.

Display refresh paces the render loop; it does not replace `run_once`. Each
runtime loop iteration MUST call `run_once` exactly once, and no other loop
calls it.

When the profile stops the loops (for example mobile background), runtime
progress stalls until they resume.

#### Render requests

The render request replaces a loop-local `render_pending` flag, because the two
loops set and consume it from different threads.

```mermaid
sequenceDiagram
  participant RL as Render loop
  participant RS as Render session
  participant CP as Compositor
  participant BE as Backend

  RL->>RS: render_update()
  alt texture mode
    RS-->>RL: map texture / frame
    RL->>CP: draw into swapchain
    CP->>BE: present
  else native-surface
    RS->>BE: present via surface session
  end
```

Requirements:

- The render request MUST be published and observed through the host language's
  atomic or synchronized mechanism. An unsynchronized field is not sufficient.
- The runtime loop MUST drain runtime events each iteration and set the render
  request when:
  - `map_render_update_available` targets this map (new map content to draw), or
  - `map_render_frame_finished` targets this map and `needs_repaint` is true
    (continuous mode needs another frame, for example ongoing camera
    transitions).
- The render loop MUST set the render request when input changes the camera and
  when a resize or reattach completes.
- The render loop MUST call `render_update` only when it consumed a set request.
- The render loop MUST consume the render request **before** calling
  `render_update`, and MUST set it again when `render_update` reports that no
  update was rendered. Consuming afterwards would discard a request the runtime
  loop published during the render call, and that frame would never be drawn.
- `map_render_frame_finished` and `map_idle` are delivered by the runtime loop's
  next `run_once` after the render loop rendered. An example MUST NOT treat
  either as a synchronous result of the `render_update` it follows, and MUST NOT
  block a render loop iteration waiting for one.
- After a session resize, the map applies its logical size on the runtime loop's
  next `run_once`, so `render_update` reports no update until then. The render
  loop MUST keep pacing and retry rather than treating it as a failure.

Texture modes: after `render_update` reports an update was rendered, MUST run
the compositor pass to copy the map texture into the host swapchain before
present.

### Viewport

The viewport value MUST contain:

| Field                               | Meaning                                                                   |
| ----------------------------------- | ------------------------------------------------------------------------- |
| `logical_width`, `logical_height`   | Map coordinate extent passed to `MapOptions` / `RenderTargetExtent`.      |
| `physical_width`, `physical_height` | Drawable pixels of the host framebuffer.                                  |
| `scale_factor`                      | Ratio between physical and logical sizes (content scale / pixel density). |

Derivation rules:

- Read logical and physical sizes from the host toolkit after surface creation
  and on every resize or backing-scale change.
- Compute logical dimensions from physical size and scale when the toolkit only
  exposes physical pixels (use `ceil(physical / scale)`, minimum `1`).
- Log viewport changes at informational level with field labels
  `logical=… physical=… scale=…`.

Pass `logical_*` and `scale_factor` to map creation, session attach, and session
`resize`.

### Map state

The map state module owns the runtime, map, and render session handles plus
map-specific setup.

#### Creation

- Create runtime with `:memory:` cache.
- Create map with current viewport extent and continuous mode.
- Load [style URL](#style).
- Apply [initial camera](#initial-camera).
- Attach a render target by dispatching on active graphics API and selected
  mode.

#### Event drain

- Drain all pending runtime events each runtime loop iteration.
- Set the render request when either:
  - `map_render_update_available` targets this map, or
  - `map_render_frame_finished` targets this map and `needs_repaint` is true.

#### Resize API

Expose `resize(viewport)` for the active render target. Resize API-level
resources separately when the graphics context requires it. When the active
render target reports `needsReattachOnResize`, destroy it and attach a
replacement for the same graphics context, map, and mode.

### Render-target modes

Shared baseline defines three render-target modes (discriminant/class, attach
paths, and present behavior). Each example implements only the modes required by
its profile ([Render-target coverage](#render-target-coverage)). Example
architecture MUST model each implemented mode.

#### Mode comparison

| Mode identifier    | C API concept                            | Compositor | Role                                                        |
| ------------------ | ---------------------------------------- | ---------- | ----------------------------------------------------------- |
| `owned-texture`    | Session-owned backend texture            | Required   | Map allocates texture, host samples it.                     |
| `borrowed-texture` | Caller-owned texture borrowed by session | Required   | Host allocates exportable texture; session renders into it. |
| `native-surface`   | Window presentation surface              | None       | Map renders directly to the host presentation target.       |

#### Startup status lines

Startup MUST print the active mode identifier and exactly one line from this
table:

| Mode identifier    | Printed line                                                                                       |
| ------------------ | -------------------------------------------------------------------------------------------------- |
| `owned-texture`    | `render target status: samples MapLibre-owned texture frames into the host swapchain`              |
| `borrowed-texture` | `render target status: renders into a host-owned texture, then samples it into the host swapchain` |
| `native-surface`   | `render target status: renders directly to the host window surface`                                |

#### `owned-texture`

- Attach with the C API owned-texture descriptor for the active graphics API.
- Pass the host graphics context handles required by that descriptor (see
  [Graphics API](#graphics-api)).
- On `render_update`, acquire the frame/image from the session, draw via
  compositor, release/close the frame per the C API frame lifetime rules.

#### `borrowed-texture`

- Host creates an exportable texture sized to the viewport (see
  [Graphics API](#graphics-api)).
- Attach with the borrowed-texture descriptor referencing host-owned handles.
- On `render_update`, sample that texture through the same compositor path as
  `owned-texture`.
- On resize, recreate the host texture and re-attach the session (see
  [Resize mechanics](#resize-mechanics); `needsReattachOnResize` is `true` for
  this mode).

#### `native-surface`

- Attach with the C API surface descriptor for host presentation (see
  [Graphics API](#graphics-api)).
- `render_update` presents through the surface render target directly.
- `drawTexture` MUST NOT be called for this mode.
- On resize, call session `resize` and rebuild host presentation; reattach when
  the host toolkit supplies a new surface handle.

### Compositor shaders

For `owned-texture` and `borrowed-texture`, the host-owned compositor that
samples the map texture into the host swapchain MUST use a fullscreen triangle
covering the viewport:

- Vertex shader: three corners with pass-through UVs spanning the visible
  `[0, 1] × [0, 1]` texture range (large-triangle technique).
- Fragment shader: `texture(map_texture, uv)` (straight copy, standard UV
  orientation).

SPIR-V, MSL, or GLSL source MAY differ by backend; the GPU output MUST match
that pass.

### Resize mechanics

- Recompute viewport on host size or scale changes; skip rendering if extent is
  empty.
- `needsReattachOnResize()` is a render-target method. It returns `true` for
  `borrowed-texture` because the host-owned exportable texture is fixed to the
  viewport size: resize destroys the render target, recreates the texture, and
  attaches again. It returns `false` for `owned-texture` and `native-surface`,
  where resize updates graphics-context resources, compositor resources for
  texture modes, and session extent in place.
- When it returns `true`, use the [reattach](#reattach); otherwise resize the
  graphics context and active render target in place.
- Set the render request after any resize.
- The render loop owns the session, so an in-place resize is a local call. The
  map applies the new logical size on the runtime loop's next `run_once`, so
  `render_update` reports no update until then.

#### Reattach

Reattaching happens entirely on the render loop thread, which owns both the
session and the graphics resources. The sequence MUST be:

1. Close the session, then destroy and recreate the host texture or surface at
   the new size.
2. Attach the render target again against the published map.
3. Set the render request.

Closing the session first matters: the render loop owns both the session and the
graphics resources it borrows, and the session must stop referencing a texture
before that texture is destroyed.

Profile sections define which host events trigger resize
([Desktop profile → Resize triggers](#resize-triggers) or
[Mobile profile → Resize triggers](#resize-triggers-1)).

### Diagnostics

- SHOULD register a native log callback during startup and clear it on shutdown.
- On setup or camera failure, print a short message including the native status
  and diagnostic strings returned by the C API.
- On startup, emit the items listed in [Startup](#startup) step 8 through the
  profile logging sink.

### Graphics API

Attach descriptors and shared context handles for each graphics API the example
binary targets. Implement only the modes required by the active profile
([Render-target coverage](#render-target-coverage)).

#### Vulkan

- One shared Vulkan context (`VkInstance`, `VkDevice`, queue, and
  `VkSurfaceKHR`) for compositor and render session.
- `owned-texture`: Vulkan owned-texture descriptor with those shared handles.
- `borrowed-texture`: exportable `VkImage` and view sized to the viewport;
  borrowed-texture descriptor.
- `native-surface`: surface / swapchain presentation descriptor for the host
  `VkSurfaceKHR`.

#### Metal

- `native-surface`: Metal surface descriptor for the host `CAMetalLayer`.
- `owned-texture`: Metal owned-texture descriptor; shared device and layer
  handles required by the C API.
- `borrowed-texture`: exportable Metal texture sized to the viewport;
  borrowed-texture descriptor.

#### OpenGL / EGL / WGL

- `native-surface`: OpenGL/EGL/WGL surface descriptor for the host platform GL
  surface.
- `owned-texture`: OpenGL owned-texture descriptor; shared GL context handles
  required by the C API.
- `borrowed-texture`: exportable GL texture sized to the viewport;
  borrowed-texture descriptor.

### Render-target coverage

| Profile | Required modes on every graphics API build variant the example ships |
| ------- | -------------------------------------------------------------------- |
| Desktop | `owned-texture`, `borrowed-texture`, `native-surface`                |
| Mobile  | `native-surface`                                                     |

---

## Desktop profile

### Scope

Desktop `*-map` examples add:

- One top-level resizable map window.
- CLI render-target mode selection across all three modes.
- Keyboard and mouse camera controls.
- Graceful process exit when the user closes the window.

### Entry

#### Render-target selection

The process MUST accept a render-target mode name:

| Mode                          | CLI value          |
| ----------------------------- | ------------------ |
| Session-owned texture         | `owned-texture`    |
| Caller-owned borrowed texture | `borrowed-texture` |
| Native window surface         | `native-surface`   |

The mode is a required positional argument (for example
`zig-map owned-texture`). There is no default mode.

On `--help`, print usage listing the three mode names and exit `0` before
creating a window. On invalid arguments, print usage listing the three mode
names and exit `1` before creating a window.

#### Other flags

The only permitted flag is `--help`. Implementations MUST NOT add other CLI
flags.

### Shell and window

- Initial logical size: `960` × `640` pixels.
- Window MUST be resizable.
- High-DPI / Retina: derive map `RenderTargetExtent` from the window's drawable
  size and content scale (see [Viewport](#viewport)).
- Shutdown triggers on window close.

### Startup logging

On startup, print the items listed in [Startup](#startup) step 8 to stdout, plus
the control help text from [Input](#input) below.

### Input

#### Control scheme

Implementations MUST provide the following interactions and MUST print this help
text once at startup:

```text
Controls:
  left drag: pan
  right drag or Ctrl+left drag: rotate with X, pitch with Y
  scroll: zoom at cursor
  arrows or WASD: pan
  + / -: zoom at center
  Q / E: rotate
  ] / [: pitch
  0: reset pitch and bearing
```

#### Behavioral constants

| Interaction                   | Behavior                                                                                                                                                                               |
| ----------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Left drag                     | `move_by` with pointer delta in logical coordinates.                                                                                                                                   |
| Right drag, or Ctrl+left drag | Adjust bearing by `0.5 × Δx` degrees; adjust pitch by `0.5 × Δy` degrees (same sign convention everywhere).                                                                            |
| Scroll                        | Zoom about cursor: `scale_by(2^(Δ * 0.25), anchor)`. Δ from the toolkit wheel event; scrolling up zooms in (use OS-adjusted deltas as reported—do not undo platform scroll inversion). |
| Arrow keys / WASD             | Pan `120` logical units per key press.                                                                                                                                                 |
| `+` / `-`                     | Zoom `1.25` / `1/1.25` about viewport center.                                                                                                                                          |
| `Q` / `E`                     | Bearing ±`10`° with keyboard animation.                                                                                                                                                |
| `]`                           | Pitch +`5`° (clamped to `[0, 60]`) with animation.                                                                                                                                     |
| `[`                           | Pitch −`5`° (clamped to `[0, 60]`) with animation.                                                                                                                                     |
| `0`                           | Animate bearing and pitch to `0` with keyboard animation.                                                                                                                              |

Keyboard animated moves SHOULD use ~`160` ms duration. Pointer drags use
immediate `move_by` / `jump_to` / `pitch_by`.

On pointer down that starts a drag, cancel in-flight camera transitions before
applying deltas.

Input handlers return whether the camera changed so the render loop can set the
render request.

### Resize triggers

- Subscribe to window size, framebuffer size, and display-scale / content-scale
  events (as available on the platform).

---

## Mobile profile

### Scope

Mobile `*-map` examples add:

- A full-screen or layout-driven map view embedded in the platform app shell.
- Touch camera controls.
- View lifecycle integration (appear, disappear, foreground, background).

### Lifecycle

Mobile examples keep runtime and map state alive across brief disappear and
background transitions. They tear down only on view destruction or app
termination.

Track view visibility and app foreground separately. Run the display-paced
render loop only while the view is visible and the app is in the foreground. The
runtime loop keeps running across these transitions, so loading continues while
the view is off screen.

When the host toolkit destroys or invalidates the presentation surface, close
the render target on the render loop thread, which is the thread that attached
it. Keep runtime and map handles alive. Attach again on that same thread when a
fresh surface is available, per [Reattach](#reattach).

| Transition                       | Behavior                                                                                                                                                           |
| -------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| View will appear                 | Mark the view visible. If the app is in the foreground, start the render loop, refresh viewport, attach or reattach the render target, and set the render request. |
| View did disappear               | Mark the view not visible. Stop the render loop. Close the render target when the presentation surface is destroyed or invalidated.                                |
| App foreground                   | Mark the app foreground. If the view is visible, start the render loop, refresh viewport, attach or reattach the render target, and set the render request.        |
| App background                   | Mark the app background. Stop the render loop. Close the render target when the presentation surface is destroyed or invalidated.                                  |
| View destroyed / app termination | Run [Shared shutdown](#shutdown).                                                                                                                                  |

### Entry and shell

- The map view fills the available layout area or the screen.
- Derive the initial viewport from the view's layout bounds and content scale
  after the view is on screen.
- Attach `native-surface` for the host `CAMetalLayer`, `VkSurfaceKHR`, or
  platform GL surface supplied by the view.
- Shutdown follows view destruction or app termination.
- Minimal platform bundle files required to run on device or simulator are
  permitted. Store distribution and installer UX remain out of scope.

### Input

Translate platform touch input into distinct one-finger pan, two-finger
scale-rotate, two-finger shove, and double-tap gesture states. Platform gesture
recognizers and custom touch trackers are both valid implementations when they
produce the required camera operations. Scale and rotation share one two-finger
state so a single gesture can zoom and rotate in the same update. Shove is an
exclusive two-finger vertical state selected only when vertical centroid motion
dominates before scale or rotation begins.

#### Control scheme

Implementations MUST provide the following touch interactions:

| Interaction                      | Behavior                                                                                                                                                               |
| -------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| One-finger drag                  | `move_by` with pointer delta in logical coordinates.                                                                                                                   |
| Pinch                            | Apply incremental scale deltas while preserving the geographic coordinate under the current two-touch centroid, resetting the scale baseline after each applied delta. |
| Two-finger rotate                | Apply incremental bearing deltas from the change in the two-touch vector angle while preserving the geographic coordinate under the current two-touch centroid.        |
| Two-finger vertical drag (shove) | `pitch -= 0.1 × Δy` degrees (clamp to `[0, 60]`), where `Δy` is the change in two-touch centroid Y in logical coordinates since the last applied update.               |
| Double-tap                       | Zoom to `round(zoom₀) + 1.0` about the tap location with animation (~`160` ms).                                                                                        |

On any gesture begin, cancel in-flight camera transitions before applying
deltas.

Input handlers return whether the camera changed so the render loop can set the
render request.

### Resize triggers

- Subscribe to layout changes, orientation changes, safe-area changes, and
  display-scale / content-scale changes (as available on the platform).

### Logging

- Emit [Startup](#startup) step 8 items and viewport diagnostics through the
  platform log sink (for example `OSLog` on Apple platforms or `logcat` on
  Android).
- Control help is not required on mobile.
