// swift-tools-version: 6.0

import PackageDescription

let nativeBuildDir = Context.environment["MLN_FFI_BUILD_DIR"] ?? "../../build/host"

let package = Package(
  name: "swift-map",
  platforms: [.macOS(.v14)],
  products: [
    .executable(name: "swift-map", targets: ["SwiftMap"]),
  ],
  targets: [
    .systemLibrary(
      name: "CMapLibreNativeC"
    ),
    .executableTarget(
      name: "SwiftMap",
      dependencies: ["CMapLibreNativeC"],
      linkerSettings: [
        .unsafeFlags(["-L\(nativeBuildDir)", "-lmaplibre-native-c"]),
        .linkedFramework("AppKit"),
        .linkedFramework("Metal"),
        .linkedFramework("QuartzCore"),
      ]
    ),
  ]
)
