# Examples

Each `*-map` example renders an interactive map through one language binding and
one host toolkit. `zig-readback` renders one still image headless and reads it
back. Examples carry no tests of their own: the `smoke` task is their only
self-check, and CI runs it on every target whose backend the runner can render.

The examples share tasks, render-target modes, and smoke output for CI.
[Concepts](../docs/src/content/docs/concepts.md) describes the execution model
that they follow.

## Tasks

Every example's `mise.toml` defines the same tasks:

- `build [preset]` builds the example against the native install of `preset`.
- `run [render-target] [--preset <preset>]` builds and launches a desktop
  example. The `ffi:example-run` template in the root `mise.toml` defaults the
  render target to `owned-texture`.
- `smoke [preset]` runs the example in smoke mode once for each render-target
  mode that it supports, and fails when any run fails. It needs no network.

## Render-target modes

A desktop program takes the mode as its one required positional argument. The
root `ffi:example-run` template lists the same names.

| Argument           | Render target                                     |
| ------------------ | ------------------------------------------------- |
| `owned-texture`    | Owned texture target, composed into the swapchain |
| `borrowed-texture` | Borrowed texture target, composed likewise        |
| `native-surface`   | Native surface of the window or view              |

`--help` prints usage and exits `0`, and an invalid argument prints usage and
exits `1`, both before a window opens. `compose-map` has one render path, a
borrowed texture shared with Skiko, so it takes no argument. A mobile example
always uses `native-surface`.

At startup, an example prints the mode, a `render target status:` line, and
`render driver: core-worker` or `render driver: caller-graphics-thread`.

## Smoke mode

- A desktop example enters smoke mode when `MLN_EXAMPLE_SMOKE` is `1`, with the
  same command line as a normal run. `android-map` enters it when its launch
  intent carries the boolean extra `smoke` set to `true`.
- In smoke mode, the example loads an inline style in place of the style URL and
  renders where no one sees it.
- After the first frame that it shows, the example prints
  `smoke: rendered a frame`, shuts down, and exits `0`. The Android smoke script
  waits for that line in logcat.
- When setup fails, or no frame renders within 60 seconds, a desktop example
  prints the failure and exits nonzero.

A Linux runner without a display runs the task under Xvfb or the toolkit's
offscreen platform, with Mesa's software drivers.

Some examples build only for part of the presets:

- `rust-map` drives OpenGL on Linux and Windows only, so a macOS OpenGL preset
  has no build of it.
- `go-map` drives OpenGL through EGL and builds on the Linux EGL presets.
- `swift-map-ios` is a UIKit app with no headless path, so CI builds it for the
  iOS simulator and runs no smoke.
- On the Windows Vulkan presets, `compose-map` shares frames with Skiko's
  Direct3D 12 through `VK_KHR_external_memory_win32`. Lavapipe, the runners'
  only Vulkan driver, lacks that extension, so CI builds `compose-map` there and
  runs no smoke.

## Shared defaults

| Setting        | Value                                                                          |
| -------------- | ------------------------------------------------------------------------------ |
| Style URL      | `https://tiles.openfreemap.org/styles/bright`                                  |
| Initial camera | Latitude `37.7749`, longitude `-122.4194`, zoom `13`, bearing `12`, pitch `30` |
| Runtime cache  | `:memory:`                                                                     |
| Window size    | `960` × `640` logical pixels, resizable                                        |

Desktop examples print the same `Controls:` help and map the same keys and
pointer gestures; keep them in step when one changes.

## Integration traps

These rules hold in every example, and the code alone does not show why:

- A wake can run on a native or submitting thread. It schedules a drain or
  driver service on the host loop and returns without blocking.
- After a rendered frame, the host re-arms a demand with the wait-for-update
  flag. The demand waits until the map has another update, including after a
  resize.
- After a frame result of target-not-ready, or a rendered frame that the host
  could not show, the host demands again after about one display refresh,
  because neither causes a map update. The demand omits the render-if-needed
  flag, because a frame that the host could not show consumed its update.
- A Vulkan core worker submits to the host's one graphics queue and takes the
  host's queue lock around each call on it, because the host compositor submits
  to that queue too and Vulkan requires the calls on one queue to be externally
  synchronized. The host takes the same lock around its own queue calls and
  releases it before calling the C API. A JVM or .NET example uses a lock
  without an owner thread, because the session takes and releases it in separate
  upcalls. The host calls `vkDeviceWaitIdle` only after detach completes.
- OpenGL sessions use the caller driver. An OpenGL owned texture attaches on a
  context shared with the host, because one on a private EGL context offers CPU
  readback instead of frame acquisition.
- Both texture modes use a ring of two textures. The host acquires the newest
  rendered frame and holds it until it acquires a newer one, so the session
  renders into the other texture while the compositor samples the held one.
  `compose-map` hands each frame to its bridge, which draws it after the render
  call returns, so it releases a frame no earlier than the next render call.
- A borrowed-texture resize releases the held frame and replaces the ring.
  Replacement also resizes the map. Keep the outgoing ring alive until the
  replacement completes, then request a frame at the new size.
- A resize retains the session's scale factor. A target replacement or a new
  attachment can change it; the map retains its creation scale factor. The
  [session reference](../include/maplibre_native_c/render_session.h) states the
  resize contract.
- The process never exits while a session can still make graphics calls: it
  detaches, or abandons the session when detach fails. When an abandon keeps
  graphics objects until the process exits, a Vulkan example keeps its device,
  surface, and instance that long too, because the kept objects are their
  children.
