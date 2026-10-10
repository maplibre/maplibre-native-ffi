# Examples

Each `*-map` example renders an interactive map through one language binding and
one host toolkit. `zig-readback` renders one still image headless and reads it
back. Examples carry no tests of their own: the `smoke` task is their only
self-check, and CI runs it on every target whose backend the runner can render.

The examples share the contract below, so that CI and maintainers can treat them
alike. [Concepts](../docs/src/content/docs/concepts.md) describes the execution
model that they follow.

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

- A wake runs on a native thread. It only schedules a drain or a driver service
  on the host loop, and returns.
- A frame result of target-not-ready, or a rendered frame that the host could
  not present, demands again after about one display refresh without the
  render-if-needed flag, because the attempt consumed the map update.
- A Vulkan core worker in a texture mode submits on a second queue from the
  graphics family, because the host compositor submits on its own queue at the
  same time. The session waits on its own queue and never on the device, so the
  compositor's queue needs no lock against it. A device with one graphics queue,
  such as MoltenVK, uses the caller driver for the texture modes. The host calls
  `vkDeviceWaitIdle` only after detach completes.
- OpenGL sessions use the caller driver. An OpenGL owned texture attaches on a
  context shared with the host, because one on a private EGL context offers CPU
  readback instead of frame acquisition.
- A core-worker borrowed texture keeps at most one demand outstanding, because
  the texture belongs to the session from a demand until its result.
- A borrowed-texture resize replaces the target and also resizes the map,
  because a target replacement leaves the map's extent unchanged.
- A session fixes its scale factor at attachment, so a scale change reattaches.
- The process never exits while a session can still make graphics calls: it
  detaches, or abandons the session when detach fails.
