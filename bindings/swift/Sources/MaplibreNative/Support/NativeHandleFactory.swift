internal import CMaplibreNativeC

enum NativeHandleFactory {
  static func create(
    nullDiagnostic: String,
    _ body: (UnsafeMutablePointer<OpaquePointer?>) throws -> Void
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(OpaquePointer?.none, body)
    guard let handle = output.value else {
      throw NativeStatusFailure.swiftNativeError(nullDiagnostic)
    }
    return handle
  }
}
