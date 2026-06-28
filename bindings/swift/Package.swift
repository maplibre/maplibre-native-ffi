// swift-tools-version: 6.0

import Foundation
import PackageDescription

func nativeLinkerFlags() -> [String] {
  guard let nativeInstallDir = Context
    .environment["MLN_FFI_NATIVE_INSTALL_DIR"]
  else {
    fatalError("MLN_FFI_NATIVE_INSTALL_DIR is required")
  }
  let libDir = "\(nativeInstallDir)/lib"
  var flags = ["-L", libDir, "-lmaplibre-native-c"]

  let isIOSDevice = Context.environment["MISE_ENV"]?
    .hasPrefix("ios-") == true &&
    Context.environment["MISE_ENV"]?.hasPrefix("ios-simulator-") != true
  if isIOSDevice {
    flags += [
      "-lmbgl-core",
      "-lmbgl-freetype",
      "-lmbgl-harfbuzz",
      "-lmbgl-vendor-csscolorparser",
      "-lmbgl-vendor-nunicode",
      "-lmbgl-vendor-parsedate",
      "-lmbgl-vendor-sqlite",
      "-lmlt-cpp",
      "-lc++",
      "-lobjc",
      "-lsqlite3",
      "-lz",
      "-framework",
      "CoreFoundation",
      "-framework",
      "CoreGraphics",
      "-framework",
      "CoreText",
      "-framework",
      "Foundation",
      "-framework",
      "ImageIO",
      "-framework",
      "Metal",
      "-framework",
      "MetalKit",
      "-framework",
      "QuartzCore",
    ]
  } else {
    flags += ["-Xlinker", "-rpath", "-Xlinker", libDir]
  }
  return flags
}

let isIOSSimulator = Context.environment["MISE_ENV"]?
  .hasPrefix("ios-simulator-") == true
let packageRoot = URL(fileURLWithPath: #filePath).deletingLastPathComponent()
let testDependencies: [Target.Dependency] = [
  "MaplibreNative",
  "CMaplibreNativeC",
]

var products: [Product] = [
  .library(name: "MaplibreNative", targets: ["MaplibreNative"]),
]

var targets: [Target] = [
  .systemLibrary(name: "CMaplibreNativeC"),
  .target(
    name: "MaplibreNative",
    dependencies: ["CMaplibreNativeC"],
    linkerSettings: [
      .unsafeFlags(nativeLinkerFlags()),
    ]
  ),
]

if isIOSSimulator {
  let testSourcesPath = "Tests/MaplibreNativeTests"
  let testSourceFiles: [String]
  do {
    testSourceFiles = try FileManager.default
      .contentsOfDirectory(atPath: packageRoot.appendingPathComponent(
        testSourcesPath
      ).path)
      .filter { $0.hasSuffix(".swift") }
      .sorted()
      .map { "MaplibreNativeTests/\($0)" }
  } catch {
    fatalError(
      "failed to list Swift test sources in \(testSourcesPath): \(error)"
    )
  }

  products.append(
    .executable(
      name: "MaplibreNativeIOSSimulatorTests",
      targets: ["MaplibreNativeIOSSimulatorTests"]
    )
  )
  targets.append(
    .executableTarget(
      name: "MaplibreNativeIOSSimulatorTests",
      dependencies: testDependencies,
      path: "Tests",
      sources: testSourceFiles +
        ["MaplibreNativeIOSSimulatorTests/Runner.swift"]
    )
  )
} else {
  targets.append(
    .testTarget(
      name: "MaplibreNativeTests",
      dependencies: testDependencies
    )
  )
}

let package = Package(
  name: "maplibre-native-swift",
  platforms: [.macOS("14.3"), .iOS("14.3")],
  products: products,
  targets: targets
)
