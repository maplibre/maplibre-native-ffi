import CMaplibreNativeC

public struct NativeLatLng: Equatable, Sendable {
  public let latitude: Double
  public let longitude: Double

  public init(latitude: Double, longitude: Double) {
    self.latitude = latitude
    self.longitude = longitude
  }

  public init(_ raw: mln_lat_lng) {
    latitude = raw.latitude
    longitude = raw.longitude
  }

  public var native: mln_lat_lng {
    mln_lat_lng(latitude: latitude, longitude: longitude)
  }
}

public struct NativeScreenPoint: Equatable, Sendable {
  public let x: Double
  public let y: Double

  public init(x: Double, y: Double) {
    self.x = x
    self.y = y
  }

  public init(_ raw: mln_screen_point) {
    x = raw.x
    y = raw.y
  }

  public var native: mln_screen_point {
    mln_screen_point(x: x, y: y)
  }
}

public struct NativeProjectedMeters: Equatable, Sendable {
  public let northing: Double
  public let easting: Double

  public init(northing: Double, easting: Double) {
    self.northing = northing
    self.easting = easting
  }

  public init(_ raw: mln_projected_meters) {
    northing = raw.northing
    easting = raw.easting
  }

  public var native: mln_projected_meters {
    mln_projected_meters(northing: northing, easting: easting)
  }
}

public struct NativeEdgeInsets: Equatable, Sendable {
  public let top: Double
  public let left: Double
  public let bottom: Double
  public let right: Double

  public init(top: Double, left: Double, bottom: Double, right: Double) {
    self.top = top
    self.left = left
    self.bottom = bottom
    self.right = right
  }

  public init(_ raw: mln_edge_insets) {
    top = raw.top
    left = raw.left
    bottom = raw.bottom
    right = raw.right
  }

  public var native: mln_edge_insets {
    mln_edge_insets(top: top, left: left, bottom: bottom, right: right)
  }
}

public struct NativeMapOptionsInput: Equatable, Sendable {
  public let width: UInt32
  public let height: UInt32
  public let scaleFactor: Double
  public let mapMode: UInt32

  public init(width: UInt32, height: UInt32, scaleFactor: Double, mapMode: UInt32) {
    self.width = width
    self.height = height
    self.scaleFactor = scaleFactor
    self.mapMode = mapMode
  }

  public func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_map_options>) throws -> Result
  ) throws -> Result {
    var options = CAPI.mapOptionsDefault()
    options.width = width
    options.height = height
    options.scale_factor = scaleFactor
    options.map_mode = mapMode
    return try withUnsafePointer(to: &options, body)
  }
}

public struct NativeCameraOptionsInput: Equatable, Sendable {
  public var center: NativeLatLng?
  public var zoom: Double?
  public var bearing: Double?
  public var pitch: Double?
  public var centerAltitude: Double?
  public var padding: NativeEdgeInsets?
  public var anchor: NativeScreenPoint?
  public var roll: Double?
  public var fieldOfView: Double?

  public init(
    center: NativeLatLng? = nil,
    zoom: Double? = nil,
    bearing: Double? = nil,
    pitch: Double? = nil,
    centerAltitude: Double? = nil,
    padding: NativeEdgeInsets? = nil,
    anchor: NativeScreenPoint? = nil,
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

  public init(_ raw: mln_camera_options) {
    center = NativeLatLng(latitude: raw.latitude, longitude: raw.longitude)
    zoom = raw.zoom
    bearing = raw.bearing
    pitch = raw.pitch
    centerAltitude = raw.center_altitude
    padding = NativeEdgeInsets(raw.padding)
    anchor = NativeScreenPoint(x: raw.anchor.x, y: raw.anchor.y)
    roll = raw.roll
    fieldOfView = raw.field_of_view
  }

  public func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_camera_options>) throws -> Result
  ) throws -> Result {
    var camera = CAPI.cameraOptionsDefault()
    if let center {
      camera.fields |= MLN_CAMERA_OPTION_CENTER.rawValue
      camera.latitude = center.latitude
      camera.longitude = center.longitude
    }
    if let zoom {
      camera.fields |= MLN_CAMERA_OPTION_ZOOM.rawValue
      camera.zoom = zoom
    }
    if let bearing {
      camera.fields |= MLN_CAMERA_OPTION_BEARING.rawValue
      camera.bearing = bearing
    }
    if let pitch {
      camera.fields |= MLN_CAMERA_OPTION_PITCH.rawValue
      camera.pitch = pitch
    }
    if let centerAltitude {
      camera.fields |= MLN_CAMERA_OPTION_CENTER_ALTITUDE.rawValue
      camera.center_altitude = centerAltitude
    }
    if let padding {
      camera.fields |= MLN_CAMERA_OPTION_PADDING.rawValue
      camera.padding = padding.native
    }
    if let anchor {
      camera.fields |= MLN_CAMERA_OPTION_ANCHOR.rawValue
      camera.anchor = anchor.native
    }
    if let roll {
      camera.fields |= MLN_CAMERA_OPTION_ROLL.rawValue
      camera.roll = roll
    }
    if let fieldOfView {
      camera.fields |= MLN_CAMERA_OPTION_FOV.rawValue
      camera.field_of_view = fieldOfView
    }
    return try withUnsafePointer(to: &camera, body)
  }
}

public struct NativeUnitBezier: Equatable, Sendable {
  public let x1: Double
  public let y1: Double
  public let x2: Double
  public let y2: Double

  public init(x1: Double, y1: Double, x2: Double, y2: Double) {
    self.x1 = x1
    self.y1 = y1
    self.x2 = x2
    self.y2 = y2
  }

  public var native: mln_unit_bezier {
    mln_unit_bezier(x1: x1, y1: y1, x2: x2, y2: y2)
  }
}

public struct NativeAnimationOptionsInput: Equatable, Sendable {
  public var durationMilliseconds: Double?
  public var velocity: Double?
  public var minimumZoom: Double?
  public var easing: NativeUnitBezier?

  public init(
    durationMilliseconds: Double? = nil,
    velocity: Double? = nil,
    minimumZoom: Double? = nil,
    easing: NativeUnitBezier? = nil
  ) {
    self.durationMilliseconds = durationMilliseconds
    self.velocity = velocity
    self.minimumZoom = minimumZoom
    self.easing = easing
  }

  public func withOptionalNativeOptions<Result>(
    _ body: (UnsafePointer<mln_animation_options>?) throws -> Result
  ) throws -> Result {
    if durationMilliseconds == nil, velocity == nil, minimumZoom == nil, easing == nil {
      return try body(nil)
    }
    var animation = CAPI.animationOptionsDefault()
    if let durationMilliseconds {
      animation.fields |= MLN_ANIMATION_OPTION_DURATION.rawValue
      animation.duration_ms = durationMilliseconds
    }
    if let velocity {
      animation.fields |= MLN_ANIMATION_OPTION_VELOCITY.rawValue
      animation.velocity = velocity
    }
    if let minimumZoom {
      animation.fields |= MLN_ANIMATION_OPTION_MIN_ZOOM.rawValue
      animation.min_zoom = minimumZoom
    }
    if let easing {
      animation.fields |= MLN_ANIMATION_OPTION_EASING.rawValue
      animation.easing = easing.native
    }
    return try withUnsafePointer(to: &animation, body)
  }
}
