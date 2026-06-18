package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.alloc
import kotlinx.cinterop.ptr
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.internal.c.MLN_ANIMATION_OPTION_DURATION
import org.maplibre.nativeffi.internal.c.MLN_ANIMATION_OPTION_EASING
import org.maplibre.nativeffi.internal.c.MLN_ANIMATION_OPTION_MIN_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_ANIMATION_OPTION_VELOCITY
import org.maplibre.nativeffi.internal.c.MLN_BOUND_OPTION_BOUNDS
import org.maplibre.nativeffi.internal.c.MLN_BOUND_OPTION_MAX_PITCH
import org.maplibre.nativeffi.internal.c.MLN_BOUND_OPTION_MAX_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_BOUND_OPTION_MIN_PITCH
import org.maplibre.nativeffi.internal.c.MLN_BOUND_OPTION_MIN_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_FIT_OPTION_BEARING
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_FIT_OPTION_PADDING
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_FIT_OPTION_PITCH
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_ANCHOR
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_BEARING
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_CENTER
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_CENTER_ALTITUDE
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_FOV
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_PADDING
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_PITCH
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_ROLL
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_FREE_CAMERA_OPTION_ORIENTATION
import org.maplibre.nativeffi.internal.c.MLN_FREE_CAMERA_OPTION_POSITION
import org.maplibre.nativeffi.internal.c.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
import org.maplibre.nativeffi.internal.c.MLN_MAP_TILE_OPTION_LOD_MODE
import org.maplibre.nativeffi.internal.c.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
import org.maplibre.nativeffi.internal.c.MLN_MAP_TILE_OPTION_LOD_SCALE
import org.maplibre.nativeffi.internal.c.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
import org.maplibre.nativeffi.internal.c.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
import org.maplibre.nativeffi.internal.c.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
import org.maplibre.nativeffi.internal.c.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
import org.maplibre.nativeffi.internal.c.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
import org.maplibre.nativeffi.internal.c.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
import org.maplibre.nativeffi.internal.c.MLN_PROJECTION_MODE_AXONOMETRIC
import org.maplibre.nativeffi.internal.c.MLN_PROJECTION_MODE_X_SKEW
import org.maplibre.nativeffi.internal.c.MLN_PROJECTION_MODE_Y_SKEW
import org.maplibre.nativeffi.internal.c.mln_animation_options
import org.maplibre.nativeffi.internal.c.mln_animation_options_default
import org.maplibre.nativeffi.internal.c.mln_bound_options
import org.maplibre.nativeffi.internal.c.mln_bound_options_default
import org.maplibre.nativeffi.internal.c.mln_camera_fit_options
import org.maplibre.nativeffi.internal.c.mln_camera_fit_options_default
import org.maplibre.nativeffi.internal.c.mln_camera_options
import org.maplibre.nativeffi.internal.c.mln_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_free_camera_options
import org.maplibre.nativeffi.internal.c.mln_free_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_map_tile_options
import org.maplibre.nativeffi.internal.c.mln_map_tile_options_default
import org.maplibre.nativeffi.internal.c.mln_map_viewport_options
import org.maplibre.nativeffi.internal.c.mln_map_viewport_options_default
import org.maplibre.nativeffi.internal.c.mln_projection_mode
import org.maplibre.nativeffi.internal.c.mln_projection_mode_default
import org.maplibre.nativeffi.map.ConstrainMode
import org.maplibre.nativeffi.map.NorthOrientation
import org.maplibre.nativeffi.map.ProjectionModeOptions
import org.maplibre.nativeffi.map.TileLodMode
import org.maplibre.nativeffi.map.TileOptions
import org.maplibre.nativeffi.map.ViewportMode
import org.maplibre.nativeffi.map.ViewportOptions

/** Materializes map and camera descriptors at the C boundary. */
@OptIn(ExperimentalForeignApi::class)
internal object MapStructs {
  fun animationOptions(value: AnimationOptions, scope: MemScope): CPointer<mln_animation_options> {
    val native = scope.alloc<mln_animation_options>()
    mln_animation_options_default().place(native.ptr)
    value.durationMs?.let {
      native.fields = native.fields or MLN_ANIMATION_OPTION_DURATION
      native.duration_ms = it
    }
    value.velocity?.let {
      native.fields = native.fields or MLN_ANIMATION_OPTION_VELOCITY
      native.velocity = it
    }
    value.minZoom?.let {
      native.fields = native.fields or MLN_ANIMATION_OPTION_MIN_ZOOM
      native.min_zoom = it
    }
    value.easing?.let {
      native.fields = native.fields or MLN_ANIMATION_OPTION_EASING
      native.easing.x1 = it.x1
      native.easing.y1 = it.y1
      native.easing.x2 = it.x2
      native.easing.y2 = it.y2
    }
    return native.ptr
  }

  fun cameraOptions(value: CameraOptions, scope: MemScope): CPointer<mln_camera_options> {
    val native = scope.alloc<mln_camera_options>()
    mln_camera_options_default().place(native.ptr)
    value.center?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_CENTER
      native.latitude = it.latitude
      native.longitude = it.longitude
    }
    value.centerAltitude?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_CENTER_ALTITUDE
      native.center_altitude = it
    }
    value.padding?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_PADDING
      native.padding.top = it.top
      native.padding.left = it.left
      native.padding.bottom = it.bottom
      native.padding.right = it.right
    }
    value.anchor?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_ANCHOR
      native.anchor.x = it.x
      native.anchor.y = it.y
    }
    value.zoom?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_ZOOM
      native.zoom = it
    }
    value.bearing?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_BEARING
      native.bearing = it
    }
    value.pitch?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_PITCH
      native.pitch = it
    }
    value.roll?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_ROLL
      native.roll = it
    }
    value.fieldOfView?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_FOV
      native.field_of_view = it
    }
    return native.ptr
  }

  fun cameraFitOptions(value: CameraFitOptions, scope: MemScope): CPointer<mln_camera_fit_options> {
    val native = scope.alloc<mln_camera_fit_options>()
    mln_camera_fit_options_default().place(native.ptr)
    value.padding?.let {
      native.fields = native.fields or MLN_CAMERA_FIT_OPTION_PADDING
      native.padding.top = it.top
      native.padding.left = it.left
      native.padding.bottom = it.bottom
      native.padding.right = it.right
    }
    value.bearing?.let {
      native.fields = native.fields or MLN_CAMERA_FIT_OPTION_BEARING
      native.bearing = it
    }
    value.pitch?.let {
      native.fields = native.fields or MLN_CAMERA_FIT_OPTION_PITCH
      native.pitch = it
    }
    return native.ptr
  }

  fun boundOptions(value: BoundOptions, scope: MemScope): CPointer<mln_bound_options> {
    val native = scope.alloc<mln_bound_options>()
    mln_bound_options_default().place(native.ptr)
    value.bounds?.let {
      native.fields = native.fields or MLN_BOUND_OPTION_BOUNDS
      native.bounds.southwest.latitude = it.southwest.latitude
      native.bounds.southwest.longitude = it.southwest.longitude
      native.bounds.northeast.latitude = it.northeast.latitude
      native.bounds.northeast.longitude = it.northeast.longitude
    }
    value.minZoom?.let {
      native.fields = native.fields or MLN_BOUND_OPTION_MIN_ZOOM
      native.min_zoom = it
    }
    value.maxZoom?.let {
      native.fields = native.fields or MLN_BOUND_OPTION_MAX_ZOOM
      native.max_zoom = it
    }
    value.minPitch?.let {
      native.fields = native.fields or MLN_BOUND_OPTION_MIN_PITCH
      native.min_pitch = it
    }
    value.maxPitch?.let {
      native.fields = native.fields or MLN_BOUND_OPTION_MAX_PITCH
      native.max_pitch = it
    }
    return native.ptr
  }

  fun boundOptions(value: mln_bound_options): BoundOptions {
    val options = BoundOptions()
    if ((value.fields and MLN_BOUND_OPTION_BOUNDS) != 0U)
      options.bounds = CoreStructs.latLngBounds(value.bounds)
    if ((value.fields and MLN_BOUND_OPTION_MIN_ZOOM) != 0U) options.minZoom = value.min_zoom
    if ((value.fields and MLN_BOUND_OPTION_MAX_ZOOM) != 0U) options.maxZoom = value.max_zoom
    if ((value.fields and MLN_BOUND_OPTION_MIN_PITCH) != 0U) options.minPitch = value.min_pitch
    if ((value.fields and MLN_BOUND_OPTION_MAX_PITCH) != 0U) options.maxPitch = value.max_pitch
    return options
  }

  fun freeCameraOptions(
    value: FreeCameraOptions,
    scope: MemScope,
  ): CPointer<mln_free_camera_options> {
    val native = scope.alloc<mln_free_camera_options>()
    mln_free_camera_options_default().place(native.ptr)
    value.position?.let {
      native.fields = native.fields or MLN_FREE_CAMERA_OPTION_POSITION
      native.position.x = it.x
      native.position.y = it.y
      native.position.z = it.z
    }
    value.orientation?.let {
      native.fields = native.fields or MLN_FREE_CAMERA_OPTION_ORIENTATION
      native.orientation.x = it.x
      native.orientation.y = it.y
      native.orientation.z = it.z
      native.orientation.w = it.w
    }
    return native.ptr
  }

  fun freeCameraOptions(value: mln_free_camera_options): FreeCameraOptions {
    val options = FreeCameraOptions()
    if ((value.fields and MLN_FREE_CAMERA_OPTION_POSITION) != 0U) {
      options.position =
        org.maplibre.nativeffi.geo.Vec3(value.position.x, value.position.y, value.position.z)
    }
    if ((value.fields and MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0U) {
      options.orientation =
        org.maplibre.nativeffi.geo.Quaternion(
          value.orientation.x,
          value.orientation.y,
          value.orientation.z,
          value.orientation.w,
        )
    }
    return options
  }

  fun viewportOptions(value: ViewportOptions, scope: MemScope): CPointer<mln_map_viewport_options> {
    val native = scope.alloc<mln_map_viewport_options>()
    mln_map_viewport_options_default().place(native.ptr)
    value.northOrientation?.let {
      require(it.isKnown) { "Unknown north orientation cannot be used as input: ${it.nativeValue}" }
      native.fields = native.fields or MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
      native.north_orientation = it.nativeValue.toUInt()
    }
    value.constrainMode?.let {
      require(it.isKnown) { "Unknown constrain mode cannot be used as input: ${it.nativeValue}" }
      native.fields = native.fields or MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
      native.constrain_mode = it.nativeValue.toUInt()
    }
    value.viewportMode?.let {
      require(it.isKnown) { "Unknown viewport mode cannot be used as input: ${it.nativeValue}" }
      native.fields = native.fields or MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
      native.viewport_mode = it.nativeValue.toUInt()
    }
    value.frustumOffset?.let {
      native.fields = native.fields or MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
      native.frustum_offset.top = it.top
      native.frustum_offset.left = it.left
      native.frustum_offset.bottom = it.bottom
      native.frustum_offset.right = it.right
    }
    return native.ptr
  }

  fun viewportOptions(value: mln_map_viewport_options): ViewportOptions {
    val options = ViewportOptions()
    if ((value.fields and MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0U) {
      options.northOrientation = NorthOrientation.fromNative(value.north_orientation)
    }
    if ((value.fields and MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0U) {
      options.constrainMode = ConstrainMode.fromNative(value.constrain_mode)
    }
    if ((value.fields and MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0U) {
      options.viewportMode = ViewportMode.fromNative(value.viewport_mode)
    }
    if ((value.fields and MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0U) {
      options.frustumOffset = CoreStructs.edgeInsets(value.frustum_offset)
    }
    return options
  }

  fun tileOptions(value: TileOptions, scope: MemScope): CPointer<mln_map_tile_options> {
    val native = scope.alloc<mln_map_tile_options>()
    mln_map_tile_options_default().place(native.ptr)
    value.prefetchZoomDelta?.let {
      native.fields = native.fields or MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
      native.prefetch_zoom_delta = it.toUInt()
    }
    value.lodMinRadius?.let {
      native.fields = native.fields or MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
      native.lod_min_radius = it
    }
    value.lodScale?.let {
      native.fields = native.fields or MLN_MAP_TILE_OPTION_LOD_SCALE
      native.lod_scale = it
    }
    value.lodPitchThreshold?.let {
      native.fields = native.fields or MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
      native.lod_pitch_threshold = it
    }
    value.lodZoomShift?.let {
      native.fields = native.fields or MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
      native.lod_zoom_shift = it
    }
    value.lodMode?.let {
      require(it.isKnown) { "Unknown tile LOD mode cannot be used as input: ${it.nativeValue}" }
      native.fields = native.fields or MLN_MAP_TILE_OPTION_LOD_MODE
      native.lod_mode = it.nativeValue.toUInt()
    }
    return native.ptr
  }

  fun tileOptions(value: mln_map_tile_options): TileOptions {
    val options = TileOptions()
    if ((value.fields and MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0U) {
      options.prefetchZoomDelta = value.prefetch_zoom_delta.toInt()
    }
    if ((value.fields and MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0U) {
      options.lodMinRadius = value.lod_min_radius
    }
    if ((value.fields and MLN_MAP_TILE_OPTION_LOD_SCALE) != 0U) {
      options.lodScale = value.lod_scale
    }
    if ((value.fields and MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0U) {
      options.lodPitchThreshold = value.lod_pitch_threshold
    }
    if ((value.fields and MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0U) {
      options.lodZoomShift = value.lod_zoom_shift
    }
    if ((value.fields and MLN_MAP_TILE_OPTION_LOD_MODE) != 0U) {
      options.lodMode = TileLodMode.fromNative(value.lod_mode)
    }
    return options
  }

  fun projectionModeOptions(
    value: ProjectionModeOptions,
    scope: MemScope,
  ): CPointer<mln_projection_mode> {
    val native = scope.alloc<mln_projection_mode>()
    mln_projection_mode_default().place(native.ptr)
    value.axonometric?.let {
      native.fields = native.fields or MLN_PROJECTION_MODE_AXONOMETRIC
      native.axonometric = it
    }
    value.xSkew?.let {
      native.fields = native.fields or MLN_PROJECTION_MODE_X_SKEW
      native.x_skew = it
    }
    value.ySkew?.let {
      native.fields = native.fields or MLN_PROJECTION_MODE_Y_SKEW
      native.y_skew = it
    }
    return native.ptr
  }

  fun projectionModeOptions(value: mln_projection_mode): ProjectionModeOptions {
    val options = ProjectionModeOptions()
    if ((value.fields and MLN_PROJECTION_MODE_AXONOMETRIC) != 0U) {
      options.axonometric = value.axonometric
    }
    if ((value.fields and MLN_PROJECTION_MODE_X_SKEW) != 0U) {
      options.xSkew = value.x_skew
    }
    if ((value.fields and MLN_PROJECTION_MODE_Y_SKEW) != 0U) {
      options.ySkew = value.y_skew
    }
    return options
  }

  fun cameraOptions(value: mln_camera_options): CameraOptions {
    val camera = CameraOptions()
    if ((value.fields and MLN_CAMERA_OPTION_CENTER) != 0U) {
      camera.center = org.maplibre.nativeffi.geo.LatLng(value.latitude, value.longitude)
    }
    if ((value.fields and MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0U) {
      camera.centerAltitude = value.center_altitude
    }
    if ((value.fields and MLN_CAMERA_OPTION_PADDING) != 0U) {
      camera.padding = CoreStructs.edgeInsets(value.padding)
    }
    if ((value.fields and MLN_CAMERA_OPTION_ANCHOR) != 0U) {
      camera.anchor = CoreStructs.screenPoint(value.anchor)
    }
    if ((value.fields and MLN_CAMERA_OPTION_ZOOM) != 0U) {
      camera.zoom = value.zoom
    }
    if ((value.fields and MLN_CAMERA_OPTION_BEARING) != 0U) {
      camera.bearing = value.bearing
    }
    if ((value.fields and MLN_CAMERA_OPTION_PITCH) != 0U) {
      camera.pitch = value.pitch
    }
    if ((value.fields and MLN_CAMERA_OPTION_ROLL) != 0U) {
      camera.roll = value.roll
    }
    if ((value.fields and MLN_CAMERA_OPTION_FOV) != 0U) {
      camera.fieldOfView = value.field_of_view
    }
    return camera
  }
}
