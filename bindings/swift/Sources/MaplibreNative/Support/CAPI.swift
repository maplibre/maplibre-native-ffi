internal import CMaplibreNativeC
import Foundation

enum CAPI {
  static func cVersion() -> UInt32 {
    mln_c_version()
  }

  static func supportedRenderBackendMask() -> UInt32 {
    mln_supported_render_backend_mask()
  }

  static func supportedOpenGLContextProviderMask() -> UInt32 {
    mln_opengl_supported_context_provider_mask()
  }

  static func networkStatus() throws -> UInt32 {
    let output = try NativeMemory.withTemporary(UInt32(0)) { rawStatus in
      try checkStatus(mln_network_status_get(rawStatus))
    }
    return output.value
  }

  static func setNetworkStatus(_ rawStatus: UInt32) throws {
    try checkStatus(mln_network_status_set(rawStatus))
  }

  static func setLogCallback(
    _ callback: mln_log_callback?,
    userData: UnsafeMutableRawPointer?
  ) throws {
    try checkStatus(mln_log_set_callback(callback, userData))
  }

  static func clearLogCallback() throws {
    try checkStatus(mln_log_clear_callback())
  }

  static func setAsyncLogSeverityMask(_ mask: UInt32) throws {
    try checkStatus(mln_log_set_async_severity_mask(mask))
  }

  static func runtimeOptionsDefault() -> mln_runtime_options {
    mln_runtime_options_default()
  }

  static func createRuntime(_ options: UnsafePointer<mln_runtime_options>) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { runtime in
      try checkStatus(mln_runtime_create(options, runtime))
    }
    guard let runtime = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_runtime_create returned a null runtime")
    }
    return runtime
  }

  static func destroyRuntime(_ runtime: OpaquePointer) throws {
    try checkStatus(mln_runtime_destroy(runtime))
  }

  static func runtimeRunOnce(_ runtime: OpaquePointer) throws {
    try checkStatus(mln_runtime_run_once(runtime))
  }

  static func runtimePollEvent(_ runtime: OpaquePointer) throws -> mln_runtime_event? {
    var event = mln_runtime_event()
    event.size = UInt32(MemoryLayout<mln_runtime_event>.size)
    let output = try NativeMemory.withTemporary(false) { hasEvent in
      try checkStatus(mln_runtime_poll_event(runtime, &event, hasEvent))
    }
    return output.value ? event : nil
  }

  static func setResourceTransform(
    _ runtime: OpaquePointer,
    _ transform: UnsafePointer<mln_resource_transform>
  ) throws {
    try checkStatus(mln_runtime_set_resource_transform(runtime, transform))
  }

  static func clearResourceTransform(_ runtime: OpaquePointer) throws {
    try checkStatus(mln_runtime_clear_resource_transform(runtime))
  }

  static func setResourceProvider(
    _ runtime: OpaquePointer,
    _ provider: UnsafePointer<mln_resource_provider>
  ) throws {
    try checkStatus(mln_runtime_set_resource_provider(runtime, provider))
  }

  static func completeResourceRequest(
    _ handle: OpaquePointer,
    _ response: UnsafePointer<mln_resource_response>
  ) throws {
    try checkStatus(mln_resource_request_complete(handle, response))
  }

  static func resourceRequestCancelled(_ handle: OpaquePointer) throws -> Bool {
    let output = try NativeMemory.withTemporary(false) { cancelled in
      try checkStatus(mln_resource_request_cancelled(handle, cancelled))
    }
    return output.value
  }

  static func releaseResourceRequest(_ handle: OpaquePointer?) {
    mln_resource_request_release(handle)
  }

  static func mapOptionsDefault() -> mln_map_options {
    mln_map_options_default()
  }

  static func createMap(
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

  static func destroyMap(_ map: OpaquePointer) throws {
    try checkStatus(mln_map_destroy(map))
  }

  static func mapSetStyleURL(_ map: OpaquePointer, _ url: String) throws {
    try NativeString.withCString(url) { url in
      try checkStatus(mln_map_set_style_url(map, url))
    }
  }

  static func mapSetStyleJSON(_ map: OpaquePointer, _ json: String) throws {
    try NativeString.withCString(json) { json in
      try checkStatus(mln_map_set_style_json(map, json))
    }
  }

  static func mapRequestRepaint(_ map: OpaquePointer) throws {
    try checkStatus(mln_map_request_repaint(map))
  }

  static func mapRequestStillImage(_ map: OpaquePointer) throws {
    try checkStatus(mln_map_request_still_image(map))
  }

  static func cameraOptionsDefault() -> mln_camera_options {
    mln_camera_options_default()
  }

  static func animationOptionsDefault() -> mln_animation_options {
    mln_animation_options_default()
  }

  static func cameraFitOptionsDefault() -> mln_camera_fit_options {
    mln_camera_fit_options_default()
  }

  static func boundOptionsDefault() -> mln_bound_options {
    mln_bound_options_default()
  }

  static func freeCameraOptionsDefault() -> mln_free_camera_options {
    mln_free_camera_options_default()
  }

  static func projectionModeDefault() -> mln_projection_mode {
    mln_projection_mode_default()
  }

  static func mapViewportOptionsDefault() -> mln_map_viewport_options {
    mln_map_viewport_options_default()
  }

  static func mapTileOptionsDefault() -> mln_map_tile_options {
    mln_map_tile_options_default()
  }

  static func mapSetDebugOptions(_ map: OpaquePointer, options: UInt32) throws {
    try checkStatus(mln_map_set_debug_options(map, options))
  }

  static func mapGetDebugOptions(_ map: OpaquePointer) throws -> UInt32 {
    try NativeMemory.withTemporary(UInt32(0)) { options in
      try checkStatus(mln_map_get_debug_options(map, options))
    }.value
  }

  static func mapSetRenderingStatsViewEnabled(_ map: OpaquePointer, enabled: Bool) throws {
    try checkStatus(mln_map_set_rendering_stats_view_enabled(map, enabled))
  }

  static func mapGetRenderingStatsViewEnabled(_ map: OpaquePointer) throws -> Bool {
    try NativeMemory.withTemporary(false) { enabled in
      try checkStatus(mln_map_get_rendering_stats_view_enabled(map, enabled))
    }.value
  }

  static func mapIsFullyLoaded(_ map: OpaquePointer) throws -> Bool {
    try NativeMemory.withTemporary(false) { loaded in
      try checkStatus(mln_map_is_fully_loaded(map, loaded))
    }.value
  }

  static func mapDumpDebugLogs(_ map: OpaquePointer) throws {
    try checkStatus(mln_map_dump_debug_logs(map))
  }

  static func mapGetViewportOptions(_ map: OpaquePointer) throws -> mln_map_viewport_options {
    var options = mapViewportOptionsDefault()
    try checkStatus(mln_map_get_viewport_options(map, &options))
    return options
  }

  static func mapSetViewportOptions(_ map: OpaquePointer, options: UnsafePointer<mln_map_viewport_options>) throws {
    try checkStatus(mln_map_set_viewport_options(map, options))
  }

  static func mapGetTileOptions(_ map: OpaquePointer) throws -> mln_map_tile_options {
    var options = mapTileOptionsDefault()
    try checkStatus(mln_map_get_tile_options(map, &options))
    return options
  }

  static func mapSetTileOptions(_ map: OpaquePointer, options: UnsafePointer<mln_map_tile_options>) throws {
    try checkStatus(mln_map_set_tile_options(map, options))
  }

  static func mapGetCamera(_ map: OpaquePointer) throws -> mln_camera_options {
    var camera = cameraOptionsDefault()
    try checkStatus(mln_map_get_camera(map, &camera))
    return camera
  }

  static func mapJumpTo(_ map: OpaquePointer, _ camera: UnsafePointer<mln_camera_options>) throws {
    try checkStatus(mln_map_jump_to(map, camera))
  }

  static func mapEaseTo(
    _ map: OpaquePointer,
    _ camera: UnsafePointer<mln_camera_options>,
    _ animation: UnsafePointer<mln_animation_options>?
  ) throws {
    try checkStatus(mln_map_ease_to(map, camera, animation))
  }

  static func mapFlyTo(
    _ map: OpaquePointer,
    _ camera: UnsafePointer<mln_camera_options>,
    _ animation: UnsafePointer<mln_animation_options>?
  ) throws {
    try checkStatus(mln_map_fly_to(map, camera, animation))
  }

  static func mapMoveBy(_ map: OpaquePointer, deltaX: Double, deltaY: Double) throws {
    try checkStatus(mln_map_move_by(map, deltaX, deltaY))
  }

  static func mapMoveByAnimated(
    _ map: OpaquePointer,
    deltaX: Double,
    deltaY: Double,
    animation: UnsafePointer<mln_animation_options>?
  ) throws {
    try checkStatus(mln_map_move_by_animated(map, deltaX, deltaY, animation))
  }

  static func mapScaleBy(
    _ map: OpaquePointer,
    scale: Double,
    anchor: UnsafePointer<mln_screen_point>?
  ) throws {
    try checkStatus(mln_map_scale_by(map, scale, anchor))
  }

  static func mapScaleByAnimated(
    _ map: OpaquePointer,
    scale: Double,
    anchor: UnsafePointer<mln_screen_point>?,
    animation: UnsafePointer<mln_animation_options>?
  ) throws {
    try checkStatus(mln_map_scale_by_animated(map, scale, anchor, animation))
  }

  static func mapRotateBy(_ map: OpaquePointer, first: NativeScreenPoint, second: NativeScreenPoint) throws {
    try checkStatus(mln_map_rotate_by(map, first.native, second.native))
  }

  static func mapRotateByAnimated(
    _ map: OpaquePointer,
    first: NativeScreenPoint,
    second: NativeScreenPoint,
    animation: UnsafePointer<mln_animation_options>?
  ) throws {
    try checkStatus(mln_map_rotate_by_animated(map, first.native, second.native, animation))
  }

  static func mapPitchBy(_ map: OpaquePointer, pitch: Double) throws {
    try checkStatus(mln_map_pitch_by(map, pitch))
  }

  static func mapPitchByAnimated(_ map: OpaquePointer, pitch: Double, animation: UnsafePointer<mln_animation_options>?) throws {
    try checkStatus(mln_map_pitch_by_animated(map, pitch, animation))
  }

  static func mapCancelTransitions(_ map: OpaquePointer) throws {
    try checkStatus(mln_map_cancel_transitions(map))
  }

  static func mapCameraForLatLngBounds(_ map: OpaquePointer, bounds: NativeLatLngBounds, fitOptions: UnsafePointer<mln_camera_fit_options>?) throws -> mln_camera_options {
    var camera = cameraOptionsDefault()
    try checkStatus(mln_map_camera_for_lat_lng_bounds(map, bounds.native, fitOptions, &camera))
    return camera
  }

  static func mapCameraForLatLngs(_ map: OpaquePointer, coordinates: UnsafePointer<mln_lat_lng>?, count: Int, fitOptions: UnsafePointer<mln_camera_fit_options>?) throws -> mln_camera_options {
    var camera = cameraOptionsDefault()
    try checkStatus(mln_map_camera_for_lat_lngs(map, coordinates, count, fitOptions, &camera))
    return camera
  }

  static func mapCameraForGeometry(_ map: OpaquePointer, geometry: UnsafePointer<mln_geometry>, fitOptions: UnsafePointer<mln_camera_fit_options>?) throws -> mln_camera_options {
    var camera = cameraOptionsDefault()
    try checkStatus(mln_map_camera_for_geometry(map, geometry, fitOptions, &camera))
    return camera
  }

  static func mapLatLngBoundsForCamera(_ map: OpaquePointer, camera: UnsafePointer<mln_camera_options>, unwrapped: Bool) throws -> NativeLatLngBounds {
    let output = try NativeMemory.withTemporary(mln_lat_lng_bounds()) { bounds in
      if unwrapped {
        try checkStatus(mln_map_lat_lng_bounds_for_camera_unwrapped(map, camera, bounds))
      } else {
        try checkStatus(mln_map_lat_lng_bounds_for_camera(map, camera, bounds))
      }
    }
    return NativeLatLngBounds(output.value)
  }

  static func mapGetBounds(_ map: OpaquePointer) throws -> mln_bound_options {
    var bounds = boundOptionsDefault()
    try checkStatus(mln_map_get_bounds(map, &bounds))
    return bounds
  }

  static func mapSetBounds(_ map: OpaquePointer, bounds: UnsafePointer<mln_bound_options>) throws {
    try checkStatus(mln_map_set_bounds(map, bounds))
  }

  static func mapGetFreeCameraOptions(_ map: OpaquePointer) throws -> mln_free_camera_options {
    var options = freeCameraOptionsDefault()
    try checkStatus(mln_map_get_free_camera_options(map, &options))
    return options
  }

  static func mapSetFreeCameraOptions(_ map: OpaquePointer, options: UnsafePointer<mln_free_camera_options>) throws {
    try checkStatus(mln_map_set_free_camera_options(map, options))
  }

  static func mapGetProjectionMode(_ map: OpaquePointer) throws -> mln_projection_mode {
    var mode = projectionModeDefault()
    try checkStatus(mln_map_get_projection_mode(map, &mode))
    return mode
  }

  static func mapSetProjectionMode(_ map: OpaquePointer, mode: UnsafePointer<mln_projection_mode>) throws {
    try checkStatus(mln_map_set_projection_mode(map, mode))
  }

  static func mapPixelForLatLng(_ map: OpaquePointer, coordinate: NativeLatLng) throws -> NativeScreenPoint {
    let output = try NativeMemory.withTemporary(mln_screen_point()) { point in
      try checkStatus(mln_map_pixel_for_lat_lng(map, coordinate.native, point))
    }
    return NativeScreenPoint(output.value)
  }

  static func mapLatLngForPixel(_ map: OpaquePointer, point: NativeScreenPoint) throws -> NativeLatLng {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_map_lat_lng_for_pixel(map, point.native, coordinate))
    }
    return NativeLatLng(output.value)
  }

  static func mapPixelsForLatLngs(_ map: OpaquePointer, coordinates: UnsafePointer<mln_lat_lng>?, count: Int, outPoints: UnsafeMutablePointer<mln_screen_point>?) throws {
    try checkStatus(mln_map_pixels_for_lat_lngs(map, coordinates, count, outPoints))
  }

  static func mapLatLngsForPixels(_ map: OpaquePointer, points: UnsafePointer<mln_screen_point>?, count: Int, outCoordinates: UnsafeMutablePointer<mln_lat_lng>?) throws {
    try checkStatus(mln_map_lat_lngs_for_pixels(map, points, count, outCoordinates))
  }

  static func mapPixelsForLatLngs(_ map: OpaquePointer, coordinates: [NativeLatLng]) throws -> [NativeScreenPoint] {
    let rawCoordinates = coordinates.map(\.native)
    var rawPoints = [mln_screen_point](repeating: mln_screen_point(), count: rawCoordinates.count)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try rawPoints.withUnsafeMutableBufferPointer { points in
        try mapPixelsForLatLngs(map, coordinates: coordinates.baseAddress, count: coordinates.count, outPoints: points.baseAddress)
      }
    }
    return rawPoints.map(NativeScreenPoint.init)
  }

  static func mapLatLngsForPixels(_ map: OpaquePointer, points: [NativeScreenPoint]) throws -> [NativeLatLng] {
    let rawPoints = points.map(\.native)
    var rawCoordinates = [mln_lat_lng](repeating: mln_lat_lng(), count: rawPoints.count)
    try rawPoints.withUnsafeBufferPointer { points in
      try rawCoordinates.withUnsafeMutableBufferPointer { coordinates in
        try mapLatLngsForPixels(map, points: points.baseAddress, count: points.count, outCoordinates: coordinates.baseAddress)
      }
    }
    return rawCoordinates.map(NativeLatLng.init)
  }

  static func createMapProjection(_ map: OpaquePointer) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { projection in
      try checkStatus(mln_map_projection_create(map, projection))
    }
    guard let projection = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_map_projection_create returned a null projection")
    }
    return projection
  }

  static func destroyMapProjection(_ projection: OpaquePointer) throws {
    try checkStatus(mln_map_projection_destroy(projection))
  }

  static func mapProjectionGetCamera(_ projection: OpaquePointer) throws -> mln_camera_options {
    var camera = cameraOptionsDefault()
    try checkStatus(mln_map_projection_get_camera(projection, &camera))
    return camera
  }

  static func mapProjectionSetCamera(
    _ projection: OpaquePointer,
    _ camera: UnsafePointer<mln_camera_options>
  ) throws {
    try checkStatus(mln_map_projection_set_camera(projection, camera))
  }

  static func mapProjectionSetVisibleCoordinates(
    _ projection: OpaquePointer,
    coordinates: UnsafePointer<mln_lat_lng>,
    count: Int,
    padding: mln_edge_insets
  ) throws {
    try checkStatus(mln_map_projection_set_visible_coordinates(projection, coordinates, count, padding))
  }

  static func mapProjectionSetVisibleGeometry(
    _ projection: OpaquePointer,
    geometry: UnsafePointer<mln_geometry>,
    padding: mln_edge_insets
  ) throws {
    try checkStatus(mln_map_projection_set_visible_geometry(projection, geometry, padding))
  }

  static func mapProjectionPixelForLatLng(
    _ projection: OpaquePointer,
    coordinate: mln_lat_lng
  ) throws -> mln_screen_point {
    let output = try NativeMemory.withTemporary(mln_screen_point()) { point in
      try checkStatus(mln_map_projection_pixel_for_lat_lng(projection, coordinate, point))
    }
    return output.value
  }

  static func mapProjectionLatLngForPixel(
    _ projection: OpaquePointer,
    point: mln_screen_point
  ) throws -> mln_lat_lng {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_map_projection_lat_lng_for_pixel(projection, point, coordinate))
    }
    return output.value
  }

  static func projectedMetersForLatLng(_ coordinate: NativeLatLng) throws -> NativeProjectedMeters {
    let output = try NativeMemory.withTemporary(mln_projected_meters()) { meters in
      try checkStatus(mln_projected_meters_for_lat_lng(coordinate.native, meters))
    }
    return NativeProjectedMeters(output.value)
  }

  static func latLngForProjectedMeters(_ meters: NativeProjectedMeters) throws -> NativeLatLng {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_lat_lng_for_projected_meters(meters.native, coordinate))
    }
    return NativeLatLng(output.value)
  }

  static func renderSessionResize(
    _ session: OpaquePointer,
    width: UInt32,
    height: UInt32,
    scaleFactor: Double
  ) throws {
    try checkStatus(mln_render_session_resize(session, width, height, scaleFactor))
  }

  static func renderSessionRenderUpdate(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_render_update(session))
  }

  static func renderSessionDetach(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_detach(session))
  }

  static func renderSessionDestroy(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_destroy(session))
  }

  static func renderSessionReduceMemoryUse(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_reduce_memory_use(session))
  }

  static func renderSessionClearData(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_clear_data(session))
  }

  static func renderSessionDumpDebugLogs(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_dump_debug_logs(session))
  }

  static func metalSurfaceDescriptorDefault() -> mln_metal_surface_descriptor {
    mln_metal_surface_descriptor_default()
  }

  static func vulkanSurfaceDescriptorDefault() -> mln_vulkan_surface_descriptor {
    mln_vulkan_surface_descriptor_default()
  }

  static func openGLSurfaceDescriptorDefault() -> mln_opengl_surface_descriptor {
    mln_opengl_surface_descriptor_default()
  }

  static func metalOwnedTextureDescriptorDefault() -> mln_metal_owned_texture_descriptor {
    mln_metal_owned_texture_descriptor_default()
  }

  static func metalBorrowedTextureDescriptorDefault() -> mln_metal_borrowed_texture_descriptor {
    mln_metal_borrowed_texture_descriptor_default()
  }

  static func vulkanOwnedTextureDescriptorDefault() -> mln_vulkan_owned_texture_descriptor {
    mln_vulkan_owned_texture_descriptor_default()
  }

  static func vulkanBorrowedTextureDescriptorDefault() -> mln_vulkan_borrowed_texture_descriptor {
    mln_vulkan_borrowed_texture_descriptor_default()
  }

  static func openGLOwnedTextureDescriptorDefault() -> mln_opengl_owned_texture_descriptor {
    mln_opengl_owned_texture_descriptor_default()
  }

  static func openGLBorrowedTextureDescriptorDefault() -> mln_opengl_borrowed_texture_descriptor {
    mln_opengl_borrowed_texture_descriptor_default()
  }

  static func textureImageInfoDefault() -> mln_texture_image_info {
    mln_texture_image_info_default()
  }

  static func metalSurfaceAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_metal_surface_descriptor>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { session in
      try checkStatus(mln_metal_surface_attach(map, descriptor, session))
    }
    guard let session = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_metal_surface_attach returned a null session")
    }
    return session
  }

  static func vulkanSurfaceAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_vulkan_surface_descriptor>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { session in
      try checkStatus(mln_vulkan_surface_attach(map, descriptor, session))
    }
    guard let session = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_vulkan_surface_attach returned a null session")
    }
    return session
  }

  static func openGLSurfaceAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_opengl_surface_descriptor>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { session in
      try checkStatus(mln_opengl_surface_attach(map, descriptor, session))
    }
    guard let session = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_opengl_surface_attach returned a null session")
    }
    return session
  }

  static func textureReadPremultipliedRGBA8(
    session: OpaquePointer,
    data: UnsafeMutablePointer<UInt8>?,
    capacity: Int
  ) throws -> mln_texture_image_info {
    var info = textureImageInfoDefault()
    try checkStatus(mln_texture_read_premultiplied_rgba8(session, data, capacity, &info))
    return info
  }

  static func metalOwnedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_metal_owned_texture_descriptor>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { session in
      try checkStatus(mln_metal_owned_texture_attach(map, descriptor, session))
    }
    guard let session = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_metal_owned_texture_attach returned a null session")
    }
    return session
  }

  static func metalBorrowedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_metal_borrowed_texture_descriptor>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { session in
      try checkStatus(mln_metal_borrowed_texture_attach(map, descriptor, session))
    }
    guard let session = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_metal_borrowed_texture_attach returned a null session")
    }
    return session
  }

  static func vulkanOwnedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_vulkan_owned_texture_descriptor>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { session in
      try checkStatus(mln_vulkan_owned_texture_attach(map, descriptor, session))
    }
    guard let session = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_vulkan_owned_texture_attach returned a null session")
    }
    return session
  }

  static func vulkanBorrowedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_vulkan_borrowed_texture_descriptor>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { session in
      try checkStatus(mln_vulkan_borrowed_texture_attach(map, descriptor, session))
    }
    guard let session = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_vulkan_borrowed_texture_attach returned a null session")
    }
    return session
  }

  static func openGLOwnedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_opengl_owned_texture_descriptor>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { session in
      try checkStatus(mln_opengl_owned_texture_attach(map, descriptor, session))
    }
    guard let session = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_opengl_owned_texture_attach returned a null session")
    }
    return session
  }

  static func openGLBorrowedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_opengl_borrowed_texture_descriptor>
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { session in
      try checkStatus(mln_opengl_borrowed_texture_attach(map, descriptor, session))
    }
    guard let session = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_opengl_borrowed_texture_attach returned a null session")
    }
    return session
  }

  static func metalOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_metal_owned_texture_frame {
    var frame = mln_metal_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_metal_owned_texture_frame>.size)
    try checkStatus(mln_metal_owned_texture_acquire_frame(session, &frame))
    return frame
  }

  static func metalOwnedTextureReleaseFrame(
    _ session: OpaquePointer,
    frame: UnsafePointer<mln_metal_owned_texture_frame>
  ) throws {
    try checkStatus(mln_metal_owned_texture_release_frame(session, frame))
  }

  static func vulkanOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_vulkan_owned_texture_frame {
    var frame = mln_vulkan_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_vulkan_owned_texture_frame>.size)
    try checkStatus(mln_vulkan_owned_texture_acquire_frame(session, &frame))
    return frame
  }

  static func vulkanOwnedTextureReleaseFrame(
    _ session: OpaquePointer,
    frame: UnsafePointer<mln_vulkan_owned_texture_frame>
  ) throws {
    try checkStatus(mln_vulkan_owned_texture_release_frame(session, frame))
  }

  static func openGLOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_opengl_owned_texture_frame {
    var frame = mln_opengl_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_opengl_owned_texture_frame>.size)
    try checkStatus(mln_opengl_owned_texture_acquire_frame(session, &frame))
    return frame
  }

  static func openGLOwnedTextureReleaseFrame(
    _ session: OpaquePointer,
    frame: UnsafePointer<mln_opengl_owned_texture_frame>
  ) throws {
    try checkStatus(mln_opengl_owned_texture_release_frame(session, frame))
  }

  static func renderSessionQueryRenderedFeatures(
    session: OpaquePointer,
    geometry: UnsafePointer<mln_rendered_query_geometry>,
    options: UnsafePointer<mln_rendered_feature_query_options>?
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { result in
      try checkStatus(mln_render_session_query_rendered_features(session, geometry, options, result))
    }
    guard let result = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "rendered feature query returned a null result")
    }
    return result
  }

  static func renderSessionQuerySourceFeatures(
    session: OpaquePointer,
    sourceId: mln_string_view,
    options: UnsafePointer<mln_source_feature_query_options>?
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { result in
      try checkStatus(mln_render_session_query_source_features(session, sourceId, options, result))
    }
    guard let result = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "source feature query returned a null result")
    }
    return result
  }

  static func featureQueryResultCount(_ result: OpaquePointer) throws -> Int {
    let output = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_feature_query_result_count(result, count))
    }
    return output.value
  }

  static func featureQueryResultGet(_ result: OpaquePointer, index: Int) throws -> NativeQueriedFeature {
    var feature = mln_queried_feature()
    feature.size = UInt32(MemoryLayout<mln_queried_feature>.size)
    try checkStatus(mln_feature_query_result_get(result, index, &feature))
    return try NativeQueriedFeature(copying: feature)
  }

  static func featureQueryResultDestroy(_ result: OpaquePointer) {
    mln_feature_query_result_destroy(result)
  }

  static func jsonSnapshotCopyValue(_ snapshot: OpaquePointer) throws -> NativeJSONValue? {
    let output = try NativeMemory.withTemporary(Optional<UnsafePointer<mln_json_value>>.none) { value in
      try checkStatus(mln_json_snapshot_get(snapshot, value))
    }
    guard let value = output.value else { return nil }
    return try NativeJSONValue(copying: value.pointee)
  }

  static func jsonSnapshotDestroy(_ snapshot: OpaquePointer) {
    mln_json_snapshot_destroy(snapshot)
  }

  static func styleIdListCopy(_ list: OpaquePointer) throws -> [String] {
    let count = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_style_id_list_count(list, count))
    }.value
    return try (0..<count).map { index in
      let output = try NativeMemory.withTemporary(mln_string_view()) { value in
        try checkStatus(mln_style_id_list_get(list, index, value))
      }
      return try NativeString.copyUTF8(data: output.value.data, size: output.value.size)
    }
  }

  static func styleIdListDestroy(_ list: OpaquePointer) {
    mln_style_id_list_destroy(list)
  }

  static func mapAddStyleSourceJSON(_ map: OpaquePointer, sourceId: mln_string_view, sourceJSON: UnsafePointer<mln_json_value>) throws {
    try checkStatus(mln_map_add_style_source_json(map, sourceId, sourceJSON))
  }

  static func mapRemoveStyleSource(_ map: OpaquePointer, sourceId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_source(map, sourceId, removed))
    }.value
  }

  static func mapStyleSourceExists(_ map: OpaquePointer, sourceId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_source_exists(map, sourceId, exists))
    }.value
  }

  static func mapGetStyleSourceType(_ map: OpaquePointer, sourceId: mln_string_view) throws -> UInt32? {
    var type = UInt32(0)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_type(map, sourceId, &type, found))
    }.value
    return found ? type : nil
  }

  static func mapGetStyleSourceInfo(_ map: OpaquePointer, sourceId: mln_string_view) throws -> NativeStyleSourceInfo? {
    var info = mln_style_source_info()
    info.size = UInt32(MemoryLayout<mln_style_source_info>.size)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_info(map, sourceId, &info, found))
    }.value
    return found ? NativeStyleSourceInfo(info) : nil
  }

  static func mapCopyStyleSourceAttribution(_ map: OpaquePointer, sourceId: mln_string_view, capacity: Int) throws -> (String?, Int) {
    var bytes = [UInt8](repeating: 0, count: capacity)
    var found = false
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { outSize in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_copy_style_source_attribution(map, sourceId, buffer.baseAddress, capacity, outSize, outFound))
          found = outFound.pointee
        }
      }.value
    }
    guard found else { return (nil, size) }
    return (String(decoding: bytes.prefix(size), as: UTF8.self), size)
  }

  static func mapListStyleSourceIds(_ map: OpaquePointer) throws -> [String] {
    let list = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { list in
      try checkStatus(mln_map_list_style_source_ids(map, list))
    }.value
    guard let list else { throw NativeStatusFailure(rawStatus: 0, diagnostic: "source ID list was null") }
    defer { styleIdListDestroy(list) }
    return try styleIdListCopy(list)
  }

  static func mapAddGeoJSONSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view) throws {
    try checkStatus(mln_map_add_geojson_source_url(map, sourceId, url))
  }

  static func mapAddGeoJSONSourceData(_ map: OpaquePointer, sourceId: mln_string_view, data: UnsafePointer<mln_geojson>) throws {
    try checkStatus(mln_map_add_geojson_source_data(map, sourceId, data))
  }

  static func mapSetGeoJSONSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view) throws {
    try checkStatus(mln_map_set_geojson_source_url(map, sourceId, url))
  }

  static func mapSetGeoJSONSourceData(_ map: OpaquePointer, sourceId: mln_string_view, data: UnsafePointer<mln_geojson>) throws {
    try checkStatus(mln_map_set_geojson_source_data(map, sourceId, data))
  }

  static func mapAddVectorSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_vector_source_url(map, sourceId, url, options))
  }

  static func mapAddVectorSourceTiles(_ map: OpaquePointer, sourceId: mln_string_view, tiles: UnsafePointer<mln_string_view>?, count: Int, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_vector_source_tiles(map, sourceId, tiles, count, options))
  }

  static func mapAddRasterSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_raster_source_url(map, sourceId, url, options))
  }

  static func mapAddRasterSourceTiles(_ map: OpaquePointer, sourceId: mln_string_view, tiles: UnsafePointer<mln_string_view>?, count: Int, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_raster_source_tiles(map, sourceId, tiles, count, options))
  }

  static func mapAddRasterDEMSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_raster_dem_source_url(map, sourceId, url, options))
  }

  static func mapAddRasterDEMSourceTiles(_ map: OpaquePointer, sourceId: mln_string_view, tiles: UnsafePointer<mln_string_view>?, count: Int, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_raster_dem_source_tiles(map, sourceId, tiles, count, options))
  }

  static func mapAddCustomGeometrySource(_ map: OpaquePointer, sourceId: mln_string_view, options: UnsafePointer<mln_custom_geometry_source_options>) throws {
    try checkStatus(mln_map_add_custom_geometry_source(map, sourceId, options))
  }

  static func mapSetCustomGeometrySourceTileData(_ map: OpaquePointer, sourceId: mln_string_view, tileId: NativeCanonicalTileID, data: UnsafePointer<mln_geojson>) throws {
    try checkStatus(mln_map_set_custom_geometry_source_tile_data(map, sourceId, tileId.native, data))
  }

  static func mapInvalidateCustomGeometrySourceTile(_ map: OpaquePointer, sourceId: mln_string_view, tileId: NativeCanonicalTileID) throws {
    try checkStatus(mln_map_invalidate_custom_geometry_source_tile(map, sourceId, tileId.native))
  }

  static func mapInvalidateCustomGeometrySourceRegion(_ map: OpaquePointer, sourceId: mln_string_view, bounds: NativeLatLngBounds) throws {
    try checkStatus(mln_map_invalidate_custom_geometry_source_region(map, sourceId, bounds.native))
  }

  static func mapSetStyleImage(_ map: OpaquePointer, imageId: mln_string_view, image: UnsafePointer<mln_premultiplied_rgba8_image>, options: UnsafePointer<mln_style_image_options>) throws {
    try checkStatus(mln_map_set_style_image(map, imageId, image, options))
  }

  static func mapRemoveStyleImage(_ map: OpaquePointer, imageId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_image(map, imageId, removed))
    }.value
  }

  static func mapStyleImageExists(_ map: OpaquePointer, imageId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_image_exists(map, imageId, exists))
    }.value
  }

  static func mapGetStyleImageInfo(_ map: OpaquePointer, imageId: mln_string_view) throws -> NativeStyleImageInfo? {
    var info = mln_style_image_info_default()
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_image_info(map, imageId, &info, found))
    }.value
    return found ? NativeStyleImageInfo(info) : nil
  }

  static func mapCopyStyleImagePremultipliedRGBA8(_ map: OpaquePointer, imageId: mln_string_view, capacity: Int) throws -> ([UInt8]?, Int) {
    var bytes = [UInt8](repeating: 0, count: capacity)
    var found = false
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { outSize in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_copy_style_image_premultiplied_rgba8(map, imageId, buffer.baseAddress, capacity, outSize, outFound))
          found = outFound.pointee
        }
      }.value
    }
    return found ? (Array(bytes.prefix(size)), size) : (nil, size)
  }

  static func mapAddImageSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: UnsafePointer<mln_lat_lng>?, count: Int, url: mln_string_view) throws {
    try checkStatus(mln_map_add_image_source_url(map, sourceId, coordinates, count, url))
  }

  static func mapAddImageSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: [NativeLatLng], url: mln_string_view) throws {
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try mapAddImageSourceURL(map, sourceId: sourceId, coordinates: coordinates.baseAddress, count: coordinates.count, url: url)
    }
  }

  static func mapAddImageSourceImage(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: UnsafePointer<mln_lat_lng>?, count: Int, image: UnsafePointer<mln_premultiplied_rgba8_image>) throws {
    try checkStatus(mln_map_add_image_source_image(map, sourceId, coordinates, count, image))
  }

  static func mapAddImageSourceImage(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: [NativeLatLng], image: UnsafePointer<mln_premultiplied_rgba8_image>) throws {
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try mapAddImageSourceImage(map, sourceId: sourceId, coordinates: coordinates.baseAddress, count: coordinates.count, image: image)
    }
  }

  static func mapSetImageSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view) throws {
    try checkStatus(mln_map_set_image_source_url(map, sourceId, url))
  }

  static func mapSetImageSourceImage(_ map: OpaquePointer, sourceId: mln_string_view, image: UnsafePointer<mln_premultiplied_rgba8_image>) throws {
    try checkStatus(mln_map_set_image_source_image(map, sourceId, image))
  }

  static func mapSetImageSourceCoordinates(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: UnsafePointer<mln_lat_lng>?, count: Int) throws {
    try checkStatus(mln_map_set_image_source_coordinates(map, sourceId, coordinates, count))
  }

  static func mapSetImageSourceCoordinates(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: [NativeLatLng]) throws {
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try mapSetImageSourceCoordinates(map, sourceId: sourceId, coordinates: coordinates.baseAddress, count: coordinates.count)
    }
  }

  static func mapGetImageSourceCoordinates(_ map: OpaquePointer, sourceId: mln_string_view) throws -> [NativeLatLng]? {
    var coordinates = [mln_lat_lng](repeating: mln_lat_lng(), count: 4)
    var found = false
    let count = try coordinates.withUnsafeMutableBufferPointer { coordinates in
      try NativeMemory.withTemporary(0) { count in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_get_image_source_coordinates(map, sourceId, coordinates.baseAddress, coordinates.count, count, outFound))
          found = outFound.pointee
        }
      }.value
    }
    return found ? coordinates.prefix(count).map(NativeLatLng.init) : nil
  }

  static func mapAddHillshadeLayer(_ map: OpaquePointer, layerId: mln_string_view, sourceId: mln_string_view, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_add_hillshade_layer(map, layerId, sourceId, beforeLayerId))
  }

  static func mapAddColorReliefLayer(_ map: OpaquePointer, layerId: mln_string_view, sourceId: mln_string_view, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_add_color_relief_layer(map, layerId, sourceId, beforeLayerId))
  }

  static func mapAddLocationIndicatorLayer(_ map: OpaquePointer, layerId: mln_string_view, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_add_location_indicator_layer(map, layerId, beforeLayerId))
  }

  static func mapSetLocationIndicatorLocation(_ map: OpaquePointer, layerId: mln_string_view, coordinate: NativeLatLng, altitude: Double) throws {
    try checkStatus(mln_map_set_location_indicator_location(map, layerId, coordinate.native, altitude))
  }

  static func mapSetLocationIndicatorBearing(_ map: OpaquePointer, layerId: mln_string_view, bearing: Double) throws {
    try checkStatus(mln_map_set_location_indicator_bearing(map, layerId, bearing))
  }

  static func mapSetLocationIndicatorAccuracyRadius(_ map: OpaquePointer, layerId: mln_string_view, radius: Double) throws {
    try checkStatus(mln_map_set_location_indicator_accuracy_radius(map, layerId, radius))
  }

  static func mapSetLocationIndicatorImageName(_ map: OpaquePointer, layerId: mln_string_view, imageKind: UInt32, imageId: mln_string_view) throws {
    try checkStatus(mln_map_set_location_indicator_image_name(map, layerId, imageKind, imageId))
  }

  static func mapAddStyleLayerJSON(_ map: OpaquePointer, layerJSON: UnsafePointer<mln_json_value>, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_add_style_layer_json(map, layerJSON, beforeLayerId))
  }

  static func mapRemoveStyleLayer(_ map: OpaquePointer, layerId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_layer(map, layerId, removed))
    }.value
  }

  static func mapStyleLayerExists(_ map: OpaquePointer, layerId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_layer_exists(map, layerId, exists))
    }.value
  }

  static func mapGetStyleLayerType(_ map: OpaquePointer, layerId: mln_string_view) throws -> String? {
    var layerType = mln_string_view()
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_layer_type(map, layerId, &layerType, found))
    }.value
    return found ? try NativeString.copyUTF8(data: layerType.data, size: layerType.size) : nil
  }

  static func mapListStyleLayerIds(_ map: OpaquePointer) throws -> [String] {
    let list = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { list in
      try checkStatus(mln_map_list_style_layer_ids(map, list))
    }.value
    guard let list else { throw NativeStatusFailure(rawStatus: 0, diagnostic: "layer ID list was null") }
    defer { styleIdListDestroy(list) }
    return try styleIdListCopy(list)
  }

  static func mapMoveStyleLayer(_ map: OpaquePointer, layerId: mln_string_view, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_move_style_layer(map, layerId, beforeLayerId))
  }

  static func mapGetStyleLayerJSON(_ map: OpaquePointer, layerId: mln_string_view) throws -> NativeJSONValue? {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try NativeMemory.withTemporary(false) { found in
        try checkStatus(mln_map_get_style_layer_json(map, layerId, snapshot, found))
        if !found.pointee { snapshot.pointee = nil }
      }
    }.value
    guard let snapshot = output else { return nil }
    defer { jsonSnapshotDestroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  static func mapSetStyleLightJSON(_ map: OpaquePointer, lightJSON: UnsafePointer<mln_json_value>) throws {
    try checkStatus(mln_map_set_style_light_json(map, lightJSON))
  }

  static func mapSetStyleLightProperty(_ map: OpaquePointer, propertyName: mln_string_view, value: UnsafePointer<mln_json_value>) throws {
    try checkStatus(mln_map_set_style_light_property(map, propertyName, value))
  }

  static func mapGetStyleLightProperty(_ map: OpaquePointer, propertyName: mln_string_view) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_map_get_style_light_property(map, propertyName, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { jsonSnapshotDestroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  static func mapSetLayerProperty(_ map: OpaquePointer, layerId: mln_string_view, propertyName: mln_string_view, value: UnsafePointer<mln_json_value>) throws {
    try checkStatus(mln_map_set_layer_property(map, layerId, propertyName, value))
  }

  static func mapGetLayerProperty(_ map: OpaquePointer, layerId: mln_string_view, propertyName: mln_string_view) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_map_get_layer_property(map, layerId, propertyName, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { jsonSnapshotDestroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  static func mapSetLayerFilter(_ map: OpaquePointer, layerId: mln_string_view, filter: UnsafePointer<mln_json_value>?) throws {
    try checkStatus(mln_map_set_layer_filter(map, layerId, filter))
  }

  static func mapGetLayerFilter(_ map: OpaquePointer, layerId: mln_string_view) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_map_get_layer_filter(map, layerId, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { jsonSnapshotDestroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  static func renderSessionQueryFeatureExtensions(
    session: OpaquePointer,
    sourceId: mln_string_view,
    feature: UnsafePointer<mln_feature>,
    extensionName: mln_string_view,
    extensionField: mln_string_view,
    arguments: UnsafePointer<mln_json_value>?
  ) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { result in
      try checkStatus(mln_render_session_query_feature_extensions(session, sourceId, feature, extensionName, extensionField, arguments, result))
    }
    guard let result = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "feature extension query returned a null result")
    }
    return result
  }

  static func featureExtensionResultCopy(_ result: OpaquePointer) throws -> NativeFeatureExtensionResult {
    var info = mln_feature_extension_result_info()
    info.size = UInt32(MemoryLayout<mln_feature_extension_result_info>.size)
    try checkStatus(mln_feature_extension_result_get(result, &info))
    return try NativeFeatureExtensionResult(copying: info)
  }

  static func featureExtensionResultDestroy(_ result: OpaquePointer) {
    mln_feature_extension_result_destroy(result)
  }

  static func runtimeRunAmbientCacheOperationStart(_ runtime: OpaquePointer, operation: UInt32) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_run_ambient_cache_operation_start(runtime, operation, operationId))
    }.value
  }

  static func runtimeOfflineOperationDiscard(_ runtime: OpaquePointer, operationId: UInt64) throws {
    try checkStatus(mln_runtime_offline_operation_discard(runtime, operationId))
  }

  static func runtimeOfflineRegionCreateStart(_ runtime: OpaquePointer, definition: UnsafePointer<mln_offline_region_definition>, metadata: Data) throws -> UInt64 {
    try metadata.withUnsafeBytes { bytes in
      try NativeMemory.withTemporary(UInt64(0)) { operationId in
        try checkStatus(mln_runtime_offline_region_create_start(runtime, definition, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count, operationId))
      }.value
    }
  }

  static func runtimeOfflineRegionGetStart(_ runtime: OpaquePointer, regionId: Int64) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_get_start(runtime, regionId, operationId))
    }.value
  }

  static func runtimeOfflineRegionsListStart(_ runtime: OpaquePointer) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_regions_list_start(runtime, operationId))
    }.value
  }

  static func runtimeOfflineRegionsMergeDatabaseStart(_ runtime: OpaquePointer, sideDatabasePath: String) throws -> UInt64 {
    try NativeString.withCString(sideDatabasePath) { path in
      try NativeMemory.withTemporary(UInt64(0)) { operationId in
        try checkStatus(mln_runtime_offline_regions_merge_database_start(runtime, path, operationId))
      }.value
    }
  }

  static func runtimeOfflineRegionUpdateMetadataStart(_ runtime: OpaquePointer, regionId: Int64, metadata: Data) throws -> UInt64 {
    try metadata.withUnsafeBytes { bytes in
      try NativeMemory.withTemporary(UInt64(0)) { operationId in
        try checkStatus(mln_runtime_offline_region_update_metadata_start(runtime, regionId, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count, operationId))
      }.value
    }
  }

  static func runtimeOfflineRegionGetStatusStart(_ runtime: OpaquePointer, regionId: Int64) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_get_status_start(runtime, regionId, operationId))
    }.value
  }

  static func runtimeOfflineRegionSetObservedStart(_ runtime: OpaquePointer, regionId: Int64, observed: Bool) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_set_observed_start(runtime, regionId, observed, operationId))
    }.value
  }

  static func runtimeOfflineRegionSetDownloadStateStart(_ runtime: OpaquePointer, regionId: Int64, state: UInt32) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_set_download_state_start(runtime, regionId, state, operationId))
    }.value
  }

  static func runtimeOfflineRegionInvalidateStart(_ runtime: OpaquePointer, regionId: Int64) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_invalidate_start(runtime, regionId, operationId))
    }.value
  }

  static func runtimeOfflineRegionDeleteStart(_ runtime: OpaquePointer, regionId: Int64) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_delete_start(runtime, regionId, operationId))
    }.value
  }

  static func runtimeOfflineRegionCreateTakeResult(_ runtime: OpaquePointer, operationId: UInt64) throws -> NativeOfflineRegionInfo {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_runtime_offline_region_create_take_result(runtime, operationId, snapshot))
    }.value
    guard let snapshot else { throw NativeStatusFailure(rawStatus: 0, diagnostic: "offline region create result was null") }
    defer { mln_offline_region_snapshot_destroy(snapshot) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func runtimeOfflineRegionGetTakeResult(_ runtime: OpaquePointer, operationId: UInt64) throws -> NativeOfflineRegionInfo? {
    var found = false
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try NativeMemory.withTemporary(false) { outFound in
        try checkStatus(mln_runtime_offline_region_get_take_result(runtime, operationId, snapshot, outFound))
        found = outFound.pointee
      }
    }.value
    guard found, let snapshot else { return nil }
    defer { mln_offline_region_snapshot_destroy(snapshot) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func runtimeOfflineRegionsListTakeResult(_ runtime: OpaquePointer, operationId: UInt64) throws -> [NativeOfflineRegionInfo] {
    let list = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { list in
      try checkStatus(mln_runtime_offline_regions_list_take_result(runtime, operationId, list))
    }.value
    guard let list else { throw NativeStatusFailure(rawStatus: 0, diagnostic: "offline region list result was null") }
    defer { mln_offline_region_list_destroy(list) }
    return try offlineRegionListCopy(list)
  }

  static func runtimeOfflineRegionsMergeDatabaseTakeResult(_ runtime: OpaquePointer, operationId: UInt64) throws -> [NativeOfflineRegionInfo] {
    let list = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { list in
      try checkStatus(mln_runtime_offline_regions_merge_database_take_result(runtime, operationId, list))
    }.value
    guard let list else { throw NativeStatusFailure(rawStatus: 0, diagnostic: "offline merge result list was null") }
    defer { mln_offline_region_list_destroy(list) }
    return try offlineRegionListCopy(list)
  }

  static func runtimeOfflineRegionUpdateMetadataTakeResult(_ runtime: OpaquePointer, operationId: UInt64) throws -> NativeOfflineRegionInfo {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_runtime_offline_region_update_metadata_take_result(runtime, operationId, snapshot))
    }.value
    guard let snapshot else { throw NativeStatusFailure(rawStatus: 0, diagnostic: "offline update metadata result was null") }
    defer { mln_offline_region_snapshot_destroy(snapshot) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func runtimeOfflineRegionGetStatusTakeResult(_ runtime: OpaquePointer, operationId: UInt64) throws -> NativeOfflineRegionStatus {
    var status = mln_offline_region_status()
    status.size = UInt32(MemoryLayout<mln_offline_region_status>.size)
    try checkStatus(mln_runtime_offline_region_get_status_take_result(runtime, operationId, &status))
    return NativeOfflineRegionStatus(status)
  }

  static func offlineRegionSnapshotCopy(_ snapshot: OpaquePointer) throws -> NativeOfflineRegionInfo {
    var info = mln_offline_region_info()
    info.size = UInt32(MemoryLayout<mln_offline_region_info>.size)
    try checkStatus(mln_offline_region_snapshot_get(snapshot, &info))
    return try NativeOfflineRegionInfo(copying: info)
  }

  static func offlineRegionListCopy(_ list: OpaquePointer) throws -> [NativeOfflineRegionInfo] {
    let count = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_offline_region_list_count(list, count))
    }.value
    return try (0..<count).map { index in
      var info = mln_offline_region_info()
      info.size = UInt32(MemoryLayout<mln_offline_region_info>.size)
      try checkStatus(mln_offline_region_list_get(list, index, &info))
      return try NativeOfflineRegionInfo(copying: info)
    }
  }

  static func renderSessionSetFeatureState(_ session: OpaquePointer, selector: UnsafePointer<mln_feature_state_selector>, state: UnsafePointer<mln_json_value>) throws {
    try checkStatus(mln_render_session_set_feature_state(session, selector, state))
  }

  static func renderSessionGetFeatureState(_ session: OpaquePointer, selector: UnsafePointer<mln_feature_state_selector>) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_render_session_get_feature_state(session, selector, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { jsonSnapshotDestroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  static func renderSessionRemoveFeatureState(_ session: OpaquePointer, selector: UnsafePointer<mln_feature_state_selector>) throws {
    try checkStatus(mln_render_session_remove_feature_state(session, selector))
  }
}
