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
  var customGeometrySourceCallbacks: [String: NativeCustomGeometrySourceCallbacks] = [:]

  public init(runtime: RuntimeHandle, options: MapOptions) throws {
    let pointer = try mapNativeFailure {
      try options.nativeInput.withNativeOptions { nativeOptions in
        try CAPI.createMap(runtime: try runtime.requireLivePointer(), options: nativeOptions)
      }
    }
    self.runtime = runtime
    nativeAddress = UInt(bitPattern: pointer)
    handle = try NativeHandleBox(typeName: "MapHandle", pointer: pointer)
    Self.register(self)
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
    for sourceId in customGeometrySourceCallbacks.keys {
      guard (try? styleSourceExists(sourceId)) == false else { return }
    }
    styleURLReplacementPending = false
    customGeometrySourceCallbacks.removeAll()
  }

  public func close() throws {
    try handle.closeOnce { pointer in
      try CAPI.destroyMap(pointer)
    }
    Self.unregister(nativeAddress)
    customGeometrySourceCallbacks.removeAll()
  }

  public func setStyleURL(_ url: String) throws {
    try mapNativeFailure {
      try CAPI.mapSetStyleURL(try handle.requireLive(), url)
      retainCallbacksUntilPendingStyleURLLoads()
    }
  }

  public func setStyleJSON(_ json: String) throws {
    try mapNativeFailure {
      try CAPI.mapSetStyleJSON(try handle.requireLive(), json)
      styleURLReplacementPending = false
      customGeometrySourceCallbacks.removeAll()
    }
  }

  public func requestRepaint() throws {
    try mapNativeFailure {
      try CAPI.mapRequestRepaint(try handle.requireLive())
    }
  }

  public func requestStillImage() throws {
    try mapNativeFailure {
      try CAPI.mapRequestStillImage(try handle.requireLive())
    }
  }

  public func camera() throws -> CameraOptions {
    try mapNativeFailure {
      CameraOptions(native: NativeCameraOptionsInput(try CAPI.mapGetCamera(try handle.requireLive())))
    }
  }

  public func jump(to camera: CameraOptions) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try CAPI.mapJumpTo(try handle.requireLive(), nativeCamera)
      }
    }
  }

  public func ease(to camera: CameraOptions, animation: AnimationOptions? = nil) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try (animation?.nativeInput ?? NativeAnimationOptionsInput()).withOptionalNativeOptions { nativeAnimation in
          try CAPI.mapEaseTo(try handle.requireLive(), nativeCamera, nativeAnimation)
        }
      }
    }
  }

  public func moveBy(deltaX: Double, deltaY: Double) throws {
    try mapNativeFailure {
      try CAPI.mapMoveBy(try handle.requireLive(), deltaX: deltaX, deltaY: deltaY)
    }
  }

  public func moveBy(deltaX: Double, deltaY: Double, animation: AnimationOptions) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { nativeAnimation in
        try CAPI.mapMoveByAnimated(
          try handle.requireLive(),
          deltaX: deltaX,
          deltaY: deltaY,
          animation: nativeAnimation
        )
      }
    }
  }

  public func scaleBy(_ scale: Double, anchor: ScreenPoint? = nil) throws {
    try mapNativeFailure {
      if var nativeAnchor = anchor?.nativeInput.native {
        try withUnsafePointer(to: &nativeAnchor) { anchor in
          try CAPI.mapScaleBy(try handle.requireLive(), scale: scale, anchor: anchor)
        }
      } else {
        try CAPI.mapScaleBy(try handle.requireLive(), scale: scale, anchor: nil)
      }
    }
  }

  public func scaleBy(_ scale: Double, anchor: ScreenPoint? = nil, animation: AnimationOptions) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { nativeAnimation in
        if var nativeAnchor = anchor?.nativeInput.native {
          try withUnsafePointer(to: &nativeAnchor) { anchor in
            try CAPI.mapScaleByAnimated(
              try handle.requireLive(),
              scale: scale,
              anchor: anchor,
              animation: nativeAnimation
            )
          }
        } else {
          try CAPI.mapScaleByAnimated(
            try handle.requireLive(),
            scale: scale,
            anchor: nil,
            animation: nativeAnimation
          )
        }
      }
    }
  }

  public func cancelTransitions() throws {
    try mapNativeFailure {
      try CAPI.mapCancelTransitions(try handle.requireLive())
    }
  }
}
