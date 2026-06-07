
internal import CMaplibreNativeC

public struct ProjectedMeters: Equatable, Sendable {
  public let northing: Double
  public let easting: Double

  public init(northing: Double, easting: Double) {
    self.northing = northing
    self.easting = easting
  }
}

public final class MapProjectionHandle {
  private let handle: NativeHandleBox

  public init(map: MapHandle) throws {
    let pointer = try mapNativeFailure {
      try NativeProjection.create(try map.requireLivePointer())
    }
    handle = try NativeHandleBox(typeName: "MapProjectionHandle", pointer: pointer)
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func close() throws {
    try handle.closeOnce { pointer in
      try checkStatus(mln_map_projection_destroy(pointer))
    }
  }

  public func camera() throws -> CameraOptions {
    try mapNativeFailure {
      CameraOptions(native: NativeCameraOptionsInput(try NativeProjection.camera(try handle.requireLive())))
    }
  }

  public func setCamera(_ camera: CameraOptions) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try checkStatus(mln_map_projection_set_camera(try handle.requireLive(), nativeCamera))
      }
    }
  }

  public func setVisibleCoordinates(_ coordinates: [LatLng], padding: EdgeInsets = EdgeInsets(top: 0, left: 0, bottom: 0, right: 0)) throws {
    try mapNativeFailure {
      guard !coordinates.isEmpty else {
        throw MaplibreError.invalidArgument("visible coordinates cannot be empty")
      }
      let nativeCoordinates = coordinates.map(\.nativeInput.native)
      try nativeCoordinates.withUnsafeBufferPointer { buffer in
        guard let baseAddress = buffer.baseAddress else {
          throw MaplibreError.invalidArgument("visible coordinates cannot be empty")
        }
        try checkStatus(mln_map_projection_set_visible_coordinates(try handle.requireLive(), baseAddress, buffer.count, padding.nativeInput.native))
      }
    }
  }

  public func setVisibleGeometry(_ geometry: Geometry, padding: EdgeInsets = EdgeInsets(top: 0, left: 0, bottom: 0, right: 0)) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try checkStatus(mln_map_projection_set_visible_geometry(try handle.requireLive(), arena.allocateGeometry(geometry.nativeGeometry), padding.nativeInput.native))
    }
  }

  public func pixel(for coordinate: LatLng) throws -> ScreenPoint {
    try mapNativeFailure {
      ScreenPoint(native: NativeScreenPoint(try NativeProjection.pixelForLatLng(
        try handle.requireLive(),
        coordinate: coordinate.nativeInput.native
      )))
    }
  }

  public func latLng(for point: ScreenPoint) throws -> LatLng {
    try mapNativeFailure {
      LatLng(native: NativeLatLng(try NativeProjection.latLngForPixel(
        try handle.requireLive(),
        point: point.nativeInput.native
      )))
    }
  }
}

extension Maplibre {
  public static func projectedMeters(for coordinate: LatLng) throws -> ProjectedMeters {
    try mapNativeFailure {
      let meters = try NativeProjection.projectedMetersForLatLng(coordinate.nativeInput)
      return ProjectedMeters(northing: meters.northing, easting: meters.easting)
    }
  }

  public static func latLng(forProjectedMeters meters: ProjectedMeters) throws -> LatLng {
    try mapNativeFailure {
      LatLng(native: try NativeProjection.latLngForProjectedMeters(
        NativeProjectedMeters(northing: meters.northing, easting: meters.easting)
      ))
    }
  }
}
