
internal import CMaplibreNativeC

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

public struct BoundOptions: Equatable, Sendable {
  public var bounds: LatLngBounds?
  public var minZoom: Double?
  public var maxZoom: Double?
  public var minPitch: Double?
  public var maxPitch: Double?

  public init(
    bounds: LatLngBounds? = nil,
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
    bounds = native.bounds.map(LatLngBounds.init(native:))
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
  func setDebugOptions(_ options: MapDebugOptions) throws {
    try mapNativeFailure { try checkStatus(mln_map_set_debug_options(
      requireLivePointer(),
      options.rawValue
    )) }
  }

  func debugOptions() throws -> MapDebugOptions {
    try mapNativeFailure {
      try MapDebugOptions(rawValue: NativeMap
        .debugOptions(requireLivePointer()))
    }
  }

  func setRenderingStatsViewEnabled(_ enabled: Bool) throws {
    try mapNativeFailure {
      try checkStatus(mln_map_set_rendering_stats_view_enabled(
        requireLivePointer(),
        enabled
      ))
    }
  }

  func renderingStatsViewEnabled() throws -> Bool {
    try mapNativeFailure {
      try NativeMap.renderingStatsViewEnabled(requireLivePointer())
    }
  }

  func isFullyLoaded() throws -> Bool {
    try mapNativeFailure { try NativeMap.isFullyLoaded(requireLivePointer()) }
  }

  func dumpDebugLogs() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_dump_debug_logs(requireLivePointer()))
    }
  }

  func viewportOptions() throws -> MapViewportOptions {
    try mapNativeFailure {
      try MapViewportOptions(native: NativeMapViewportOptionsInput(NativeMap
          .viewportOptions(requireLivePointer())))
    }
  }

  func setViewportOptions(_ options: MapViewportOptions) throws {
    try mapNativeFailure {
      try options.nativeInput
        .withNativeOptions { try checkStatus(mln_map_set_viewport_options(
          requireLivePointer(),
          $0
        )) }
    }
  }

  func tileOptions() throws -> MapTileOptions {
    try mapNativeFailure {
      try MapTileOptions(native: NativeMapTileOptionsInput(NativeMap
          .tileOptions(requireLivePointer())))
    }
  }

  func setTileOptions(_ options: MapTileOptions) throws {
    try mapNativeFailure {
      try options.nativeInput
        .withNativeOptions { try checkStatus(mln_map_set_tile_options(
          requireLivePointer(),
          $0
        )) }
    }
  }

  func fly(to camera: CameraOptions,
           animation: AnimationOptions? = nil) throws
  {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try (animation?.nativeInput ?? NativeAnimationOptionsInput())
          .withOptionalNativeOptions { nativeAnimation in
            try checkStatus(mln_map_fly_to(
              requireLivePointer(),
              nativeCamera,
              nativeAnimation
            ))
          }
      }
    }
  }

  func rotateBy(first: ScreenPoint, second: ScreenPoint) throws {
    try mapNativeFailure { try checkStatus(mln_map_rotate_by(
      requireLivePointer(),
      first.nativeInput.native,
      second.nativeInput.native
    )) }
  }

  func rotateBy(
    first: ScreenPoint,
    second: ScreenPoint,
    animation: AnimationOptions
  ) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { animation in
        try checkStatus(mln_map_rotate_by_animated(
          requireLivePointer(),
          first.nativeInput.native,
          second.nativeInput.native,
          animation
        ))
      }
    }
  }

  func pitchBy(_ pitch: Double) throws {
    try mapNativeFailure { try checkStatus(mln_map_pitch_by(
      requireLivePointer(),
      pitch
    )) }
  }

  func pitchBy(_ pitch: Double, animation: AnimationOptions) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { animation in
        try checkStatus(mln_map_pitch_by_animated(
          requireLivePointer(),
          pitch,
          animation
        ))
      }
    }
  }

  func cameraForLatLngBounds(
    _ bounds: LatLngBounds,
    fitOptions: CameraFitOptions? = nil
  ) throws -> CameraOptions {
    try mapNativeFailure {
      try (fitOptions?.nativeInput ?? NativeCameraFitOptionsInput())
        .withOptionalNativeOptions { fitOptions in
          try CameraOptions(native: NativeCameraOptionsInput(NativeMap
              .cameraForLatLngBounds(
                requireLivePointer(),
                bounds: bounds.nativeInput,
                fitOptions: fitOptions
              )))
        }
    }
  }

  func cameraForLatLngs(
    _ coordinates: [LatLng],
    fitOptions: CameraFitOptions? = nil
  ) throws -> CameraOptions {
    try mapNativeFailure {
      let native = coordinates.map { $0.nativeInput.native }
      return try native.withUnsafeBufferPointer { coordinates in
        try (fitOptions?.nativeInput ?? NativeCameraFitOptionsInput())
          .withOptionalNativeOptions { fitOptions in
            try CameraOptions(native: NativeCameraOptionsInput(NativeMap
                .cameraForLatLngs(
                  requireLivePointer(),
                  coordinates: coordinates.baseAddress,
                  count: coordinates.count,
                  fitOptions: fitOptions
                )))
          }
      }
    }
  }

  func cameraForGeometry(
    _ geometry: Geometry,
    fitOptions: CameraFitOptions? = nil
  ) throws -> CameraOptions {
    try mapNativeFailure {
      let arena = NativeInputArena()
      return try (fitOptions?.nativeInput ?? NativeCameraFitOptionsInput())
        .withOptionalNativeOptions { fitOptions in
          try CameraOptions(native: NativeCameraOptionsInput(NativeMap
              .cameraForGeometry(
                requireLivePointer(),
                geometry: arena.allocateGeometry(geometry.nativeGeometry),
                fitOptions: fitOptions
              )))
        }
    }
  }

  func latLngBounds(for camera: CameraOptions,
                    unwrapped: Bool = false) throws -> LatLngBounds
  {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { camera in
        try LatLngBounds(native: NativeMap.latLngBoundsForCamera(
          requireLivePointer(),
          camera: camera,
          unwrapped: unwrapped
        ))
      }
    }
  }

  func bounds() throws -> BoundOptions {
    try mapNativeFailure {
      try BoundOptions(native: NativeBoundOptionsInput(NativeMap
          .bounds(requireLivePointer())))
    }
  }

  func setBounds(_ bounds: BoundOptions) throws {
    try mapNativeFailure {
      try bounds.nativeInput
        .withNativeOptions { try checkStatus(mln_map_set_bounds(
          requireLivePointer(),
          $0
        )) }
    }
  }

  func freeCameraOptions() throws -> FreeCameraOptions {
    try mapNativeFailure {
      try FreeCameraOptions(native: NativeFreeCameraOptionsInput(NativeMap
          .freeCameraOptions(requireLivePointer())))
    }
  }

  func setFreeCameraOptions(_ options: FreeCameraOptions) throws {
    try mapNativeFailure {
      try options.nativeInput
        .withNativeOptions { try checkStatus(mln_map_set_free_camera_options(
          requireLivePointer(),
          $0
        )) }
    }
  }

  func projectionMode() throws -> ProjectionMode {
    try mapNativeFailure {
      try ProjectionMode(native: NativeProjectionModeInput(NativeMap
          .projectionMode(requireLivePointer())))
    }
  }

  func setProjectionMode(_ mode: ProjectionMode) throws {
    try mapNativeFailure {
      try mode.nativeInput
        .withNativeMode { try checkStatus(mln_map_set_projection_mode(
          requireLivePointer(),
          $0
        )) }
    }
  }

  func pixel(for coordinate: LatLng) throws -> ScreenPoint {
    try mapNativeFailure { try ScreenPoint(native: NativeMap.pixelForLatLng(
      requireLivePointer(),
      coordinate: coordinate.nativeInput
    )) }
  }

  func latLng(for point: ScreenPoint) throws -> LatLng {
    try mapNativeFailure { try LatLng(native: NativeMap.latLngForPixel(
      requireLivePointer(),
      point: point.nativeInput
    )) }
  }

  func pixels(for coordinates: [LatLng]) throws -> [ScreenPoint] {
    try mapNativeFailure {
      try NativeMap.pixelsForLatLngs(
        requireLivePointer(),
        coordinates: coordinates.map(\.nativeInput)
      ).map(ScreenPoint.init(native:))
    }
  }

  func latLngs(for points: [ScreenPoint]) throws -> [LatLng] {
    try mapNativeFailure {
      try NativeMap.latLngsForPixels(
        requireLivePointer(),
        points: points.map(\.nativeInput)
      ).map(LatLng.init(native:))
    }
  }
}
