internal import CMaplibreNativeC
import Foundation

public enum MapMode: UInt32, Sendable, Hashable {
  case continuous = 0
  case `static` = 1
  case tile = 2
}

public struct MapOptions: Equatable, Sendable {
  /// Initial logical width in UI pixels.
  public var width: UInt32
  /// Initial logical height in UI pixels.
  public var height: UInt32
  /// UI-to-device pixel scale, fixed for the lifetime of the map. It selects
  /// sprites, glyphs, and raster tiles.
  public var scaleFactor: Double
  public var mode: MapMode
  /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
  /// encodings, fixed for the lifetime of the map. A map created with this
  /// `false` logs a tile parse warning for such tiles.
  public var fastPFOREnabled: Bool
  /// Map-originated event types this map queues, every type the library
  /// reports unless the host narrows it. The mask applies during construction.
  public var eventMask: RuntimeEventMask

  public init(
    width: UInt32,
    height: UInt32,
    scaleFactor: Double = 1.0,
    mode: MapMode = .continuous,
    fastPFOREnabled: Bool = false,
    eventMask: RuntimeEventMask = .mapOptionsDefault
  ) {
    self.width = width
    self.height = height
    self.scaleFactor = scaleFactor
    self.mode = mode
    self.fastPFOREnabled = fastPFOREnabled
    self.eventMask = eventMask
  }

  var nativeInput: NativeMapOptionsInput {
    NativeMapOptionsInput(
      width: width,
      height: height,
      scaleFactor: scaleFactor,
      mapMode: mode.rawValue,
      fastPFOREnabled: fastPFOREnabled,
      eventMask: eventMask.rawValue
    )
  }
}

public struct MapLogicalExtent: Equatable, Sendable {
  public var width: UInt32
  public var height: UInt32
  public var scaleFactor: Double

  public init(width: UInt32, height: UInt32, scaleFactor: Double) {
    self.width = width
    self.height = height
    self.scaleFactor = scaleFactor
  }

  var native: mln_logical_extent {
    mln_logical_extent(
      width: width,
      height: height,
      scale_factor: scaleFactor
    )
  }

  init(native: mln_logical_extent) {
    width = native.width
    height = native.height
    scaleFactor = native.scale_factor
  }
}

/// The latest immutable map state, published by every committed map command.
///
/// A command's finished event reports the generation its commit published, so
/// a snapshot whose ``generation`` is at or past that value observes the
/// commit.
public struct MapSnapshot: Equatable, Sendable {
  public let generation: UInt64
  /// Debug overlays currently drawn over the map.
  public let debugOptions: MapDebugOptions
  public let camera: CameraOptions
  public let logicalExtent: MapLogicalExtent
  public let projectionMode: ProjectionMode
  public let viewportOptions: MapViewportOptions
  /// True once every requested style and tile resource finished loading.
  public let isFullyLoaded: Bool
  public let renderingStatsViewEnabled: Bool
  public let needsRepaint: Bool
  public let eventMask: RuntimeEventMask
  public let latestRenderUpdateGeneration: UInt64
  public let tileOptions: MapTileOptions
  public let bounds: BoundOptions
  public let freeCameraOptions: FreeCameraOptions

  init(native: mln_map_snapshot) {
    generation = native.generation
    debugOptions = MapDebugOptions(rawValue: native.debug_options)
    camera = CameraOptions(native: NativeCameraOptionsInput(native.camera))
    logicalExtent = MapLogicalExtent(native: native.logical_extent)
    projectionMode = ProjectionMode(
      native: NativeProjectionModeInput(native.projection_mode)
    )
    viewportOptions = MapViewportOptions(
      native: NativeMapViewportOptionsInput(native.viewport)
    )
    isFullyLoaded = native.fully_loaded
    renderingStatsViewEnabled = native.rendering_stats_view_enabled
    needsRepaint = native.repaint_demand
    eventMask = RuntimeEventMask(rawValue: native.event_mask)
    latestRenderUpdateGeneration = native.latest_render_update_generation
    tileOptions = MapTileOptions(native: NativeMapTileOptionsInput(native.tile))
    bounds = BoundOptions(native: NativeBoundOptionsInput(native.bounds))
    freeCameraOptions = FreeCameraOptions(
      native: NativeFreeCameraOptionsInput(native.free_camera)
    )
  }
}

public struct CameraSnapshot: Equatable, Sendable {
  public let generation: UInt64
  public let camera: CameraOptions
}

public final class MapHandle: @unchecked Sendable {
  private let handle: NativeHandleBox<NativeMapHandle>
  private let runtime: RuntimeHandle
  private let lifecycleLock = NSLock()
  private var isClosing = false
  private let mapId: MapId

  public init(runtime: RuntimeHandle, options: MapOptions) async throws {
    let operation = try mapNativeFailure {
      try options.nativeInput.withNativeOptions { nativeOptions in
        try NativeMap.createStart(
          runtime: runtime.requireLiveHandle(),
          options: nativeOptions
        )
      }
    }
    defer { mln_operation_release(operation.raw) }
    try await mapNativeFailure {
      try await runtime.waitForOperation(operation)
    }
    let native = try mapNativeFailure {
      try NativeMap.createTakeResult(operation)
    }
    self.runtime = runtime
    mapId = MapId(value: native.raw)
    handle = try NativeHandleBox(typeName: "MapHandle", handle: native)
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func isSource(of event: RuntimeEvent) -> Bool {
    guard case let .map(source) = event.source else {
      return false
    }
    return source == mapId
  }

  func requireLiveHandle() throws -> NativeMapHandle {
    try lifecycleLock.withLock {
      guard !isClosing else {
        throw NativeStatusFailure.swiftNativeError("MapHandle is closing")
      }
      return try handle.requireLive()
    }
  }

  var runtimeForOperations: RuntimeHandle {
    runtime
  }

  func submitCommand(
    _ submit: (NativeMapHandle, UnsafeMutablePointer<UInt64>) throws -> Void
  ) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { commandId in
      try submit(requireLiveHandle(), commandId)
    }.value
  }

  /// Selects which map-originated event types this map queues.
  ///
  /// The call reads the bits in ``RuntimeEventMask/allMapEvents`` and ignores
  /// the rest, so ``RuntimeEventMask/all`` selects every one of them. Narrowing
  /// gates later events and keeps queued ones, so a host drains what it already
  /// caused.
  ///
  /// Select every type the host reads: map-render-update-available is the map's
  /// only invalidation report, the two still-image types are the only reports
  /// that a still-image request finished, and map-loading-failed and
  /// map-render-error carry native failure text.
  @discardableResult
  public func setEventMask(_ mask: RuntimeEventMask) throws -> UInt64 {
    try mapNativeFailure {
      try NativeMap.setEventMask(handle.requireLive(), mask: mask.rawValue)
    }
  }

  /// The mask in the latest immutable map snapshot.
  public var eventMask: RuntimeEventMask {
    get throws {
      try snapshot().eventMask
    }
  }

  /// Closes this map after its worker has retired the native handle.
  public func close() async throws {
    let operation = try mapNativeFailure {
      try lifecycleLock.withLock {
        guard !isClosing else {
          throw NativeStatusFailure.swiftNativeError("MapHandle is closing")
        }
        guard !handle.isClosed else { return NativeOperationHandle(raw: 0) }
        let operation = try NativeMap.closeStart(handle.requireLive())
        isClosing = true
        return operation
      }
    }
    guard !operation.isNull else { return }
    defer { mln_operation_release(operation.raw) }
    do {
      try await mapNativeFailure {
        try await runtime.waitForOperation(operation)
      }
      try handle.closeOnce { _ in }
      lifecycleLock.withLock { isClosing = false }
    } catch {
      lifecycleLock.withLock { isClosing = false }
      throw error
    }
  }

  func closeBlockingForTests() throws {
    let operation = try mapNativeFailure {
      try lifecycleLock.withLock {
        guard !isClosing else {
          throw NativeStatusFailure.swiftNativeError("MapHandle is closing")
        }
        guard !handle.isClosed else { return NativeOperationHandle(raw: 0) }
        let operation = try NativeMap.closeStart(handle.requireLive())
        isClosing = true
        return operation
      }
    }
    guard !operation.isNull else { return }
    defer { mln_operation_release(operation.raw) }
    do {
      try mapNativeFailure {
        try NativeOperation.waitForSuccessBlocking(operation)
      }
      try handle.closeOnce { _ in }
      lifecycleLock.withLock { isClosing = false }
    } catch {
      lifecycleLock.withLock { isClosing = false }
      throw error
    }
  }

  /// Loads a style URL.
  ///
  /// Loading is asynchronous: a missing, unreachable, or malformed style still
  /// returns normally here and reports through a map-loading-failed runtime
  /// event. A style MapLibre rejects semantically, such as one with an unknown
  /// `version`, produces neither an error nor an event.
  @discardableResult
  public func setStyleURL(_ url: String) throws -> UInt64 {
    try mapNativeFailure {
      try NativeString.withCString(url) { url in
        try NativeMemory.withTemporary(UInt64(0)) { commandId in
          try checkStatus(mln_map_set_style_url(
            handle.requireLive().raw,
            url,
            commandId
          ))
        }.value
      }
    }
  }

  /// Loads inline style JSON.
  ///
  /// Malformed JSON is reported twice: this call throws the parse error, and
  /// the same message arrives as a map-loading-failed runtime event. A style
  /// MapLibre rejects semantically, such as one with an unknown `version`,
  /// produces neither an error nor an event.
  @discardableResult
  public func setStyleJSON(_ json: Data) throws -> UInt64 {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_style_json(
          handle.requireLive().raw,
          arena.view(json),
          commandId
        ))
      }.value
    }
  }

  /// Copies the style document this map's style was last parsed from, byte for
  /// byte, rather than a serialization of the live style. Runtime mutations do
  /// not change it, and a failed parse leaves the previous document in place.
  /// The result is empty when no document has been parsed.
  public func loadedStyleJSON() async throws -> Data {
    let operation = try mapNativeFailure {
      try NativeStyle.mapReadStart(
        handle.requireLive(),
        start: mln_map_loaded_style_json_start
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtime.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.mapDataTakeResult(
        operation,
        take: mln_map_loaded_style_json_take_result
      )
    }
  }

  public func styleURL() async throws -> String {
    let operation = try mapNativeFailure {
      try NativeStyle.mapReadStart(
        handle.requireLive(),
        start: mln_map_style_url_start
      )
    }
    defer { mln_operation_release(operation.raw) }
    try await runtime.waitForOperation(operation)
    return try mapNativeFailure {
      try NativeStyle.mapTextTakeResult(
        operation,
        take: mln_map_style_url_take_result
      )
    }
  }

  /// Requests a repaint and returns its runtime-wide command ID.
  @discardableResult
  public func requestRepaint() throws -> UInt64 {
    try mapNativeFailure {
      try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_request_repaint(
          handle.requireLive().raw,
          commandId
        ))
      }.value
    }
  }

  /// Requests and awaits one noncoalescing still-image operation.
  public func requestStillImage() async throws {
    let operation = try mapNativeFailure {
      try NativeMap.requestStillImageStart(handle.requireLive())
    }
    defer { mln_operation_release(operation.raw) }
    try await mapNativeFailure {
      try await runtime.waitForOperation(operation)
    }
  }

  /// Copies the latest immutable map state.
  public func snapshot() throws -> MapSnapshot {
    try mapNativeFailure {
      try MapSnapshot(native: NativeMap.snapshot(handle.requireLive()))
    }
  }

  /// Copies the camera from the latest immutable map snapshot.
  public func cameraSnapshot() throws -> CameraSnapshot {
    try mapNativeFailure {
      let snapshot = try NativeMap.cameraSnapshot(handle.requireLive())
      return CameraSnapshot(
        generation: snapshot.generation,
        camera: CameraOptions(
          native: NativeCameraOptionsInput(snapshot.camera)
        )
      )
    }
  }

  /// Reads the camera after every command accepted before this call.
  public func queryCamera() async throws -> CameraSnapshot {
    let operation = try mapNativeFailure {
      try NativeMap.cameraQueryStart(handle.requireLive())
    }
    defer { mln_operation_release(operation.raw) }
    try await mapNativeFailure {
      try await runtime.waitForOperation(operation)
    }
    return try mapNativeFailure {
      let result = try NativeMap.cameraQueryTakeResult(operation)
      return CameraSnapshot(
        generation: result.generation,
        camera: CameraOptions(native: NativeCameraOptionsInput(result.camera))
      )
    }
  }

  /// Submits one atomic camera command and returns its runtime-wide command ID.
  @discardableResult
  public func updateCamera(_ update: CameraUpdate) throws -> UInt64 {
    try mapNativeFailure {
      try update.withNativeUpdate { update in
        try NativeMemory.withTemporary(UInt64(0)) { commandId in
          try checkStatus(mln_map_update_camera(
            handle.requireLive().raw,
            update,
            commandId
          ))
        }.value
      }
    }
  }

  /// Resizes the logical map viewport and returns its command ID.
  @discardableResult
  public func resize(to extent: MapLogicalExtent) throws -> UInt64 {
    try mapNativeFailure {
      try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_resize(
          handle.requireLive().raw,
          extent.native,
          commandId
        ))
      }.value
    }
  }
}
