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
  /// UI-to-device pixel scale, fixed for the lifetime of the map. It selects
  /// sprites, glyphs, and raster tiles; a render session attached or resized
  /// with a different scale factor logs a warning.
  public var scaleFactor: Double
  public var mode: MapMode
  /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
  /// encodings, fixed for the lifetime of the map. A map created with this
  /// `false` logs a tile parse warning for such tiles.
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
  /// session from a thread other than the map's owner thread.
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

  /// Loads a style URL.
  ///
  /// Loading is asynchronous: a missing, unreachable, or malformed style still
  /// returns normally here and reports through a map-loading-failed runtime
  /// event. A style MapLibre rejects semantically, such as one with an unknown
  /// `version`, produces neither an error nor an event.
  public func setStyleURL(_ url: String) throws {
    try mapNativeFailure {
      try NativeString.withCString(url) { url in
        try checkStatus(mln_map_set_style_url(handle.requireLive().raw, url))
      }
      retainCallbacksUntilPendingStyleURLLoads()
    }
  }

  /// Loads inline style JSON.
  ///
  /// Malformed JSON is reported twice: this call throws the parse error, and
  /// the same message arrives as a map-loading-failed runtime event. A style
  /// MapLibre rejects semantically, such as one with an unknown `version`,
  /// produces neither an error nor an event.
  public func setStyleJSON(_ json: String) throws {
    try mapNativeFailure {
      try NativeString.withCString(json) { json in
        try checkStatus(mln_map_set_style_json(handle.requireLive().raw, json))
      }
      resetCallbackRetentionState()
    }
  }

  /// Copies the style document this map's style was last parsed from, byte for
  /// byte, rather than a serialization of the live style. Runtime mutations do
  /// not change it, and a failed parse leaves the previous document in place.
  /// The result is empty when no document has been parsed.
  public func loadedStyleJSON() throws -> String {
    try mapNativeFailure {
      try NativeStyle
        .copyMapText(handle.requireLive()) { map, text, capacity, size in
          mln_map_copy_loaded_style_json(map, text, capacity, size)
        }
    }
  }

  /// Copies the URL this map's style was last requested from.
  ///
  /// ``setStyleURL(_:)`` records the URL when the request is made, before the
  /// document parses, so this can disagree with ``loadedStyleJSON()`` while a
  /// load is in flight or after one fails. The result is empty for inline JSON,
  /// for a map that has loaded no style, and for an empty URL alike.
  public func styleURL() throws -> String {
    try mapNativeFailure {
      try NativeStyle
        .copyMapText(handle.requireLive()) { map, text, capacity, size in
          mln_map_copy_style_url(map, text, capacity, size)
        }
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

  /// Marks whether a host-driven gesture is in progress. The flag stays set
  /// until the host clears it, so pair every `true` with a `false`.
  public func setGestureInProgress(_ inProgress: Bool) throws {
    try mapNativeFailure {
      try checkStatus(mln_map_set_gesture_in_progress(
        handle.requireLive().raw,
        inProgress
      ))
    }
  }

  /// Returns whether a host-driven gesture is currently in progress.
  public func isGestureInProgress() throws -> Bool {
    try mapNativeFailure {
      try NativeMap.isGestureInProgress(handle.requireLive())
    }
  }
}
