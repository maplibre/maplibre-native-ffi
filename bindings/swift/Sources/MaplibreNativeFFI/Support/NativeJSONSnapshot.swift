internal import CMaplibreNativeC

enum NativeJSONSnapshot {
  static func copyValue(_ snapshot: NativeJSONSnapshotHandle) throws
    -> NativeJSONValue?
  {
    let output = try NativeMemory
      .withTemporary(UnsafePointer<mln_json_value>?.none) { value in
        try checkStatus(mln_json_snapshot_get(snapshot.raw, value))
      }
    guard let value = output.value else { return nil }
    return try NativeJSONValue(copying: value.pointee)
  }
}
