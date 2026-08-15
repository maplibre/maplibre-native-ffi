internal import CMaplibreNativeC

enum NativeProjection {
  static func createStart(_ map: NativeMapHandle) throws
    -> NativeOperationHandle
  {
    let raw = try NativeMemory.withTemporary(mln_operation(0)) { operation in
      try checkStatus(mln_map_projection_create_start(map.raw, operation))
    }.value
    return NativeOperationHandle(raw: raw)
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
