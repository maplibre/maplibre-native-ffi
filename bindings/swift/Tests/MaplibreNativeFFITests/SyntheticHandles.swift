@testable import MaplibreNativeFFI

/// Handle values for tests that exercise binding-owned bookkeeping without a
/// live native object. Each value carries the kind byte the C API assigns to
/// the type it stands in for. The C API rejects one as never created.
enum SyntheticHandles {
  static func resourceRequest(_ ordinal: UInt64 = 1)
    -> NativeResourceRequestHandle
  {
    NativeResourceRequestHandle(raw: kind(0x0C) | ordinal)
  }

  private static func kind(_ value: UInt64) -> UInt64 {
    value << 56
  }
}
