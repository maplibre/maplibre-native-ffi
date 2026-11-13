# WPF example

A WPF app in C# that shows a continuous map with the .NET binding, on Windows.
The library presents each frame to a Win32 child window with a Vulkan device
that it creates.

## Run it

From the repository root, build the native library and the app, and run it:

```bash
mise run //examples/wpf:run
```

On Linux, the task builds the Windows preset with the MSVC SDK and runs the app
under Wine, as the development overview describes:

```bash
mise -E msvc run //examples/wpf:run
```

Under Wine, loading tiles and glyphs over HTTPS stalls in Wine's TLS, so the map
draws only its background color.

## How it fits together

The app builds with `dotnet build` against the binding's project and the native
library's install, which `bindings/dotnet/Maplibre.NativeFfi.C.Local.props`
copies beside it. An app outside this repository references the
`Maplibre.NativeFfi` package instead.

`MapView.cs` is an `HwndHost` that holds the map's window, a Win32 child window.
`MapController.cs` attaches a render session to it with `Attach` and a Win32
window target. The session renders on a library thread with a Vulkan device that
the library creates, and presents to the window itself. A redraw wake asks for a
frame, and the controller demands it at WPF's next frame, from
`CompositionTarget.Rendering`, which the desktop compositor paces to the
display. WPF renders frames only while something subscribes to that event, so an
idle map costs nothing. A frame wake drains the frame results. While the window
is minimized, the frames finish without presenting.

The child window's client size, in pixels, sets the session's size, which the
map's scale factor turns into logical pixels. The map keeps the scale factor of
the display where it started. When the window goes, the controller releases the
session's target and waits for the release, so the session never presents to a
destroyed window. A later window gets the same session. Before the main window
closes, it releases the session, the map, and the runtime.

WPF can't draw over a hosted window, so the status shows in `StatusOverlay.cs`,
a window of its own that the main window owns and that follows the map's corner.
`MapView.cs` turns mouse, touch, and key input into camera changes. Input to a
hosted window skips WPF's input system, so it feeds its mouse and touch points
to WPF's `ManipulationProcessor2D`. When a gesture ends, it passes the
processor's release velocities to the map, which keeps the camera moving after a
flick. Everything else matches the Compose example in `examples/compose`.
