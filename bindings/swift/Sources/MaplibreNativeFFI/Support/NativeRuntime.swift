internal import CMaplibreNativeC

enum NativeRuntime {
  static func createStart(_ options: UnsafePointer<mln_runtime_options>) throws
    -> NativeOperationHandle
  {
    let raw = try NativeMemory.withTemporary(mln_operation(0)) { operation in
      try checkStatus(mln_runtime_create_start(options, operation))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func createTakeResult(_ operation: NativeOperationHandle) throws
    -> NativeRuntimeHandle
  {
    try NativeHandleFactory.create(
      nullDiagnostic: "mln_runtime_create_take_result returned a null runtime"
    ) { runtime in
      try checkStatus(mln_runtime_create_take_result(operation.raw, runtime))
    }
  }

  static func barrierStart(_ runtime: NativeRuntimeHandle) throws
    -> NativeOperationHandle
  {
    let raw = try NativeMemory.withTemporary(mln_operation(0)) { operation in
      try checkStatus(mln_runtime_barrier_start(runtime.raw, operation))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func release(_ runtime: NativeRuntimeHandle) throws {
    try checkStatus(mln_runtime_release(runtime.raw))
  }

  static func drainEvents(_ runtime: NativeRuntimeHandle) throws
    -> NativeRuntimeEventBatch
  {
    var batch: mln_event_batch = 0
    try checkStatus(mln_runtime_drain_events(runtime.raw, &batch))
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
