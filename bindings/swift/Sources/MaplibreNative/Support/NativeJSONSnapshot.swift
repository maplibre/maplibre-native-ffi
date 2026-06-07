internal import CMaplibreNativeC

enum NativeJSONSnapshot {
  static func copyValue(_ snapshot: OpaquePointer) throws -> NativeJSONValue? {
    let output = try NativeMemory.withTemporary(Optional<UnsafePointer<mln_json_value>>.none) { value in
      try checkStatus(mln_json_snapshot_get(snapshot, value))
    }
    guard let value = output.value else { return nil }
    return try NativeJSONValue(copying: value.pointee)
  }
}
