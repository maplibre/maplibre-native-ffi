import CMaplibreNativeC

public enum CAPI {
  public static func cVersion() -> UInt32 {
    mln_c_version()
  }

  public static func supportedRenderBackendMask() -> UInt32 {
    mln_supported_render_backend_mask()
  }

  public static func networkStatus() throws -> UInt32 {
    let output = try NativeMemory.withTemporary(UInt32(0)) { rawStatus in
      try checkStatus(mln_network_status_get(rawStatus))
    }
    return output.value
  }

  public static func setNetworkStatus(_ rawStatus: UInt32) throws {
    try checkStatus(mln_network_status_set(rawStatus))
  }

  public static func setLogCallback(
    _ callback: mln_log_callback?,
    userData: UnsafeMutableRawPointer?
  ) throws {
    try checkStatus(mln_log_set_callback(callback, userData))
  }

  public static func clearLogCallback() throws {
    try checkStatus(mln_log_clear_callback())
  }

  public static func setAsyncLogSeverityMask(_ mask: UInt32) throws {
    try checkStatus(mln_log_set_async_severity_mask(mask))
  }

  public static func runtimeOptionsDefault() -> mln_runtime_options {
    mln_runtime_options_default()
  }

  public static func createRuntime(_ options: UnsafePointer<mln_runtime_options>) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { runtime in
      try checkStatus(mln_runtime_create(options, runtime))
    }
    guard let runtime = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_runtime_create returned a null runtime")
    }
    return runtime
  }

  public static func destroyRuntime(_ runtime: OpaquePointer) throws {
    try checkStatus(mln_runtime_destroy(runtime))
  }

  public static func runtimeRunOnce(_ runtime: OpaquePointer) throws {
    try checkStatus(mln_runtime_run_once(runtime))
  }

  public static func runtimePollEvent(_ runtime: OpaquePointer) throws -> mln_runtime_event? {
    var event = mln_runtime_event()
    event.size = UInt32(MemoryLayout<mln_runtime_event>.size)
    let output = try NativeMemory.withTemporary(false) { hasEvent in
      try checkStatus(mln_runtime_poll_event(runtime, &event, hasEvent))
    }
    return output.value ? event : nil
  }

  public static func setResourceTransform(
    _ runtime: OpaquePointer,
    _ transform: UnsafePointer<mln_resource_transform>
  ) throws {
    try checkStatus(mln_runtime_set_resource_transform(runtime, transform))
  }

  public static func clearResourceTransform(_ runtime: OpaquePointer) throws {
    try checkStatus(mln_runtime_clear_resource_transform(runtime))
  }

  public static func setResourceProvider(
    _ runtime: OpaquePointer,
    _ provider: UnsafePointer<mln_resource_provider>
  ) throws {
    try checkStatus(mln_runtime_set_resource_provider(runtime, provider))
  }

  public static func completeResourceRequest(
    _ handle: OpaquePointer,
    _ response: UnsafePointer<mln_resource_response>
  ) throws {
    try checkStatus(mln_resource_request_complete(handle, response))
  }

  public static func resourceRequestCancelled(_ handle: OpaquePointer) throws -> Bool {
    let output = try NativeMemory.withTemporary(false) { cancelled in
      try checkStatus(mln_resource_request_cancelled(handle, cancelled))
    }
    return output.value
  }

  public static func releaseResourceRequest(_ handle: OpaquePointer?) {
    mln_resource_request_release(handle)
  }
}
