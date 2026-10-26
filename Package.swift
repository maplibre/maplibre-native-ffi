// swift-tools-version: 6.2

// This manifest sits at the repository root rather than beside the sources it
// describes, because SwiftPM resolves a package from the repository root and
// offers no way to point at a manifest in a subdirectory. Every target names an
// explicit path under bindings/swift/ in return.

import PackageDescription

let testDependencies: [Target.Dependency] = [
  "MaplibreNativeFFI",
  "MaplibreNativeC",
  "GraphicsSupport",
]

/// The simulator test runner is a root executable target, which SwiftPM builds
/// by --product without listing it here for consumers.
let products: [Product] = [
  .library(name: "MaplibreNativeFFI", targets: ["MaplibreNativeFFI"]),
]

/// Only the API reference task needs the DocC plugin, so consumers don't
/// resolve it.
let dependencies: [Package.Dependency] =
  Context.environment["MLN_FFI_SWIFT_DOCC"] == nil ? [] : [
    .package(
      url: "https://github.com/swiftlang/swift-docc-plugin",
      from: "1.4.3"
    ),
  ]

let targets: [Target] = [
  .target(
    name: "GraphicsSupport",
    path: "tests/graphics",
    exclude: ["README.md"],
    cSettings: [
      .headerSearchPath(
        "../../third_party/maplibre-native/vendor/Vulkan-Headers/include"
      ),
    ],
    linkerSettings: [.linkedLibrary("dl", .when(platforms: [.linux]))]
  ),
  .systemLibrary(
    name: "MaplibreNativeC",
    path: "bindings/swift/Sources/MaplibreNativeC",
    pkgConfig: "maplibre-native-c"
  ),
  .target(
    name: "MaplibreNativeFFI",
    dependencies: ["MaplibreNativeC"],
    path: "bindings/swift/Sources/MaplibreNativeFFI",
    // An async operation runs on its caller's executor until it first
    // suspends, which is after it submits to native, so operations that one
    // actor starts in order reach native in that order.
    swiftSettings: [.enableUpcomingFeature("NonisolatedNonsendingByDefault")]
  ),
  .target(
    name: "MaplibreNativeFFITestCases",
    dependencies: testDependencies,
    path: "bindings/swift/Tests/MaplibreNativeFFITests"
  ),
  .testTarget(
    name: "MaplibreNativeFFITests",
    dependencies: ["MaplibreNativeFFITestCases"],
    path: "bindings/swift/Tests/MaplibreNativeFFITestsHost"
  ),
  .executableTarget(
    name: "MaplibreNativeFFIIOSSimulatorTests",
    dependencies: ["MaplibreNativeFFITestCases"],
    path: "bindings/swift/Tests/MaplibreNativeFFIIOSSimulatorTests"
  ),
]

let package = Package(
  name: "maplibre-native-ffi",
  platforms: [
    .macOS("14.3"),
    .iOS("15.0"),
    .macCatalyst("15.0"),
    .tvOS("15.0"),
    .visionOS("1.0"),
  ],
  products: products,
  dependencies: dependencies,
  targets: targets
)
