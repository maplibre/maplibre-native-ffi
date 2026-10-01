internal import CMaplibreNativeC

/// The C ABI contract version that this binding's generated code was written
/// against.
let expectedCAbiVersion: UInt32 = 0

/// The loaded library's C ABI version check.
///
/// A library with another version would misread every struct that the binding
/// passes. SwiftPM links the library that pkg-config names at build time, and
/// a different build can replace it at run time, so the binding checks the
/// version that the loaded library reports.
enum NativeAbi {
  /// The loaded library's mismatch, computed once, on first use.
  private static let loadedMismatch = mismatch(actual: mln_c_version())

  /// Throws when the loaded library has another C ABI version.
  ///
  /// Every generated entry point without a receiver calls this first, and
  /// every other entry point needs a handle that one of those created. A
  /// value's generated `default` reads its native defaults without this
  /// check, so on a mismatched library a program that builds a value first
  /// reads that struct before its first call throws.
  static func ensureCompatible() throws {
    if let loadedMismatch { throw loadedMismatch }
  }

  /// Returns the error for a library that reports `actual`, or nil when it
  /// matches this binding.
  static func mismatch(actual: UInt32) -> MaplibreError? {
    if actual == expectedCAbiVersion { return nil }
    return MaplibreError(
      kind: .abiVersionMismatch,
      rawStatus: nil,
      diagnostic: "MapLibre Native C ABI version \(actual) is incompatible "
        + "with this binding; expected \(expectedCAbiVersion)."
    )
  }
}
