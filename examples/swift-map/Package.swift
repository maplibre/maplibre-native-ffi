// swift-tools-version: 6.0

import Foundation
import PackageDescription

let xtoolBuild = ProcessInfo.processInfo.environment["MLN_FFI_XTOOL_BUILD"] == "1"
let swiftMapIOSDependencies: [Target.Dependency] = [
  .product(name: "MaplibreNative", package: "maplibre-native-swift"),
]
let swiftMapIOSLinkerSettings: [LinkerSetting] = [
  .linkedFramework("Metal"),
  .linkedFramework("QuartzCore"),
  .linkedFramework("UIKit"),
]
let swiftMapIOSTarget: Target = xtoolBuild
  ? .target(
    name: "SwiftMapIOS",
    dependencies: swiftMapIOSDependencies,
    linkerSettings: swiftMapIOSLinkerSettings
  )
  : .executableTarget(
    name: "SwiftMapIOS",
    dependencies: swiftMapIOSDependencies,
    linkerSettings: swiftMapIOSLinkerSettings
  )

let package = Package(
  name: "swift-map",
  platforms: [.macOS("14.3"), .iOS("14.3")],
  products: [
    .executable(name: "swift-map", targets: ["SwiftMap"]),
    xtoolBuild
      ? .library(name: "swift-map-ios", targets: ["SwiftMapIOS"])
      : .executable(name: "swift-map-ios", targets: ["SwiftMapIOS"]),
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
    swiftMapIOSTarget,
  ]
)
