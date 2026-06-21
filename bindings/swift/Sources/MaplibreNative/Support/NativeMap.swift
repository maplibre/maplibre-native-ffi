internal import CMaplibreNativeC

enum NativeMap {
  static func create(
    runtime: OpaquePointer,
    options: UnsafePointer<mln_map_options>
  ) throws -> OpaquePointer {
    try NativeHandleFactory
      .create(nullDiagnostic: "mln_map_create returned a null map") { map in
        try checkStatus(mln_map_create(runtime, options, map))
      }
  }

  static func debugOptions(_ map: OpaquePointer) throws -> UInt32 {
    try NativeMemory.withTemporary(UInt32(0)) { options in
      try checkStatus(mln_map_get_debug_options(map, options))
    }.value
  }

  static func renderingStatsViewEnabled(_ map: OpaquePointer) throws -> Bool {
    try NativeMemory.withTemporary(false) { enabled in
      try checkStatus(mln_map_get_rendering_stats_view_enabled(map, enabled))
    }.value
  }

  static func isFullyLoaded(_ map: OpaquePointer) throws -> Bool {
    try NativeMemory.withTemporary(false) { loaded in
      try checkStatus(mln_map_is_fully_loaded(map, loaded))
    }.value
  }

  static func viewportOptions(_ map: OpaquePointer) throws
    -> mln_map_viewport_options
  {
    var options = mln_map_viewport_options_default()
    try checkStatus(mln_map_get_viewport_options(map, &options))
    return options
  }

  static func tileOptions(_ map: OpaquePointer) throws -> mln_map_tile_options {
    var options = mln_map_tile_options_default()
    try checkStatus(mln_map_get_tile_options(map, &options))
    return options
  }

  static func camera(_ map: OpaquePointer) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_get_camera(map, &camera))
    return camera
  }

  static func cameraForLatLngBounds(
    _ map: OpaquePointer,
    bounds: NativeLatLngBounds,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_camera_for_lat_lng_bounds(
      map,
      bounds.native,
      fitOptions,
      &camera
    ))
    return camera
  }

  static func cameraForLatLngs(
    _ map: OpaquePointer,
    coordinates: UnsafePointer<mln_lat_lng>?,
    count: Int,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_camera_for_lat_lngs(
      map,
      coordinates,
      count,
      fitOptions,
      &camera
    ))
    return camera
  }

  static func cameraForGeometry(
    _ map: OpaquePointer,
    geometry: UnsafePointer<mln_geometry>,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_camera_for_geometry(
      map,
      geometry,
      fitOptions,
      &camera
    ))
    return camera
  }

  static func latLngBoundsForCamera(
    _ map: OpaquePointer,
    camera: UnsafePointer<mln_camera_options>,
    unwrapped: Bool
  ) throws -> NativeLatLngBounds {
    let output = try NativeMemory
      .withTemporary(mln_lat_lng_bounds()) { bounds in
        if unwrapped {
          try checkStatus(mln_map_lat_lng_bounds_for_camera_unwrapped(
            map,
            camera,
            bounds
          ))
        } else {
          try checkStatus(mln_map_lat_lng_bounds_for_camera(
            map,
            camera,
            bounds
          ))
        }
      }
    return NativeLatLngBounds(output.value)
  }

  static func bounds(_ map: OpaquePointer) throws -> mln_bound_options {
    var bounds = mln_bound_options_default()
    try checkStatus(mln_map_get_bounds(map, &bounds))
    return bounds
  }

  static func freeCameraOptions(_ map: OpaquePointer) throws
    -> mln_free_camera_options
  {
    var options = mln_free_camera_options_default()
    try checkStatus(mln_map_get_free_camera_options(map, &options))
    return options
  }

  static func projectionMode(_ map: OpaquePointer) throws
    -> mln_projection_mode
  {
    var mode = mln_projection_mode_default()
    try checkStatus(mln_map_get_projection_mode(map, &mode))
    return mode
  }

  static func pixelForLatLng(_ map: OpaquePointer,
                             coordinate: NativeLatLng) throws
    -> NativeScreenPoint
  {
    let output = try NativeMemory.withTemporary(mln_screen_point()) { point in
      try checkStatus(mln_map_pixel_for_lat_lng(map, coordinate.native, point))
    }
    return NativeScreenPoint(output.value)
  }

  static func latLngForPixel(_ map: OpaquePointer,
                             point: NativeScreenPoint) throws -> NativeLatLng
  {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_map_lat_lng_for_pixel(map, point.native, coordinate))
    }
    return NativeLatLng(output.value)
  }

  static func pixelsForLatLngs(
    _ map: OpaquePointer,
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
          map,
          coordinates.baseAddress,
          coordinates.count,
          points.baseAddress
        ))
      }
    }
    return rawPoints.map(NativeScreenPoint.init)
  }

  static func latLngsForPixels(
    _ map: OpaquePointer,
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
          map,
          points.baseAddress,
          points.count,
          coordinates.baseAddress
        ))
      }
    }
    return rawCoordinates.map(NativeLatLng.init)
  }
}
