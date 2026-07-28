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

  public init(
    width: UInt32,
    height: UInt32,
    scaleFactor: Double = 1.0,
    mode: MapMode = .continuous
  ) {
    self.width = width
    self.height = height
    self.scaleFactor = scaleFactor
    self.mode = mode
  }

  var nativeInput: NativeMapOptionsInput {
    NativeMapOptionsInput(
      width: width,
      height: height,
      scaleFactor: scaleFactor,
      mapMode: mode.rawValue
    )
  }
}

public final class MapHandle {
  private static let registryLock = NSLock()
  private nonisolated(unsafe) static var registry: [UInt: WeakMapHandle] = [:]

  private let runtime: RuntimeHandle
  private let handle: NativeHandleBox
  private let nativeAddress: UInt
  private var styleURLReplacementPending = false
  private var customGeometrySourceCallbacks: [
    String: NativeCustomGeometrySourceCallbacks
  ] =
    [:]

  public init(runtime: RuntimeHandle, options: MapOptions) throws {
    let pointer = try mapNativeFailure {
      try options.nativeInput.withNativeOptions { nativeOptions in
        try NativeMap.create(
          runtime: runtime.requireLivePointer(),
          options: nativeOptions
        )
      }
    }
    self.runtime = runtime
    nativeAddress = UInt(bitPattern: pointer)
    handle = try NativeHandleBox(typeName: "MapHandle", pointer: pointer)
    Self.register(self)
  }

  deinit {
    Self.unregister(nativeAddress)
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
    return source.addressBitPattern == nativeAddress
  }

  func requireLivePointer() throws -> OpaquePointer {
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
    _ = try requireLivePointer()
    return MapAttachRef(handle: handle)
  }

  private static func register(_ map: MapHandle) {
    registryLock.withLock {
      registry[map.nativeAddress] = WeakMapHandle(map)
    }
  }

  private static func unregister(_ nativeAddress: UInt) {
    registryLock.withLock {
      _ = registry.removeValue(forKey: nativeAddress)
    }
  }

  static func handleRuntimeEvent(_ event: RuntimeEvent) {
    guard event.type == .mapStyleLoaded,
          case let .map(source) = event.source
    else { return }

    let map = registryLock
      .withLock { registry[source.addressBitPattern]?.value }
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
    try handle.closeOnce { pointer in
      try checkStatus(mln_map_destroy(pointer))
    }
    Self.unregister(nativeAddress)
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
        try checkStatus(mln_map_set_style_url(handle.requireLive(), url))
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
        try checkStatus(mln_map_set_style_json(handle.requireLive(), json))
      }
      resetCallbackRetentionState()
    }
  }

  public func requestRepaint() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_request_repaint(handle.requireLive()))
    }
  }

  public func requestStillImage() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_request_still_image(handle.requireLive()))
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
        try checkStatus(mln_map_jump_to(handle.requireLive(), nativeCamera))
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
              handle.requireLive(),
              nativeCamera,
              nativeAnimation
            ))
          }
      }
    }
  }

  public func moveBy(deltaX: Double, deltaY: Double) throws {
    try mapNativeFailure {
      try checkStatus(mln_map_move_by(handle.requireLive(), deltaX, deltaY))
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
          handle.requireLive(),
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
            handle.requireLive(),
            scale,
            anchor
          ))
        }
      } else {
        try checkStatus(mln_map_scale_by(handle.requireLive(), scale, nil))
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
              handle.requireLive(),
              scale,
              anchor,
              nativeAnimation
            ))
          }
        } else {
          try checkStatus(mln_map_scale_by_animated(
            handle.requireLive(),
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
      try checkStatus(mln_map_cancel_transitions(handle.requireLive()))
    }
  }
}
