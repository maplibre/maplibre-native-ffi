internal import CMaplibreNativeC
import Foundation

private final class WeakMapHandle {
  weak var value: MapHandle?

  init(_ value: MapHandle) {
    self.value = value
  }
}

public enum MapMode: UInt32, Sendable, Hashable {
  case continuous = 0
  case `static` = 1
  case tile = 2
}

public struct MapOptions: Equatable, Sendable {
  /// Initial logical width in UI pixels, replaced by the extent of the first
  /// attached render session.
  public var width: UInt32
  /// Initial logical height in UI pixels, replaced by the extent of the first
  /// attached render session.
  public var height: UInt32
  /// UI-to-device pixel scale, fixed for the lifetime of the map.
  ///
  /// This selects sprites, glyphs, and raster tiles for every frame. Render
  /// targets carry their own scale factor for geometry, so attaching or
  /// resizing a session with a different one logs a warning and renders styled
  /// imagery chosen for this density.
  public var scaleFactor: Double
  public var mode: MapMode
  /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
  /// encodings, fixed for the lifetime of the map.
  ///
  /// Enable this on maps that read vector sources created with
  /// ``VectorTileEncoding/mlt`` from a tile set that uses FastPFOR. A map
  /// created with this `false` decodes every other MLT encoding and logs a tile
  /// parse warning for the FastPFOR ones.
  public var fastPFOREnabled: Bool

  public init(
    width: UInt32,
    height: UInt32,
    scaleFactor: Double = 1.0,
    mode: MapMode = .continuous,
    fastPFOREnabled: Bool = false
  ) {
    self.width = width
    self.height = height
    self.scaleFactor = scaleFactor
    self.mode = mode
    self.fastPFOREnabled = fastPFOREnabled
  }

  var nativeInput: NativeMapOptionsInput {
    NativeMapOptionsInput(
      width: width,
      height: height,
      scaleFactor: scaleFactor,
      mapMode: mode.rawValue,
      fastPFOREnabled: fastPFOREnabled
    )
  }
}

public final class MapHandle {
  private static let registryLock = NSLock()
  private nonisolated(unsafe) static var registry: [UInt64: WeakMapHandle] = [:]

  private let runtime: RuntimeHandle
  private let handle: NativeHandleBox<NativeMapHandle>
  private let mapId: MapId
  private var styleURLReplacementPending = false
  private var customGeometrySourceCallbacks: [
    String: NativeCustomGeometrySourceCallbacks
  ] =
    [:]

  public init(runtime: RuntimeHandle, options: MapOptions) throws {
    let native = try mapNativeFailure {
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
    Self.register(self)
  }

  deinit {
    Self.unregister(mapId)
    if !handle.isClosed {
      abandonNativeOwnedCustomGeometrySourceCallbacks()
    }
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

  /// Produces a `Sendable` reference to this map for attaching a render
  /// session.
  ///
  /// A render session is owned by the thread that attaches it, which need not
  /// be
  /// the map's owner thread. ``MapHandle`` is not `Sendable`, so this is how
  /// the
  /// thread driving a render loop names the map it renders while the map itself
  /// stays on the runtime owner thread.
  public func attachRef() throws -> MapAttachRef {
    // Resolve once so a closed map fails here rather than at the first attach.
    _ = try requireLiveHandle()
    return MapAttachRef(handle: handle)
  }

  private static func register(_ map: MapHandle) {
    registryLock.withLock {
      registry[map.mapId.value] = WeakMapHandle(map)
    }
  }

  private static func unregister(_ mapId: MapId) {
    registryLock.withLock {
      _ = registry.removeValue(forKey: mapId.value)
    }
  }

  static func handleRuntimeEvent(_ event: RuntimeEvent) {
    guard event.type == .mapStyleLoaded,
          case let .map(source) = event.source
    else { return }

    let map = registryLock
      .withLock { registry[source.value]?.value }
    map?.releaseCallbacksForLoadedStyleURLIfNeeded()
  }

  private func retainCallbacksUntilPendingStyleURLLoads() {
    styleURLReplacementPending = true
  }

  func releaseCallbacksForLoadedStyleURLIfNeeded() {
    guard styleURLReplacementPending else { return }

    for sourceId in Array(customGeometrySourceCallbacks.keys) {
      guard (try? styleSourceType(sourceId)) != .customVector else { continue }
      customGeometrySourceCallbacks.removeValue(forKey: sourceId)
    }

    if customGeometrySourceCallbacks.isEmpty {
      styleURLReplacementPending = false
    }
  }

  func storeCustomGeometrySourceCallbacks(
    _ callbacks: NativeCustomGeometrySourceCallbacks,
    sourceId: String
  ) {
    customGeometrySourceCallbacks[sourceId] = callbacks
  }

  func removeCustomGeometrySourceCallbacks(sourceId: String) {
    _ = customGeometrySourceCallbacks.removeValue(forKey: sourceId)
  }

  func retainsCustomGeometrySourceCallbacks(sourceId: String) -> Bool {
    customGeometrySourceCallbacks[sourceId] != nil
  }

  private func resetCallbackRetentionState() {
    styleURLReplacementPending = false
    customGeometrySourceCallbacks.removeAll()
  }

  private func abandonNativeOwnedCustomGeometrySourceCallbacks() {
    for callbacks in customGeometrySourceCallbacks.values {
      callbacks.abandonRetainedBox()
    }
  }

  public func close() throws {
    try handle.closeOnce { handle in
      try checkStatus(mln_map_destroy(handle.raw))
    }
    Self.unregister(mapId)
    resetCallbackRetentionState()
  }

  /// Loads a style URL through MapLibre Native style APIs.
  ///
  /// Loading is asynchronous, so a style that is missing, unreachable, or
  /// malformed still returns normally here and reports through a
  /// map-loading-failed runtime event. Watch the runtime event queue to observe
  /// style load failures.
  ///
  /// A well-formed style that MapLibre rejects semantically, such as an
  /// unknown `version` or a layer naming a missing source, produces neither
  /// an error nor an event: MapLibre logs it and renders what it can.
  public func setStyleURL(_ url: String) throws {
    try mapNativeFailure {
      try NativeString.withCString(url) { url in
        try checkStatus(mln_map_set_style_url(handle.requireLive().raw, url))
      }
      retainCallbacksUntilPendingStyleURLLoads()
    }
  }

  /// Loads inline style JSON through MapLibre Native style APIs.
  ///
  /// Malformed JSON is reported twice: this call throws the parse error
  /// synchronously, and the same message also arrives as a map-loading-failed
  /// runtime event. Handle both so a queued failure event is not a surprise.
  ///
  /// A well-formed style that MapLibre rejects semantically, such as an
  /// unknown `version` or a layer naming a missing source, produces neither
  /// an error nor an event: MapLibre logs it and renders what it can.
  public func setStyleJSON(_ json: String) throws {
    try mapNativeFailure {
      try NativeString.withCString(json) { json in
        try checkStatus(mln_map_set_style_json(handle.requireLive().raw, json))
      }
      resetCallbackRetentionState()
    }
  }

  public func requestRepaint() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_request_repaint(handle.requireLive().raw))
    }
  }

  public func requestStillImage() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_request_still_image(handle.requireLive().raw))
    }
  }

  public func camera() throws -> CameraOptions {
    try mapNativeFailure {
      try CameraOptions(native: NativeCameraOptionsInput(NativeMap
          .camera(handle.requireLive())))
    }
  }

  public func jump(to camera: CameraOptions) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try checkStatus(mln_map_jump_to(handle.requireLive().raw, nativeCamera))
      }
    }
  }

  public func ease(
    to camera: CameraOptions,
    animation: AnimationOptions? = nil
  ) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try (animation?.nativeInput ?? NativeAnimationOptionsInput())
          .withOptionalNativeOptions { nativeAnimation in
            try checkStatus(mln_map_ease_to(
              handle.requireLive().raw,
              nativeCamera,
              nativeAnimation
            ))
          }
      }
    }
  }

  public func moveBy(deltaX: Double, deltaY: Double) throws {
    try mapNativeFailure {
      try checkStatus(mln_map_move_by(handle.requireLive().raw, deltaX, deltaY))
    }
  }

  public func moveBy(
    deltaX: Double,
    deltaY: Double,
    animation: AnimationOptions
  ) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { nativeAnimation in
        try checkStatus(mln_map_move_by_animated(
          handle.requireLive().raw,
          deltaX,
          deltaY,
          nativeAnimation
        ))
      }
    }
  }

  public func scaleBy(_ scale: Double, anchor: ScreenPoint? = nil) throws {
    try mapNativeFailure {
      if var nativeAnchor = anchor?.nativeInput.native {
        try withUnsafePointer(to: &nativeAnchor) { anchor in
          try checkStatus(mln_map_scale_by(
            handle.requireLive().raw,
            scale,
            anchor
          ))
        }
      } else {
        try checkStatus(mln_map_scale_by(handle.requireLive().raw, scale, nil))
      }
    }
  }

  public func scaleBy(
    _ scale: Double,
    anchor: ScreenPoint? = nil,
    animation: AnimationOptions
  ) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { nativeAnimation in
        if var nativeAnchor = anchor?.nativeInput.native {
          try withUnsafePointer(to: &nativeAnchor) { anchor in
            try checkStatus(mln_map_scale_by_animated(
              handle.requireLive().raw,
              scale,
              anchor,
              nativeAnimation
            ))
          }
        } else {
          try checkStatus(mln_map_scale_by_animated(
            handle.requireLive().raw,
            scale,
            nil,
            nativeAnimation
          ))
        }
      }
    }
  }

  public func cancelTransitions() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_cancel_transitions(handle.requireLive().raw))
    }
  }
}
