internal import CMaplibreNativeC

enum NativeRuntime {
  static func create(_ options: UnsafePointer<mln_runtime_options>) throws
    -> NativeRuntimeHandle
  {
    try NativeHandleFactory
      .create(nullDiagnostic: "mln_runtime_create returned a null runtime") { outHandle in
        try checkStatus(mln_runtime_create(options, outHandle))
      }
  }

  static func drainEvents(
    _ runtime: NativeRuntimeHandle,
    maxEvents: Int
  ) throws -> NativeRuntimeEventBatch {
    var batch = mln_runtime_event_batch_default()
    try checkStatus(mln_runtime_drain_events(runtime.raw, maxEvents, &batch))
    return try NativeRuntimeEventBatch(copying: batch)
  }

  static func setEventMask(
    _ runtime: NativeRuntimeHandle,
    mask: UInt64
  ) throws {
    try checkStatus(mln_runtime_set_event_mask(runtime.raw, mask))
  }

  static func eventMask(_ runtime: NativeRuntimeHandle) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { mask in
      try checkStatus(mln_runtime_get_event_mask(runtime.raw, mask))
    }.value
  }
}
