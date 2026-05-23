import MaplibreNativeSupport

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
  private let runtime: RuntimeHandle
  private let handle: NativeHandleBox
  var customGeometrySourceCallbacks: [String: NativeCustomGeometrySourceCallbacks] = [:]

  public init(runtime: RuntimeHandle, options: MapOptions) throws {
    let pointer = try mapNativeFailure {
      try options.nativeInput.withNativeOptions { nativeOptions in
        try CAPI.createMap(runtime: try runtime.requireLivePointer(), options: nativeOptions)
      }
    }
    self.runtime = runtime
    handle = try NativeHandleBox(typeName: "MapHandle", pointer: pointer)
  }

  deinit {
    try? close()
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  func requireLivePointer() throws -> OpaquePointer {
    try handle.requireLive()
  }

  public func close() throws {
    try handle.closeOnce { pointer in
      try CAPI.destroyMap(pointer)
    }
    customGeometrySourceCallbacks.removeAll()
  }

  public func setStyleURL(_ url: String) throws {
    try mapNativeFailure {
      try CAPI.mapSetStyleURL(try handle.requireLive(), url)
    }
  }

  public func setStyleJSON(_ json: String) throws {
    try mapNativeFailure {
      try CAPI.mapSetStyleJSON(try handle.requireLive(), json)
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
