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
  public var width: UInt32
  public var height: UInt32
  public var scaleFactor: Double
  public var mode: MapMode

  public init(width: UInt32, height: UInt32, scaleFactor: Double = 1.0, mode: MapMode = .continuous) {
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
  private var customGeometrySourceCallbacks: [String: NativeCustomGeometrySourceCallbacks] = [:]

  public init(runtime: RuntimeHandle, options: MapOptions) throws {
    let pointer = try mapNativeFailure {
      try options.nativeInput.withNativeOptions { nativeOptions in
        try NativeMap.create(runtime: try runtime.requireLivePointer(), options: nativeOptions)
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

  func requireLivePointer() throws -> OpaquePointer {
    try handle.requireLive()
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
      case .map(let source) = event.source
    else { return }

    let map = registryLock.withLock { registry[source.addressBitPattern]?.value }
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

  func storeCustomGeometrySourceCallbacks(_ callbacks: NativeCustomGeometrySourceCallbacks, sourceId: String) {
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

  public func setStyleURL(_ url: String) throws {
    try mapNativeFailure {
      try NativeString.withCString(url) { url in
        try checkStatus(mln_map_set_style_url(try handle.requireLive(), url))
      }
      retainCallbacksUntilPendingStyleURLLoads()
    }
  }

  public func setStyleJSON(_ json: String) throws {
    try mapNativeFailure {
      try NativeString.withCString(json) { json in
        try checkStatus(mln_map_set_style_json(try handle.requireLive(), json))
      }
      resetCallbackRetentionState()
    }
  }

  public func requestRepaint() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_request_repaint(try handle.requireLive()))
    }
  }

  public func requestStillImage() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_request_still_image(try handle.requireLive()))
    }
  }

  public func camera() throws -> CameraOptions {
    try mapNativeFailure {
      CameraOptions(native: NativeCameraOptionsInput(try NativeMap.camera(try handle.requireLive())))
    }
  }

  public func jump(to camera: CameraOptions) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try checkStatus(mln_map_jump_to(try handle.requireLive(), nativeCamera))
      }
    }
  }

  public func ease(to camera: CameraOptions, animation: AnimationOptions? = nil) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try (animation?.nativeInput ?? NativeAnimationOptionsInput()).withOptionalNativeOptions { nativeAnimation in
          try checkStatus(mln_map_ease_to(try handle.requireLive(), nativeCamera, nativeAnimation))
        }
      }
    }
  }

  public func moveBy(deltaX: Double, deltaY: Double) throws {
    try mapNativeFailure {
      try checkStatus(mln_map_move_by(try handle.requireLive(), deltaX, deltaY))
    }
  }

  public func moveBy(deltaX: Double, deltaY: Double, animation: AnimationOptions) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { nativeAnimation in
        try checkStatus(mln_map_move_by_animated(try handle.requireLive(), deltaX, deltaY, nativeAnimation))
      }
    }
  }

  public func scaleBy(_ scale: Double, anchor: ScreenPoint? = nil) throws {
    try mapNativeFailure {
      if var nativeAnchor = anchor?.nativeInput.native {
        try withUnsafePointer(to: &nativeAnchor) { anchor in
          try checkStatus(mln_map_scale_by(try handle.requireLive(), scale, anchor))
        }
      } else {
        try checkStatus(mln_map_scale_by(try handle.requireLive(), scale, nil))
      }
    }
  }

  public func scaleBy(_ scale: Double, anchor: ScreenPoint? = nil, animation: AnimationOptions) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { nativeAnimation in
        if var nativeAnchor = anchor?.nativeInput.native {
          try withUnsafePointer(to: &nativeAnchor) { anchor in
            try checkStatus(mln_map_scale_by_animated(try handle.requireLive(), scale, anchor, nativeAnimation))
          }
        } else {
          try checkStatus(mln_map_scale_by_animated(try handle.requireLive(), scale, nil, nativeAnimation))
        }
      }
    }
  }

  public func cancelTransitions() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_cancel_transitions(try handle.requireLive()))
    }
  }
}
