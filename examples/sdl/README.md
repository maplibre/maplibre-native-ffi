# SDL example

An SDL 3 app in Zig that shows a continuous map with the Zig binding, on Linux
and FreeBSD under Wayland or X11, on Windows, and on macOS. The library presents
each frame to SDL's window with a graphics context that it creates.

## Run it

From the repository root, build the native library and the app, and run it:

```bash
mise run //examples/sdl:run
```

The task builds this machine's default preset. Pass another one, such as
`linux-gnu-x64-egl`, to run an OpenGL build.

To build the Windows preset with the MSVC SDK on Linux and run the app under
Wine, as the development overview describes:

```bash
mise -E msvc run //examples/sdl:run windows-x64-vulkan
```

Under Wine, loading tiles and glyphs over HTTPS stalls in Wine's TLS, so the map
draws only its background color. The macOS build is untested.

## How it fits together

The app is a Zig package of its own. It depends on the binding in this checkout
by path, and builds against the native library's install with `-Dlocal-build`.
An app outside this repository fetches the binding from a release instead, as
the Zig install guide shows. It also depends on
[castholm/SDL](https://github.com/castholm/SDL), which builds SDL from source
with Zig, so the build needs no SDL, Wayland, or X11 packages from the system.
SDL loads the system's Wayland or X11 libraries at run time.

`src/main.zig` runs the app with SDL's main callbacks, which iterate only after
events arrive, so an idle map costs nothing. On Linux, it asks SDL for its
Wayland driver, and for its X11 driver where there is no Wayland compositor. It
creates a window for an external graphics context, so SDL doesn't draw to it, at
the display's pixel density.

`src/MapView.zig` attaches a render session to the window's native surface with
`Map.attach` and a `RenderTarget`: its `wl_surface` as a `wayland_surface`, its
X11 window as an `xlib_window`, its `HWND` as a `win32_window`, or a
`CAMetalLayer` from SDL's Metal view as a `metal_layer`. With no GPU device, the
session creates one on the platform's own backend, and renders on a library
thread. The wakes and the event handler run on library threads too, so they set
flags and push an SDL event that wakes the main loop, which acts on them. A
redraw wake asks for a frame, and a frame wake drains the frame results.

`src/pacer.zig` paces the frames to the display, since the library presents
without waiting for it, and SDL has no frame clock for a window that it doesn't
draw. On Wayland, each demanded frame commits a frame callback on the window's
surface, and the next demand waits until the compositor calls it. So a hidden
window, which gets no frame callbacks, demands no frames. On X11, a thread waits
out the display's refresh period. On Windows, that thread first waits on
`DwmFlush` for the desktop compositor's next frame. On macOS, the frame pacer
from `FramePacer.create` follows the display and demands the frames.

The window's size in pixels and its display's scale factor set the session's
size in logical pixels and the map's scale factor. When the window changes size
or moves to a display with another scale factor, the view resizes the session
with both. On Wayland, SDL holds a resize back until a frame of the new size
arrives, and asks for one with an expose event, which demands a frame even when
nothing changed. While the window is minimized on Windows, frames finish without
presenting, and the view demands one again when the window shows. Before the
window closes, the view releases the session, the map, and the runtime.

`src/Gestures.zig` turns mouse, touch, touchpad pinch, and key input into camera
changes, and measures each gesture's speed so the map keeps moving after
release. SDL can't tell a touchpad scroll from a mouse wheel, so both zoom.
Everything else matches the browser example in `examples/browser`.
