internal import CMaplibreNativeC

/// A handle the C API issued. The C API spells every handle as one integer
/// type, so each kind gets its own wrapper here to stay distinct at compile
/// time. `raw` names one object for the life of the process, carries no
/// ownership, and is safe to copy. Zero is the null handle.
protocol NativeHandle: Hashable, Sendable {
  var raw: UInt64 { get }
  init(raw: UInt64)
  func disposeAbandoned() -> Bool
}

extension NativeHandle {
  func disposeAbandoned() -> Bool {
    false
  }

  var isNull: Bool {
    raw == 0
  }
}
