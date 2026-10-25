internal import MaplibreNativeC

let expectedCAbiVersion: UInt32 = 0

/// The loaded library's C ABI version check.
///
/// A library with another version would misread every struct that the binding
/// passes. SwiftPM links the library that pkg-config names at build time, and
/// a different build can replace it at run time, so the binding checks the
/// version that the loaded library reports.
enum NativeAbi {
  private static let loadedMismatch = mismatch(actual: mln_c_version())

  /// Throws when the loaded library has another C ABI version.
  ///
  /// Every generated entry point without a receiver calls this first, and
  /// every other entry point needs a handle that one of those created.
  static func ensureCompatible() throws {
    if let loadedMismatch {
      throw loadedMismatch
    }
  }

  static func mismatch(actual: UInt32) -> AbiVersionMismatchError? {
    if actual == expectedCAbiVersion {
      return nil
    }
    return AbiVersionMismatchError(
      expected: expectedCAbiVersion,
      actual: actual
    )
  }
}

/// Reads a value's native defaults after the C ABI version check. A value's
/// `default` has no error channel, so a library with another version traps.
func nativeDefault<Raw>(_ read: () -> Raw) -> Raw {
  do {
    try NativeAbi.ensureCompatible()
  } catch {
    preconditionFailure(String(describing: error))
  }
  return read()
}
