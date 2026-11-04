# React Native example

A React Native app that shows a continuous map with the TypeScript binding's
Hermes entry, `@maplibre/native-ffi/hermes`, on Android and iOS. It renders with
OpenGL ES through EGL on Android and with Metal on iOS.

## Run it

From the repository root, serve the JavaScript:

```bash
mise run //examples/react-native:start
```

Then, in another terminal, build the native library and the app, and run it:

```bash
mise -E android run //examples/react-native:android -- --device <serial>
mise run //examples/react-native:ios -- --udid <udid>
```

The Android task builds the library for ARM64, which covers devices and
emulators on ARM64 hosts.

## How it fits together

React Native doesn't know about the binding, so the app carries a small native
host in `host/`, which React Native autolinks:

- `host/cpp/Host.cpp` installs `mln::hermes::Module` in the runtime when
  JavaScript first loads the host's module, with a scheduler that posts work to
  the JavaScript thread through React Native's `CallInvoker`. It cleans the
  module up before React Native destroys the runtime, as on a reload, when React
  Native invalidates the host's module: on the JavaScript thread on iOS, and
  after that thread stops on Android.
- `MapSurfaceView` is a native view that lends its surface to JavaScript: a
  `CAMetalLayer` on iOS, and an EGL window surface of a `SurfaceView` on
  Android. It reports when the platform provides the surface and when it takes
  the surface away, such as when the app goes to the background.
  `host/cpp/Surfaces.cpp` keeps each surface until the session stops using it.
- `src/MapController.ts` attaches a render session to the surface, gives the
  session each new surface, and runs the frame loop, as the browser demo in
  `docs/demo` does. The EGL session renders on the JavaScript thread, and the
  Metal session renders on a library thread.
