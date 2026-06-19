// swift-tools-version: 6.0

import Foundation
import PackageDescription

func nativeLinkerFlags() -> [String] {
  guard let nativeBuildDir = Context.environment["MLN_FFI_BUILD_DIR"] else {
    fatalError("MLN_FFI_BUILD_DIR is required")
  }
  let flagsFile = "\(nativeBuildDir)/maplibre-native-c.swift-linker-flags"

  do {
    return try String(contentsOfFile: flagsFile, encoding: .utf8)
      .split { $0.isNewline }
      .map(String.init)
  } catch {
    fatalError("failed to read native linker flags from \(flagsFile): \(error)")
  }
}

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
        .unsafeFlags(nativeLinkerFlags()),
      ]
    ),
    .testTarget(
      name: "MaplibreNativeTests",
      dependencies: ["MaplibreNative", "CMaplibreNativeC"]
    ),
  ]
)
