internal import CMaplibreNativeC

enum NativeRuntime {
  static func create(_ options: UnsafePointer<mln_runtime_options>) throws
    -> OpaquePointer
  {
    try NativeHandleFactory
      .create(nullDiagnostic: "mln_runtime_create returned a null runtime") { runtime in
        try checkStatus(mln_runtime_create(options, runtime))
      }
  }

  static func pollEvent(_ runtime: OpaquePointer) throws -> mln_runtime_event? {
    var event = mln_runtime_event()
    event.size = UInt32(MemoryLayout<mln_runtime_event>.size)
    let output = try NativeMemory.withTemporary(false) { hasEvent in
      try checkStatus(mln_runtime_poll_event(runtime, &event, hasEvent))
    }
    return output.value ? event : nil
  }
}
