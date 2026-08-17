internal import CMaplibreNativeC

enum NativeMap {
  static func createStart(
    runtime: NativeRuntimeHandle,
    options: UnsafePointer<mln_map_options>
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(mln_operation(0)) { operation in
      try checkStatus(mln_map_create_start(runtime.raw, options, operation))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func createTakeResult(_ operation: NativeOperationHandle) throws
    -> NativeMapHandle
  {
    try NativeHandleFactory.create(
      nullDiagnostic: "mln_map_create_take_result returned a null map"
    ) { map in
      try checkStatus(mln_map_create_take_result(operation.raw, map))
    }
  }

  static func release(_ map: NativeMapHandle) throws {
    try checkStatus(mln_map_release(map.raw))
  }

  static func snapshot(_ map: NativeMapHandle) throws -> mln_map_snapshot {
    var snapshot = mln_map_snapshot()
    snapshot.size = UInt32(MemoryLayout<mln_map_snapshot>.size)
    try checkStatus(mln_map_snapshot_get(map.raw, &snapshot))
    return snapshot
  }

  static func cameraSnapshot(_ map: NativeMapHandle) throws
    -> (camera: mln_camera_options, generation: UInt64)
  {
    var camera = mln_camera_options_default()
    var generation: UInt64 = 0
    try checkStatus(mln_map_camera_snapshot_get(
      map.raw,
      &camera,
      &generation
    ))
    return (camera, generation)
  }

  static func cameraQueryStart(_ map: NativeMapHandle) throws
    -> NativeOperationHandle
  {
    let raw = try NativeMemory.withTemporary(mln_operation(0)) { operation in
      try checkStatus(mln_map_camera_query_start(map.raw, operation))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func cameraQueryTakeResult(_ operation: NativeOperationHandle) throws
    -> mln_camera_query_result
  {
    var result = mln_camera_query_result()
    result.size = UInt32(MemoryLayout<mln_camera_query_result>.size)
    try checkStatus(mln_map_camera_query_take_result(operation.raw, &result))
    return result
  }

  static func requestStillImageStart(_ map: NativeMapHandle) throws
    -> NativeOperationHandle
  {
    let raw = try NativeMemory.withTemporary(mln_operation(0)) { operation in
      try checkStatus(mln_map_request_still_image_start(map.raw, operation))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func setEventMask(_ map: NativeMapHandle, mask: UInt64) throws
    -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { commandId in
      try checkStatus(mln_map_set_event_mask(map.raw, mask, commandId))
    }.value
  }

  static func cameraForLatLngBoundsStart(
    _ map: NativeMapHandle,
    bounds: NativeLatLngBounds,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> NativeOperationHandle {
    try startOperation {
      mln_map_camera_for_lat_lng_bounds_start(
        map.raw, bounds.native, fitOptions, $0
      )
    }
  }

  static func cameraForLatLngsStart(
    _ map: NativeMapHandle,
    coordinates: UnsafePointer<mln_lat_lng>?,
    count: Int,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> NativeOperationHandle {
    try startOperation {
      mln_map_camera_for_lat_lngs_start(
        map.raw, coordinates, count, fitOptions, $0
      )
    }
  }

  static func cameraForGeometryStart(
    _ map: NativeMapHandle,
    geometry: mln_buffer_view,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> NativeOperationHandle {
    try startOperation {
      mln_map_camera_for_geometry_start(map.raw, geometry, fitOptions, $0)
    }
  }

  static func cameraForLatLngBoundsTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> mln_camera_options {
    try takeCamera(
      operation,
      with: mln_map_camera_for_lat_lng_bounds_take_result
    )
  }

  static func cameraForLatLngsTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> mln_camera_options {
    try takeCamera(operation, with: mln_map_camera_for_lat_lngs_take_result)
  }

  static func cameraForGeometryTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> mln_camera_options {
    try takeCamera(operation, with: mln_map_camera_for_geometry_take_result)
  }

  private static func takeCamera(
    _ operation: NativeOperationHandle,
    with take: (mln_operation, UnsafeMutablePointer<mln_camera_options>)
      -> mln_status
  ) throws -> mln_camera_options {
    var camera = mln_camera_options_default()
    try checkStatus(take(operation.raw, &camera))
    return camera
  }

  static func latLngBoundsForCameraStart(
    _ map: NativeMapHandle,
    camera: UnsafePointer<mln_camera_options>,
    unwrapped: Bool
  ) throws -> NativeOperationHandle {
    try startOperation {
      if unwrapped {
        mln_map_lat_lng_bounds_for_camera_unwrapped_start(
          map.raw, camera, $0
        )
      } else {
        mln_map_lat_lng_bounds_for_camera_start(map.raw, camera, $0)
      }
    }
  }

  static func latLngBoundsForCameraTakeResult(
    _ operation: NativeOperationHandle,
    unwrapped: Bool
  ) throws -> NativeLatLngBounds {
    let output = try NativeMemory
      .withTemporary(mln_lat_lng_bounds()) { bounds in
        if unwrapped {
          try checkStatus(
            mln_map_lat_lng_bounds_for_camera_unwrapped_take_result(
              operation.raw, bounds
            )
          )
        } else {
          try checkStatus(mln_map_lat_lng_bounds_for_camera_take_result(
            operation.raw, bounds
          ))
        }
      }
    return NativeLatLngBounds(output.value)
  }

  static func pixelForLatLngStart(
    _ map: NativeMapHandle,
    coordinate: NativeLatLng
  ) throws -> NativeOperationHandle {
    try startOperation {
      mln_map_pixel_for_lat_lng_start(map.raw, coordinate.native, $0)
    }
  }

  static func pixelForLatLngTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> NativeScreenPoint {
    let output = try NativeMemory.withTemporary(mln_screen_point()) { point in
      try checkStatus(mln_map_pixel_for_lat_lng_take_result(
        operation.raw, point
      ))
    }
    return NativeScreenPoint(output.value)
  }

  static func latLngForPixelStart(
    _ map: NativeMapHandle,
    point: NativeScreenPoint
  ) throws -> NativeOperationHandle {
    try startOperation {
      mln_map_lat_lng_for_pixel_start(map.raw, point.native, $0)
    }
  }

  static func latLngForPixelTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> NativeLatLng {
    let output = try NativeMemory.withTemporary(mln_lat_lng()) { coordinate in
      try checkStatus(mln_map_lat_lng_for_pixel_take_result(
        operation.raw, coordinate
      ))
    }
    return NativeLatLng(output.value)
  }

  static func pixelsForLatLngsStart(
    _ map: NativeMapHandle,
    coordinates: [NativeLatLng]
  ) throws -> NativeOperationHandle {
    let raw = coordinates.map(\.native)
    return try raw.withUnsafeBufferPointer { coordinates in
      try startOperation {
        mln_map_pixels_for_lat_lngs_start(
          map.raw, coordinates.baseAddress, coordinates.count, $0
        )
      }
    }
  }

  static func pixelsForLatLngsTakeResult(
    _ operation: NativeOperationHandle,
    count: Int
  ) throws -> [NativeScreenPoint] {
    var points = [mln_screen_point](repeating: mln_screen_point(), count: count)
    var outputCount = 0
    try points.withUnsafeMutableBufferPointer { points in
      try checkStatus(mln_map_pixels_for_lat_lngs_take_result(
        operation.raw, points.baseAddress, points.count, &outputCount
      ))
    }
    return points.prefix(outputCount).map(NativeScreenPoint.init)
  }

  static func latLngsForPixelsStart(
    _ map: NativeMapHandle,
    points: [NativeScreenPoint]
  ) throws -> NativeOperationHandle {
    let raw = points.map(\.native)
    return try raw.withUnsafeBufferPointer { points in
      try startOperation {
        mln_map_lat_lngs_for_pixels_start(
          map.raw, points.baseAddress, points.count, $0
        )
      }
    }
  }

  static func latLngsForPixelsTakeResult(
    _ operation: NativeOperationHandle,
    count: Int
  ) throws -> [NativeLatLng] {
    var coordinates = [mln_lat_lng](repeating: mln_lat_lng(), count: count)
    var outputCount = 0
    try coordinates.withUnsafeMutableBufferPointer { coordinates in
      try checkStatus(mln_map_lat_lngs_for_pixels_take_result(
        operation.raw,
        coordinates.baseAddress,
        coordinates.count,
        &outputCount
      ))
    }
    return coordinates.prefix(outputCount).map(NativeLatLng.init)
  }

  private static func startOperation(
    _ start: (UnsafeMutablePointer<mln_operation>) -> mln_status
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(mln_operation(0)) { operation in
      try checkStatus(start(operation))
    }.value
    return NativeOperationHandle(raw: raw)
  }
}
