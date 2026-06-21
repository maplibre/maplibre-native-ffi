
public struct LatLng: Equatable, Sendable {
  public var latitude: Double
  public var longitude: Double

  public init(latitude: Double, longitude: Double) {
    self.latitude = latitude
    self.longitude = longitude
  }

  init(native: NativeLatLng) {
    latitude = native.latitude
    longitude = native.longitude
  }

  var nativeInput: NativeLatLng {
    NativeLatLng(latitude: latitude, longitude: longitude)
  }
}

public struct ScreenPoint: Equatable, Sendable {
  public var x: Double
  public var y: Double

  public init(x: Double, y: Double) {
    self.x = x
    self.y = y
  }

  init(native: NativeScreenPoint) {
    x = native.x
    y = native.y
  }

  var nativeInput: NativeScreenPoint {
    NativeScreenPoint(x: x, y: y)
  }
}

public struct LatLngBounds: Equatable, Sendable {
  public var southwest: LatLng
  public var northeast: LatLng

  public init(southwest: LatLng, northeast: LatLng) {
    self.southwest = southwest
    self.northeast = northeast
  }

  init(native: NativeLatLngBounds) {
    southwest = LatLng(native: native.southwest)
    northeast = LatLng(native: native.northeast)
  }

  var nativeInput: NativeLatLngBounds {
    NativeLatLngBounds(
      southwest: southwest.nativeInput,
      northeast: northeast.nativeInput
    )
  }
}

public struct EdgeInsets: Equatable, Sendable {
  public var top: Double
  public var left: Double
  public var bottom: Double
  public var right: Double

  public init(top: Double, left: Double, bottom: Double, right: Double) {
    self.top = top
    self.left = left
    self.bottom = bottom
    self.right = right
  }

  init(native: NativeEdgeInsets) {
    top = native.top
    left = native.left
    bottom = native.bottom
    right = native.right
  }

  var nativeInput: NativeEdgeInsets {
    NativeEdgeInsets(top: top, left: left, bottom: bottom, right: right)
  }
}
