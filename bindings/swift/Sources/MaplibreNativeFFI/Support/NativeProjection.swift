internal import CMaplibreNativeC

enum NativeProjection {
  private static func startOperation(
    _ body: (UnsafeMutablePointer<mln_operation>) throws -> mln_status
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(mln_operation(0)) {
      try checkStatus(body($0))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func createStart(_ map: NativeMapHandle) throws
    -> NativeOperationHandle
  {
    try startOperation { mln_map_projection_create_start(map.raw, $0) }
  }

  static func createTakeResult(_ operation: NativeOperationHandle) throws
    -> NativeMapProjectionHandle
  {
    try NativeHandleFactory.create(
      nullDiagnostic:
      "mln_map_projection_create_take_result returned a null projection"
    ) {
      try checkStatus(mln_map_projection_create_take_result(operation.raw, $0))
    }
  }

  static func closeStart(_ projection: NativeMapProjectionHandle) throws
    -> NativeOperationHandle
  {
    try startOperation { mln_map_projection_close_start(projection.raw, $0) }
  }

  static func cameraStart(_ projection: NativeMapProjectionHandle) throws
    -> NativeOperationHandle
  {
    try startOperation {
      mln_map_projection_get_camera_start(projection.raw, $0)
    }
  }

  static func cameraTakeResult(_ operation: NativeOperationHandle) throws
    -> mln_camera_options
  {
    var camera = mln_camera_options_default()
    try checkStatus(mln_map_projection_get_camera_take_result(
      operation.raw, &camera
    ))
    return camera
  }

  static func pixelForLatLngStart(
    _ projection: NativeMapProjectionHandle,
    coordinate: mln_lat_lng
  ) throws -> NativeOperationHandle {
    try startOperation {
      mln_map_projection_pixel_for_lat_lng_start(
        projection.raw, coordinate, $0
      )
    }
  }

  static func pixelForLatLngTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> mln_screen_point {
    try NativeMemory.withTemporary(mln_screen_point()) { point in
      try checkStatus(mln_map_projection_pixel_for_lat_lng_take_result(
        operation.raw, point
      ))
    }.value
  }

  static func latLngForPixelStart(
    _ projection: NativeMapProjectionHandle,
    point: mln_screen_point
  ) throws -> NativeOperationHandle {
    try startOperation {
      mln_map_projection_lat_lng_for_pixel_start(projection.raw, point, $0)
    }
  }

  static func latLngForPixelTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> mln_lat_lng {
    try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_map_projection_lat_lng_for_pixel_take_result(
        operation.raw, coordinate
      ))
    }.value
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
