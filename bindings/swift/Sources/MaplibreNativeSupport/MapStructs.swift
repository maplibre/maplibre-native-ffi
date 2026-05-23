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

public struct NativeCameraFitOptionsInput: Equatable, Sendable {
  public var padding: NativeEdgeInsets?
  public var bearing: Double?
  public var pitch: Double?

  public init(padding: NativeEdgeInsets? = nil, bearing: Double? = nil, pitch: Double? = nil) {
    self.padding = padding
    self.bearing = bearing
    self.pitch = pitch
  }

  public func withOptionalNativeOptions<Result>(_ body: (UnsafePointer<mln_camera_fit_options>?) throws -> Result) throws -> Result {
    if padding == nil, bearing == nil, pitch == nil { return try body(nil) }
    var options = CAPI.cameraFitOptionsDefault()
    if let padding {
      options.fields |= MLN_CAMERA_FIT_OPTION_PADDING.rawValue
      options.padding = padding.native
    }
    if let bearing {
      options.fields |= MLN_CAMERA_FIT_OPTION_BEARING.rawValue
      options.bearing = bearing
    }
    if let pitch {
      options.fields |= MLN_CAMERA_FIT_OPTION_PITCH.rawValue
      options.pitch = pitch
    }
    return try withUnsafePointer(to: &options, body)
  }
}

public struct NativeBoundOptionsInput: Equatable, Sendable {
  public var bounds: NativeLatLngBounds?
  public var minZoom: Double?
  public var maxZoom: Double?
  public var minPitch: Double?
  public var maxPitch: Double?

  public init(bounds: NativeLatLngBounds? = nil, minZoom: Double? = nil, maxZoom: Double? = nil, minPitch: Double? = nil, maxPitch: Double? = nil) {
    self.bounds = bounds
    self.minZoom = minZoom
    self.maxZoom = maxZoom
    self.minPitch = minPitch
    self.maxPitch = maxPitch
  }

  public init(_ raw: mln_bound_options) {
    bounds = (raw.fields & MLN_BOUND_OPTION_BOUNDS.rawValue) != 0 ? NativeLatLngBounds(raw.bounds) : nil
    minZoom = (raw.fields & MLN_BOUND_OPTION_MIN_ZOOM.rawValue) != 0 ? raw.min_zoom : nil
    maxZoom = (raw.fields & MLN_BOUND_OPTION_MAX_ZOOM.rawValue) != 0 ? raw.max_zoom : nil
    minPitch = (raw.fields & MLN_BOUND_OPTION_MIN_PITCH.rawValue) != 0 ? raw.min_pitch : nil
    maxPitch = (raw.fields & MLN_BOUND_OPTION_MAX_PITCH.rawValue) != 0 ? raw.max_pitch : nil
  }

  public func withNativeOptions<Result>(_ body: (UnsafePointer<mln_bound_options>) throws -> Result) throws -> Result {
    var options = CAPI.boundOptionsDefault()
    if let bounds {
      options.fields |= MLN_BOUND_OPTION_BOUNDS.rawValue
      options.bounds = bounds.native
    }
    if let minZoom {
      options.fields |= MLN_BOUND_OPTION_MIN_ZOOM.rawValue
      options.min_zoom = minZoom
    }
    if let maxZoom {
      options.fields |= MLN_BOUND_OPTION_MAX_ZOOM.rawValue
      options.max_zoom = maxZoom
    }
    if let minPitch {
      options.fields |= MLN_BOUND_OPTION_MIN_PITCH.rawValue
      options.min_pitch = minPitch
    }
    if let maxPitch {
      options.fields |= MLN_BOUND_OPTION_MAX_PITCH.rawValue
      options.max_pitch = maxPitch
    }
    return try withUnsafePointer(to: &options, body)
  }
}

public struct NativeVec3: Equatable, Sendable {
  public let x: Double
  public let y: Double
  public let z: Double
  public init(x: Double, y: Double, z: Double) { self.x = x; self.y = y; self.z = z }
  public init(_ raw: mln_vec3) { x = raw.x; y = raw.y; z = raw.z }
  public var native: mln_vec3 { mln_vec3(x: x, y: y, z: z) }
}

public struct NativeQuaternion: Equatable, Sendable {
  public let x: Double
  public let y: Double
  public let z: Double
  public let w: Double
  public init(x: Double, y: Double, z: Double, w: Double) { self.x = x; self.y = y; self.z = z; self.w = w }
  public init(_ raw: mln_quaternion) { x = raw.x; y = raw.y; z = raw.z; w = raw.w }
  public var native: mln_quaternion { mln_quaternion(x: x, y: y, z: z, w: w) }
}

public struct NativeFreeCameraOptionsInput: Equatable, Sendable {
  public var position: NativeVec3?
  public var orientation: NativeQuaternion?
  public init(position: NativeVec3? = nil, orientation: NativeQuaternion? = nil) { self.position = position; self.orientation = orientation }
  public init(_ raw: mln_free_camera_options) {
    position = (raw.fields & MLN_FREE_CAMERA_OPTION_POSITION.rawValue) != 0 ? NativeVec3(raw.position) : nil
    orientation = (raw.fields & MLN_FREE_CAMERA_OPTION_ORIENTATION.rawValue) != 0 ? NativeQuaternion(raw.orientation) : nil
  }
  public func withNativeOptions<Result>(_ body: (UnsafePointer<mln_free_camera_options>) throws -> Result) throws -> Result {
    var options = CAPI.freeCameraOptionsDefault()
    if let position { options.fields |= MLN_FREE_CAMERA_OPTION_POSITION.rawValue; options.position = position.native }
    if let orientation { options.fields |= MLN_FREE_CAMERA_OPTION_ORIENTATION.rawValue; options.orientation = orientation.native }
    return try withUnsafePointer(to: &options, body)
  }
}

public struct NativeProjectionModeInput: Equatable, Sendable {
  public var axonometric: Bool?
  public var xSkew: Double?
  public var ySkew: Double?
  public init(axonometric: Bool? = nil, xSkew: Double? = nil, ySkew: Double? = nil) { self.axonometric = axonometric; self.xSkew = xSkew; self.ySkew = ySkew }
  public init(_ raw: mln_projection_mode) {
    axonometric = (raw.fields & MLN_PROJECTION_MODE_AXONOMETRIC.rawValue) != 0 ? raw.axonometric : nil
    xSkew = (raw.fields & MLN_PROJECTION_MODE_X_SKEW.rawValue) != 0 ? raw.x_skew : nil
    ySkew = (raw.fields & MLN_PROJECTION_MODE_Y_SKEW.rawValue) != 0 ? raw.y_skew : nil
  }
  public func withNativeMode<Result>(_ body: (UnsafePointer<mln_projection_mode>) throws -> Result) throws -> Result {
    var mode = CAPI.projectionModeDefault()
    if let axonometric { mode.fields |= MLN_PROJECTION_MODE_AXONOMETRIC.rawValue; mode.axonometric = axonometric }
    if let xSkew { mode.fields |= MLN_PROJECTION_MODE_X_SKEW.rawValue; mode.x_skew = xSkew }
    if let ySkew { mode.fields |= MLN_PROJECTION_MODE_Y_SKEW.rawValue; mode.y_skew = ySkew }
    return try withUnsafePointer(to: &mode, body)
  }
}

public struct NativeMapViewportOptionsInput: Equatable, Sendable {
  public var northOrientation: UInt32?
  public var constrainMode: UInt32?
  public var viewportMode: UInt32?
  public var frustumOffset: NativeEdgeInsets?
  public init(northOrientation: UInt32? = nil, constrainMode: UInt32? = nil, viewportMode: UInt32? = nil, frustumOffset: NativeEdgeInsets? = nil) {
    self.northOrientation = northOrientation; self.constrainMode = constrainMode; self.viewportMode = viewportMode; self.frustumOffset = frustumOffset
  }
  public init(_ raw: mln_map_viewport_options) {
    northOrientation = (raw.fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION.rawValue) != 0 ? raw.north_orientation : nil
    constrainMode = (raw.fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE.rawValue) != 0 ? raw.constrain_mode : nil
    viewportMode = (raw.fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE.rawValue) != 0 ? raw.viewport_mode : nil
    frustumOffset = (raw.fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET.rawValue) != 0 ? NativeEdgeInsets(raw.frustum_offset) : nil
  }
  public func withNativeOptions<Result>(_ body: (UnsafePointer<mln_map_viewport_options>) throws -> Result) throws -> Result {
    var options = CAPI.mapViewportOptionsDefault()
    if let northOrientation { options.fields |= MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION.rawValue; options.north_orientation = northOrientation }
    if let constrainMode { options.fields |= MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE.rawValue; options.constrain_mode = constrainMode }
    if let viewportMode { options.fields |= MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE.rawValue; options.viewport_mode = viewportMode }
    if let frustumOffset { options.fields |= MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET.rawValue; options.frustum_offset = frustumOffset.native }
    return try withUnsafePointer(to: &options, body)
  }
}

public struct NativeMapTileOptionsInput: Equatable, Sendable {
  public var prefetchZoomDelta: UInt32?
  public var lodMinRadius: Double?
  public var lodScale: Double?
  public var lodPitchThreshold: Double?
  public var lodZoomShift: Double?
  public var lodMode: UInt32?
  public init(prefetchZoomDelta: UInt32? = nil, lodMinRadius: Double? = nil, lodScale: Double? = nil, lodPitchThreshold: Double? = nil, lodZoomShift: Double? = nil, lodMode: UInt32? = nil) {
    self.prefetchZoomDelta = prefetchZoomDelta; self.lodMinRadius = lodMinRadius; self.lodScale = lodScale; self.lodPitchThreshold = lodPitchThreshold; self.lodZoomShift = lodZoomShift; self.lodMode = lodMode
  }
  public init(_ raw: mln_map_tile_options) {
    prefetchZoomDelta = (raw.fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA.rawValue) != 0 ? raw.prefetch_zoom_delta : nil
    lodMinRadius = (raw.fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS.rawValue) != 0 ? raw.lod_min_radius : nil
    lodScale = (raw.fields & MLN_MAP_TILE_OPTION_LOD_SCALE.rawValue) != 0 ? raw.lod_scale : nil
    lodPitchThreshold = (raw.fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD.rawValue) != 0 ? raw.lod_pitch_threshold : nil
    lodZoomShift = (raw.fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT.rawValue) != 0 ? raw.lod_zoom_shift : nil
    lodMode = (raw.fields & MLN_MAP_TILE_OPTION_LOD_MODE.rawValue) != 0 ? raw.lod_mode : nil
  }
  public func withNativeOptions<Result>(_ body: (UnsafePointer<mln_map_tile_options>) throws -> Result) throws -> Result {
    var options = CAPI.mapTileOptionsDefault()
    if let prefetchZoomDelta { options.fields |= MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA.rawValue; options.prefetch_zoom_delta = prefetchZoomDelta }
    if let lodMinRadius { options.fields |= MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS.rawValue; options.lod_min_radius = lodMinRadius }
    if let lodScale { options.fields |= MLN_MAP_TILE_OPTION_LOD_SCALE.rawValue; options.lod_scale = lodScale }
    if let lodPitchThreshold { options.fields |= MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD.rawValue; options.lod_pitch_threshold = lodPitchThreshold }
    if let lodZoomShift { options.fields |= MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT.rawValue; options.lod_zoom_shift = lodZoomShift }
    if let lodMode { options.fields |= MLN_MAP_TILE_OPTION_LOD_MODE.rawValue; options.lod_mode = lodMode }
    return try withUnsafePointer(to: &options, body)
  }
}
