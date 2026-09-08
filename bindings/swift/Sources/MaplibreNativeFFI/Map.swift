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
/// A command completion reports the generation its commit published, so a
/// snapshot whose ``generation`` is at or past that value observes the commit.
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
  /// True while a gesture is in progress. The map defers tile prefetching and
  /// similar work until the gesture ends.
  public let gestureInProgress: Bool
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
    gestureInProgress = native.gesture_in_progress
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

/// One map owned by a runtime.
///
/// A host may share one map across threads and across concurrent tasks: every
/// stored property is either immutable or the lock-guarded handle box, and the
/// C API leases the map for each entry point. Commands commit in the order that
/// they were accepted in.
public final class MapHandle: @unchecked Sendable {
  private let handle: NativeHandleBox<NativeMapHandle>
  /// Keeps the runtime alive for at least as long as the map it owns.
  private let runtime: RuntimeHandle
  private let mapId: MapId

  public init(runtime: RuntimeHandle, options: MapOptions) async throws {
    let native = try await awaitNative {
      try options.nativeInput.withNativeOptions { nativeOptions in
        try NativeMap.create(
          runtime: runtime.requireLiveHandle(),
          options: nativeOptions
        )
      }
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
    try handle.requireLive()
  }

  func startCommand(
    _ submit: (mln_map, UnsafePointer<mln_completion>) throws -> mln_status
  ) throws -> NativeFuture<CommandCompletion> {
    let map = try requireLiveHandle()
    return try NativeCompletion.startCommand { try submit(map.raw, $0) }
  }

  func submitCommand(
    _ submit: (mln_map, UnsafePointer<mln_completion>) throws -> mln_status
  ) async throws -> CommandCompletion {
    try await awaitNative { try startCommand(submit) }
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
  public func setEventMask(
    _ mask: RuntimeEventMask
  ) async throws -> CommandCompletion {
    try await submitCommand { mln_map_set_event_mask($0, mask.rawValue, $1) }
  }

  /// Releases this map's public native handle.
  public func close() async throws {
    guard let teardown = try startClose() else { return }
    try await mapNativeFailure { try await teardown.value() }
  }

  private func startClose() throws -> NativeFuture<Void>? {
    var teardown: NativeFuture<Void>?
    try mapNativeFailure {
      try handle.closeOnce { teardown = try NativeMap.release($0) }
    }
    return teardown
  }

  func closeBlockingForTests() throws {
    guard let teardown = try startClose() else { return }
    try mapNativeFailure { try teardown.valueBlocking() }
  }

  /// Loads a style URL.
  ///
  /// Loading is asynchronous: a missing, unreachable, or malformed style still
  /// returns normally here and reports through a map-loading-failed runtime
  /// event. A style MapLibre rejects semantically, such as one with an unknown
  /// `version`, produces neither an error nor an event.
  @discardableResult
  public func setStyleURL(_ url: String) async throws -> CommandCompletion {
    try await awaitNative {
      try NativeString.withCString(url) { url in
        try startCommand { mln_map_set_style_url($0, url, $1) }
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
  public func setStyleJSON(_ json: Data) async throws -> CommandCompletion {
    try await awaitNative {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try startCommand {
        mln_map_set_style_json($0, arena.view(json), $1)
      }
    }
  }

  /// Submits a per-feature-state update. The state must contain one UTF-8
  /// JSON object.
  @discardableResult
  public func setFeatureState(
    selector: FeatureStateSelector,
    state: Data
  ) async throws -> CommandCompletion {
    try await awaitNative {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try selector.nativeSelector.withNativeSelector { selector in
        try startCommand {
          mln_map_set_feature_state($0, selector, arena.view(state), $1)
        }
      }
    }
  }

  /// Reads per-feature state from the map store, observing every earlier
  /// accepted command. Missing state is an empty JSON object.
  public func featureState(
    selector: FeatureStateSelector
  ) async throws -> Data {
    try await awaitNative {
      let map = try requireLiveHandle()
      return try selector.nativeSelector.withNativeSelector { selector in
        try NativeCompletion.start(
          { mln_map_get_feature_state(map.raw, selector, $0) },
          convert: NativeCompletion.data
        )
      }
    }
  }

  /// Submits a per-feature-state removal scoped by the selector: one key, one
  /// feature, or every feature in the source.
  @discardableResult
  public func removeFeatureState(
    selector: FeatureStateSelector
  ) async throws -> CommandCompletion {
    try await awaitNative {
      try selector.nativeSelector.withNativeSelector { selector in
        try startCommand { mln_map_remove_feature_state($0, selector, $1) }
      }
    }
  }

  /// Copies the style document this map's style was last parsed from, byte for
  /// byte, rather than a serialization of the live style. Runtime mutations do
  /// not change it, and a failed parse leaves the previous document in place.
  /// The result is empty when no document has been parsed.
  public func loadedStyleJSON() async throws -> Data {
    try await awaitNative {
      let map = try requireLiveHandle()
      return try NativeCompletion.start(
        { mln_map_loaded_style_json(map.raw, $0) },
        convert: NativeCompletion.data
      )
    }
  }

  public func styleURL() async throws -> String {
    try await awaitNative {
      let map = try requireLiveHandle()
      return try NativeCompletion.start(
        { mln_map_style_url(map.raw, $0) },
        convert: NativeCompletion.string
      )
    }
  }

  /// Requests a repaint. Only a continuous map accepts one.
  @discardableResult
  public func requestRepaint() async throws -> CommandCompletion {
    try await submitCommand(mln_map_request_repaint)
  }

  /// Requests and awaits one noncoalescing still-image operation.
  ///
  /// Only a static or tile map accepts one. A request made while another is
  /// pending fails through the completion rather than at submission.
  public func requestStillImage() async throws {
    try await awaitNative {
      try NativeMap.requestStillImage(handle.requireLive())
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
    try await awaitNative {
      try NativeMap.cameraQuery(handle.requireLive())
    }
  }

  /// Submits one atomic camera command.
  @discardableResult
  public func updateCamera(
    _ update: CameraUpdate
  ) async throws -> CommandCompletion {
    try await awaitNative {
      try update.withNativeUpdate { update in
        try startCommand { mln_map_update_camera($0, update, $1) }
      }
    }
  }

  /// Submits one relative camera operation.
  @discardableResult
  public func applyCameraDelta(
    _ delta: CameraDelta
  ) async throws -> CommandCompletion {
    try await awaitNative {
      try delta.withNativeDelta { delta in
        try startCommand { mln_map_apply_camera_delta($0, delta, $1) }
      }
    }
  }

  /// Resizes the logical map viewport.
  ///
  /// `extent` must carry the scale factor the map was created with, which is
  /// fixed for the map's life; only the width and height may change. While a
  /// render session is attached, resize through
  /// ``RenderSessionHandle/resize(_:)`` instead: it submits this command
  /// itself, and a direct map resize to a different extent leaves the session
  /// waiting for an update the map never publishes.
  @discardableResult
  public func resize(
    to extent: MapLogicalExtent
  ) async throws -> CommandCompletion {
    try await submitCommand { mln_map_resize($0, extent.native, $1) }
  }

  /// Cancels the camera transitions running when this command commits.
  ///
  /// A cancelled transition that carried an
  /// ``AnimationOptions/transitionId`` still reports its
  /// ``RuntimeEventType/mapCameraTransitionFinished`` event, the way a
  /// completed one does, and the camera keeps the value it reached. Cancelling
  /// with no transition running commits and changes nothing.
  @discardableResult
  public func cancelTransitions() async throws -> CommandCompletion {
    try await submitCommand(mln_map_cancel_transitions)
  }
}
