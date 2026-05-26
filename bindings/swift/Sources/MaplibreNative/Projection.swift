
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
      try CAPI.createMapProjection(try map.requireLivePointer())
    }
    handle = try NativeHandleBox(typeName: "MapProjectionHandle", pointer: pointer)
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func close() throws {
    try handle.closeOnce { pointer in
      try CAPI.destroyMapProjection(pointer)
    }
  }

  public func camera() throws -> CameraOptions {
    try mapNativeFailure {
      CameraOptions(native: NativeCameraOptionsInput(try CAPI.mapProjectionGetCamera(try handle.requireLive())))
    }
  }

  public func setCamera(_ camera: CameraOptions) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try CAPI.mapProjectionSetCamera(try handle.requireLive(), nativeCamera)
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
        try CAPI.mapProjectionSetVisibleCoordinates(
          try handle.requireLive(),
          coordinates: baseAddress,
          count: buffer.count,
          padding: padding.nativeInput.native
        )
      }
    }
  }

  public func setVisibleGeometry(_ geometry: Geometry, padding: EdgeInsets = EdgeInsets(top: 0, left: 0, bottom: 0, right: 0)) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      try CAPI.mapProjectionSetVisibleGeometry(
        try handle.requireLive(),
        geometry: arena.allocateGeometry(geometry.nativeGeometry),
        padding: padding.nativeInput.native
      )
    }
  }

  public func pixel(for coordinate: LatLng) throws -> ScreenPoint {
    try mapNativeFailure {
      ScreenPoint(native: NativeScreenPoint(try CAPI.mapProjectionPixelForLatLng(
        try handle.requireLive(),
        coordinate: coordinate.nativeInput.native
      )))
    }
  }

  public func latLng(for point: ScreenPoint) throws -> LatLng {
    try mapNativeFailure {
      LatLng(native: NativeLatLng(try CAPI.mapProjectionLatLngForPixel(
        try handle.requireLive(),
        point: point.nativeInput.native
      )))
    }
  }
}

extension Maplibre {
  public static func projectedMeters(for coordinate: LatLng) throws -> ProjectedMeters {
    try mapNativeFailure {
      let meters = try CAPI.projectedMetersForLatLng(coordinate.nativeInput)
      return ProjectedMeters(northing: meters.northing, easting: meters.easting)
    }
  }

  public static func latLng(forProjectedMeters meters: ProjectedMeters) throws -> LatLng {
    try mapNativeFailure {
      LatLng(native: try CAPI.latLngForProjectedMeters(
        NativeProjectedMeters(northing: meters.northing, easting: meters.easting)
      ))
    }
  }
}
