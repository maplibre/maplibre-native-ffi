// swift-tools-version: 6.0

import PackageDescription

let nativeBuildDir = Context.environment["MLN_FFI_BUILD_DIR"] ?? {
  fatalError("MLN_FFI_BUILD_DIR is required")
}()

let package = Package(
  name: "maplibre-native-swift",
  platforms: [.macOS("14.3")],
  products: [
    .library(name: "MaplibreNative", targets: ["MaplibreNative"]),
  ],
  targets: [
    .systemLibrary(
      name: "CMaplibreNativeC"
    ),
    .target(
      name: "MaplibreNative",
      dependencies: ["CMaplibreNativeC"],
      linkerSettings: [
        .unsafeFlags([
          "-L\(nativeBuildDir)",
          "-lmaplibre-native-c",
          "-Xlinker",
          "-rpath",
          "-Xlinker",
          nativeBuildDir,
        ])
      ]
    ),
    .testTarget(
      name: "MaplibreNativeTests",
      dependencies: ["MaplibreNative", "CMaplibreNativeC"]
    ),
  ]
)
