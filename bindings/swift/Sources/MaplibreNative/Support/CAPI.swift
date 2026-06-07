internal import CMaplibreNativeC
import Foundation

enum CAPI {
  static func createRuntime(_ options: UnsafePointer<mln_runtime_options>) throws -> OpaquePointer {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { runtime in
      try checkStatus(mln_runtime_create(options, runtime))
    }
    guard let runtime = output.value else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "mln_runtime_create returned a null runtime")
    }
    return runtime
  }

  static func runtimePollEvent(_ runtime: OpaquePointer) throws -> mln_runtime_event? {
    var event = mln_runtime_event()
    event.size = UInt32(MemoryLayout<mln_runtime_event>.size)
    let output = try NativeMemory.withTemporary(false) { hasEvent in
      try checkStatus(mln_runtime_poll_event(runtime, &event, hasEvent))
    }
    return output.value ? event : nil
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

  static func mapGetDebugOptions(_ map: OpaquePointer) throws -> UInt32 {
    try NativeMemory.withTemporary(UInt32(0)) { options in
      try checkStatus(mln_map_get_debug_options(map, options))
    }.value
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

  static func mapGetViewportOptions(_ map: OpaquePointer) throws -> mln_map_viewport_options {
    var options = mln_map_viewport_options_default()
    try checkStatus(mln_map_get_viewport_options(map, &options))
    return options
  }

  static func mapGetTileOptions(_ map: OpaquePointer) throws -> mln_map_tile_options {
    var options = mln_map_tile_options_default()
    try checkStatus(mln_map_get_tile_options(map, &options))
    return options
  }

  static func mapGetCamera(_ map: OpaquePointer) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_get_camera(map, &camera))
    return camera
  }

  static func mapCameraForLatLngBounds(_ map: OpaquePointer, bounds: NativeLatLngBounds, fitOptions: UnsafePointer<mln_camera_fit_options>?) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_camera_for_lat_lng_bounds(map, bounds.native, fitOptions, &camera))
    return camera
  }

  static func mapCameraForLatLngs(_ map: OpaquePointer, coordinates: UnsafePointer<mln_lat_lng>?, count: Int, fitOptions: UnsafePointer<mln_camera_fit_options>?) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_camera_for_lat_lngs(map, coordinates, count, fitOptions, &camera))
    return camera
  }

  static func mapCameraForGeometry(_ map: OpaquePointer, geometry: UnsafePointer<mln_geometry>, fitOptions: UnsafePointer<mln_camera_fit_options>?) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
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
    var bounds = mln_bound_options_default()
    try checkStatus(mln_map_get_bounds(map, &bounds))
    return bounds
  }

  static func mapGetFreeCameraOptions(_ map: OpaquePointer) throws -> mln_free_camera_options {
    var options = mln_free_camera_options_default()
    try checkStatus(mln_map_get_free_camera_options(map, &options))
    return options
  }

  static func mapGetProjectionMode(_ map: OpaquePointer) throws -> mln_projection_mode {
    var mode = mln_projection_mode_default()
    try checkStatus(mln_map_get_projection_mode(map, &mode))
    return mode
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

  static func mapPixelsForLatLngs(_ map: OpaquePointer, coordinates: [NativeLatLng]) throws -> [NativeScreenPoint] {
    let rawCoordinates = coordinates.map(\.native)
    var rawPoints = [mln_screen_point](repeating: mln_screen_point(), count: rawCoordinates.count)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try rawPoints.withUnsafeMutableBufferPointer { points in
        try checkStatus(mln_map_pixels_for_lat_lngs(map, coordinates.baseAddress, coordinates.count, points.baseAddress))
      }
    }
    return rawPoints.map(NativeScreenPoint.init)
  }

  static func mapLatLngsForPixels(_ map: OpaquePointer, points: [NativeScreenPoint]) throws -> [NativeLatLng] {
    let rawPoints = points.map(\.native)
    var rawCoordinates = [mln_lat_lng](repeating: mln_lat_lng(), count: rawPoints.count)
    try rawPoints.withUnsafeBufferPointer { points in
      try rawCoordinates.withUnsafeMutableBufferPointer { coordinates in
        try checkStatus(mln_map_lat_lngs_for_pixels(map, points.baseAddress, points.count, coordinates.baseAddress))
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

  static func mapProjectionGetCamera(_ projection: OpaquePointer) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_projection_get_camera(projection, &camera))
    return camera
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
    var info = mln_texture_image_info_default()
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

  static func vulkanOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_vulkan_owned_texture_frame {
    var frame = mln_vulkan_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_vulkan_owned_texture_frame>.size)
    try checkStatus(mln_vulkan_owned_texture_acquire_frame(session, &frame))
    return frame
  }

  static func openGLOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_opengl_owned_texture_frame {
    var frame = mln_opengl_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_opengl_owned_texture_frame>.size)
    try checkStatus(mln_opengl_owned_texture_acquire_frame(session, &frame))
    return frame
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

  static func jsonSnapshotCopyValue(_ snapshot: OpaquePointer) throws -> NativeJSONValue? {
    let output = try NativeMemory.withTemporary(Optional<UnsafePointer<mln_json_value>>.none) { value in
      try checkStatus(mln_json_snapshot_get(snapshot, value))
    }
    guard let value = output.value else { return nil }
    return try NativeJSONValue(copying: value.pointee)
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
    guard size <= capacity else {
      throw NativeStatusFailure(rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue, diagnostic: "native style source attribution size exceeded caller buffer")
    }
    return (String(decoding: bytes.prefix(size), as: UTF8.self), size)
  }

  static func mapListStyleSourceIds(_ map: OpaquePointer) throws -> [String] {
    let list = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { list in
      try checkStatus(mln_map_list_style_source_ids(map, list))
    }.value
    guard let list else { throw NativeStatusFailure(rawStatus: 0, diagnostic: "source ID list was null") }
    defer { mln_style_id_list_destroy(list) }
    return try styleIdListCopy(list)
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
    guard found else { return (nil, size) }
    guard size <= capacity else {
      throw NativeStatusFailure(rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue, diagnostic: "native style image byte size exceeded caller buffer")
    }
    return (Array(bytes.prefix(size)), size)
  }

  static func mapAddImageSourceURL(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: [NativeLatLng], url: mln_string_view) throws {
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try checkStatus(mln_map_add_image_source_url(map, sourceId, coordinates.baseAddress, coordinates.count, url))
    }
  }

  static func mapAddImageSourceImage(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: [NativeLatLng], image: UnsafePointer<mln_premultiplied_rgba8_image>) throws {
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try checkStatus(mln_map_add_image_source_image(map, sourceId, coordinates.baseAddress, coordinates.count, image))
    }
  }

  static func mapSetImageSourceCoordinates(_ map: OpaquePointer, sourceId: mln_string_view, coordinates: [NativeLatLng]) throws {
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try checkStatus(mln_map_set_image_source_coordinates(map, sourceId, coordinates.baseAddress, coordinates.count))
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
    defer { mln_style_id_list_destroy(list) }
    return try styleIdListCopy(list)
  }

  static func mapGetStyleLayerJSON(_ map: OpaquePointer, layerId: mln_string_view) throws -> NativeJSONValue? {
    let output = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try NativeMemory.withTemporary(false) { found in
        try checkStatus(mln_map_get_style_layer_json(map, layerId, snapshot, found))
        if !found.pointee { snapshot.pointee = nil }
      }
    }.value
    guard let snapshot = output else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  static func mapGetStyleLightProperty(_ map: OpaquePointer, propertyName: mln_string_view) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_map_get_style_light_property(map, propertyName, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  static func mapGetLayerProperty(_ map: OpaquePointer, layerId: mln_string_view, propertyName: mln_string_view) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_map_get_layer_property(map, layerId, propertyName, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }

  static func mapGetLayerFilter(_ map: OpaquePointer, layerId: mln_string_view) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_map_get_layer_filter(map, layerId, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
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

  static func runtimeRunAmbientCacheOperationStart(_ runtime: OpaquePointer, operation: UInt32) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_run_ambient_cache_operation_start(runtime, operation, operationId))
    }.value
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

  static func renderSessionGetFeatureState(_ session: OpaquePointer, selector: UnsafePointer<mln_feature_state_selector>) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory.withTemporary(Optional<OpaquePointer>.none) { snapshot in
      try checkStatus(mln_render_session_get_feature_state(session, selector, snapshot))
    }.value
    guard let snapshot else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
    return try jsonSnapshotCopyValue(snapshot)
  }
}
