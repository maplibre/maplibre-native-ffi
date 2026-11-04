# SwiftUI example

A SwiftUI app that shows a continuous map with the Swift binding on iOS and
macOS. It renders with Metal into a `CAMetalLayer`.

## Run it

From the repository root, build the native library and the app, and run it:

```bash
mise run //examples/swiftui:ios -- --udid <udid>
mise run //examples/swiftui:macos
```

Without `--udid`, the iOS task runs the app in this worktree's simulator from
devices-cli. The C library has an arm64 simulator slice only.

## How it fits together

The app depends on the Swift package at the repository root, and links the C
library as an XCFramework that the tasks make from the local builds in `build/`.
[XcodeGen](https://github.com/yonaskolb/XcodeGen) writes the Xcode project from
`project.yml`. To work in Xcode, run `mise run //examples/swiftui:project`, then
open `MaplibreExample.xcodeproj`. When Xcode resolves the package without the
`MLN_FFI_SWIFT_DOCC` variable that the tasks set, it deletes the repository's
`Package.resolved`; restore it with Git.

`Sources/MapSurface.swift` hosts a view backed by a `CAMetalLayer`, and reports
its layer, size, and scale factor. `Sources/MapController.swift` attaches a
render session to the layer on a library thread, and paces its frames with a
frame pacer that follows the display. When the app goes to the background, the
controller releases the session's target and keeps the session, and gives it the
layer again when the app comes back. A move to a display of another density
takes a new map, as the map renders at one scale factor for its life. Everything
else matches the browser example in `examples/browser`.
