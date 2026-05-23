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

  public static func mapOptionsDefault() -> mln_map_options {
    mln_map_options_default()
  }

  public static func createMap(
    runtime: OpaquePointer,
    options: UnsafePointer<mln_map_options>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { map in
      try checkStatus(mln_map_create(runtime, options, map))
    }
    guard let map = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_map_create returned a null map")
    }
    return map
  }

  public static func destroyMap(_ map: OpaquePointer) throws {
    try checkStatus(mln_map_destroy(map))
  }

  public static func mapSetStyleURL(_ map: OpaquePointer, _ url: String) throws {
    try NativeString.withCString(url) { url in
      try checkStatus(mln_map_set_style_url(map, url))
    }
  }

  public static func mapSetStyleJSON(_ map: OpaquePointer, _ json: String) throws {
    try NativeString.withCString(json) { json in
      try checkStatus(mln_map_set_style_json(map, json))
    }
  }

  public static func mapRequestRepaint(_ map: OpaquePointer) throws {
    try checkStatus(mln_map_request_repaint(map))
  }

  public static func mapRequestStillImage(_ map: OpaquePointer) throws {
    try checkStatus(mln_map_request_still_image(map))
  }

  public static func cameraOptionsDefault() -> mln_camera_options {
    mln_camera_options_default()
  }

  public static func animationOptionsDefault() -> mln_animation_options {
    mln_animation_options_default()
  }

  public static func mapGetCamera(_ map: OpaquePointer) throws -> mln_camera_options {
    var camera = cameraOptionsDefault()
    try checkStatus(mln_map_get_camera(map, &camera))
    return camera
  }

  public static func mapJumpTo(_ map: OpaquePointer, _ camera: UnsafePointer<mln_camera_options>) throws {
    try checkStatus(mln_map_jump_to(map, camera))
  }

  public static func mapEaseTo(
    _ map: OpaquePointer,
    _ camera: UnsafePointer<mln_camera_options>,
    _ animation: UnsafePointer<mln_animation_options>?
  ) throws {
    try checkStatus(mln_map_ease_to(map, camera, animation))
  }

  public static func mapMoveBy(_ map: OpaquePointer, deltaX: Double, deltaY: Double) throws {
    try checkStatus(mln_map_move_by(map, deltaX, deltaY))
  }

  public static func mapMoveByAnimated(
    _ map: OpaquePointer,
    deltaX: Double,
    deltaY: Double,
    animation: UnsafePointer<mln_animation_options>?
  ) throws {
    try checkStatus(mln_map_move_by_animated(map, deltaX, deltaY, animation))
  }

  public static func mapScaleBy(
    _ map: OpaquePointer,
    scale: Double,
    anchor: UnsafePointer<mln_screen_point>?
  ) throws {
    try checkStatus(mln_map_scale_by(map, scale, anchor))
  }

  public static func mapScaleByAnimated(
    _ map: OpaquePointer,
    scale: Double,
    anchor: UnsafePointer<mln_screen_point>?,
    animation: UnsafePointer<mln_animation_options>?
  ) throws {
    try checkStatus(mln_map_scale_by_animated(map, scale, anchor, animation))
  }

  public static func mapCancelTransitions(_ map: OpaquePointer) throws {
    try checkStatus(mln_map_cancel_transitions(map))
  }
}
