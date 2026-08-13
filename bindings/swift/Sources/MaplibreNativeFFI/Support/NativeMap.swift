internal import CMaplibreNativeC

enum NativeMap {
  static func create(
    runtime: NativeRuntimeHandle,
    options: UnsafePointer<mln_map_options>
  ) throws -> NativeMapHandle {
    try NativeHandleFactory
      .create(nullDiagnostic: "mln_map_create returned a null map") { outHandle in
        try checkStatus(mln_map_create(runtime.raw, options, outHandle))
      }
  }

  static func setEventMask(_ map: NativeMapHandle, mask: UInt64) throws {
    try checkStatus(mln_map_set_event_mask(map.raw, mask))
  }

  static func eventMask(_ map: NativeMapHandle) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { mask in
      try checkStatus(mln_map_get_event_mask(map.raw, mask))
    }.value
  }

  static func debugOptions(_ map: NativeMapHandle) throws -> UInt32 {
    try NativeMemory.withTemporary(UInt32(0)) { options in
      try checkStatus(mln_map_get_debug_options(map.raw, options))
    }.value
  }

  static func renderingStatsViewEnabled(_ map: NativeMapHandle) throws -> Bool {
    try NativeMemory.withTemporary(false) { enabled in
      try checkStatus(mln_map_get_rendering_stats_view_enabled(
        map.raw,
        enabled
      ))
    }.value
  }

  static func isFullyLoaded(_ map: NativeMapHandle) throws -> Bool {
    try NativeMemory.withTemporary(false) { loaded in
      try checkStatus(mln_map_is_fully_loaded(map.raw, loaded))
    }.value
  }

  static func isGestureInProgress(_ map: NativeMapHandle) throws -> Bool {
    try NativeMemory.withTemporary(false) { inProgress in
      try checkStatus(mln_map_is_gesture_in_progress(map.raw, inProgress))
    }.value
  }

  static func size(_ map: NativeMapHandle) throws
    -> (width: UInt32, height: UInt32, scaleFactor: Double)
  {
    var width: UInt32 = 0
    var height: UInt32 = 0
    var scaleFactor: Double = 0
    try checkStatus(mln_map_get_size(map.raw, &width, &height, &scaleFactor))
    return (width: width, height: height, scaleFactor: scaleFactor)
  }

  static func viewportOptions(_ map: NativeMapHandle) throws
    -> mln_map_viewport_options
  {
    var options = mln_map_viewport_options_default()
    try checkStatus(mln_map_get_viewport_options(map.raw, &options))
    return options
  }

  static func tileOptions(_ map: NativeMapHandle) throws
    -> mln_map_tile_options
  {
    var options = mln_map_tile_options_default()
    try checkStatus(mln_map_get_tile_options(map.raw, &options))
    return options
  }

  static func camera(_ map: NativeMapHandle) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_get_camera(map.raw, &camera))
    return camera
  }

  static func cameraForLatLngBounds(
    _ map: NativeMapHandle,
    bounds: NativeLatLngBounds,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_camera_for_lat_lng_bounds(
      map.raw,
      bounds.native,
      fitOptions,
      &camera
    ))
    return camera
  }

  static func cameraForLatLngs(
    _ map: NativeMapHandle,
    coordinates: UnsafePointer<mln_lat_lng>?,
    count: Int,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_camera_for_lat_lngs(
      map.raw,
      coordinates,
      count,
      fitOptions,
      &camera
    ))
    return camera
  }

  static func cameraForGeometry(
    _ map: NativeMapHandle,
    geometry: mln_buffer_view,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_camera_for_geometry(
      map.raw,
      geometry,
      fitOptions,
      &camera
    ))
    return camera
  }

  static func latLngBoundsForCamera(
    _ map: NativeMapHandle,
    camera: UnsafePointer<mln_camera_options>,
    unwrapped: Bool
  ) throws -> NativeLatLngBounds {
    let output = try NativeMemory
      .withTemporary(mln_lat_lng_bounds()) { bounds in
        if unwrapped {
          try checkStatus(mln_map_lat_lng_bounds_for_camera_unwrapped(
            map.raw,
            camera,
            bounds
          ))
        } else {
          try checkStatus(mln_map_lat_lng_bounds_for_camera(
            map.raw,
            camera,
            bounds
          ))
        }
      }
    return NativeLatLngBounds(output.value)
  }

  static func bounds(_ map: NativeMapHandle) throws -> mln_bound_options {
    var bounds = mln_bound_options_default()
    try checkStatus(mln_map_get_bounds(map.raw, &bounds))
    return bounds
  }

  static func freeCameraOptions(_ map: NativeMapHandle) throws
    -> mln_free_camera_options
  {
    var options = mln_free_camera_options_default()
    try checkStatus(mln_map_get_free_camera_options(map.raw, &options))
    return options
  }

  static func projectionMode(_ map: NativeMapHandle) throws
    -> mln_projection_mode
  {
    var mode = mln_projection_mode_default()
    try checkStatus(mln_map_get_projection_mode(map.raw, &mode))
    return mode
  }

  static func pixelForLatLng(_ map: NativeMapHandle,
                             coordinate: NativeLatLng) throws
    -> NativeScreenPoint
  {
    let output = try NativeMemory.withTemporary(mln_screen_point()) { point in
      try checkStatus(mln_map_pixel_for_lat_lng(
        map.raw,
        coordinate.native,
        point
      ))
    }
    return NativeScreenPoint(output.value)
  }

  static func latLngForPixel(_ map: NativeMapHandle,
                             point: NativeScreenPoint) throws -> NativeLatLng
  {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_map_lat_lng_for_pixel(
        map.raw,
        point.native,
        coordinate
      ))
    }
    return NativeLatLng(output.value)
  }

  static func pixelsForLatLngs(
    _ map: NativeMapHandle,
    coordinates: [NativeLatLng]
  ) throws -> [NativeScreenPoint] {
    let rawCoordinates = coordinates.map(\.native)
    var rawPoints = [mln_screen_point](
      repeating: mln_screen_point(),
      count: rawCoordinates.count
    )
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try rawPoints.withUnsafeMutableBufferPointer { points in
        try checkStatus(mln_map_pixels_for_lat_lngs(
          map.raw,
          coordinates.baseAddress,
          coordinates.count,
          points.baseAddress
        ))
      }
    }
    return rawPoints.map(NativeScreenPoint.init)
  }

  static func latLngsForPixels(
    _ map: NativeMapHandle,
    points: [NativeScreenPoint]
  ) throws -> [NativeLatLng] {
    let rawPoints = points.map(\.native)
    var rawCoordinates = [mln_lat_lng](
      repeating: mln_lat_lng(),
      count: rawPoints.count
    )
    try rawPoints.withUnsafeBufferPointer { points in
      try rawCoordinates.withUnsafeMutableBufferPointer { coordinates in
        try checkStatus(mln_map_lat_lngs_for_pixels(
          map.raw,
          points.baseAddress,
          points.count,
          coordinates.baseAddress
        ))
      }
    }
    return rawCoordinates.map(NativeLatLng.init)
  }
}
