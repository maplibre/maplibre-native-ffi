// swift-tools-version: 6.0

import PackageDescription

let testDependencies: [Target.Dependency] = [
  "MaplibreNative",
  "CMaplibreNativeC",
]

let testSourceFiles = [
  "MaplibreNativeTests/CameraAdvancedTests.swift",
  "MaplibreNativeTests/HandleIdentityTests.swift",
  "MaplibreNativeTests/LoggingTests.swift",
  "MaplibreNativeTests/MapHandleTests.swift",
  "MaplibreNativeTests/MaplibreTests.swift",
  "MaplibreNativeTests/NativeHandleLeakTestSupport.swift",
  "MaplibreNativeTests/OfflineTests.swift",
  "MaplibreNativeTests/ProjectionTests.swift",
  "MaplibreNativeTests/QueryTests.swift",
  "MaplibreNativeTests/RenderTests.swift",
  "MaplibreNativeTests/RuntimeTests.swift",
  "MaplibreNativeTests/StyleTests.swift",
  "MaplibreNativeTests/SupportHelperTests.swift",
  "MaplibreNativeTests/SyntheticHandles.swift",
  "MaplibreNativeTests/ValueTests.swift",
  "MaplibreNativeTests/WakeSourceTests.swift",
]

let products: [Product] = [
  .library(name: "MaplibreNative", targets: ["MaplibreNative"]),
  .executable(
    name: "MaplibreNativeIOSSimulatorTests",
    targets: ["MaplibreNativeIOSSimulatorTests"]
  ),
]

let targets: [Target] = [
  .systemLibrary(
    name: "CMaplibreNativeC",
    pkgConfig: "maplibre-native-c"
  ),
  .target(
    name: "MaplibreNative",
    dependencies: ["CMaplibreNativeC"],
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
    name: "MaplibreNativeTestCases",
    dependencies: testDependencies,
    path: "Tests",
    exclude: [
      "MaplibreNativeIOSSimulatorTests",
      "MaplibreNativeTestsHost",
    ],
    sources: testSourceFiles
  ),
  .testTarget(
    name: "MaplibreNativeTests",
    dependencies: ["MaplibreNativeTestCases"],
    path: "Tests/MaplibreNativeTestsHost"
  ),
  .executableTarget(
    name: "MaplibreNativeIOSSimulatorTests",
    dependencies: ["MaplibreNativeTestCases"],
    path: "Tests/MaplibreNativeIOSSimulatorTests"
  ),
]

let package = Package(
  name: "maplibre-native-swift",
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
