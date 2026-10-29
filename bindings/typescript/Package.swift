// swift-tools-version: 6.0
import PackageDescription

let package = Package(
  name: "MaplibreNativeFfi",
  platforms: [.iOS("15.1")],
  products: [.library(
    name: "MaplibreNativeFfi",
    targets: ["MaplibreNativeFfi"]
  )],
  // RN mounts npm packages at build/generated/autolinking/libs/<name>.
  dependencies: [
    .package(name: "ReactNative", path: "../../../../xcframeworks"),
    .package(name: "React-GeneratedCode", path: "../../../ios"),
  ],
  targets: [
    .binaryTarget(
      name: "MaplibreNativeC",
      path: "native/MaplibreNativeC.xcframework"
    ),
    .target(
      name: "MaplibreNativeFfi",
      dependencies: [
        "MaplibreNativeC",
        .product(name: "ReactHeaders", package: "ReactNative"),
        .product(name: "ReactNativeHeaders", package: "ReactNative"),
        .product(
          name: "ReactNativeDependenciesHeaders",
          package: "ReactNative"
        ),
        .product(name: "ReactAppHeaders", package: "React-GeneratedCode"),
      ],
      path: "native",
      exclude: ["MaplibreNativeC.xcframework"],
      sources: [
        "generated_addon.c", "generated_values.c", "runtime.c", "realm.c",
        "react-native/shim.cpp", "react-native/module.cpp",
        "react-native/ios/MaplibreNativeFfi.mm",
      ],
      publicHeadersPath: "react-native/ios",
      cSettings: [
        .headerSearchPath("include"),
        .headerSearchPath("node-api-headers"),
        .headerSearchPath("react-native"),
        .unsafeFlags(["-include", "symbols.h"]),
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
