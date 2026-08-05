@testable import MaplibreNativeFFI

/// Handle values for tests that exercise binding-owned bookkeeping without a
/// live native object. Each value carries the kind byte the C API assigns to
/// the type it stands in for. The C API rejects one as never created.
enum SyntheticHandles {
  static func runtime(_ ordinal: UInt64 = 1) -> NativeRuntimeHandle {
    NativeRuntimeHandle(raw: kind(0x01) | ordinal)
  }

  static func map(_ ordinal: UInt64 = 1) -> NativeMapHandle {
    NativeMapHandle(raw: kind(0x02) | ordinal)
  }

  static func mapProjection(_ ordinal: UInt64 = 1)
    -> NativeMapProjectionHandle
  {
    NativeMapProjectionHandle(raw: kind(0x03) | ordinal)
  }

  static func renderSession(_ ordinal: UInt64 = 1)
    -> NativeRenderSessionHandle
  {
    NativeRenderSessionHandle(raw: kind(0x04) | ordinal)
  }

  static func wakeSource(_ ordinal: UInt64 = 1) -> NativeWakeSourceHandle {
    NativeWakeSourceHandle(raw: kind(0x0B) | ordinal)
  }

  static func resourceRequest(_ ordinal: UInt64 = 1)
    -> NativeResourceRequestHandle
  {
    NativeResourceRequestHandle(raw: kind(0x0C) | ordinal)
  }

  private static func kind(_ value: UInt64) -> UInt64 {
    value << 56
  }
}
