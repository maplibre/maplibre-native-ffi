// swift-tools-version: 6.0
import Foundation
import PackageDescription

let ffiNative = URL(fileURLWithPath: #filePath).resolvingSymlinksInPath()
  .deletingLastPathComponent()
  .appendingPathComponent("../node_modules/@maplibre/native-ffi/native").path
let includes = [
  "-I",
  ffiNative + "/react-native",
  "-I",
  ffiNative + "/node-api-headers",
  "-include",
  ffiNative + "/react-native/symbols.h",
]

let package = Package(
  name: "LifecycleProbe",
  platforms: [.iOS("15.1")],
  products: [.library(name: "LifecycleProbe", targets: ["LifecycleProbe"])],
  dependencies: [
    .package(name: "MaplibreNativeFfi", path: "../MaplibreNativeFfi"),
    .package(name: "ReactNative", path: "../../../../xcframeworks"),
    .package(name: "React-GeneratedCode", path: "../../../ios"),
  ],
  targets: [
    .target(
      name: "LifecycleProbe",
      dependencies: [
        .product(name: "MaplibreNativeFfi", package: "MaplibreNativeFfi"),
        .product(name: "ReactHeaders", package: "ReactNative"),
        .product(name: "ReactNativeHeaders", package: "ReactNative"),
        .product(
          name: "ReactNativeDependenciesHeaders",
          package: "ReactNative"
        ),
        .product(name: "ReactAppHeaders", package: "React-GeneratedCode"),
      ],
      path: ".",
      sources: [
        "ios/LifecycleProbe.mm", "native/Probe.cpp", "native/shim_test.c",
        "native/shim.cpp",
        "native/test_graphics.c", "support/native_probe.c",
        "graphics/graphics.c",
      ],
      publicHeadersPath: "ios",
      cSettings: [
        .headerSearchPath("native"),
        .unsafeFlags(["-fvisibility=hidden"]),
        .headerSearchPath("support"), .headerSearchPath("graphics/include"),
        .headerSearchPath("vulkan"), .unsafeFlags(includes),
      ],
      cxxSettings: [
        .define("DEBUG", .when(configuration: .debug)),
        .define("NDEBUG", .when(configuration: .release)),
      ]
    ),
  ],
  cLanguageStandard: .c11,
  cxxLanguageStandard: .cxx20
)
