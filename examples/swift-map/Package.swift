// swift-tools-version: 6.0

import PackageDescription

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
        .unsafeFlags(["-L../../build", "-lmaplibre-native-c"]),
        .linkedFramework("AppKit"),
        .linkedFramework("Metal"),
        .linkedFramework("QuartzCore"),
      ]
    ),
  ]
)
