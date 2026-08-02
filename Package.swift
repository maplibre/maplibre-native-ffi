// swift-tools-version: 6.0

// This manifest sits at the repository root rather than beside the sources it
// describes, because SwiftPM resolves a package from the repository root and
// offers no way to point at a manifest in a subdirectory. Every target names an
// explicit path under bindings/swift/ in return.

import PackageDescription

let testDependencies: [Target.Dependency] = [
  "MaplibreNativeFFI",
  "CMaplibreNativeC",
]

let testSourceFiles = [
  "MaplibreNativeFFITests/CameraAdvancedTests.swift",
  "MaplibreNativeFFITests/HandleIdentityTests.swift",
  "MaplibreNativeFFITests/LoggingTests.swift",
  "MaplibreNativeFFITests/MapHandleTests.swift",
  "MaplibreNativeFFITests/MaplibreTests.swift",
  "MaplibreNativeFFITests/NativeHandleLeakTestSupport.swift",
  "MaplibreNativeFFITests/OfflineTests.swift",
  "MaplibreNativeFFITests/ProjectionTests.swift",
  "MaplibreNativeFFITests/QueryTests.swift",
  "MaplibreNativeFFITests/RenderTests.swift",
  "MaplibreNativeFFITests/RuntimeTests.swift",
  "MaplibreNativeFFITests/StyleTests.swift",
  "MaplibreNativeFFITests/SupportHelperTests.swift",
  "MaplibreNativeFFITests/SyntheticHandles.swift",
  "MaplibreNativeFFITests/ValueTests.swift",
  "MaplibreNativeFFITests/WakeSourceTests.swift",
]

let products: [Product] = [
  .library(name: "MaplibreNativeFFI", targets: ["MaplibreNativeFFI"]),
  .executable(
    name: "MaplibreNativeFFIIOSSimulatorTests",
    targets: ["MaplibreNativeFFIIOSSimulatorTests"]
  ),
]

let targets: [Target] = [
  .systemLibrary(
    name: "CMaplibreNativeC",
    path: "bindings/swift/Sources/CMaplibreNativeC",
    pkgConfig: "maplibre-native-c"
  ),
  .target(
    name: "MaplibreNativeFFI",
    dependencies: ["CMaplibreNativeC"],
    path: "bindings/swift/Sources/MaplibreNativeFFI",
    linkerSettings: [
      .linkedLibrary("c++", .when(platforms: [.iOS])),
      .linkedLibrary("objc", .when(platforms: [.iOS])),
      .linkedLibrary("sqlite3", .when(platforms: [.iOS])),
      .linkedLibrary("z", .when(platforms: [.iOS])),
      .linkedFramework("CoreFoundation", .when(platforms: [.iOS])),
      .linkedFramework("CoreGraphics", .when(platforms: [.iOS])),
      .linkedFramework("CoreText", .when(platforms: [.iOS])),
      .linkedFramework("Foundation", .when(platforms: [.iOS])),
      .linkedFramework("ImageIO", .when(platforms: [.iOS])),
      .linkedFramework("Metal", .when(platforms: [.iOS])),
      .linkedFramework("MetalKit", .when(platforms: [.iOS])),
      .linkedFramework("QuartzCore", .when(platforms: [.iOS])),
    ]
  ),
  .target(
    name: "MaplibreNativeFFITestCases",
    dependencies: testDependencies,
    path: "bindings/swift/Tests",
    exclude: [
      "MaplibreNativeFFIIOSSimulatorTests",
      "MaplibreNativeFFITestsHost",
    ],
    sources: testSourceFiles
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
  platforms: [.macOS("14.3"), .iOS("14.3")],
  products: products,
  dependencies: [
    .package(
      url: "https://github.com/swiftlang/swift-docc-plugin",
      from: "1.4.3"
    ),
  ],
  targets: targets
)
