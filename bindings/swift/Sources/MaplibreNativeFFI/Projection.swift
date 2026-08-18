
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

/// A standalone projection copied from a map transform at creation.
///
/// Every call after creation, including close, is synchronous, runs on the
/// calling thread, and is serialized by a native lock, so a projection is
/// usable from any thread. A projection never observes map changes made after
/// its creation and remains usable after its source map and runtime close.
public final class MapProjectionHandle: @unchecked Sendable {
  private let handle: NativeHandleBox<NativeMapProjectionHandle>

  public init(map: MapHandle) async throws {
    let nativeMap = try map.requireLiveHandle()
    let future = try mapNativeFailure {
      try NativeCompletion.start(
        { mln_map_projection_create(nativeMap.raw, $0) }
      ) { result in
        try NativeMapProjectionHandle(
          raw: NativeCompletion.value(result, as: mln_map_projection.self)
        )
      }
    }
    let projection = try await mapNativeFailure { try await future.value() }
    handle = try NativeHandleBox(
      typeName: "MapProjectionHandle",
      handle: projection
    )
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  /// Closes this projection. The native call waits for calls already running
  /// on other threads before it retires the handle.
  public func close() throws {
    try mapNativeFailure {
      try handle.closeOnce { projection in
        try checkStatus(mln_map_projection_close(projection.raw))
      }
    }
  }

  /// Copies the projection camera, observing every earlier setter.
  public func camera() throws -> CameraOptions {
    try mapNativeFailure {
      let native = try handle.withLive { projection in
        try NativeMemory
          .withTemporary(mln_camera_options_default()) { camera in
            try checkStatus(mln_map_projection_get_camera(
              projection.raw, camera
            ))
          }.value
      }
      return CameraOptions(native: NativeCameraOptionsInput(native))
    }
  }

  /// Applies a camera update before returning. The source map's camera is
  /// unaffected.
  public func setCamera(_ camera: CameraOptions) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try handle.withLive { projection in
          try checkStatus(mln_map_projection_set_camera(
            projection.raw, nativeCamera
          ))
        }
      }
    }
  }

  public func setVisibleCoordinates(
    _ coordinates: [LatLng],
    padding: EdgeInsets = EdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
  ) throws {
    guard !coordinates.isEmpty else {
      throw MaplibreError.invalidArgument("visible coordinates cannot be empty")
    }
    try mapNativeFailure {
      let nativeCoordinates = coordinates.map(\.nativeInput.native)
      try nativeCoordinates.withUnsafeBufferPointer { buffer in
        try handle.withLive { projection in
          try checkStatus(mln_map_projection_set_visible_coordinates(
            projection.raw,
            buffer.baseAddress,
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
      let point = try handle.withLive { projection in
        try NativeMemory
          .withTemporary(mln_screen_point()) { point in
            try checkStatus(mln_map_projection_pixel_for_lat_lng(
              projection.raw, coordinate.nativeInput.native, point
            ))
          }.value
      }
      return ScreenPoint(native: NativeScreenPoint(point))
    }
  }

  public func latLng(for point: ScreenPoint) throws -> LatLng {
    try mapNativeFailure {
      let coordinate = try handle.withLive { projection in
        try NativeMemory
          .withTemporary(mln_lat_lng()) { coordinate in
            try checkStatus(mln_map_projection_lat_lng_for_pixel(
              projection.raw, point.nativeInput.native, coordinate
            ))
          }.value
      }
      return LatLng(native: NativeLatLng(coordinate))
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
