import 'dart:ffi';

import '../../camera/camera.dart';
import '../../geo/geo.dart';
import '../c/maplibre_native_c.g.dart' as raw;

/// Converts a Dart coordinate to the native C value shape.
raw.mln_lat_lng latLngToNative(LatLng value) {
  final result = Struct.create<raw.mln_lat_lng>();
  result.latitude = value.latitude;
  result.longitude = value.longitude;
  return result;
}

/// Converts a native coordinate to a Dart value.
LatLng latLngFromNative(raw.mln_lat_lng value) =>
    LatLng(value.latitude, value.longitude);

/// Converts Dart bounds to the native C value shape.
raw.mln_lat_lng_bounds latLngBoundsToNative(LatLngBounds value) {
  final result = Struct.create<raw.mln_lat_lng_bounds>();
  result.southwest = latLngToNative(value.southwest);
  result.northeast = latLngToNative(value.northeast);
  return result;
}

/// Converts a native bounds value to Dart.
LatLngBounds latLngBoundsFromNative(raw.mln_lat_lng_bounds value) =>
    LatLngBounds(
      latLngFromNative(value.southwest),
      latLngFromNative(value.northeast),
    );

/// Converts Dart projected meters to the native C value shape.
raw.mln_projected_meters projectedMetersToNative(ProjectedMeters value) {
  final result = Struct.create<raw.mln_projected_meters>();
  result.northing = value.northing;
  result.easting = value.easting;
  return result;
}

/// Converts native projected meters to Dart.
ProjectedMeters projectedMetersFromNative(raw.mln_projected_meters value) =>
    ProjectedMeters(value.northing, value.easting);

/// Converts Dart screen point to the native C value shape.
raw.mln_screen_point screenPointToNative(ScreenPoint value) {
  final result = Struct.create<raw.mln_screen_point>();
  result.x = value.x;
  result.y = value.y;
  return result;
}

/// Converts a native screen point to Dart.
ScreenPoint screenPointFromNative(raw.mln_screen_point value) =>
    ScreenPoint(value.x, value.y);

/// Converts Dart edge insets to the native C value shape.
raw.mln_edge_insets edgeInsetsToNative(EdgeInsets value) {
  final result = Struct.create<raw.mln_edge_insets>();
  result.top = value.top;
  result.left = value.left;
  result.bottom = value.bottom;
  result.right = value.right;
  return result;
}

/// Converts native edge insets to Dart.
EdgeInsets edgeInsetsFromNative(raw.mln_edge_insets value) => EdgeInsets(
  top: value.top,
  left: value.left,
  bottom: value.bottom,
  right: value.right,
);

/// Converts Dart vector to the native C value shape.
raw.mln_vec3 vec3ToNative(Vec3 value) {
  final result = Struct.create<raw.mln_vec3>();
  result.x = value.x;
  result.y = value.y;
  result.z = value.z;
  return result;
}

/// Converts Dart quaternion to the native C value shape.
raw.mln_quaternion quaternionToNative(Quaternion value) {
  final result = Struct.create<raw.mln_quaternion>();
  result.x = value.x;
  result.y = value.y;
  result.z = value.z;
  result.w = value.w;
  return result;
}

/// Converts Dart unit bezier to the native C value shape.
raw.mln_unit_bezier unitBezierToNative(UnitBezier value) {
  final result = Struct.create<raw.mln_unit_bezier>();
  result.x1 = value.x1;
  result.y1 = value.y1;
  result.x2 = value.x2;
  result.y2 = value.y2;
  return result;
}

/// Materializes Dart camera options into a native C struct.
raw.mln_camera_options cameraOptionsToNative(
  CameraOptions value,
  raw.mln_camera_options result,
) {
  final center = value.center;
  if (center != null) {
    result.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_CENTER.value;
    result.latitude = center.latitude;
    result.longitude = center.longitude;
  }
  final zoom = value.zoom;
  if (zoom != null) {
    result.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_ZOOM.value;
    result.zoom = zoom;
  }
  final bearing = value.bearing;
  if (bearing != null) {
    result.fields |=
        raw.mln_camera_option_field.MLN_CAMERA_OPTION_BEARING.value;
    result.bearing = bearing;
  }
  final pitch = value.pitch;
  if (pitch != null) {
    result.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_PITCH.value;
    result.pitch = pitch;
  }
  final centerAltitude = value.centerAltitude;
  if (centerAltitude != null) {
    result.fields |=
        raw.mln_camera_option_field.MLN_CAMERA_OPTION_CENTER_ALTITUDE.value;
    result.center_altitude = centerAltitude;
  }
  final padding = value.padding;
  if (padding != null) {
    result.fields |=
        raw.mln_camera_option_field.MLN_CAMERA_OPTION_PADDING.value;
    result.padding = edgeInsetsToNative(padding);
  }
  final anchor = value.anchor;
  if (anchor != null) {
    result.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_ANCHOR.value;
    result.anchor = screenPointToNative(anchor);
  }
  final roll = value.roll;
  if (roll != null) {
    result.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_ROLL.value;
    result.roll = roll;
  }
  final fieldOfView = value.fieldOfView;
  if (fieldOfView != null) {
    result.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_FOV.value;
    result.field_of_view = fieldOfView;
  }
  return result;
}

/// Copies native camera options into a Dart value.
CameraOptions cameraOptionsFromNative(raw.mln_camera_options value) {
  final fields = value.fields;
  return CameraOptions(
    center:
        (fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_CENTER.value) ==
            0
        ? null
        : LatLng(value.latitude, value.longitude),
    zoom:
        (fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_ZOOM.value) == 0
        ? null
        : value.zoom,
    bearing:
        (fields &
                raw.mln_camera_option_field.MLN_CAMERA_OPTION_BEARING.value) ==
            0
        ? null
        : value.bearing,
    pitch:
        (fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_PITCH.value) ==
            0
        ? null
        : value.pitch,
    centerAltitude:
        (fields &
                raw
                    .mln_camera_option_field
                    .MLN_CAMERA_OPTION_CENTER_ALTITUDE
                    .value) ==
            0
        ? null
        : value.center_altitude,
    padding:
        (fields &
                raw.mln_camera_option_field.MLN_CAMERA_OPTION_PADDING.value) ==
            0
        ? null
        : edgeInsetsFromNative(value.padding),
    anchor:
        (fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_ANCHOR.value) ==
            0
        ? null
        : screenPointFromNative(value.anchor),
    roll:
        (fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_ROLL.value) == 0
        ? null
        : value.roll,
    fieldOfView:
        (fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_FOV.value) == 0
        ? null
        : value.field_of_view,
  );
}

/// Materializes Dart camera fit options into a native C struct.
raw.mln_camera_fit_options cameraFitOptionsToNative(
  CameraFitOptions value,
  raw.mln_camera_fit_options result,
) {
  final padding = value.padding;
  if (padding != null) {
    result.fields |=
        raw.mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_PADDING.value;
    result.padding = edgeInsetsToNative(padding);
  }
  final bearing = value.bearing;
  if (bearing != null) {
    result.fields |=
        raw.mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_BEARING.value;
    result.bearing = bearing;
  }
  final pitch = value.pitch;
  if (pitch != null) {
    result.fields |=
        raw.mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_PITCH.value;
    result.pitch = pitch;
  }
  return result;
}

/// Materializes Dart bound options into a native C struct.
raw.mln_bound_options boundOptionsToNative(
  BoundOptions value,
  raw.mln_bound_options result,
) {
  final bounds = value.bounds;
  if (bounds != null) {
    result.fields |= raw.mln_bound_option_field.MLN_BOUND_OPTION_BOUNDS.value;
    result.bounds = latLngBoundsToNative(bounds);
  }
  final minZoom = value.minZoom;
  if (minZoom != null) {
    result.fields |= raw.mln_bound_option_field.MLN_BOUND_OPTION_MIN_ZOOM.value;
    result.min_zoom = minZoom;
  }
  final maxZoom = value.maxZoom;
  if (maxZoom != null) {
    result.fields |= raw.mln_bound_option_field.MLN_BOUND_OPTION_MAX_ZOOM.value;
    result.max_zoom = maxZoom;
  }
  final minPitch = value.minPitch;
  if (minPitch != null) {
    result.fields |=
        raw.mln_bound_option_field.MLN_BOUND_OPTION_MIN_PITCH.value;
    result.min_pitch = minPitch;
  }
  final maxPitch = value.maxPitch;
  if (maxPitch != null) {
    result.fields |=
        raw.mln_bound_option_field.MLN_BOUND_OPTION_MAX_PITCH.value;
    result.max_pitch = maxPitch;
  }
  return result;
}

/// Copies native bound options into Dart.
BoundOptions boundOptionsFromNative(raw.mln_bound_options value) {
  final fields = value.fields;
  return BoundOptions(
    bounds:
        (fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_BOUNDS.value) == 0
        ? null
        : latLngBoundsFromNative(value.bounds),
    minZoom:
        (fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_MIN_ZOOM.value) ==
            0
        ? null
        : value.min_zoom,
    maxZoom:
        (fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_MAX_ZOOM.value) ==
            0
        ? null
        : value.max_zoom,
    minPitch:
        (fields &
                raw.mln_bound_option_field.MLN_BOUND_OPTION_MIN_PITCH.value) ==
            0
        ? null
        : value.min_pitch,
    maxPitch:
        (fields &
                raw.mln_bound_option_field.MLN_BOUND_OPTION_MAX_PITCH.value) ==
            0
        ? null
        : value.max_pitch,
  );
}

/// Materializes Dart free camera options into a native C struct.
raw.mln_free_camera_options freeCameraOptionsToNative(
  FreeCameraOptions value,
  raw.mln_free_camera_options result,
) {
  final position = value.position;
  if (position != null) {
    result.fields |=
        raw.mln_free_camera_option_field.MLN_FREE_CAMERA_OPTION_POSITION.value;
    result.position = vec3ToNative(position);
  }
  final orientation = value.orientation;
  if (orientation != null) {
    result.fields |= raw
        .mln_free_camera_option_field
        .MLN_FREE_CAMERA_OPTION_ORIENTATION
        .value;
    result.orientation = quaternionToNative(orientation);
  }
  return result;
}

/// Copies native free camera options into Dart.
FreeCameraOptions freeCameraOptionsFromNative(
  raw.mln_free_camera_options value,
) {
  final fields = value.fields;
  return FreeCameraOptions(
    position:
        (fields &
                raw
                    .mln_free_camera_option_field
                    .MLN_FREE_CAMERA_OPTION_POSITION
                    .value) ==
            0
        ? null
        : Vec3(value.position.x, value.position.y, value.position.z),
    orientation:
        (fields &
                raw
                    .mln_free_camera_option_field
                    .MLN_FREE_CAMERA_OPTION_ORIENTATION
                    .value) ==
            0
        ? null
        : Quaternion(
            value.orientation.x,
            value.orientation.y,
            value.orientation.z,
            value.orientation.w,
          ),
  );
}

/// Materializes Dart projection mode options into a native C struct.
raw.mln_projection_mode projectionModeOptionsToNative(
  ProjectionModeOptions value,
  raw.mln_projection_mode result,
) {
  final axonometric = value.axonometric;
  if (axonometric != null) {
    result.fields |=
        raw.mln_projection_mode_field.MLN_PROJECTION_MODE_AXONOMETRIC.value;
    result.axonometric = axonometric;
  }
  final xSkew = value.xSkew;
  if (xSkew != null) {
    result.fields |=
        raw.mln_projection_mode_field.MLN_PROJECTION_MODE_X_SKEW.value;
    result.x_skew = xSkew;
  }
  final ySkew = value.ySkew;
  if (ySkew != null) {
    result.fields |=
        raw.mln_projection_mode_field.MLN_PROJECTION_MODE_Y_SKEW.value;
    result.y_skew = ySkew;
  }
  return result;
}

/// Copies native projection mode options into Dart.
ProjectionModeOptions projectionModeOptionsFromNative(
  raw.mln_projection_mode value,
) {
  final fields = value.fields;
  return ProjectionModeOptions(
    axonometric:
        (fields &
                raw
                    .mln_projection_mode_field
                    .MLN_PROJECTION_MODE_AXONOMETRIC
                    .value) ==
            0
        ? null
        : value.axonometric,
    xSkew:
        (fields &
                raw
                    .mln_projection_mode_field
                    .MLN_PROJECTION_MODE_X_SKEW
                    .value) ==
            0
        ? null
        : value.x_skew,
    ySkew:
        (fields &
                raw
                    .mln_projection_mode_field
                    .MLN_PROJECTION_MODE_Y_SKEW
                    .value) ==
            0
        ? null
        : value.y_skew,
  );
}

/// Materializes Dart viewport options into a native C struct.
raw.mln_map_viewport_options mapViewportOptionsToNative(
  MapViewportOptions value,
  raw.mln_map_viewport_options result,
) {
  final northOrientation = value.northOrientation;
  if (northOrientation != null) {
    result.fields |= raw
        .mln_map_viewport_option_field
        .MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
        .value;
    result.north_orientation = northOrientation.rawValue;
  }
  final constrainMode = value.constrainMode;
  if (constrainMode != null) {
    result.fields |= raw
        .mln_map_viewport_option_field
        .MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
        .value;
    result.constrain_mode = constrainMode.rawValue;
  }
  final viewportMode = value.viewportMode;
  if (viewportMode != null) {
    result.fields |= raw
        .mln_map_viewport_option_field
        .MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
        .value;
    result.viewport_mode = viewportMode.rawValue;
  }
  final frustumOffset = value.frustumOffset;
  if (frustumOffset != null) {
    result.fields |= raw
        .mln_map_viewport_option_field
        .MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
        .value;
    result.frustum_offset = edgeInsetsToNative(frustumOffset);
  }
  return result;
}

/// Copies native viewport options into Dart.
MapViewportOptions mapViewportOptionsFromNative(
  raw.mln_map_viewport_options value,
) {
  final fields = value.fields;
  return MapViewportOptions(
    northOrientation:
        (fields &
                raw
                    .mln_map_viewport_option_field
                    .MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
                    .value) ==
            0
        ? null
        : NorthOrientation.fromRawValue(value.north_orientation),
    constrainMode:
        (fields &
                raw
                    .mln_map_viewport_option_field
                    .MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
                    .value) ==
            0
        ? null
        : ConstrainMode.fromRawValue(value.constrain_mode),
    viewportMode:
        (fields &
                raw
                    .mln_map_viewport_option_field
                    .MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
                    .value) ==
            0
        ? null
        : ViewportMode.fromRawValue(value.viewport_mode),
    frustumOffset:
        (fields &
                raw
                    .mln_map_viewport_option_field
                    .MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
                    .value) ==
            0
        ? null
        : edgeInsetsFromNative(value.frustum_offset),
  );
}

/// Materializes Dart tile options into a native C struct.
raw.mln_map_tile_options mapTileOptionsToNative(
  MapTileOptions value,
  raw.mln_map_tile_options result,
) {
  final prefetchZoomDelta = value.prefetchZoomDelta;
  if (prefetchZoomDelta != null) {
    result.fields |= raw
        .mln_map_tile_option_field
        .MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
        .value;
    result.prefetch_zoom_delta = prefetchZoomDelta;
  }
  final lodMinRadius = value.lodMinRadius;
  if (lodMinRadius != null) {
    result.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS.value;
    result.lod_min_radius = lodMinRadius;
  }
  final lodScale = value.lodScale;
  if (lodScale != null) {
    result.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_SCALE.value;
    result.lod_scale = lodScale;
  }
  final lodPitchThreshold = value.lodPitchThreshold;
  if (lodPitchThreshold != null) {
    result.fields |= raw
        .mln_map_tile_option_field
        .MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
        .value;
    result.lod_pitch_threshold = lodPitchThreshold;
  }
  final lodZoomShift = value.lodZoomShift;
  if (lodZoomShift != null) {
    result.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT.value;
    result.lod_zoom_shift = lodZoomShift;
  }
  final lodMode = value.lodMode;
  if (lodMode != null) {
    result.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_MODE.value;
    result.lod_mode = lodMode.rawValue;
  }
  return result;
}

/// Copies native tile options into Dart.
MapTileOptions mapTileOptionsFromNative(raw.mln_map_tile_options value) {
  final fields = value.fields;
  return MapTileOptions(
    prefetchZoomDelta:
        (fields &
                raw
                    .mln_map_tile_option_field
                    .MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
                    .value) ==
            0
        ? null
        : value.prefetch_zoom_delta,
    lodMinRadius:
        (fields &
                raw
                    .mln_map_tile_option_field
                    .MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
                    .value) ==
            0
        ? null
        : value.lod_min_radius,
    lodScale:
        (fields &
                raw
                    .mln_map_tile_option_field
                    .MLN_MAP_TILE_OPTION_LOD_SCALE
                    .value) ==
            0
        ? null
        : value.lod_scale,
    lodPitchThreshold:
        (fields &
                raw
                    .mln_map_tile_option_field
                    .MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
                    .value) ==
            0
        ? null
        : value.lod_pitch_threshold,
    lodZoomShift:
        (fields &
                raw
                    .mln_map_tile_option_field
                    .MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
                    .value) ==
            0
        ? null
        : value.lod_zoom_shift,
    lodMode:
        (fields &
                raw
                    .mln_map_tile_option_field
                    .MLN_MAP_TILE_OPTION_LOD_MODE
                    .value) ==
            0
        ? null
        : TileLodMode.fromRawValue(value.lod_mode),
  );
}

/// Materializes Dart animation options into a native C struct.
raw.mln_animation_options animationOptionsToNative(
  AnimationOptions value,
  raw.mln_animation_options result,
) {
  final durationMs = value.durationMs;
  if (durationMs != null) {
    result.fields |=
        raw.mln_animation_option_field.MLN_ANIMATION_OPTION_DURATION.value;
    result.duration_ms = durationMs;
  }
  final velocity = value.velocity;
  if (velocity != null) {
    result.fields |=
        raw.mln_animation_option_field.MLN_ANIMATION_OPTION_VELOCITY.value;
    result.velocity = velocity;
  }
  final minZoom = value.minZoom;
  if (minZoom != null) {
    result.fields |=
        raw.mln_animation_option_field.MLN_ANIMATION_OPTION_MIN_ZOOM.value;
    result.min_zoom = minZoom;
  }
  final easing = value.easing;
  if (easing != null) {
    result.fields |=
        raw.mln_animation_option_field.MLN_ANIMATION_OPTION_EASING.value;
    result.easing = unitBezierToNative(easing);
  }
  return result;
}
