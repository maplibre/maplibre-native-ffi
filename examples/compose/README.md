# Jetpack Compose example

An Android app that shows a continuous map with the Kotlin binding, in a single
Compose screen. It renders with OpenGL ES into the `SurfaceView` of Compose's
`AndroidExternalSurface`.

## Run it

From the repository root, build the native library and the app, install it on
the connected device or emulator, and start it:

```bash
mise -E android run //examples/compose:android
```

The task builds for ARM64, which covers devices and emulators on ARM64 hosts.
Pass another architecture, such as `x64`, for an x86-64 emulator, and set
`ANDROID_SERIAL` to pick a device.

## How it fits together

The app has no native code of its own. It depends on the binding and its OpenGL
runtime by their Maven coordinates, as any app does. `settings.gradle.kts`
includes the repository's own Gradle build, which supplies both from this
checkout and packages the C library that `//:build` installed under `build/`.
The tasks pass that build its `-Pmaplibre.*` properties.

`MapController.kt` holds the map in a view model, so the map and its render
session outlive the activity when a rotation recreates it. It attaches the
session to the first surface, gives the session each later one, and releases the
surface when the platform destroys it, as the React Native example in
`examples/react-native` does. The session renders on a library thread with an
OpenGL ES context that the library makes for the window, and a frame pacer on
the main thread's choreographer demands its frames. `MapGestures.kt` turns
touches into camera changes.
