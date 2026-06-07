import CMaplibreNativeC
import Testing

@testable import MaplibreNative

@Test func advancedCameraDescriptorsMaterializeFieldMasks() throws {
  try CameraFitOptions(padding: EdgeInsets(top: 1, left: 2, bottom: 3, right: 4), bearing: 5, pitch: 6)
    .nativeInput.withOptionalNativeOptions { native in
      #expect(native != nil)
      #expect(native!.pointee.fields == (
        MLN_CAMERA_FIT_OPTION_PADDING.rawValue |
          MLN_CAMERA_FIT_OPTION_BEARING.rawValue |
          MLN_CAMERA_FIT_OPTION_PITCH.rawValue
      ))
      #expect(native!.pointee.padding.left == 2)
      #expect(native!.pointee.bearing == 5)
      #expect(native!.pointee.pitch == 6)
    }

  try BoundOptions(
    bounds: LatLngBounds(southwest: LatLng(latitude: 1, longitude: 2), northeast: LatLng(latitude: 3, longitude: 4)),
    minZoom: 1,
    maxZoom: 10,
    minPitch: 0,
    maxPitch: 60
  ).nativeInput.withNativeOptions { native in
    #expect((native.pointee.fields & MLN_BOUND_OPTION_BOUNDS.rawValue) != 0)
    #expect((native.pointee.fields & MLN_BOUND_OPTION_MAX_PITCH.rawValue) != 0)
    #expect(native.pointee.bounds.northeast.longitude == 4)
    #expect(native.pointee.max_pitch == 60)
  }

  try FreeCameraOptions(
    position: Vec3(x: 1, y: 2, z: 3),
    orientation: Quaternion(x: 0, y: 0, z: 0, w: 1)
  ).nativeInput.withNativeOptions { native in
    #expect(native.pointee.fields == (MLN_FREE_CAMERA_OPTION_POSITION.rawValue | MLN_FREE_CAMERA_OPTION_ORIENTATION.rawValue))
    #expect(native.pointee.position.z == 3)
    #expect(native.pointee.orientation.w == 1)
  }
}

@Test func nativeCameraOptionsPreserveAbsentFieldMasks() throws {
  var raw = mln_camera_options_default()
  raw.fields = MLN_CAMERA_OPTION_ZOOM.rawValue
  raw.latitude = 12
  raw.longitude = 34
  raw.zoom = 5
  raw.bearing = 90

  let camera = CameraOptions(native: NativeCameraOptionsInput(raw))
  #expect(camera.center == nil)
  #expect(camera.zoom == 5)
  #expect(camera.bearing == nil)

  try camera.nativeInput.withNativeOptions { native in
    #expect(native.pointee.fields == MLN_CAMERA_OPTION_ZOOM.rawValue)
    #expect(native.pointee.zoom == 5)
  }
}

@Test func mapViewportTileAndProjectionDescriptorsMaterializeFieldMasks() throws {
  try ProjectionMode(axonometric: true, xSkew: 0.1, ySkew: 0.2).nativeInput.withNativeMode { native in
    #expect(native.pointee.fields == (
      MLN_PROJECTION_MODE_AXONOMETRIC.rawValue |
        MLN_PROJECTION_MODE_X_SKEW.rawValue |
        MLN_PROJECTION_MODE_Y_SKEW.rawValue
    ))
    #expect(native.pointee.axonometric)
    #expect(native.pointee.x_skew == 0.1)
    #expect(native.pointee.y_skew == 0.2)
  }

  try MapViewportOptions(
    northOrientation: .left,
    constrainMode: .screen,
    viewportMode: .flippedY,
    frustumOffset: EdgeInsets(top: 1, left: 2, bottom: 3, right: 4)
  ).nativeInput.withNativeOptions { native in
    #expect((native.pointee.fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET.rawValue) != 0)
    #expect(native.pointee.north_orientation == MLN_NORTH_ORIENTATION_LEFT.rawValue)
    #expect(native.pointee.constrain_mode == MLN_CONSTRAIN_MODE_SCREEN.rawValue)
    #expect(native.pointee.viewport_mode == MLN_VIEWPORT_MODE_FLIPPED_Y.rawValue)
    #expect(native.pointee.frustum_offset.right == 4)
  }

  try MapTileOptions(
    prefetchZoomDelta: 4,
    lodMinRadius: 1,
    lodScale: 2,
    lodPitchThreshold: 3,
    lodZoomShift: 4,
    lodMode: .distance
  ).nativeInput.withNativeOptions { native in
    #expect((native.pointee.fields & MLN_MAP_TILE_OPTION_LOD_MODE.rawValue) != 0)
    #expect(native.pointee.prefetch_zoom_delta == 4)
    #expect(native.pointee.lod_scale == 2)
    #expect(native.pointee.lod_mode == MLN_TILE_LOD_MODE_DISTANCE.rawValue)
  }
}
