internal import CMaplibreNativeC

enum NativeProjection {
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
