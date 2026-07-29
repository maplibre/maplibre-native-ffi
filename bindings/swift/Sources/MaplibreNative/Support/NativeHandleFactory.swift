internal import CMaplibreNativeC

enum NativeHandleFactory {
  /// Runs `body` with storage for a handle out-parameter and returns the issued
  /// handle, rejecting the null handle the C API writes on failure.
  static func create<Handle: NativeHandle>(
    nullDiagnostic: String,
    _ body: (UnsafeMutablePointer<UInt64>) throws -> Void
  ) throws -> Handle {
    let output = try NativeMemory.withTemporary(UInt64(0), body)
    let handle = Handle(raw: output.value)
    guard !handle.isNull else {
      throw NativeStatusFailure.swiftNativeError(nullDiagnostic)
    }
    return handle
  }
}
