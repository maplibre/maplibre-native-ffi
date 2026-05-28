// swift-tools-version: 6.0

import PackageDescription

let package = Package(
  name: "swift-map",
  platforms: [.macOS("14.3")],
  products: [
    .executable(name: "swift-map", targets: ["SwiftMap"]),
  ],
  dependencies: [
    .package(name: "maplibre-native-swift", path: "../../bindings/swift"),
  ],
  targets: [
    .executableTarget(
      name: "SwiftMap",
      dependencies: [
        .product(name: "MaplibreNative", package: "maplibre-native-swift"),
      ],
      linkerSettings: [
        .linkedFramework("AppKit"),
        .linkedFramework("Metal"),
        .linkedFramework("QuartzCore"),
      ]
    ),
  ]
)
