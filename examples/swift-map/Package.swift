// swift-tools-version: 6.0

import PackageDescription

let isIOS = Context.environment["MISE_ENV"]?
  .hasPrefix("ios-") == true

var products: [Product] = []
var targets: [Target] = []

if isIOS {
  products.append(.executable(name: "swift-map-ios", targets: ["SwiftMapIOS"]))
  targets.append(
    .executableTarget(
      name: "SwiftMapIOS",
      dependencies: [
        .product(name: "MaplibreNative", package: "maplibre-native-swift"),
      ],
      linkerSettings: [
        .linkedFramework("Metal"),
        .linkedFramework("QuartzCore"),
        .linkedFramework("UIKit"),
      ]
    )
  )
} else {
  products.append(.executable(name: "swift-map", targets: ["SwiftMap"]))
  targets.append(
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
    )
  )
}

let package = Package(
  name: "swift-map",
  platforms: [.macOS("14.3"), .iOS("14.3")],
  products: products,
  dependencies: [
    .package(name: "maplibre-native-swift", path: "../../bindings/swift"),
  ],
  targets: targets
)
