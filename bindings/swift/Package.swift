// swift-tools-version: 6.0

import Foundation
import PackageDescription

func requiredEnvironment(_ name: String) -> String {
  guard let value = Context
    .environment[name]
  else {
    fatalError("\(name) is required")
  }
  return value
}

let miseEnv = requiredEnvironment("MISE_ENV")

func nativeLinkerFlags() -> [String] {
  let nativeInstallDir = requiredEnvironment("MLN_FFI_NATIVE_INSTALL_DIR")
  let libDir = "\(nativeInstallDir)/lib"
  var flags = ["-L", libDir, "-lmaplibre-native-c"]

  let isIOSDevice = miseEnv.hasPrefix("ios-") &&
    !miseEnv.hasPrefix("ios-simulator-")
  if isIOSDevice {
    flags += [
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

let isIOSSimulator = miseEnv.hasPrefix("ios-simulator-")
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
