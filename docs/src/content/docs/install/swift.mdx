---
title: Swift
description: Add the Swift package to a project.
---

## Install

Add the package from its
[distribution repository](https://github.com/maplibre/maplibre-native-ffi-distribution),
which tags each release with its version. It needs Swift 6.2 or later.

```swift title="Package.swift"
.package(
  url: "https://github.com/maplibre/maplibre-native-ffi-distribution",
  exact: "{{version}}"
)
```

Then depend on its `MaplibreNativeFFI` product:

```swift title="Package.swift"
.product(name: "MaplibreNativeFFI", package: "maplibre-native-ffi-distribution")
```

## Apple platforms

On Apple platforms, the package uses the release's
[XCFramework](/maplibre-native-ffi/install/c/#swift-package-manager), so it
needs nothing else.

## Other platforms

On Linux, Windows, and Android, download and unpack the
[C library](/maplibre-native-ffi/install/c/) for your preset, and point
pkg-config at it before you build:

```bash frame="terminal"
export PKG_CONFIG_PATH=<prefix>/share/pkgconfig
```

On Linux, SwiftPM ignores the rpath that pkg-config adds, so pass the prefix's
`lib` directory to the linker:

```bash frame="terminal"
swift build -Xlinker -rpath -Xlinker <prefix>/lib
```

On Windows, put the prefix's `bin` directory on `PATH`, or copy
`maplibre-native-c.dll` next to the executable.

## Android

Initialize the library once per process, before you create a runtime, with the
calling thread's JNI environment and an Android `Context`:

```swift title="main.swift"
try initAndroid(
  jniEnv: NativePointer(bitPattern: jniEnv),
  context: NativePointer(bitPattern: context))
```

## Snapshots

Depend on this repository at `main`, and install the C library from the
[snapshot release](https://github.com/maplibre/maplibre-native-ffi/releases/tag/unstable-native-snapshot)
as for other platforms. This applies to Apple platforms too. SwiftPM names the
package after the repository:

```swift title="Package.swift"
.package(url: "https://github.com/maplibre/maplibre-native-ffi", branch: "main")
```

```swift title="Package.swift"
.product(name: "MaplibreNativeFFI", package: "maplibre-native-ffi")
```
