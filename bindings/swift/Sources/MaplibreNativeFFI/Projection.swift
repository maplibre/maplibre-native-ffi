
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

public final class MapProjectionHandle: @unchecked Sendable {
  private let handle: NativeHandleBox<NativeMapProjectionHandle>
  private let map: MapHandle
  private let runtime: RuntimeHandle

  public init(map: MapHandle) async throws {
    self.map = map
    let operationRuntime = map.runtimeForOperations
    runtime = operationRuntime
    let operation = try mapNativeFailure {
      try NativeProjection.createStart(map.requireLiveHandle())
    }
    defer { mln_operation_release(operation.raw) }
    try await mapNativeFailure {
      try await operationRuntime.waitForOperation(operation)
    }
    let projection = try mapNativeFailure {
      try NativeProjection.createTakeResult(operation)
    }
    handle = try NativeHandleBox(
      typeName: "MapProjectionHandle",
      handle: projection
    )
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func close() async throws {
    let operation = try mapNativeFailure {
      try NativeProjection.closeStart(handle.requireLive())
    }
    defer { mln_operation_release(operation.raw) }
    try await mapNativeFailure { try await runtime.waitForOperation(operation) }
    try handle.closeOnce { _ in }
  }

  func closeBlockingForTests() throws {
    let operation = try mapNativeFailure {
      try NativeProjection.closeStart(handle.requireLive())
    }
    defer { mln_operation_release(operation.raw) }
    try mapNativeFailure {
      try NativeOperation.waitForSuccessBlocking(operation)
    }
    try handle.closeOnce { _ in }
  }

  private func orderedResult<Result>(
    start: (NativeMapProjectionHandle) throws -> NativeOperationHandle,
    take: (NativeOperationHandle) throws -> Result
  ) async throws -> Result {
    let operation = try mapNativeFailure { try start(handle.requireLive()) }
    defer { mln_operation_release(operation.raw) }
    try await mapNativeFailure { try await runtime.waitForOperation(operation) }
    return try mapNativeFailure { try take(operation) }
  }

  private func submitCommand(
    _ submit: (
      NativeMapProjectionHandle, UnsafeMutablePointer<UInt64>
    ) throws -> Void
  ) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { commandId in
      try submit(handle.requireLive(), commandId)
    }.value
  }

  public func camera() async throws -> CameraOptions {
    let native = try await orderedResult(
      start: NativeProjection.cameraStart,
      take: NativeProjection.cameraTakeResult
    )
    return CameraOptions(native: NativeCameraOptionsInput(native))
  }

  @discardableResult
  public func setCamera(_ camera: CameraOptions) throws -> UInt64 {
    try camera.nativeInput.withNativeOptions { nativeCamera in
      try submitCommand { projection, commandId in
        try checkStatus(mln_map_projection_set_camera(
          projection.raw, nativeCamera, commandId
        ))
      }
    }
  }

  @discardableResult
  public func setVisibleCoordinates(
    _ coordinates: [LatLng],
    padding: EdgeInsets = EdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
  ) throws -> UInt64 {
    guard !coordinates.isEmpty else {
      throw MaplibreError.invalidArgument("visible coordinates cannot be empty")
    }
    let nativeCoordinates = coordinates.map(\.nativeInput.native)
    return try nativeCoordinates.withUnsafeBufferPointer { buffer in
      try submitCommand { projection, commandId in
        try checkStatus(mln_map_projection_set_visible_coordinates(
          projection.raw,
          buffer.baseAddress,
          buffer.count,
          padding.nativeInput.native,
          commandId
        ))
      }
    }
  }

  @discardableResult
  public func setVisibleGeometry(
    _ geometry: Data,
    padding: EdgeInsets = EdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
  ) throws -> UInt64 {
    let arena = NativeInputArena()
    defer { withExtendedLifetime(arena) {} }
    return try submitCommand { projection, commandId in
      try checkStatus(mln_map_projection_set_visible_geometry(
        projection.raw,
        arena.view(geometry),
        padding.nativeInput.native,
        commandId
      ))
    }
  }

  public func pixel(for coordinate: LatLng) async throws -> ScreenPoint {
    let operation = try mapNativeFailure {
      try NativeProjection.pixelForLatLngStart(
        handle.requireLive(), coordinate: coordinate.nativeInput.native
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await mapNativeFailure { try await runtime.waitForOperation(operation) }
    let point = try mapNativeFailure {
      try NativeProjection.pixelForLatLngTakeResult(operation)
    }
    return ScreenPoint(native: NativeScreenPoint(point))
  }

  public func latLng(for point: ScreenPoint) async throws -> LatLng {
    let operation = try mapNativeFailure {
      try NativeProjection.latLngForPixelStart(
        handle.requireLive(), point: point.nativeInput.native
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await mapNativeFailure { try await runtime.waitForOperation(operation) }
    let coordinate = try mapNativeFailure {
      try NativeProjection.latLngForPixelTakeResult(operation)
    }
    return LatLng(native: NativeLatLng(coordinate))
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
