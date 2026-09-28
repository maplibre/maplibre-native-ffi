import CMaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

@Test func advancedCameraDescriptorsMaterializeFieldMasks() {
  do { let native = CameraFitOptions(
    padding: EdgeInsets(top: 1, left: 2, bottom: 3, right: 4),
    bearing: 5,
    pitch: 6
  ).nativeValue()

  #expect(native.fields == (
    MLN_CAMERA_FIT_OPTION_PADDING.rawValue |
      MLN_CAMERA_FIT_OPTION_BEARING.rawValue |
      MLN_CAMERA_FIT_OPTION_PITCH.rawValue
  ))
  #expect(native.padding.left == 2)
  #expect(native.bearing == 5)
  #expect(native.pitch == 6)
  }

  do { let native = BoundOptions(
    bounds: LatLngBounds(
      southwest: LatLng(latitude: 1, longitude: 2),
      northeast: LatLng(latitude: 3, longitude: 4)
    ),
    minZoom: 1,
    maxZoom: 10,
    minPitch: 0,
    maxPitch: 60
  ).nativeValue()
  #expect((native.fields & MLN_BOUND_OPTION_BOUNDS.rawValue) != 0)
  #expect((native.fields & MLN_BOUND_OPTION_UNBOUNDED.rawValue) == 0)
  #expect((native.fields & MLN_BOUND_OPTION_MAX_PITCH.rawValue) != 0)
  #expect(native.bounds.northeast.longitude == 4)
  #expect(native.max_pitch == 60)
  }

  do { let native = BoundOptions(unbounded: true).nativeValue()
    #expect(native.fields == MLN_BOUND_OPTION_UNBOUNDED.rawValue)
  }

  do { let native = FreeCameraOptions(
    position: Vec3(x: 1, y: 2, z: 3),
    orientation: Quaternion(x: 0, y: 0, z: 0, w: 1)
  ).nativeValue()
  #expect(native
    .fields ==
    (MLN_FREE_CAMERA_OPTION_POSITION
      .rawValue | MLN_FREE_CAMERA_OPTION_ORIENTATION.rawValue))
  #expect(native.position.z == 3)
  #expect(native.orientation.w == 1)
  }
}

@Test func animationOptionsDistinguishAbsentAndZeroTransitionId() {
  do { let native = AnimationOptions(transitionId: 0).nativeValue()

    #expect(native.fields == MLN_ANIMATION_OPTION_TRANSITION_ID
      .rawValue)
    #expect(native.transition_id == 0)
  }

  do { let native = AnimationOptions().nativeValue()
    #expect(native.fields == 0)
  }
}

@Test func nativeCameraOptionsPreserveAbsentFieldMasks() {
  var raw = mln_camera_options_default()
  raw.fields = MLN_CAMERA_OPTION_ZOOM.rawValue
  raw.latitude = 12
  raw.longitude = 34
  raw.zoom = 5
  raw.bearing = 90

  let camera = CameraOptions(raw: raw)
  #expect(camera.center == nil)
  #expect(camera.zoom == 5)
  #expect(camera.bearing == nil)

  do { let native = camera.nativeValue()
    #expect(native.fields == MLN_CAMERA_OPTION_ZOOM.rawValue)
    #expect(native.zoom == 5)
  }
}

@Test func mapViewportTileAndProjectionDescriptorsMaterializeFieldMasks() {
  do {
    let native = ProjectionMode(axonometric: true, xSkew: 0.1, ySkew: 0.2)
      .nativeValue()
    #expect(native.fields == (
      MLN_PROJECTION_MODE_AXONOMETRIC.rawValue |
        MLN_PROJECTION_MODE_X_SKEW.rawValue |
        MLN_PROJECTION_MODE_Y_SKEW.rawValue
    ))
    #expect(native.axonometric)
    #expect(native.x_skew == 0.1)
    #expect(native.y_skew == 0.2)
  }

  do { let native = MapViewportOptions(
    northOrientation: .left,
    constrainMode: .screen,
    viewportMode: .flippedY,
    frustumOffset: EdgeInsets(top: 1, left: 2, bottom: 3, right: 4)
  ).nativeValue()
  #expect((native.fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
      .rawValue) != 0)
  #expect(native.north_orientation == MLN_NORTH_ORIENTATION_LEFT
    .rawValue)
  #expect(native.constrain_mode == MLN_CONSTRAIN_MODE_SCREEN.rawValue)
  #expect(native.viewport_mode == MLN_VIEWPORT_MODE_FLIPPED_Y
    .rawValue)
  #expect(native.frustum_offset.right == 4)
  }

  do { let native = MapTileOptions(
    prefetchZoomDelta: 4,
    lodMinRadius: 1,
    lodScale: 2,
    lodPitchThreshold: 3,
    lodZoomShift: 4,
    lodMode: .distance
  ).nativeValue()
  #expect((native.fields & MLN_MAP_TILE_OPTION_LOD_MODE.rawValue) !=
    0)
  #expect(native.prefetch_zoom_delta == 4)
  #expect(native.lod_scale == 2)
  #expect(native.lod_mode == MLN_TILE_LOD_MODE_DISTANCE.rawValue)
  }
}
