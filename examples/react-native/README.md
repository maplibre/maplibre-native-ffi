# React Native example

A React Native app that shows a continuous map with the TypeScript binding's
Hermes entry, `@maplibre/native-ffi/hermes`, on Android and iOS. It renders with
OpenGL ES on Android and with Metal on iOS.

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

The app has no native code of its own. It imports the binding from
[`@maplibre/native-ffi-react-native`](../../bindings/typescript/react-native),
which installs the binding in the runtime and draws on a native view,
`MaplibreSurfaceView`.

`src/MapController.ts` attaches a render session to the view's surface, gives
the session each new surface, and runs the frame loop, as the browser demo in
`examples/browser` does. Both sessions render on a library thread, and on
Android the library makes the OpenGL ES context for the window.
