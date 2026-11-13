# @maplibre/native-ffi-react-native

Runs MapLibre Native FFI's Hermes binding in a React Native app, on Android and
iOS, with the new architecture. Add it to the app, and import the binding from
this package:

```tsx
import {
  MaplibreSurfaceView,
  RuntimeHandle,
  surfaces,
} from "@maplibre/native-ffi-react-native";
```

Importing the package installs the binding in the runtime first, so the app
needs no native code of its own. The package carries the C library, built with
OpenGL ES on Android and Metal on iOS.

## Surfaces

`MaplibreSurfaceView` lends its native surface to JavaScript: a `CAMetalLayer`
on iOS, and the window of a `SurfaceView` on Android. It reports a surface ID
with `onSurfaceAvailable` when the platform provides the surface, and with
`onSurfaceLost` when the platform takes it away, such as when the app goes to
the background.

Pass the ID to `surfaces.acquire()` for the handles to attach a session with, as
the `metalLayer` or `androidWindow` target of `MapHandle.attach()`. Once the
session no longer uses the surface, call `surfaces.release()`. The package keeps
each surface until then, even after the view lets go of it. See
`examples/react-native` for a full app.

## Lifecycle

The module installs the binding when JavaScript first loads it, with a scheduler
that posts work to the JavaScript thread through React Native's `CallInvoker`.
React Native invalidates the module before it destroys the runtime, as on a
reload, and the module cleans the binding up then. That ends every session and
releases every surface that JavaScript holds.
