internal import CMaplibreNativeC

enum NativeProjection {
  static func create(_ map: OpaquePointer) throws -> OpaquePointer {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_map_projection_create returned a null projection"
      ) { projection in
        try checkStatus(mln_map_projection_create(map, projection))
      }
  }

  static func camera(_ projection: OpaquePointer) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_projection_get_camera(projection, &camera))
    return camera
  }

  static func pixelForLatLng(
    _ projection: OpaquePointer,
    coordinate: mln_lat_lng
  ) throws -> mln_screen_point {
    let output = try NativeMemory.withTemporary(mln_screen_point()) { point in
      try checkStatus(mln_map_projection_pixel_for_lat_lng(
        projection,
        coordinate,
        point
      ))
    }
    return output.value
  }

  static func latLngForPixel(
    _ projection: OpaquePointer,
    point: mln_screen_point
  ) throws -> mln_lat_lng {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_map_projection_lat_lng_for_pixel(
        projection,
        point,
        coordinate
      ))
    }
    return output.value
  }

  static func projectedMetersForLatLng(_ coordinate: NativeLatLng) throws
    -> NativeProjectedMeters
  {
    let output = try NativeMemory
      .withTemporary(mln_projected_meters()) { meters in
        try checkStatus(mln_projected_meters_for_lat_lng(
          coordinate.native,
          meters
        ))
      }
    return NativeProjectedMeters(output.value)
  }

  static func latLngForProjectedMeters(_ meters: NativeProjectedMeters) throws
    -> NativeLatLng
  {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_lat_lng_for_projected_meters(
        meters.native,
        coordinate
      ))
    }
    return NativeLatLng(output.value)
  }
}
