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

  public static func createMapProjection(_ map: OpaquePointer) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { projection in
      try checkStatus(mln_map_projection_create(map, projection))
    }
    guard let projection = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_map_projection_create returned a null projection")
    }
    return projection
  }

  public static func destroyMapProjection(_ projection: OpaquePointer) throws {
    try checkStatus(mln_map_projection_destroy(projection))
  }

  public static func mapProjectionGetCamera(_ projection: OpaquePointer) throws -> mln_camera_options {
    var camera = cameraOptionsDefault()
    try checkStatus(mln_map_projection_get_camera(projection, &camera))
    return camera
  }

  public static func mapProjectionSetCamera(
    _ projection: OpaquePointer,
    _ camera: UnsafePointer<mln_camera_options>
  ) throws {
    try checkStatus(mln_map_projection_set_camera(projection, camera))
  }

  public static func mapProjectionSetVisibleCoordinates(
    _ projection: OpaquePointer,
    coordinates: UnsafePointer<mln_lat_lng>,
    count: Int,
    padding: mln_edge_insets
  ) throws {
    try checkStatus(mln_map_projection_set_visible_coordinates(projection, coordinates, count, padding))
  }

  public static func mapProjectionPixelForLatLng(
    _ projection: OpaquePointer,
    coordinate: mln_lat_lng
  ) throws -> mln_screen_point {
    let output = try NativeMemory.withTemporary(mln_screen_point()) { point in
      try checkStatus(mln_map_projection_pixel_for_lat_lng(projection, coordinate, point))
    }
    return output.value
  }

  public static func mapProjectionLatLngForPixel(
    _ projection: OpaquePointer,
    point: mln_screen_point
  ) throws -> mln_lat_lng {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_map_projection_lat_lng_for_pixel(projection, point, coordinate))
    }
    return output.value
  }

  public static func projectedMetersForLatLng(_ coordinate: NativeLatLng) throws -> NativeProjectedMeters {
    let output = try NativeMemory.withTemporary(mln_projected_meters()) { meters in
      try checkStatus(mln_projected_meters_for_lat_lng(coordinate.native, meters))
    }
    return NativeProjectedMeters(output.value)
  }

  public static func latLngForProjectedMeters(_ meters: NativeProjectedMeters) throws -> NativeLatLng {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_lat_lng_for_projected_meters(meters.native, coordinate))
    }
    return NativeLatLng(output.value)
  }

  public static func renderSessionResize(
    _ session: OpaquePointer,
    width: UInt32,
    height: UInt32,
    scaleFactor: Double
  ) throws {
    try checkStatus(mln_render_session_resize(session, width, height, scaleFactor))
  }

  public static func renderSessionRenderUpdate(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_render_update(session))
  }

  public static func renderSessionDetach(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_detach(session))
  }

  public static func renderSessionDestroy(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_destroy(session))
  }

  public static func renderSessionReduceMemoryUse(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_reduce_memory_use(session))
  }

  public static func renderSessionClearData(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_clear_data(session))
  }

  public static func renderSessionDumpDebugLogs(_ session: OpaquePointer) throws {
    try checkStatus(mln_render_session_dump_debug_logs(session))
  }

  public static func metalSurfaceDescriptorDefault() -> mln_metal_surface_descriptor {
    mln_metal_surface_descriptor_default()
  }

  public static func vulkanSurfaceDescriptorDefault() -> mln_vulkan_surface_descriptor {
    mln_vulkan_surface_descriptor_default()
  }

  public static func metalOwnedTextureDescriptorDefault() -> mln_metal_owned_texture_descriptor {
    mln_metal_owned_texture_descriptor_default()
  }

  public static func metalBorrowedTextureDescriptorDefault() -> mln_metal_borrowed_texture_descriptor {
    mln_metal_borrowed_texture_descriptor_default()
  }

  public static func vulkanOwnedTextureDescriptorDefault() -> mln_vulkan_owned_texture_descriptor {
    mln_vulkan_owned_texture_descriptor_default()
  }

  public static func vulkanBorrowedTextureDescriptorDefault() -> mln_vulkan_borrowed_texture_descriptor {
    mln_vulkan_borrowed_texture_descriptor_default()
  }

  public static func textureImageInfoDefault() -> mln_texture_image_info {
    mln_texture_image_info_default()
  }

  public static func metalSurfaceAttach(
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

  public static func vulkanSurfaceAttach(
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

  public static func textureReadPremultipliedRGBA8(
    session: OpaquePointer,
    data: UnsafeMutablePointer<UInt8>?,
    capacity: Int
  ) throws -> mln_texture_image_info {
    var info = textureImageInfoDefault()
    try checkStatus(mln_texture_read_premultiplied_rgba8(session, data, capacity, &info))
    return info
  }

  public static func metalOwnedTextureAttach(
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

  public static func metalBorrowedTextureAttach(
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

  public static func vulkanOwnedTextureAttach(
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

  public static func vulkanBorrowedTextureAttach(
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

  public static func metalOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_metal_owned_texture_frame {
    var frame = mln_metal_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_metal_owned_texture_frame>.size)
    try checkStatus(mln_metal_owned_texture_acquire_frame(session, &frame))
    return frame
  }

  public static func metalOwnedTextureReleaseFrame(
    _ session: OpaquePointer,
    frame: UnsafePointer<mln_metal_owned_texture_frame>
  ) throws {
    try checkStatus(mln_metal_owned_texture_release_frame(session, frame))
  }

  public static func vulkanOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_vulkan_owned_texture_frame {
    var frame = mln_vulkan_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_vulkan_owned_texture_frame>.size)
    try checkStatus(mln_vulkan_owned_texture_acquire_frame(session, &frame))
    return frame
  }

  public static func vulkanOwnedTextureReleaseFrame(
    _ session: OpaquePointer,
    frame: UnsafePointer<mln_vulkan_owned_texture_frame>
  ) throws {
    try checkStatus(mln_vulkan_owned_texture_release_frame(session, frame))
  }

  public static func renderSessionQueryRenderedFeatures(
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

  public static func renderSessionQuerySourceFeatures(
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

  public static func featureQueryResultCount(_ result: OpaquePointer) throws -> Int {
    let output = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_feature_query_result_count(result, count))
    }
    return output.value
  }

  public static func featureQueryResultGet(_ result: OpaquePointer, index: Int) throws -> NativeQueriedFeature {
    var feature = mln_queried_feature()
    feature.size = UInt32(MemoryLayout<mln_queried_feature>.size)
    try checkStatus(mln_feature_query_result_get(result, index, &feature))
    return try NativeQueriedFeature(copying: feature)
  }

  public static func featureQueryResultDestroy(_ result: OpaquePointer) {
    mln_feature_query_result_destroy(result)
  }

  public static func jsonSnapshotCopyValue(_ snapshot: OpaquePointer) throws -> NativeJSONValue? {
    let output = try NativeMemory.withTemporary(Optional<UnsafePointer<mln_json_value>>.none) { value in
      try checkStatus(mln_json_snapshot_get(snapshot, value))
    }
    guard let value = output.value else { return nil }
    return try NativeJSONValue(copying: value.pointee)
  }

  public static func jsonSnapshotDestroy(_ snapshot: OpaquePointer) {
    mln_json_snapshot_destroy(snapshot)
  }

  public static func styleIdListCopy(_ list: OpaquePointer) throws -> [String] {
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

  public static func styleIdListDestroy(_ list: OpaquePointer) {
    mln_style_id_list_destroy(list)
  }

  public static func mapAddStyleSourceJSON(_ map: OpaquePointer, sourceId: mln_string_view, sourceJSON: UnsafePointer<mln_json_value>) throws {
    try checkStatus(mln_map_add_style_source_json(map, sourceId, sourceJSON))
  }

  public static func mapRemoveStyleSource(_ map: OpaquePointer, sourceId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_source(map, sourceId, removed))
    }.value
  }

  public static func mapStyleSourceExists(_ map: OpaquePointer, sourceId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_source_exists(map, sourceId, exists))
    }.value
  }

  public static func mapGetStyleSourceType(_ map: OpaquePointer, sourceId: mln_string_view) throws -> UInt32? {
    var type = UInt32(0)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_type(map, sourceId, &type, found))
    }.value
    return found ? type : nil
  }

  public static func mapGetStyleSourceInfo(_ map: OpaquePointer, sourceId: mln_string_view) throws -> NativeStyleSourceInfo? {
    var info = mln_style_source_info()
    info.size = UInt32(MemoryLayout<mln_style_source_info>.size)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_info(map, sourceId, &info, found))
    }.value
    return found ? NativeStyleSourceInfo(info) : nil
  }

  public static func mapCopyStyleSourceAttribution(_ map: OpaquePointer, sourceId: mln_string_view, capacity: Int) throws -> (String?, Int) {
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

  public static func mapListStyleSourceIds(_ map: OpaquePointer) throws -> [String] {
    let list = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { list in
      try checkStatus(mln_map_list_style_source_ids(map, list))
    }.value
    guard let list else { throw NativeStatusFailure(rawStatus: 0, diagnostic: "source ID list was null") }
    defer { styleIdListDestroy(list) }
    return try styleIdListCopy(list)
  }

  public static func mapAddGeoJSONSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view) throws {
    try checkStatus(mln_map_add_geojson_source_url(map, sourceId, url))
  }

  public static func mapAddGeoJSONSourceData(_ map: OpaquePointer, sourceId: mln_string_view, data: UnsafePointer<mln_geojson>) throws {
    try checkStatus(mln_map_add_geojson_source_data(map, sourceId, data))
  }

  public static func mapSetGeoJSONSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view) throws {
    try checkStatus(mln_map_set_geojson_source_url(map, sourceId, url))
  }

  public static func mapSetGeoJSONSourceData(_ map: OpaquePointer, sourceId: mln_string_view, data: UnsafePointer<mln_geojson>) throws {
    try checkStatus(mln_map_set_geojson_source_data(map, sourceId, data))
  }

  public static func mapAddVectorSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_vector_source_url(map, sourceId, url, options))
  }

  public static func mapAddVectorSourceTiles(_ map: OpaquePointer, sourceId: mln_string_view, tiles: UnsafePointer<mln_string_view>?, count: Int, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_vector_source_tiles(map, sourceId, tiles, count, options))
  }

  public static func mapAddRasterSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_raster_source_url(map, sourceId, url, options))
  }

  public static func mapAddRasterSourceTiles(_ map: OpaquePointer, sourceId: mln_string_view, tiles: UnsafePointer<mln_string_view>?, count: Int, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_raster_source_tiles(map, sourceId, tiles, count, options))
  }

  public static func mapAddRasterDEMSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_raster_dem_source_url(map, sourceId, url, options))
  }

  public static func mapAddRasterDEMSourceTiles(_ map: OpaquePointer, sourceId: mln_string_view, tiles: UnsafePointer<mln_string_view>?, count: Int, options: UnsafePointer<mln_style_tile_source_options>?) throws {
    try checkStatus(mln_map_add_raster_dem_source_tiles(map, sourceId, tiles, count, options))
  }

  public static func mapSetStyleImage(_ map: OpaquePointer, imageId: mln_string_view, image: UnsafePointer<mln_premultiplied_rgba8_image>, options: UnsafePointer<mln_style_image_options>) throws {
    try checkStatus(mln_map_set_style_image(map, imageId, image, options))
  }

  public static func mapRemoveStyleImage(_ map: OpaquePointer, imageId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_image(map, imageId, removed))
    }.value
  }

  public static func mapStyleImageExists(_ map: OpaquePointer, imageId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_image_exists(map, imageId, exists))
    }.value
  }

  public static func mapGetStyleImageInfo(_ map: OpaquePointer, imageId: mln_string_view) throws -> NativeStyleImageInfo? {
    var info = mln_style_image_info_default()
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_image_info(map, imageId, &info, found))
    }.value
    return found ? NativeStyleImageInfo(info) : nil
  }

  public static func mapCopyStyleImagePremultipliedRGBA8(_ map: OpaquePointer, imageId: mln_string_view, capacity: Int) throws -> ([UInt8]?, Int) {
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

  public static func mapAddImageSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: UnsafePointer<mln_lat_lng>?, count: Int, url: mln_string_view) throws {
    try checkStatus(mln_map_add_image_source_url(map, sourceId, coordinates, count, url))
  }

  public static func mapAddImageSourceImage(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: UnsafePointer<mln_lat_lng>?, count: Int, image: UnsafePointer<mln_premultiplied_rgba8_image>) throws {
    try checkStatus(mln_map_add_image_source_image(map, sourceId, coordinates, count, image))
  }

  public static func mapSetImageSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, url: mln_string_view) throws {
    try checkStatus(mln_map_set_image_source_url(map, sourceId, url))
  }

  public static func mapSetImageSourceImage(_ map: OpaquePointer, sourceId: mln_string_view, image: UnsafePointer<mln_premultiplied_rgba8_image>) throws {
    try checkStatus(mln_map_set_image_source_image(map, sourceId, image))
  }

  public static func mapSetImageSourceCoordinates(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: UnsafePointer<mln_lat_lng>?, count: Int) throws {
    try checkStatus(mln_map_set_image_source_coordinates(map, sourceId, coordinates, count))
  }

  public static func mapGetImageSourceCoordinates(_ map: OpaquePointer, sourceId: mln_string_view) throws -> [NativeLatLng]? {
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

  public static func mapAddHillshadeLayer(_ map: OpaquePointer, layerId: mln_string_view, sourceId: mln_string_view, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_add_hillshade_layer(map, layerId, sourceId, beforeLayerId))
  }

  public static func mapAddColorReliefLayer(_ map: OpaquePointer, layerId: mln_string_view, sourceId: mln_string_view, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_add_color_relief_layer(map, layerId, sourceId, beforeLayerId))
  }

  public static func mapAddLocationIndicatorLayer(_ map: OpaquePointer, layerId: mln_string_view, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_add_location_indicator_layer(map, layerId, beforeLayerId))
  }

  public static func mapSetLocationIndicatorLocation(_ map: OpaquePointer, layerId: mln_string_view, coordinate: NativeLatLng, altitude: Double) throws {
    try checkStatus(mln_map_set_location_indicator_location(map, layerId, coordinate.native, altitude))
  }

  public static func mapSetLocationIndicatorBearing(_ map: OpaquePointer, layerId: mln_string_view, bearing: Double) throws {
    try checkStatus(mln_map_set_location_indicator_bearing(map, layerId, bearing))
  }

  public static func mapSetLocationIndicatorAccuracyRadius(_ map: OpaquePointer, layerId: mln_string_view, radius: Double) throws {
    try checkStatus(mln_map_set_location_indicator_accuracy_radius(map, layerId, radius))
  }

  public static func mapSetLocationIndicatorImageName(_ map: OpaquePointer, layerId: mln_string_view, imageKind: UInt32, imageId: mln_string_view) throws {
    try checkStatus(mln_map_set_location_indicator_image_name(map, layerId, imageKind, imageId))
  }

  public static func mapAddStyleLayerJSON(_ map: OpaquePointer, layerJSON: UnsafePointer<mln_json_value>, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_add_style_layer_json(map, layerJSON, beforeLayerId))
  }

  public static func mapRemoveStyleLayer(_ map: OpaquePointer, layerId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_layer(map, layerId, removed))
    }.value
  }

  public static func mapStyleLayerExists(_ map: OpaquePointer, layerId: mln_string_view) throws -> Bool {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_layer_exists(map, layerId, exists))
    }.value
  }

  public static func mapGetStyleLayerType(_ map: OpaquePointer, layerId: mln_string_view) throws -> String? {
    var layerType = mln_string_view()
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_layer_type(map, layerId, &layerType, found))
    }.value
    return found ? try NativeString.copyUTF8(data: layerType.data, size: layerType.size) : nil
  }

  public static func mapListStyleLayerIds(_ map: OpaquePointer) throws -> [String] {
    let list = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { list in
      try checkStatus(mln_map_list_style_layer_ids(map, list))
    }.value
    guard let list else { throw NativeStatusFailure(rawStatus: 0, diagnostic: "layer ID list was null") }
    defer { styleIdListDestroy(list) }
    return try styleIdListCopy(list)
  }

  public static func mapMoveStyleLayer(_ map: OpaquePointer, layerId: mln_string_view, beforeLayerId: mln_string_view) throws {
    try checkStatus(mln_map_move_style_layer(map, layerId, beforeLayerId))
  }

  public static func mapGetStyleLayerJSON(_ map: OpaquePointer, layerId: mln_string_view) throws -> NativeJSONValue? {
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

  public static func mapSetStyleLightJSON(_ map: OpaquePointer, lightJSON: UnsafePointer<mln_json_value>) throws {
    try checkStatus(mln_map_set_style_light_json(map, lightJSON))
  }

  public static func mapSetStyleLightProperty(_ map: OpaquePointer, propertyName: mln_string_view, value: UnsafePointer<mln_json_value>) throws {
    try checkStatus(mln_map_set_style_light_property(map, propertyName, value))
  }

  public static func mapGetStyleLightProperty(_ map: OpaquePointer, propertyName: mln_string_view) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_map_get_style_light_property(map, propertyName, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { jsonSnapshotDestroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  public static func mapSetLayerProperty(_ map: OpaquePointer, layerId: mln_string_view, propertyName: mln_string_view, value: UnsafePointer<mln_json_value>) throws {
    try checkStatus(mln_map_set_layer_property(map, layerId, propertyName, value))
  }

  public static func mapGetLayerProperty(_ map: OpaquePointer, layerId: mln_string_view, propertyName: mln_string_view) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_map_get_layer_property(map, layerId, propertyName, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { jsonSnapshotDestroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  public static func mapSetLayerFilter(_ map: OpaquePointer, layerId: mln_string_view, filter: UnsafePointer<mln_json_value>?) throws {
    try checkStatus(mln_map_set_layer_filter(map, layerId, filter))
  }

  public static func mapGetLayerFilter(_ map: OpaquePointer, layerId: mln_string_view) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_map_get_layer_filter(map, layerId, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { jsonSnapshotDestroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }
}
