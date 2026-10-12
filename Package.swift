// swift-tools-version: 6.2

// This manifest sits at the repository root rather than beside the sources it
// describes, because SwiftPM resolves a package from the repository root and
// offers no way to point at a manifest in a subdirectory. Every target names an
// explicit path under bindings/swift/ in return.

import PackageDescription

let testDependencies: [Target.Dependency] = [
  "MaplibreNativeFFI",
  "CMaplibreNativeC",
  "GraphicsSupport",
]

let products: [Product] = [
  .library(name: "MaplibreNativeFFI", targets: ["MaplibreNativeFFI"]),
  .executable(
    name: "MaplibreNativeFFIIOSSimulatorTests",
    targets: ["MaplibreNativeFFIIOSSimulatorTests"]
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
    name: "CMaplibreNativeC",
    path: "bindings/swift/Sources/CMaplibreNativeC",
    pkgConfig: "maplibre-native-c"
  ),
  .target(
    name: "MaplibreNativeFFI",
    dependencies: ["CMaplibreNativeC"],
    path: "bindings/swift/Sources/MaplibreNativeFFI",
    // An async operation runs on its caller's executor until it first
    // suspends, which is after it submits to native, so operations that one
    // actor starts in order reach native in that order.
    swiftSettings: [.enableUpcomingFeature("NonisolatedNonsendingByDefault")],
    linkerSettings: [
      .linkedLibrary("c++", .when(platforms: [.iOS, .tvOS])),
      .linkedLibrary("objc", .when(platforms: [.iOS, .tvOS])),
      .linkedLibrary("sqlite3", .when(platforms: [.iOS, .tvOS])),
      .linkedLibrary("z", .when(platforms: [.iOS, .tvOS])),
      .linkedFramework("CoreFoundation", .when(platforms: [.iOS, .tvOS])),
      .linkedFramework("CoreGraphics", .when(platforms: [.iOS, .tvOS])),
      .linkedFramework("CoreText", .when(platforms: [.iOS, .tvOS])),
      .linkedFramework("Foundation", .when(platforms: [.iOS, .tvOS])),
      .linkedFramework("ImageIO", .when(platforms: [.iOS, .tvOS])),
      .linkedFramework("Metal", .when(platforms: [.iOS, .tvOS])),
      .linkedFramework("MetalKit", .when(platforms: [.iOS, .tvOS])),
      .linkedFramework("QuartzCore", .when(platforms: [.iOS, .tvOS])),
    ]
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
    .iOS("15.5"),
    .macCatalyst("15.5"),
    .tvOS("15.5"),
  ],
  products: products,
  dependencies: [
    .package(
      url: "https://github.com/swiftlang/swift-docc-plugin",
      from: "1.4.3"
    ),
  ],
  targets: targets
)
