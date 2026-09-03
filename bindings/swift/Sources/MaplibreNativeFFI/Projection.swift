
internal import CMaplibreNativeC
import Foundation

public struct ProjectedMeters: Equatable, Sendable {
  public let northing: Double
  public let easting: Double

  public init(northing: Double, easting: Double) {
    self.northing = northing
    self.easting = easting
  }
}

/// An any-thread projection snapshot whose native calls are serialized.
public final class MapProjectionHandle: @unchecked Sendable {
  private let handle: NativeHandleBox<NativeMapProjectionHandle>

  public init(map: MapHandle) throws {
    let projection = try mapNativeFailure {
      try NativeProjection.create(map.requireLiveHandle())
    }
    handle = try NativeHandleBox(
      typeName: "MapProjectionHandle",
      handle: projection
    )
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func close() throws {
    try handle.closeOnce { projection in
      try checkStatus(mln_map_projection_destroy(projection.raw))
    }
  }

  public func camera() throws -> CameraOptions {
    try mapNativeFailure {
      try handle.withLive { projection in
        try CameraOptions(native: NativeCameraOptionsInput(NativeProjection
            .camera(projection)))
      }
    }
  }

  public func setCamera(_ camera: CameraOptions) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try handle.withLive { projection in
          try checkStatus(mln_map_projection_set_camera(
            projection.raw,
            nativeCamera
          ))
        }
      }
    }
  }

  public func setVisibleCoordinates(
    _ coordinates: [LatLng],
    padding: EdgeInsets = EdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
  ) throws {
    try mapNativeFailure {
      guard !coordinates.isEmpty else {
        throw MaplibreError
          .invalidArgument("visible coordinates cannot be empty")
      }
      let nativeCoordinates = coordinates.map(\.nativeInput.native)
      try nativeCoordinates.withUnsafeBufferPointer { buffer in
        guard let baseAddress = buffer.baseAddress else {
          throw MaplibreError
            .invalidArgument("visible coordinates cannot be empty")
        }
        try handle.withLive { projection in
          try checkStatus(mln_map_projection_set_visible_coordinates(
            projection.raw,
            baseAddress,
            buffer.count,
            padding.nativeInput.native
          ))
        }
      }
    }
  }

  public func setVisibleGeometry(
    _ geometry: Data,
    padding: EdgeInsets = EdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
  ) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try handle.withLive { projection in
        try checkStatus(mln_map_projection_set_visible_geometry(
          projection.raw,
          arena.view(geometry),
          padding.nativeInput.native
        ))
      }
    }
  }

  public func pixel(for coordinate: LatLng) throws -> ScreenPoint {
    try mapNativeFailure {
      try handle.withLive { projection in
        try ScreenPoint(native: NativeScreenPoint(NativeProjection
            .pixelForLatLng(
              projection,
              coordinate: coordinate.nativeInput.native
            )))
      }
    }
  }

  /// Converts a screen point to a geographic coordinate.
  ///
  /// The longitude is wrapped to the range from -180 to 180 degrees.
  public func latLng(for point: ScreenPoint) throws -> LatLng {
    try mapNativeFailure {
      try handle.withLive { projection in
        try LatLng(native: NativeLatLng(NativeProjection.latLngForPixel(
          projection,
          point: point.nativeInput.native
        )))
      }
    }
  }

  /// Converts a screen point to an unwrapped geographic coordinate.
  ///
  /// The longitude preserves the visible world copy and may fall outside
  /// -180 to 180.
  public func latLngUnwrapped(for point: ScreenPoint) throws -> LatLng {
    try mapNativeFailure {
      try handle.withLive { projection in
        try LatLng(native: NativeLatLng(NativeProjection
            .latLngForPixelUnwrapped(
              projection,
              point: point.nativeInput.native
            )))
      }
    }
  }
}

public extension Maplibre {
  static func projectedMeters(for coordinate: LatLng) throws
    -> ProjectedMeters
  {
    try mapNativeFailure {
      let meters = try NativeProjection
        .projectedMetersForLatLng(coordinate.nativeInput)
      return ProjectedMeters(northing: meters.northing, easting: meters.easting)
    }
  }

  static func latLng(forProjectedMeters meters: ProjectedMeters) throws
    -> LatLng
  {
    try mapNativeFailure {
      try LatLng(native: NativeProjection.latLngForProjectedMeters(
        NativeProjectedMeters(
          northing: meters.northing,
          easting: meters.easting
        )
      ))
    }
  }
}
