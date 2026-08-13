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
    var batch: mln_event_batch = 0
    try checkStatus(mln_runtime_drain_events(runtime.raw, maxEvents, &batch))
    defer { mln_event_batch_release(batch) }
    var view = mln_runtime_event_batch_view()
    view.size = UInt32(MemoryLayout<mln_runtime_event_batch_view>.size)
    try checkStatus(mln_event_batch_get(batch, &view))
    return try NativeRuntimeEventBatch(copying: view)
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
