
public struct CameraOptions: Equatable, Sendable {
  public var center: LatLng?
  public var zoom: Double?
  public var bearing: Double?
  public var pitch: Double?
  public var centerAltitude: Double?
  public var padding: EdgeInsets?
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

  public init(
    durationMilliseconds: Double? = nil,
    velocity: Double? = nil,
    minimumZoom: Double? = nil,
    easing: UnitBezier? = nil
  ) {
    self.durationMilliseconds = durationMilliseconds
    self.velocity = velocity
    self.minimumZoom = minimumZoom
    self.easing = easing
  }

  var nativeInput: NativeAnimationOptionsInput {
    NativeAnimationOptionsInput(
      durationMilliseconds: durationMilliseconds,
      velocity: velocity,
      minimumZoom: minimumZoom,
      easing: easing?.nativeInput
    )
  }
}
