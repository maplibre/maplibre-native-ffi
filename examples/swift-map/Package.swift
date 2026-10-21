// swift-tools-version: 6.0

import PackageDescription

let package = Package(
  name: "swift-map",
  platforms: [.macOS("14.3"), .iOS("15.0")],
  products: [
    .executable(name: "swift-map", targets: ["SwiftMap"]),
    .executable(name: "swift-map-ios", targets: ["SwiftMapIOS"]),
  ],
  dependencies: [
    .package(name: "maplibre-native-ffi", path: "../.."),
  ],
  targets: [
    .executableTarget(
      name: "SwiftMap",
      dependencies: [
        .product(name: "MaplibreNativeFFI", package: "maplibre-native-ffi"),
      ],
      linkerSettings: [
        .linkedFramework("AppKit"),
        .linkedFramework("Metal"),
        .linkedFramework("QuartzCore"),
      ]
    ),
    .executableTarget(
      name: "SwiftMapIOS",
      dependencies: [
        .product(name: "MaplibreNativeFFI", package: "maplibre-native-ffi"),
      ],
      linkerSettings: [
        .linkedFramework("Metal"),
        .linkedFramework("QuartzCore"),
        .linkedFramework("UIKit"),
      ]
    ),
  ]
)
