internal import CMaplibreNativeC

public struct CameraOptions: Equatable, Sendable {
  public var center: LatLng?
  public var zoom: Double?
  public var bearing: Double?
  public var pitch: Double?
  public var centerAltitude: Double?
  public var padding: EdgeInsets?
  /// Input-only screen point the camera pivots around. Camera updates honor it;
  /// MapLibre leaves it `nil` on camera reads.
  public var anchor: ScreenPoint?
  public var roll: Double?
  public var fieldOfView: Double?

  public init(
    center: LatLng? = nil,
    zoom: Double? = nil,
    bearing: Double? = nil,
    pitch: Double? = nil,
    centerAltitude: Double? = nil,
    padding: EdgeInsets? = nil,
    anchor: ScreenPoint? = nil,
    roll: Double? = nil,
    fieldOfView: Double? = nil
  ) {
    self.center = center
    self.zoom = zoom
    self.bearing = bearing
    self.pitch = pitch
    self.centerAltitude = centerAltitude
    self.padding = padding
    self.anchor = anchor
    self.roll = roll
    self.fieldOfView = fieldOfView
  }

  init(native: NativeCameraOptionsInput) {
    center = native.center.map(LatLng.init(native:))
    zoom = native.zoom
    bearing = native.bearing
    pitch = native.pitch
    centerAltitude = native.centerAltitude
    padding = native.padding.map(EdgeInsets.init(native:))
    anchor = native.anchor.map { ScreenPoint(x: $0.x, y: $0.y) }
    roll = native.roll
    fieldOfView = native.fieldOfView
  }

  var nativeInput: NativeCameraOptionsInput {
    NativeCameraOptionsInput(
      center: center?.nativeInput,
      zoom: zoom,
      bearing: bearing,
      pitch: pitch,
      centerAltitude: centerAltitude,
      padding: padding?.nativeInput,
      anchor: anchor?.nativeInput,
      roll: roll,
      fieldOfView: fieldOfView
    )
  }
}

public struct UnitBezier: Equatable, Sendable {
  public var x1: Double
  public var y1: Double
  public var x2: Double
  public var y2: Double

  public init(x1: Double, y1: Double, x2: Double, y2: Double) {
    self.x1 = x1
    self.y1 = y1
    self.x2 = x2
    self.y2 = y2
  }

  var nativeInput: NativeUnitBezier {
    NativeUnitBezier(x1: x1, y1: y1, x2: x2, y2: y2)
  }
}

public struct AnimationOptions: Equatable, Sendable {
  public var durationMilliseconds: Double?
  public var velocity: Double?
  public var minimumZoom: Double?
  public var easing: UnitBezier?
  /// Caller-chosen identity for the transition these options start. When
  /// present, the transition emits one `.mapCameraTransitionFinished` runtime
  /// event carrying this value; when absent, it emits none.
  ///
  /// The event fires exactly once however the transition ends, including when a
  /// later camera command supersedes it, and reports no outcome. A rejected
  /// command starts no transition and emits no event.
  public var transitionId: UInt64?

  public init(
    durationMilliseconds: Double? = nil,
    velocity: Double? = nil,
    minimumZoom: Double? = nil,
    easing: UnitBezier? = nil,
    transitionId: UInt64? = nil
  ) {
    self.durationMilliseconds = durationMilliseconds
    self.velocity = velocity
    self.minimumZoom = minimumZoom
    self.easing = easing
    self.transitionId = transitionId
  }

  var nativeInput: NativeAnimationOptionsInput {
    NativeAnimationOptionsInput(
      durationMilliseconds: durationMilliseconds,
      velocity: velocity,
      minimumZoom: minimumZoom,
      easing: easing?.nativeInput,
      transitionId: transitionId
    )
  }
}

public enum CameraUpdateMode: UInt32, Sendable, Hashable {
  case jump = 0
  case ease = 1
  case fly = 2
}

public enum GesturePhase: UInt32, Sendable, Hashable {
  case none = 0
  case begin = 1
  case update = 2
  case end = 3
  case cancel = 4
}

/// One camera command copied atomically by the runtime worker.
public struct CameraUpdate: Equatable, Sendable {
  public var mode: CameraUpdateMode
  public var camera: CameraOptions
  public var animation: AnimationOptions
  public var gesturePhase: GesturePhase
  public var gestureId: UInt64
  public var animationId: UInt64

  public init(
    mode: CameraUpdateMode = .jump,
    camera: CameraOptions,
    animation: AnimationOptions = AnimationOptions(),
    gesturePhase: GesturePhase = .none,
    gestureId: UInt64 = 0,
    animationId: UInt64 = 0
  ) {
    self.mode = mode
    self.camera = camera
    self.animation = animation
    self.gesturePhase = gesturePhase
    self.gestureId = gestureId
    self.animationId = animationId
  }

  func withNativeUpdate<Result>(
    _ body: (UnsafePointer<mln_camera_update>) throws -> Result
  ) throws -> Result {
    try camera.nativeInput.withNativeOptions { camera in
      try animation.nativeInput.withOptionalNativeOptions { animation in
        var update = mln_camera_update_default()
        update.mode = mode.rawValue
        update.camera = camera.pointee
        if let animation {
          update.animation = animation.pointee
        }
        update.gesture_phase = gesturePhase.rawValue
        update.gesture_id = gestureId
        update.animation_id = animationId
        return try withUnsafePointer(to: &update, body)
      }
    }
  }
}
