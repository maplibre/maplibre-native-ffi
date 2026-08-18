internal import CMaplibreNativeC

enum NativeMap {
  static func create(
    runtime: NativeRuntimeHandle,
    options: UnsafePointer<mln_map_options>
  ) throws -> NativeFuture<NativeMapHandle> {
    try NativeCompletion.start(
      { mln_map_create(runtime.raw, options, $0) }
    ) { result in
      try NativeMapHandle(raw: NativeCompletion.value(result, as: mln_map.self))
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

  static func cameraQuery(
    _ map: NativeMapHandle
  ) throws -> NativeFuture<CameraSnapshot> {
    try NativeCompletion.start(
      { mln_map_camera_query(map.raw, $0) }
    ) { result in
      let value: mln_camera_query_result = try NativeCompletion.value(result)
      return CameraSnapshot(
        generation: value.generation,
        camera: CameraOptions(native: NativeCameraOptionsInput(value.camera))
      )
    }
  }

  static func requestStillImage(
    _ map: NativeMapHandle
  ) throws -> NativeFuture<Void> {
    try NativeCompletion.startUnit {
      mln_map_request_still_image(map.raw, $0)
    }
  }

  static func cameraForLatLngBounds(
    _ map: NativeMapHandle,
    bounds: NativeLatLngBounds,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> NativeFuture<CameraOptions> {
    try camera {
      mln_map_camera_for_lat_lng_bounds(
        map.raw,
        bounds.native,
        fitOptions,
        $0
      )
    }
  }

  static func cameraForLatLngs(
    _ map: NativeMapHandle,
    coordinates: UnsafePointer<mln_lat_lng>?,
    count: Int,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> NativeFuture<CameraOptions> {
    try camera {
      mln_map_camera_for_lat_lngs(
        map.raw,
        coordinates,
        count,
        fitOptions,
        $0
      )
    }
  }

  static func cameraForGeometry(
    _ map: NativeMapHandle,
    geometry: mln_buffer_view,
    fitOptions: UnsafePointer<mln_camera_fit_options>?
  ) throws -> NativeFuture<CameraOptions> {
    try camera {
      mln_map_camera_for_geometry(map.raw, geometry, fitOptions, $0)
    }
  }

  private static func camera(
    _ start: (UnsafePointer<mln_completion>) -> mln_status
  ) throws -> NativeFuture<CameraOptions> {
    try NativeCompletion.start(start) { result in
      let value: mln_camera_options = try NativeCompletion.value(result)
      return CameraOptions(native: NativeCameraOptionsInput(value))
    }
  }

  static func latLngBoundsForCamera(
    _ map: NativeMapHandle,
    camera: UnsafePointer<mln_camera_options>,
    unwrapped: Bool
  ) throws -> NativeFuture<LatLngBounds> {
    try NativeCompletion.start(
      { completion in
        if unwrapped {
          mln_map_lat_lng_bounds_for_camera_unwrapped(
            map.raw,
            camera,
            completion
          )
        } else {
          mln_map_lat_lng_bounds_for_camera(map.raw, camera, completion)
        }
      }
    ) { result in
      let value: mln_lat_lng_bounds = try NativeCompletion.value(result)
      return LatLngBounds(native: NativeLatLngBounds(value))
    }
  }

  static func pixelForLatLng(
    _ map: NativeMapHandle,
    coordinate: NativeLatLng
  ) throws -> NativeFuture<ScreenPoint> {
    try NativeCompletion.start(
      { mln_map_pixel_for_lat_lng(map.raw, coordinate.native, $0) }
    ) { result in
      let value: mln_screen_point = try NativeCompletion.value(result)
      return ScreenPoint(native: NativeScreenPoint(value))
    }
  }

  static func latLngForPixel(
    _ map: NativeMapHandle,
    point: NativeScreenPoint
  ) throws -> NativeFuture<LatLng> {
    try NativeCompletion.start(
      { mln_map_lat_lng_for_pixel(map.raw, point.native, $0) }
    ) { result in
      let value: mln_lat_lng = try NativeCompletion.value(result)
      return LatLng(native: NativeLatLng(value))
    }
  }

  static func pixelsForLatLngs(
    _ map: NativeMapHandle,
    coordinates: [NativeLatLng]
  ) throws -> NativeFuture<[ScreenPoint]> {
    let raw = coordinates.map(\.native)
    return try raw.withUnsafeBufferPointer { coordinates in
      try NativeCompletion.start(
        {
          mln_map_pixels_for_lat_lngs(
            map.raw,
            coordinates.baseAddress,
            coordinates.count,
            $0
          )
        }
      ) { result in
        try NativeCompletion.values(result, as: mln_screen_point.self).map {
          ScreenPoint(native: NativeScreenPoint($0))
        }
      }
    }
  }

  static func latLngsForPixels(
    _ map: NativeMapHandle,
    points: [NativeScreenPoint]
  ) throws -> NativeFuture<[LatLng]> {
    let raw = points.map(\.native)
    return try raw.withUnsafeBufferPointer { points in
      try NativeCompletion.start(
        {
          mln_map_lat_lngs_for_pixels(
            map.raw,
            points.baseAddress,
            points.count,
            $0
          )
        }
      ) { result in
        try NativeCompletion.values(result, as: mln_lat_lng.self).map {
          LatLng(native: NativeLatLng($0))
        }
      }
    }
  }
}
