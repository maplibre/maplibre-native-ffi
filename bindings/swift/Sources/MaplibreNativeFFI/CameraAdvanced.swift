
internal import CMaplibreNativeC
import Foundation

public struct CameraFitOptions: Equatable, Sendable {
  public var padding: EdgeInsets?
  public var bearing: Double?
  public var pitch: Double?

  public init(
    padding: EdgeInsets? = nil,
    bearing: Double? = nil,
    pitch: Double? = nil
  ) {
    self.padding = padding
    self.bearing = bearing
    self.pitch = pitch
  }

  var nativeInput: NativeCameraFitOptionsInput {
    NativeCameraFitOptionsInput(
      padding: padding?.nativeInput,
      bearing: bearing,
      pitch: pitch
    )
  }
}

/// Geographic constraint applied to the map camera center.
public enum BoundsConstraint: Equatable, Sendable {
  /// Keeps the camera center inside the given bounds.
  case bounded(LatLngBounds)
  /// Leaves the camera center unconstrained, so the map pans freely across
  /// the antimeridian. This differs from world bounds of -90/-180 to 90/180,
  /// which clamp longitude to that range.
  case unbounded

  init(native: NativeBoundsConstraintInput) {
    switch native {
    case let .bounded(bounds):
      self = .bounded(LatLngBounds(native: bounds))
    case .unbounded:
      self = .unbounded
    }
  }

  var nativeInput: NativeBoundsConstraintInput {
    switch self {
    case let .bounded(bounds):
      .bounded(bounds.nativeInput)
    case .unbounded:
      .unbounded
    }
  }
}

public struct BoundOptions: Equatable, Sendable {
  public var bounds: BoundsConstraint?
  public var minZoom: Double?
  public var maxZoom: Double?
  public var minPitch: Double?
  public var maxPitch: Double?

  public init(
    bounds: BoundsConstraint? = nil,
    minZoom: Double? = nil,
    maxZoom: Double? = nil,
    minPitch: Double? = nil,
    maxPitch: Double? = nil
  ) {
    self.bounds = bounds
    self.minZoom = minZoom
    self.maxZoom = maxZoom
    self.minPitch = minPitch
    self.maxPitch = maxPitch
  }

  init(native: NativeBoundOptionsInput) {
    bounds = native.bounds.map(BoundsConstraint.init(native:))
    minZoom = native.minZoom
    maxZoom = native.maxZoom
    minPitch = native.minPitch
    maxPitch = native.maxPitch
  }

  var nativeInput: NativeBoundOptionsInput {
    NativeBoundOptionsInput(
      bounds: bounds?.nativeInput,
      minZoom: minZoom,
      maxZoom: maxZoom,
      minPitch: minPitch,
      maxPitch: maxPitch
    )
  }
}

public struct Vec3: Equatable, Sendable {
  public var x: Double
  public var y: Double
  public var z: Double

  public init(x: Double, y: Double, z: Double) {
    self.x = x
    self.y = y
    self.z = z
  }

  init(native: NativeVec3) {
    x = native.x
    y = native.y
    z = native.z
  }

  var nativeInput: NativeVec3 {
    NativeVec3(x: x, y: y, z: z)
  }
}

public struct Quaternion: Equatable, Sendable {
  public var x: Double
  public var y: Double
  public var z: Double
  public var w: Double

  public init(x: Double, y: Double, z: Double, w: Double) {
    self.x = x
    self.y = y
    self.z = z
    self.w = w
  }

  init(native: NativeQuaternion) {
    x = native.x
    y = native.y
    z = native.z
    w = native.w
  }

  var nativeInput: NativeQuaternion {
    NativeQuaternion(x: x, y: y, z: z, w: w)
  }
}

public struct FreeCameraOptions: Equatable, Sendable {
  public var position: Vec3?
  public var orientation: Quaternion?

  public init(position: Vec3? = nil, orientation: Quaternion? = nil) {
    self.position = position
    self.orientation = orientation
  }

  init(native: NativeFreeCameraOptionsInput) {
    position = native.position.map(Vec3.init(native:))
    orientation = native.orientation.map(Quaternion.init(native:))
  }

  var nativeInput: NativeFreeCameraOptionsInput {
    NativeFreeCameraOptionsInput(
      position: position?.nativeInput,
      orientation: orientation?.nativeInput
    )
  }
}

public struct ProjectionMode: Equatable, Sendable {
  public var axonometric: Bool?
  public var xSkew: Double?
  public var ySkew: Double?

  public init(
    axonometric: Bool? = nil,
    xSkew: Double? = nil,
    ySkew: Double? = nil
  ) {
    self.axonometric = axonometric
    self.xSkew = xSkew
    self.ySkew = ySkew
  }

  init(native: NativeProjectionModeInput) {
    axonometric = native.axonometric
    xSkew = native.xSkew
    ySkew = native.ySkew
  }

  var nativeInput: NativeProjectionModeInput {
    NativeProjectionModeInput(
      axonometric: axonometric,
      xSkew: xSkew,
      ySkew: ySkew
    )
  }
}

public struct MapDebugOptions: OptionSet, Sendable, Hashable {
  public let rawValue: UInt32
  public init(rawValue: UInt32) {
    self.rawValue = rawValue
  }

  public static let tileBorders = Self(rawValue: 1 << 1)
  public static let parseStatus = Self(rawValue: 1 << 2)
  public static let timestamps = Self(rawValue: 1 << 3)
  public static let collision = Self(rawValue: 1 << 4)
  public static let overdraw = Self(rawValue: 1 << 5)
  public static let stencilClip = Self(rawValue: 1 << 6)
  public static let depthBuffer = Self(rawValue: 1 << 7)
}

public enum NorthOrientation: UInt32, Sendable, Hashable {
  case up = 0
  case right = 1
  case down = 2
  case left = 3
}

public enum ConstrainMode: UInt32, Sendable, Hashable {
  case none = 0
  case heightOnly = 1
  case widthAndHeight = 2
  case screen = 3
}

public enum ViewportMode: UInt32, Sendable, Hashable {
  case `default` = 0
  case flippedY = 1
}

public struct MapViewportOptions: Equatable, Sendable {
  public var northOrientation: NorthOrientation?
  public var constrainMode: ConstrainMode?
  public var viewportMode: ViewportMode?
  public var frustumOffset: EdgeInsets?

  public init(
    northOrientation: NorthOrientation? = nil,
    constrainMode: ConstrainMode? = nil,
    viewportMode: ViewportMode? = nil,
    frustumOffset: EdgeInsets? = nil
  ) {
    self.northOrientation = northOrientation
    self.constrainMode = constrainMode
    self.viewportMode = viewportMode
    self.frustumOffset = frustumOffset
  }

  init(native: NativeMapViewportOptionsInput) {
    northOrientation = native.northOrientation
      .flatMap(NorthOrientation.init(rawValue:))
    constrainMode = native.constrainMode.flatMap(ConstrainMode.init(rawValue:))
    viewportMode = native.viewportMode.flatMap(ViewportMode.init(rawValue:))
    frustumOffset = native.frustumOffset.map(EdgeInsets.init(native:))
  }

  var nativeInput: NativeMapViewportOptionsInput {
    NativeMapViewportOptionsInput(
      northOrientation: northOrientation?.rawValue,
      constrainMode: constrainMode?.rawValue,
      viewportMode: viewportMode?.rawValue,
      frustumOffset: frustumOffset?.nativeInput
    )
  }
}

public enum TileLODMode: UInt32, Sendable, Hashable {
  case `default` = 0
  case distance = 1
}

public struct MapTileOptions: Equatable, Sendable {
  public var prefetchZoomDelta: UInt32?
  public var lodMinRadius: Double?
  public var lodScale: Double?
  public var lodPitchThreshold: Double?
  public var lodZoomShift: Double?
  public var lodMode: TileLODMode?

  public init(
    prefetchZoomDelta: UInt32? = nil,
    lodMinRadius: Double? = nil,
    lodScale: Double? = nil,
    lodPitchThreshold: Double? = nil,
    lodZoomShift: Double? = nil,
    lodMode: TileLODMode? = nil
  ) {
    self.prefetchZoomDelta = prefetchZoomDelta
    self.lodMinRadius = lodMinRadius
    self.lodScale = lodScale
    self.lodPitchThreshold = lodPitchThreshold
    self.lodZoomShift = lodZoomShift
    self.lodMode = lodMode
  }

  init(native: NativeMapTileOptionsInput) {
    prefetchZoomDelta = native.prefetchZoomDelta
    lodMinRadius = native.lodMinRadius
    lodScale = native.lodScale
    lodPitchThreshold = native.lodPitchThreshold
    lodZoomShift = native.lodZoomShift
    lodMode = native.lodMode.flatMap(TileLODMode.init(rawValue:))
  }

  var nativeInput: NativeMapTileOptionsInput {
    NativeMapTileOptionsInput(
      prefetchZoomDelta: prefetchZoomDelta,
      lodMinRadius: lodMinRadius,
      lodScale: lodScale,
      lodPitchThreshold: lodPitchThreshold,
      lodZoomShift: lodZoomShift,
      lodMode: lodMode?.rawValue
    )
  }
}

public extension MapHandle {
  @discardableResult
  func setDebugOptions(
    _ options: MapDebugOptions
  ) async throws -> CommandCompletion {
    try await submitCommand {
      mln_map_set_debug_options($0, options.rawValue, $1)
    }
  }

  @discardableResult
  func setRenderingStatsViewEnabled(
    _ enabled: Bool
  ) async throws -> CommandCompletion {
    try await submitCommand {
      mln_map_set_rendering_stats_view_enabled($0, enabled, $1)
    }
  }

  @discardableResult
  func dumpDebugLogs() async throws -> CommandCompletion {
    try await submitCommand(mln_map_dump_debug_logs)
  }

  @discardableResult
  func setViewportOptions(
    _ options: MapViewportOptions
  ) async throws -> CommandCompletion {
    try await awaitNative {
      try options.nativeInput.withNativeOptions { native in
        try startCommand { mln_map_set_viewport_options($0, native, $1) }
      }
    }
  }

  @discardableResult
  func setTileOptions(
    _ options: MapTileOptions
  ) async throws -> CommandCompletion {
    try await awaitNative {
      try options.nativeInput.withNativeOptions { native in
        try startCommand { mln_map_set_tile_options($0, native, $1) }
      }
    }
  }

  func cameraForLatLngBounds(
    _ bounds: LatLngBounds,
    fitOptions: CameraFitOptions? = nil
  ) async throws -> CameraOptions {
    try await awaitNative {
      try (fitOptions?.nativeInput ?? NativeCameraFitOptionsInput())
        .withOptionalNativeOptions {
          try NativeMap.cameraForLatLngBounds(
            requireLiveHandle(),
            bounds: bounds.nativeInput,
            fitOptions: $0
          )
        }
    }
  }

  func cameraForLatLngs(
    _ coordinates: [LatLng],
    fitOptions: CameraFitOptions? = nil
  ) async throws -> CameraOptions {
    try await awaitNative {
      let native = coordinates.map { $0.nativeInput.native }
      return try native.withUnsafeBufferPointer { coordinates in
        try (fitOptions?.nativeInput ?? NativeCameraFitOptionsInput())
          .withOptionalNativeOptions {
            try NativeMap.cameraForLatLngs(
              requireLiveHandle(),
              coordinates: coordinates.baseAddress,
              count: coordinates.count,
              fitOptions: $0
            )
          }
      }
    }
  }

  func cameraForGeometry(
    _ geometry: Data,
    fitOptions: CameraFitOptions? = nil
  ) async throws -> CameraOptions {
    try await awaitNative {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      return try (fitOptions?.nativeInput ?? NativeCameraFitOptionsInput())
        .withOptionalNativeOptions {
          try NativeMap.cameraForGeometry(
            requireLiveHandle(),
            geometry: arena.view(geometry),
            fitOptions: $0
          )
        }
    }
  }

  /// Returns geographic bounds for a camera from the viewport corners.
  func latLngBounds(
    for camera: CameraOptions,
    unwrapped: Bool = false
  ) async throws -> LatLngBounds {
    try await awaitNative {
      try camera.nativeInput.withNativeOptions {
        try NativeMap.latLngBoundsForCamera(
          requireLiveHandle(),
          camera: $0,
          unwrapped: unwrapped
        )
      }
    }
  }

  @discardableResult
  func setBounds(_ bounds: BoundOptions) async throws -> CommandCompletion {
    try await awaitNative {
      try bounds.nativeInput.withNativeOptions { native in
        try startCommand { mln_map_set_bounds($0, native, $1) }
      }
    }
  }

  @discardableResult
  func setFreeCameraOptions(
    _ options: FreeCameraOptions
  ) async throws -> CommandCompletion {
    try await awaitNative {
      try options.nativeInput.withNativeOptions { native in
        try startCommand { mln_map_set_free_camera_options($0, native, $1) }
      }
    }
  }

  @discardableResult
  func setProjectionMode(
    _ mode: ProjectionMode
  ) async throws -> CommandCompletion {
    try await awaitNative {
      try mode.nativeInput.withNativeMode { native in
        try startCommand { mln_map_set_projection_mode($0, native, $1) }
      }
    }
  }

  func pixel(for coordinate: LatLng) async throws -> ScreenPoint {
    try await awaitNative {
      try NativeMap.pixelForLatLng(
        requireLiveHandle(),
        coordinate: coordinate.nativeInput
      )
    }
  }

  /// Converts a screen point to a geographic coordinate.
  ///
  /// The longitude is wrapped to the range from -180 to 180 degrees unless
  /// `unwrapped` is `true`, in which case it preserves the visible world copy
  /// and may fall outside that range.
  func latLng(
    for point: ScreenPoint,
    unwrapped: Bool = false
  ) async throws -> LatLng {
    try await awaitNative {
      try NativeMap.latLngForPixel(
        requireLiveHandle(),
        point: point.nativeInput,
        unwrapped: unwrapped
      )
    }
  }

  func pixels(for coordinates: [LatLng]) async throws -> [ScreenPoint] {
    try await awaitNative {
      try NativeMap.pixelsForLatLngs(
        requireLiveHandle(),
        coordinates: coordinates.map(\.nativeInput)
      )
    }
  }

  /// Converts screen points to geographic coordinates.
  ///
  /// Each longitude is wrapped to the range from -180 to 180 degrees unless
  /// `unwrapped` is `true`, in which case it preserves its visible world copy
  /// and may fall outside that range.
  func latLngs(
    for points: [ScreenPoint],
    unwrapped: Bool = false
  ) async throws -> [LatLng] {
    try await awaitNative {
      try NativeMap.latLngsForPixels(
        requireLiveHandle(),
        points: points.map(\.nativeInput),
        unwrapped: unwrapped
      )
    }
  }
}
