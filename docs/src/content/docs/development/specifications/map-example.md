---
title: Map example
description: Specification for interactive *-map example programs.
sidebar:
  order: 5
---

Specification for interactive `*-map` example programs: small apps that exercise
language bindings and render-target integrations through a focused map demo.

## Scope

### What every example provides

- All map, runtime, and render access from application code through the
  project’s language binding for that language.
- One top-level map window with resize support.
- Continuous map mode: runtime pumping, event draining, and repaint driven by
  map render events and user input.
- Initial style URL and camera per [Shared defaults](#shared-defaults).
- Camera controls per [Input](#input).
- Support for the three render-target modes (see
  [Render-target modes](#render-target-modes)).
- Graceful process exit when the user closes the window.
- Startup logging that identifies the selected render-target mode and which
  native render backends the loaded library supports.

### What an example is not

A `*-map` program is a focused map demo. It MUST NOT include automated tests or
packaging/installer UX.

---

## Implementations

| Example              | Binding  | Toolkit         | Platforms             | Backends              |
| -------------------- | -------- | --------------- | --------------------- | --------------------- |
| `examples/zig-map`   | Zig      | SDL3            | Linux, macOS, Windows | Vulkan, Metal, OpenGL |
| `examples/rust-map`  | Rust     | winit           | Linux, macOS, Windows | Vulkan                |
| `examples/lwjgl-map` | Java FFM | GLFW, LWJGL     | Linux, macOS, Windows | Vulkan                |
| `examples/swift-map` | Swift    | AppKit, SwiftUI | macOS                 | Metal                 |

---

## Shared defaults

### Style

- Style URL: `https://tiles.openfreemap.org/styles/bright`
- Load the style during map initialization, before the first render.

### Initial camera

| Field   | Value                                                     |
| ------- | --------------------------------------------------------- |
| Center  | latitude `37.7749`, longitude `-122.4194` (San Francisco) |
| Zoom    | `13.0`                                                    |
| Bearing | `12.0` degrees                                            |
| Pitch   | `30.0` degrees                                            |

Apply with an immediate `jump_to` on startup.

### Window

- Initial logical size: `960` × `640` pixels.
- Window MUST be resizable.
- High-DPI / Retina: derive map `RenderTargetExtent` from the window’s drawable
  size and content scale (see [Viewport](#viewport)).

### Map and runtime

- Runtime cache path: `:memory:` (in-memory).
- Map mode: continuous (`MLN_MAP_MODE_CONTINUOUS`).

### Compositor shaders (texture modes)

For `owned-texture` and `borrowed-texture`, the host-owned compositor that
samples the map texture into the window swapchain MUST use a fullscreen textured
quad:

- Vertex shader: fullscreen triangle with pass-through UVs.
- Fragment shader: `texture(map_texture, uv)` (straight copy, standard UV
  orientation).

SPIR-V, MSL, or GLSL source MAY differ by backend; the GPU output MUST match
that pass.

---

## Command-line interface

### Render-target selection

The process MUST accept a render-target mode name:

| Mode                          | CLI value          |
| ----------------------------- | ------------------ |
| Session-owned texture         | `owned-texture`    |
| Caller-owned borrowed texture | `borrowed-texture` |
| Native window surface         | `native-surface`   |

The mode is a required positional argument (for example
`zig-map owned-texture`). There is no default mode.

On `--help` or invalid arguments, print usage listing the three mode names and
exit before creating a window.

Implementations MAY omit support for modes their graphics stack does not provide
(see [Conditional requirements](#conditional-requirements)); omitted modes MUST
be rejected at startup with a clear error if requested on the command line.

### Other flags

The only permitted flag is `--help`. Implementations MUST NOT add other CLI
flags.

---

## Architecture

### Overview

Every `*-map` example splits host responsibilities into the same logical
modules. Names differ by language; boundaries MUST NOT be collapsed into a
single monolithic type.

```mermaid
flowchart TB
  subgraph shell["App shell"]
    EL[Event loop]
    VP[Viewport]
    IN[Input]
  end
  subgraph mapstate["Map state"]
    RT[Runtime]
    MP[Map]
    RS[Render-target session]
  end
  subgraph gfx["Graphics host"]
    BE[Backend context]
    CP[Compositor]
    SC[Presentation]
  end
  shell --> mapstate
  mapstate --> gfx
```

### Logical modules

| Module                | Responsibility                                                                                                       |
| --------------------- | -------------------------------------------------------------------------------------------------------------------- |
| App shell             | Process entry, argument parsing, window creation, main event loop, idle pacing, shutdown ordering.                   |
| Viewport              | Map logical size, physical drawable size, and `scale_factor` for `RenderTargetExtent`.                               |
| Map state             | Owns runtime, map, and render session; loads style and initial camera; attaches render target for the selected mode. |
| Render-target session | Thin wrapper over `RenderSessionHandle`: resize, `render_update`, close; dispatches by texture vs surface.           |
| Backend               | Host-owned device context and window presentation for the active graphics API.                                       |
| Compositor            | Host pass that draws a map-owned or borrowed texture into the swapchain.                                             |
| Input                 | Pointer and keyboard → map camera APIs; prints control help once at startup.                                         |
| Diagnostics           | Optional log callback and consistent error messages on failed setup or camera commands.                              |

Implementations SHOULD mirror this layout in the source tree (separate files or
packages per module).

### Backend and mode matrix

The backend module MUST be a discriminated implementation per render-target mode
(union, sealed hierarchy, or sum type). Adding a mode or backend MUST require a
localized change (new enum variant and dedicated module). Keep each graphics API
and each render-target mode in its own variant or submodule rather than
branching ad hoc through shared draw code.

Each backend variant implements, at minimum:

- `init` / `deinit`
- `resize(viewport)`
- `attachRenderTarget(map, viewport) → session`
- `finishFrame()` (window presentation upkeep; see
  [Conditional requirements](#conditional-requirements))
- `drawTexture(session, viewport)` for texture modes
- `needsRenderTargetReattachOnResize() → bool` (see [Resize](#resize))

---

## Lifecycle

### Startup

Order MUST be:

1. Parse CLI; exit on help or invalid mode.
2. Validate that the loaded native library supports the graphics backend(s) this
   binary targets; fail fast with a readable message if not.
3. Create the window (initial size [Window](#window)).
4. Initialize the graphics backend for the selected mode.
5. Create runtime (`:memory:` cache).
6. Create map with extent from the initial viewport and continuous mode.
7. Load style and apply initial camera.
8. Attach render session for the selected mode.
9. Print render-target mode (and control help).

On failure after partial setup, release already-created handles in reverse order
(session → map → runtime → graphics).

### Shutdown

On window close or fatal error, close resources in order:

1. Finish or wait on in-flight GPU work if the backend requires it.
2. Render session (compositor first when it owns GPU objects separate from the
   session).
3. Map
4. Runtime
5. Graphics context and window.

### Handle ownership

- One runtime per process (owner thread drives `run_once` / pump).
- One map per runtime for the demo.
- One live render session per map at a time.
- Map configuration (style, camera) uses the map handle; render-target extent
  and present use the render session.

---

## Frame loop

Each frame iteration MUST follow this logical sequence:

```mermaid
sequenceDiagram
  participant EL as Event loop
  participant RT as Runtime
  participant MP as Map
  participant RS as Render session
  participant GFX as Backend / compositor

  EL->>EL: Poll window + input events
  Note over EL: Resize may reattach target
  EL->>RT: run_once()
  EL->>RT: poll events → render_pending
  EL->>GFX: finishFrame() / swapchain upkeep
  alt render_pending
    EL->>RS: render_update()
    alt texture mode
      RS->>GFX: acquire frame / draw compositor / present
    else native surface
      RS->>GFX: present via surface session
    end
  end
  Note over EL: Idle sleep when no work
```

Requirements:

- MUST call runtime `run_once` once per loop iteration while the app is running.
- MUST drain runtime events each iteration and set `render_pending` when a
  `map_render_update_available` event targets this map (and MAY also react to
  `map_render_frame_finished` when the event reports `needs_repaint`).
- MUST set `render_pending` when input changes the camera.
- MUST call `render_update` only while `render_pending` is true; clear the flag
  after a successful update that does not need an immediate retry.
- SHOULD treat `invalid_state` from `render_update` as “nothing to draw yet” and
  continue.
- SHOULD idle-sleep briefly when an iteration makes no progress (event poll,
  render, or runtime work).

Texture modes: after a successful `render_update`, MUST run the compositor pass
to copy the map texture into the window swapchain before present.

---

## Viewport

The viewport value MUST contain:

| Field                               | Meaning                                                                   |
| ----------------------------------- | ------------------------------------------------------------------------- |
| `logical_width`, `logical_height`   | Map coordinate extent passed to `MapOptions` / `RenderTargetExtent`.      |
| `physical_width`, `physical_height` | Drawable pixels of the window framebuffer.                                |
| `scale_factor`                      | Ratio between physical and logical sizes (content scale / pixel density). |

Derivation rules:

- Read logical and physical sizes from the window toolkit after creation and on
  every resize / backing-scale change.
- Compute logical dimensions from physical size and scale when the toolkit only
  exposes physical pixels (use `ceil(physical / scale)`, minimum `1`).
- Log viewport changes at informational level with field labels
  `logical=… physical=… scale=…`.

Pass `logical_*` and `scale_factor` to map creation, session attach, and session
`resize`.

---

## Map state

The map state module owns the runtime, map, and render session handles plus
map-specific setup.

### Creation

- Create runtime with `:memory:` cache.
- Create map with current viewport extent and continuous mode.
- Load [style URL](#style).
- Apply [initial camera](#initial-camera).
- Delegate render-session attachment to the backend for the CLI-selected mode.

### Event drain

- Drain all pending runtime events each frame.
- When `map_render_update_available` references this map’s id/source, return
  `render_update_available = true` to the frame loop.

### Resize API

Expose `resize(viewport)` that forwards to the render-target session. For
texture modes, also resize the compositor. When the backend reports
`needsRenderTargetReattachOnResize`, expose
`resizeWithReattachedTarget(viewport, backend)` that destroys the session,
resizes backend-owned textures/surfaces, and re-attaches.

---

## Render-target modes

Three modes MUST be modeled in every example’s architecture (CLI parsing,
backend discriminant, and attach paths). Implementations MUST implement every
mode required by [Conditional requirements](#conditional-requirements) for their
graphics API.

### Mode comparison

| CLI value          | C API concept                            | Compositor | Role                                                        |
| ------------------ | ---------------------------------------- | ---------- | ----------------------------------------------------------- |
| `owned-texture`    | Session-owned backend texture            | Required   | Map allocates texture, host samples it.                     |
| `borrowed-texture` | Caller-owned texture borrowed by session | Required   | Host allocates exportable texture; session renders into it. |
| `native-surface`   | Window presentation surface              | None       | Map renders directly to the window presentation target.     |

### Startup status lines

Startup MUST print the active mode’s CLI value and exactly one line from this
table (character-for-character, including the prefix):

| CLI value          | Printed line                                                                                       |
| ------------------ | -------------------------------------------------------------------------------------------------- |
| `owned-texture`    | `render target status: samples MapLibre-owned texture frames into the host swapchain`              |
| `borrowed-texture` | `render target status: renders into a host-owned texture, then samples it into the host swapchain` |
| `native-surface`   | `render target status: renders directly to the host window surface`                                |

### `owned-texture`

- Attach with the C API owned-texture descriptor for the active graphics API.
- Pass the host graphics context handles required by that descriptor (see
  [Conditional requirements](#conditional-requirements)).
- On `render_update`, acquire the frame/image from the session, draw via
  compositor, release/close the frame per the C API frame lifetime rules.
- Resize the compositor and session per the C API and conditional requirements
  for the active graphics API.

### `borrowed-texture`

- Host creates an exportable texture sized to the viewport (see
  [Conditional requirements](#conditional-requirements)).
- Attach with the borrowed-texture descriptor referencing host-owned handles.
- On `render_update`, sample that texture through the same compositor path as
  `owned-texture`.
- `needsRenderTargetReattachOnResize` MUST return `true`: on resize, destroy the
  render session, recreate host textures, and attach a new session for the new
  extent.

### `native-surface`

- Attach with the C API surface descriptor for window presentation (see
  [Conditional requirements](#conditional-requirements)).
- `render_update` presents through the surface session directly.
- `drawTexture` MUST NOT be called for this mode.
- On resize, call session `resize` and rebuild host presentation; reattach when
  the window toolkit supplies a new surface handle.

---

## Resize

- Subscribe to window size, framebuffer size, and display-scale / content-scale
  events (as available on the platform).
- Recompute viewport; skip rendering if extent is empty.
- If `needsRenderTargetReattachOnResize()` → full session reattach path.
- Else → resize backend swapchain/context, resize compositor, call session
  `resize` with new extent.
- Set `render_pending` after any resize.

---

## Input

### Control scheme

Implementations MUST provide the following interactions and MUST print this help
text once at startup (wording MAY vary only for platform-specific key names):

```text
Controls:
  left drag: pan
  right drag or Ctrl+left drag: rotate with X, pitch with Y
  scroll: zoom at cursor
  arrows or WASD: pan
  + / -: zoom at center
  Q / E: rotate
  PageUp / PageDown or [ / ]: pitch
  0: reset pitch and bearing
```

### Behavioral constants

| Interaction                   | Behavior                                                                                                    |
| ----------------------------- | ----------------------------------------------------------------------------------------------------------- |
| Left drag                     | `move_by` with pointer delta in logical coordinates.                                                        |
| Right drag, or Ctrl+left drag | Adjust bearing by `0.5 × Δx` degrees; adjust pitch by `0.5 × Δy` degrees (same sign convention everywhere). |
| Scroll                        | Zoom about cursor: `scale_by(2^(Δ * 0.25), anchor)`; negate axis as needed for toolkit scroll direction.    |
| Arrow keys / WASD             | Pan `120` logical units per key press.                                                                      |
| `+` / `-`                     | Zoom `1.25` / `1/1.25` about viewport center.                                                               |
| `Q` / `E`                     | Bearing ±`10`° with keyboard animation.                                                                     |
| PageUp / `]`                  | Pitch +`5`° (clamped to `[0, 60]`) with animation.                                                          |
| PageDown / `[`                | Pitch −`5`° with animation.                                                                                 |
| `0`                           | Animate bearing and pitch to `0` (duration ~`220` ms).                                                      |

Keyboard animated moves SHOULD use ~`160` ms duration. Pointer drags use
immediate `move_by` / `jump_to` / `pitch_by`.

On pointer down that starts a drag, cancel in-flight camera transitions before
applying deltas.

Input handlers return whether the camera changed so the frame loop can set
`render_pending`.

---

## Diagnostics

- SHOULD register a native log callback during startup and clear it on shutdown.
- On setup or camera failure, print a short message including the native status
  and diagnostic strings returned by the C API.
- On startup, print which render-target mode is active and the status line for
  that mode (see [Render-target modes](#render-target-modes)).
- MUST print supported native render backends (`metal`, `vulkan`, `opengl`) from
  `mln_supported_render_backend_mask()`.

---

## Conditional requirements

### When Vulkan presents the window

Applies when: the example uses Vulkan for the window surface and swapchain.

- MUST implement render-target modes `owned-texture` and `native-surface`.
- SHOULD implement `borrowed-texture` when the swapchain supports exportable
  textures.
- MUST use one shared Vulkan context (`VkInstance`, `VkDevice`, queue, and
  `VkSurfaceKHR`) for compositor and render session.
- `owned-texture`: attach with the Vulkan owned-texture descriptor; pass those
  shared handles.
- `borrowed-texture`: host allocates an exportable `VkImage` (and view) sized to
  the viewport; attach with the borrowed-texture descriptor.
- `native-surface`: attach with the Vulkan surface / swapchain presentation
  descriptor for the window’s `VkSurfaceKHR`.
- `finishFrame()` MUST maintain the swapchain (recreate or resize on window
  resize, acquire/present each frame as required by the host).
- On viewport resize for texture modes, resize both the compositor and the
  render session when the C API requires both.
- The compositor MUST follow
  [Compositor shaders](#compositor-shaders-texture-modes).

### When presentation goes through a Metal layer or surface

Applies when: the example uses Metal for window presentation.

- MUST implement `native-surface`.
- MAY implement `owned-texture` and `borrowed-texture`.
- `native-surface`: attach with the Metal surface descriptor for the window’s
  `CAMetalLayer`.
- `owned-texture`: attach with the Metal owned-texture descriptor; pass the
  shared Metal device and layer handles required by the C API.
- `borrowed-texture`: host allocates an exportable Metal texture sized to the
  viewport; attach with the borrowed-texture descriptor.

### When the host uses OpenGL or EGL

Applies when: the example uses OpenGL or EGL for window presentation.

- SHOULD implement all three render-target modes when the GL/EGL stack exposes
  owned-texture, borrowed-texture, and surface attach paths.
- `native-surface`: attach with the OpenGL or EGL surface descriptor for the
  window’s platform GL surface.
- `owned-texture`: attach with the OpenGL owned-texture descriptor; pass the
  shared GL context handles required by the C API.
- `borrowed-texture`: host allocates an exportable GL texture sized to the
  viewport; attach with the borrowed-texture descriptor.
- The compositor MUST follow
  [Compositor shaders](#compositor-shaders-texture-modes).

### When map work runs on a single UI thread

Applies when: the platform or host integration requires runtime, map, and render
session use on one UI owner thread.

- All map and render calls MUST run on that thread.
- Window and layer setup MUST follow the C API owner-thread contract for that
  integration.

### When the window toolkit has no single cross-platform event pump

Applies when: the host UI framework does not deliver input, resize, and idle
ticks through one portable poll loop.

- The example MUST still run the [frame loop](#frame-loop) logic each tick
  (runtime pump, event drain, conditional render).
- The example SHOULD drive ticks at roughly display refresh (for example ~60 Hz
  timer or display link).
- [Input](#input) behavior and constants are unchanged; only the event source
  differs.
