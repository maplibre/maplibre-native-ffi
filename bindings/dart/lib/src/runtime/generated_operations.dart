// Generated from the C headers by tools/bindgen. Do not edit.
part of 'runtime.dart';

MetalOwnedTextureFrame _readMetalOwnedTextureFrame(
  raw.mln_metal_owned_texture_frame source,
) => MetalOwnedTextureFrame(
  generation: uint64FromNative(source.generation),
  width: source.width,
  height: source.height,
  scaleFactor: source.scale_factor,
  frameId: uint64FromNative(source.frame_id),
  texture: NativePointer(source.texture.address),
  device: NativePointer(source.device.address),
  pixelFormat: uint64FromNative(source.pixel_format),
);

OpenglOwnedTextureFrame _readOpenglOwnedTextureFrame(
  raw.mln_opengl_owned_texture_frame source,
) => OpenglOwnedTextureFrame(
  generation: uint64FromNative(source.generation),
  width: source.width,
  height: source.height,
  scaleFactor: source.scale_factor,
  frameId: uint64FromNative(source.frame_id),
  texture: source.texture,
  target: source.target,
  internalFormat: source.internal_format,
  format: source.format,
  type: source.type,
);

Pointer<raw.mln_gpu_sync> _writeGpuSync(GpuSync value, Arena arena) {
  final result = arena<raw.mln_gpu_sync>();
  result.ref = raw.mln_gpu_sync_default();
  result.ref.kind = value.kind.rawValue;
  result.ref.object = uint64ToNative(value.object, 'uint64_t');
  result.ref.value = uint64ToNative(value.value, 'uint64_t');
  return result;
}

GpuSync _readGpuSync(raw.mln_gpu_sync source) => GpuSync(
  kind: GpuSyncKind.fromRawValue(source.kind),
  object: uint64FromNative(source.object),
  value: uint64FromNative(source.value),
);

RenderFrameResult _readRenderFrameResult(raw.mln_render_frame_result source) =>
    RenderFrameResult(
      disposition: RenderResult.fromRawValue(source.disposition),
      token: uint64FromNative(source.token),
      mapUpdateGeneration: uint64FromNative(source.map_update_generation),
      extentGeneration: uint64FromNative(source.extent_generation),
      frameGeneration: uint64FromNative(source.frame_generation),
      needsRepaint: source.needs_repaint,
    );

VulkanOwnedTextureFrame _readVulkanOwnedTextureFrame(
  raw.mln_vulkan_owned_texture_frame source,
) => VulkanOwnedTextureFrame(
  generation: uint64FromNative(source.generation),
  width: source.width,
  height: source.height,
  scaleFactor: source.scale_factor,
  frameId: uint64FromNative(source.frame_id),
  image: uint64FromNative(source.image),
  imageView: uint64FromNative(source.image_view),
  device: NativePointer(source.device.address),
  format: source.format,
  layout: source.layout,
);

WebgpuOwnedTextureFrame _readWebgpuOwnedTextureFrame(
  raw.mln_webgpu_owned_texture_frame source,
) => WebgpuOwnedTextureFrame(
  generation: uint64FromNative(source.generation),
  width: source.width,
  height: source.height,
  scaleFactor: source.scale_factor,
  frameId: uint64FromNative(source.frame_id),
  texture: NativePointer(source.texture.address),
  textureView: NativePointer(source.texture_view.address),
  device: NativePointer(source.device.address),
  format: source.format,
);

Pointer<raw.mln_unit_bezier> _writeUnitBezier(UnitBezier value, Arena arena) {
  final result = arena<raw.mln_unit_bezier>();
  result.ref.x1 = value.x1;
  result.ref.y1 = value.y1;
  result.ref.x2 = value.x2;
  result.ref.y2 = value.y2;
  return result;
}

UnitBezier _readUnitBezier(raw.mln_unit_bezier source) =>
    UnitBezier(source.x1, source.y1, source.x2, source.y2);

Pointer<raw.mln_animation_options> _writeAnimationOptions(
  AnimationOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_animation_options>();
  result.ref = raw.mln_animation_options_default();
  if (value.durationMs != null) {
    result.ref.fields |= raw.MLN_ANIMATION_OPTION_DURATION;
    result.ref.duration_ms = value.durationMs!;
  }
  if (value.velocity != null) {
    result.ref.fields |= raw.MLN_ANIMATION_OPTION_VELOCITY;
    result.ref.velocity = value.velocity!;
  }
  if (value.minZoom != null) {
    result.ref.fields |= raw.MLN_ANIMATION_OPTION_MIN_ZOOM;
    result.ref.min_zoom = value.minZoom!;
  }
  if (value.easing != null) {
    result.ref.fields |= raw.MLN_ANIMATION_OPTION_EASING;
    result.ref.easing = _writeUnitBezier(value.easing!, arena).ref;
  }
  if (value.transitionId != null) {
    result.ref.fields |= raw.MLN_ANIMATION_OPTION_TRANSITION_ID;
    result.ref.transition_id = uint64ToNative(value.transitionId!, 'uint64_t');
  }
  return result;
}

AnimationOptions _readAnimationOptions(raw.mln_animation_options source) =>
    AnimationOptions(
      durationMs: (source.fields & raw.MLN_ANIMATION_OPTION_DURATION) != 0
          ? source.duration_ms
          : null,
      velocity: (source.fields & raw.MLN_ANIMATION_OPTION_VELOCITY) != 0
          ? source.velocity
          : null,
      minZoom: (source.fields & raw.MLN_ANIMATION_OPTION_MIN_ZOOM) != 0
          ? source.min_zoom
          : null,
      easing: (source.fields & raw.MLN_ANIMATION_OPTION_EASING) != 0
          ? _readUnitBezier(source.easing)
          : null,
      transitionId:
          (source.fields & raw.MLN_ANIMATION_OPTION_TRANSITION_ID) != 0
          ? uint64FromNative(source.transition_id)
          : null,
    );

Pointer<raw.mln_lat_lng> _writeLatLng(LatLng value, Arena arena) {
  final result = arena<raw.mln_lat_lng>();
  result.ref.latitude = value.latitude;
  result.ref.longitude = value.longitude;
  return result;
}

LatLng _readLatLng(raw.mln_lat_lng source) =>
    LatLng(source.latitude, source.longitude);

Pointer<raw.mln_lat_lng_bounds> _writeLatLngBounds(
  LatLngBounds value,
  Arena arena,
) {
  final result = arena<raw.mln_lat_lng_bounds>();
  result.ref.southwest = _writeLatLng(value.southwest, arena).ref;
  result.ref.northeast = _writeLatLng(value.northeast, arena).ref;
  return result;
}

LatLngBounds _readLatLngBounds(raw.mln_lat_lng_bounds source) => LatLngBounds(
  southwest: _readLatLng(source.southwest),
  northeast: _readLatLng(source.northeast),
);

Pointer<raw.mln_bound_options> _writeBoundOptions(
  BoundOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_bound_options>();
  result.ref = raw.mln_bound_options_default();
  if (value.unbounded) {
    result.ref.fields |= raw.MLN_BOUND_OPTION_UNBOUNDED;
  }
  if (value.bounds != null) {
    result.ref.fields |= raw.MLN_BOUND_OPTION_BOUNDS;
    result.ref.bounds = _writeLatLngBounds(value.bounds!, arena).ref;
  }
  if (value.minZoom != null) {
    result.ref.fields |= raw.MLN_BOUND_OPTION_MIN_ZOOM;
    result.ref.min_zoom = value.minZoom!;
  }
  if (value.maxZoom != null) {
    result.ref.fields |= raw.MLN_BOUND_OPTION_MAX_ZOOM;
    result.ref.max_zoom = value.maxZoom!;
  }
  if (value.minPitch != null) {
    result.ref.fields |= raw.MLN_BOUND_OPTION_MIN_PITCH;
    result.ref.min_pitch = value.minPitch!;
  }
  if (value.maxPitch != null) {
    result.ref.fields |= raw.MLN_BOUND_OPTION_MAX_PITCH;
    result.ref.max_pitch = value.maxPitch!;
  }
  return result;
}

BoundOptions _readBoundOptions(raw.mln_bound_options source) => BoundOptions(
  unbounded: (source.fields & raw.MLN_BOUND_OPTION_UNBOUNDED) != 0,
  bounds: (source.fields & raw.MLN_BOUND_OPTION_BOUNDS) != 0
      ? _readLatLngBounds(source.bounds)
      : null,
  minZoom: (source.fields & raw.MLN_BOUND_OPTION_MIN_ZOOM) != 0
      ? source.min_zoom
      : null,
  maxZoom: (source.fields & raw.MLN_BOUND_OPTION_MAX_ZOOM) != 0
      ? source.max_zoom
      : null,
  minPitch: (source.fields & raw.MLN_BOUND_OPTION_MIN_PITCH) != 0
      ? source.min_pitch
      : null,
  maxPitch: (source.fields & raw.MLN_BOUND_OPTION_MAX_PITCH) != 0
      ? source.max_pitch
      : null,
);

Pointer<raw.mln_screen_point> _writeScreenPoint(
  ScreenPoint value,
  Arena arena,
) {
  final result = arena<raw.mln_screen_point>();
  result.ref.x = value.x;
  result.ref.y = value.y;
  return result;
}

ScreenPoint _readScreenPoint(raw.mln_screen_point source) =>
    ScreenPoint(source.x, source.y);

Pointer<raw.mln_camera_delta> _writeCameraDelta(
  CameraDelta value,
  Arena arena,
) {
  final result = arena<raw.mln_camera_delta>();
  result.ref = raw.mln_camera_delta_default();
  result.ref.kind = value.kind.rawValue;
  result.ref.offset = _writeScreenPoint(value.offset, arena).ref;
  result.ref.amount = value.amount;
  if (value.anchor != null) {
    result.ref.has_anchor = true;
    result.ref.anchor = _writeScreenPoint(value.anchor!, arena).ref;
  }
  result.ref.animation = _writeAnimationOptions(value.animation, arena).ref;
  return result;
}

CameraDelta _readCameraDelta(raw.mln_camera_delta source) => CameraDelta(
  kind: CameraDeltaKind.fromRawValue(source.kind),
  offset: _readScreenPoint(source.offset),
  amount: source.amount,
  anchor: source.has_anchor ? _readScreenPoint(source.anchor) : null,
  animation: _readAnimationOptions(source.animation),
);

Pointer<raw.mln_edge_insets> _writeEdgeInsets(EdgeInsets value, Arena arena) {
  final result = arena<raw.mln_edge_insets>();
  result.ref.top = value.top;
  result.ref.left = value.left;
  result.ref.bottom = value.bottom;
  result.ref.right = value.right;
  return result;
}

EdgeInsets _readEdgeInsets(raw.mln_edge_insets source) => EdgeInsets(
  top: source.top,
  left: source.left,
  bottom: source.bottom,
  right: source.right,
);

Pointer<raw.mln_camera_fit_options> _writeCameraFitOptions(
  CameraFitOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_camera_fit_options>();
  result.ref = raw.mln_camera_fit_options_default();
  if (value.padding != null) {
    result.ref.fields |= raw.MLN_CAMERA_FIT_OPTION_PADDING;
    result.ref.padding = _writeEdgeInsets(value.padding!, arena).ref;
  }
  if (value.bearing != null) {
    result.ref.fields |= raw.MLN_CAMERA_FIT_OPTION_BEARING;
    result.ref.bearing = value.bearing!;
  }
  if (value.pitch != null) {
    result.ref.fields |= raw.MLN_CAMERA_FIT_OPTION_PITCH;
    result.ref.pitch = value.pitch!;
  }
  return result;
}

CameraFitOptions _readCameraFitOptions(raw.mln_camera_fit_options source) =>
    CameraFitOptions(
      padding: (source.fields & raw.MLN_CAMERA_FIT_OPTION_PADDING) != 0
          ? _readEdgeInsets(source.padding)
          : null,
      bearing: (source.fields & raw.MLN_CAMERA_FIT_OPTION_BEARING) != 0
          ? source.bearing
          : null,
      pitch: (source.fields & raw.MLN_CAMERA_FIT_OPTION_PITCH) != 0
          ? source.pitch
          : null,
    );

Pointer<raw.mln_camera_options> _writeCameraOptions(
  CameraOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_camera_options>();
  result.ref = raw.mln_camera_options_default();
  if (value.center != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_CENTER;
    result.ref.latitude = value.center!.latitude;
    result.ref.longitude = value.center!.longitude;
  }
  if (value.centerAltitude != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_CENTER_ALTITUDE;
    result.ref.center_altitude = value.centerAltitude!;
  }
  if (value.padding != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_PADDING;
    result.ref.padding = _writeEdgeInsets(value.padding!, arena).ref;
  }
  if (value.anchor != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_ANCHOR;
    result.ref.anchor = _writeScreenPoint(value.anchor!, arena).ref;
  }
  if (value.zoom != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_ZOOM;
    result.ref.zoom = value.zoom!;
  }
  if (value.bearing != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_BEARING;
    result.ref.bearing = value.bearing!;
  }
  if (value.pitch != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_PITCH;
    result.ref.pitch = value.pitch!;
  }
  if (value.roll != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_ROLL;
    result.ref.roll = value.roll!;
  }
  if (value.fieldOfView != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_FOV;
    result.ref.field_of_view = value.fieldOfView!;
  }
  return result;
}

CameraOptions _readCameraOptions(
  raw.mln_camera_options source,
) => CameraOptions(
  center: (source.fields & raw.MLN_CAMERA_OPTION_CENTER) != 0
      ? LatLng(source.latitude, source.longitude)
      : null,
  centerAltitude: (source.fields & raw.MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0
      ? source.center_altitude
      : null,
  padding: (source.fields & raw.MLN_CAMERA_OPTION_PADDING) != 0
      ? _readEdgeInsets(source.padding)
      : null,
  anchor: (source.fields & raw.MLN_CAMERA_OPTION_ANCHOR) != 0
      ? _readScreenPoint(source.anchor)
      : null,
  zoom: (source.fields & raw.MLN_CAMERA_OPTION_ZOOM) != 0 ? source.zoom : null,
  bearing: (source.fields & raw.MLN_CAMERA_OPTION_BEARING) != 0
      ? source.bearing
      : null,
  pitch: (source.fields & raw.MLN_CAMERA_OPTION_PITCH) != 0
      ? source.pitch
      : null,
  roll: (source.fields & raw.MLN_CAMERA_OPTION_ROLL) != 0 ? source.roll : null,
  fieldOfView: (source.fields & raw.MLN_CAMERA_OPTION_FOV) != 0
      ? source.field_of_view
      : null,
);

Pointer<raw.mln_camera_update> _writeCameraUpdate(
  CameraUpdate value,
  Arena arena,
) {
  final result = arena<raw.mln_camera_update>();
  result.ref = raw.mln_camera_update_default();
  result.ref.mode = value.mode.rawValue;
  result.ref.camera = _writeCameraOptions(value.camera, arena).ref;
  result.ref.animation = _writeAnimationOptions(value.animation, arena).ref;
  result.ref.gesture_phase = value.gesturePhase.rawValue;
  return result;
}

CameraUpdate _readCameraUpdate(raw.mln_camera_update source) => CameraUpdate(
  mode: CameraUpdateMode.fromRawValue(source.mode),
  camera: _readCameraOptions(source.camera),
  animation: _readAnimationOptions(source.animation),
  gesturePhase: GesturePhase.fromRawValue(source.gesture_phase),
);

Pointer<raw.mln_canonical_tile_id> _writeCanonicalTileId(
  CanonicalTileId value,
  Arena arena,
) {
  final result = arena<raw.mln_canonical_tile_id>();
  result.ref.z = _nativeInteger(value.z, 0, 4294967295);
  result.ref.x = _nativeInteger(value.x, 0, 4294967295);
  result.ref.y = _nativeInteger(value.y, 0, 4294967295);
  return result;
}

_NativeRegistration<raw.mln_custom_geometry_source_options>
_prepareCustomGeometrySourceOptions(
  CustomGeometrySourceOptions value,
  _NativeCallbackPorts roots,
) {
  final arena = Arena();
  _NativeCallbackPort? port;
  try {
    final result = arena<raw.mln_custom_geometry_source_options>();
    result.ref = raw.mln_custom_geometry_source_options_default();
    if (value.fetchTile == null && value.cancelTile == null) {
      return _NativeRegistration(result, () {}, arena.releaseAll);
    }
    port = roots.register({
      if (value.fetchTile != null)
        (raw.MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE &
            0xffffffff): (message) => value.fetchTile!(
          CanonicalTileId(
            z: message[1] as int,
            x: message[2] as int,
            y: message[3] as int,
          ),
        ),
      if (value.cancelTile != null)
        (raw.MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_CANCEL_TILE &
            0xffffffff): (message) => value.cancelTile!(
          CanonicalTileId(
            z: message[1] as int,
            x: message[2] as int,
            y: message[3] as int,
          ),
        ),
    });
    result.ref.fetch_tile = value.fetchTile == null
        ? nullptr
        : raw
              .mln_adapter_dart_port_function(
                (raw.MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE &
                    0xffffffff),
              )
              .cast();
    result.ref.cancel_tile = value.cancelTile == null
        ? nullptr
        : raw
              .mln_adapter_dart_port_function(
                (raw.MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_CANCEL_TILE &
                    0xffffffff),
              )
              .cast();
    if (value.minZoom != null) {
      result.ref.fields |= raw.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
      result.ref.min_zoom = value.minZoom!;
    }
    if (value.maxZoom != null) {
      result.ref.fields |= raw.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
      result.ref.max_zoom = value.maxZoom!;
    }
    if (value.tolerance != null) {
      result.ref.fields |= raw.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
      result.ref.tolerance = value.tolerance!;
    }
    if (value.tileSize != null) {
      result.ref.fields |= raw.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
      result.ref.tile_size = _nativeInteger(value.tileSize!, 0, 4294967295);
    }
    if (value.buffer != null) {
      result.ref.fields |= raw.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
      result.ref.buffer = _nativeInteger(value.buffer!, 0, 4294967295);
    }
    if (value.clip != null) {
      result.ref.fields |= raw.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
      result.ref.clip = value.clip!;
    }
    if (value.wrap != null) {
      result.ref.fields |= raw.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
      result.ref.wrap = value.wrap!;
    }
    result.ref.user_data = port.context;
    result.ref.release_user_data =
        Native.addressOf<
              NativeFunction<raw.mln_runtime_callback_releaseFunction>
            >(raw.mln_adapter_dart_port_release)
            .cast();
    return _NativeRegistration(result, port.reject, arena.releaseAll);
  } catch (_) {
    port?.reject();
    arena.releaseAll();
    rethrow;
  }
}

CustomGeometrySourceOptions _readCustomGeometrySourceOptions(
  raw.mln_custom_geometry_source_options source,
) => CustomGeometrySourceOptions(
  fetchTile: null,
  cancelTile: null,
  minZoom: source.min_zoom,
  maxZoom: source.max_zoom,
  tolerance: source.tolerance,
  tileSize: source.tile_size,
  buffer: source.buffer,
  clip: source.clip,
  wrap: source.wrap,
);

_NativeRegistration<raw.mln_custom_mvt_vector_source_options>
_prepareCustomMvtVectorSourceOptions(
  CustomMvtVectorSourceOptions value,
  _NativeCallbackPorts roots,
) {
  final arena = Arena();
  _NativeCallbackPort? port;
  try {
    final result = arena<raw.mln_custom_mvt_vector_source_options>();
    result.ref = raw.mln_custom_mvt_vector_source_options_default();
    if (value.fetchTile == null && value.cancelTile == null) {
      return _NativeRegistration(result, () {}, arena.releaseAll);
    }
    port = roots.register({
      if (value.fetchTile != null)
        (raw.MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_FETCH_TILE &
            0xffffffff): (message) => value.fetchTile!(
          CanonicalTileId(
            z: message[1] as int,
            x: message[2] as int,
            y: message[3] as int,
          ),
        ),
      if (value.cancelTile != null)
        (raw.MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_CANCEL_TILE &
            0xffffffff): (message) => value.cancelTile!(
          CanonicalTileId(
            z: message[1] as int,
            x: message[2] as int,
            y: message[3] as int,
          ),
        ),
    });
    result.ref.fetch_tile = value.fetchTile == null
        ? nullptr
        : raw
              .mln_adapter_dart_port_function(
                (raw.MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_FETCH_TILE &
                    0xffffffff),
              )
              .cast();
    result.ref.cancel_tile = value.cancelTile == null
        ? nullptr
        : raw
              .mln_adapter_dart_port_function(
                (raw.MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_CANCEL_TILE &
                    0xffffffff),
              )
              .cast();
    if (value.minZoom != null) {
      result.ref.fields |= raw.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
      result.ref.min_zoom = value.minZoom!;
    }
    if (value.maxZoom != null) {
      result.ref.fields |= raw.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
      result.ref.max_zoom = value.maxZoom!;
    }
    result.ref.user_data = port.context;
    result.ref.release_user_data =
        Native.addressOf<
              NativeFunction<raw.mln_runtime_callback_releaseFunction>
            >(raw.mln_adapter_dart_port_release)
            .cast();
    return _NativeRegistration(result, port.reject, arena.releaseAll);
  } catch (_) {
    port?.reject();
    arena.releaseAll();
    rethrow;
  }
}

CustomMvtVectorSourceOptions _readCustomMvtVectorSourceOptions(
  raw.mln_custom_mvt_vector_source_options source,
) => CustomMvtVectorSourceOptions(
  fetchTile: null,
  cancelTile: null,
  minZoom: source.min_zoom,
  maxZoom: source.max_zoom,
);

RenderingStats _readRenderingStats(raw.mln_rendering_stats source) =>
    RenderingStats(
      encodingTime: source.encoding_time,
      renderingTime: source.rendering_time,
      frameCount: source.frame_count,
      drawCallCount: source.draw_call_count,
      totalDrawCallCount: source.total_draw_call_count,
    );

RuntimeEventRenderFrame _readRuntimeEventRenderFrame(
  raw.mln_runtime_event_render_frame source,
) => RuntimeEventRenderFrame(
  mode: RenderMode.fromRawValue(source.mode),
  needsRepaint: source.needs_repaint,
  placementChanged: source.placement_changed,
  stats: _readRenderingStats(source.stats),
);

RuntimeEventRenderMap _readRuntimeEventRenderMap(
  raw.mln_runtime_event_render_map source,
) => RuntimeEventRenderMap(mode: RenderMode.fromRawValue(source.mode));

TileId _readTileId(raw.mln_tile_id source) => TileId(
  overscaledZ: source.overscaled_z,
  wrap: source.wrap,
  canonicalZ: source.canonical_z,
  canonicalX: source.canonical_x,
  canonicalY: source.canonical_y,
);

RuntimeEventTileAction _readRuntimeEventTileAction(
  raw.mln_runtime_event_tile_action source,
) => RuntimeEventTileAction(
  operation: TileOperation.fromRawValue(source.operation),
  tileId: _readTileId(source.tile_id),
);

OfflineRegionStatus _readOfflineRegionStatus(
  raw.mln_offline_region_status source,
) => OfflineRegionStatus(
  downloadState: OfflineRegionDownloadState.fromRawValue(source.download_state),
  completedResourceCount: uint64FromNative(source.completed_resource_count),
  completedResourceSize: uint64FromNative(source.completed_resource_size),
  completedTileCount: uint64FromNative(source.completed_tile_count),
  requiredTileCount: uint64FromNative(source.required_tile_count),
  completedTileSize: uint64FromNative(source.completed_tile_size),
  requiredResourceCount: uint64FromNative(source.required_resource_count),
  requiredResourceCountIsPrecise: source.required_resource_count_is_precise,
  complete: source.complete,
);

RuntimeEventOfflineRegionStatus _readRuntimeEventOfflineRegionStatus(
  raw.mln_runtime_event_offline_region_status source,
) => RuntimeEventOfflineRegionStatus(
  regionId: source.region_id,
  status: _readOfflineRegionStatus(source.status),
);

RuntimeEventOfflineRegionResponseError
_readRuntimeEventOfflineRegionResponseError(
  raw.mln_runtime_event_offline_region_response_error source,
) => RuntimeEventOfflineRegionResponseError(
  regionId: source.region_id,
  reason: ResourceErrorReason.fromRawValue(source.reason),
);

RuntimeEventOfflineRegionTileCountLimit
_readRuntimeEventOfflineRegionTileCountLimit(
  raw.mln_runtime_event_offline_region_tile_count_limit source,
) => RuntimeEventOfflineRegionTileCountLimit(
  regionId: source.region_id,
  limit: uint64FromNative(source.limit),
);

RuntimeEventCameraTransitionFinished _readRuntimeEventCameraTransitionFinished(
  raw.mln_runtime_event_camera_transition_finished source,
) => RuntimeEventCameraTransitionFinished(
  transitionId: uint64FromNative(source.transition_id),
);

RuntimeEvent _readRuntimeEvent(
  raw.mln_runtime_event source, {
  Uint8List Function()? rawRecord,
  String message = '',
}) => RuntimeEvent(
  type: RuntimeEventType.fromRawValue(source.type),
  sourceType: RuntimeEventSourceType.fromRawValue(source.source_type),
  source: uint64FromNative(source.source),
  code: source.code,
  messageOffset: uint64FromNative(source.message_offset),
  messageSize: source.message_size,
  message: message,
  payload: switch (source.payload_type) {
    1 => RuntimeEventPayloadRenderFrame(
      _readRuntimeEventRenderFrame(source.payload.render_frame),
    ),
    2 => RuntimeEventPayloadRenderMap(
      _readRuntimeEventRenderMap(source.payload.render_map),
    ),
    4 => RuntimeEventPayloadTileAction(
      _readRuntimeEventTileAction(source.payload.tile_action),
    ),
    5 => RuntimeEventPayloadOfflineRegionStatus(
      _readRuntimeEventOfflineRegionStatus(
        source.payload.offline_region_status,
      ),
    ),
    6 => RuntimeEventPayloadOfflineRegionResponseError(
      _readRuntimeEventOfflineRegionResponseError(
        source.payload.offline_region_response_error,
      ),
    ),
    7 => RuntimeEventPayloadOfflineRegionTileCountLimit(
      _readRuntimeEventOfflineRegionTileCountLimit(
        source.payload.offline_region_tile_count_limit,
      ),
    ),
    9 => RuntimeEventPayloadCameraTransitionFinished(
      _readRuntimeEventCameraTransitionFinished(
        source.payload.camera_transition_finished,
      ),
    ),
    0 => const RuntimeEventPayloadNone(),
    final tag => RuntimeEventPayloadUnknown(
      tag,
      rawRecord?.call() ??
          withNativeArena((arena) {
            final copy = arena<raw.mln_runtime_event>()..ref = source;
            return Uint8List.fromList(
              copy.cast<Uint8>().asTypedList(sizeOf<raw.mln_runtime_event>()),
            );
          }),
    ),
  },
);

RuntimeEventBatchView _readRuntimeEventBatchView(
  raw.mln_runtime_event_batch_view source,
) => RuntimeEventBatchView(
  events: (() {
    if (source.event_size < sizeOf<raw.mln_runtime_event>()) {
      throwInvalidState('native record stride is too small');
    }
    return List<RuntimeEvent>.unmodifiable(
      List.generate(
        source.event_count,
        (index) => _readRuntimeEvent(
          (source.events.cast<Uint8>() + index * source.event_size)
              .cast<raw.mln_runtime_event>()
              .ref,
          rawRecord: () =>
              (source.events.cast<Uint8>() + index * source.event_size)
                  .cast<raw.mln_runtime_event>()
                  .cast<Uint8>()
                  .asTypedList(source.event_size),
          message: _arenaUtf8(
            source.messages.cast(),
            source.messages_size,
            (source.events.cast<Uint8>() + index * source.event_size)
                .cast<raw.mln_runtime_event>()
                .ref
                .message_offset,
            (source.events.cast<Uint8>() + index * source.event_size)
                .cast<raw.mln_runtime_event>()
                .ref
                .message_size,
          ),
        ),
      ),
    );
  })(),
);

Pointer<raw.mln_frame_demand> _writeFrameDemand(
  FrameDemand value,
  Arena arena,
) {
  final result = arena<raw.mln_frame_demand>();
  result.ref = raw.mln_frame_demand_default();
  result.ref.flags = value.flags.rawValue;
  result.ref.token = uint64ToNative(value.token, 'uint64_t');
  result.ref.coalescing_boundary = uint64ToNative(
    value.coalescingBoundary,
    'uint64_t',
  );
  result.ref.timeout_ns = uint64ToNative(value.timeoutNs, 'uint64_t');
  return result;
}

FrameDemand _readFrameDemand(raw.mln_frame_demand source) => FrameDemand(
  flags: FrameDemandFlag.fromRawValue(source.flags),
  token: uint64FromNative(source.token),
  coalescingBoundary: uint64FromNative(source.coalescing_boundary),
  timeoutNs: uint64FromNative(source.timeout_ns),
);

Pointer<raw.mln_vec3> _writeVec3(Vec3 value, Arena arena) {
  final result = arena<raw.mln_vec3>();
  result.ref.x = value.x;
  result.ref.y = value.y;
  result.ref.z = value.z;
  return result;
}

Vec3 _readVec3(raw.mln_vec3 source) => Vec3(source.x, source.y, source.z);

Pointer<raw.mln_quaternion> _writeQuaternion(Quaternion value, Arena arena) {
  final result = arena<raw.mln_quaternion>();
  result.ref.x = value.x;
  result.ref.y = value.y;
  result.ref.z = value.z;
  result.ref.w = value.w;
  return result;
}

Quaternion _readQuaternion(raw.mln_quaternion source) =>
    Quaternion(source.x, source.y, source.z, source.w);

Pointer<raw.mln_free_camera_options> _writeFreeCameraOptions(
  FreeCameraOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_free_camera_options>();
  result.ref = raw.mln_free_camera_options_default();
  if (value.position != null) {
    result.ref.fields |= raw.MLN_FREE_CAMERA_OPTION_POSITION;
    result.ref.position = _writeVec3(value.position!, arena).ref;
  }
  if (value.orientation != null) {
    result.ref.fields |= raw.MLN_FREE_CAMERA_OPTION_ORIENTATION;
    result.ref.orientation = _writeQuaternion(value.orientation!, arena).ref;
  }
  return result;
}

FreeCameraOptions _readFreeCameraOptions(raw.mln_free_camera_options source) =>
    FreeCameraOptions(
      position: (source.fields & raw.MLN_FREE_CAMERA_OPTION_POSITION) != 0
          ? _readVec3(source.position)
          : null,
      orientation: (source.fields & raw.MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0
          ? _readQuaternion(source.orientation)
          : null,
    );

Pointer<raw.mln_geojson_source_options> _writeGeojsonSourceOptions(
  GeojsonSourceOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_geojson_source_options>();
  result.ref = raw.mln_geojson_source_options_default();
  if (value.minZoom != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
    result.ref.min_zoom = value.minZoom!;
  }
  if (value.maxZoom != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
    result.ref.max_zoom = value.maxZoom!;
  }
  if (value.tolerance != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
    result.ref.tolerance = value.tolerance!;
  }
  if (value.clusterMaxZoom != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
    result.ref.cluster_max_zoom = value.clusterMaxZoom!;
  }
  if (value.clusterProperties != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
    result.ref.cluster_properties = nativeBufferView(
      value.clusterProperties!,
      arena,
    );
  }
  if (value.tileSize != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE;
    result.ref.tile_size = _nativeInteger(value.tileSize!, 0, 4294967295);
  }
  if (value.buffer != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_BUFFER;
    result.ref.buffer = _nativeInteger(value.buffer!, 0, 4294967295);
  }
  if (value.clusterRadius != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
    result.ref.cluster_radius = _nativeInteger(
      value.clusterRadius!,
      0,
      4294967295,
    );
  }
  if (value.clusterMinPoints != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
    result.ref.cluster_min_points = _nativeInteger(
      value.clusterMinPoints!,
      0,
      4294967295,
    );
  }
  if (value.lineMetrics != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
    result.ref.line_metrics = value.lineMetrics!;
  }
  if (value.cluster != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
    result.ref.cluster = value.cluster!;
  }
  if (value.synchronousTiling != null) {
    result.ref.fields |= raw.MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
    result.ref.synchronous_tiling = value.synchronousTiling!;
  }
  return result;
}

GeojsonSourceOptions _readGeojsonSourceOptions(
  raw.mln_geojson_source_options source,
) => GeojsonSourceOptions(
  minZoom: (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM) != 0
      ? source.min_zoom
      : null,
  maxZoom: (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM) != 0
      ? source.max_zoom
      : null,
  tolerance: (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE) != 0
      ? source.tolerance
      : null,
  clusterMaxZoom:
      (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM) != 0
      ? source.cluster_max_zoom
      : null,
  clusterProperties:
      (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES) != 0
      ? _copyBufferView(source.cluster_properties)
      : null,
  tileSize: (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE) != 0
      ? source.tile_size
      : null,
  buffer: (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_BUFFER) != 0
      ? source.buffer
      : null,
  clusterRadius:
      (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS) != 0
      ? source.cluster_radius
      : null,
  clusterMinPoints:
      (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS) != 0
      ? source.cluster_min_points
      : null,
  lineMetrics: (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS) != 0
      ? source.line_metrics
      : null,
  cluster: (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_CLUSTER) != 0
      ? source.cluster
      : null,
  synchronousTiling:
      (source.fields & raw.MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING) != 0
      ? source.synchronous_tiling
      : null,
);

Pointer<raw.mln_projected_meters> _writeProjectedMeters(
  ProjectedMeters value,
  Arena arena,
) {
  final result = arena<raw.mln_projected_meters>();
  result.ref.northing = value.northing;
  result.ref.easting = value.easting;
  return result;
}

ProjectedMeters _readProjectedMeters(raw.mln_projected_meters source) =>
    ProjectedMeters(northing: source.northing, easting: source.easting);

void _deliverLogCallback(LogCallback callback, List<dynamic> message) {
  final record = Pointer<raw.mln_adapter_deferred_call_record>.fromAddress(
    message[1] as int,
  );
  try {
    final arguments = record.ref.arguments
        .cast<raw.mln_adapter_log_callback_arguments>()
        .ref;
    callback(
      LogSeverity.fromRawValue(arguments.severity),
      LogEvent.fromRawValue(arguments.event),
      arguments.code,
      arguments.message.cast<Utf8>().toDartString(),
    );
  } finally {
    raw.mln_adapter_deferred_call_record_destroy(record);
  }
}

Pointer<raw.mln_premultiplied_rgba8_image> _writePremultipliedRgba8Image(
  PremultipliedRgba8Image value,
  Arena arena,
) {
  final result = arena<raw.mln_premultiplied_rgba8_image>();
  result.ref = raw.mln_premultiplied_rgba8_image_default();
  result.ref.width = _nativeInteger(value.width, 0, 4294967295);
  result.ref.height = _nativeInteger(value.height, 0, 4294967295);
  result.ref.stride = _nativeInteger(value.stride, 0, 4294967295);
  final bytespixels = nativeBufferView(value.pixels, arena);
  result.ref.pixels = bytespixels.data.cast();
  result.ref.byte_length = bytespixels.size;
  return result;
}

PremultipliedRgba8Image _readPremultipliedRgba8Image(
  raw.mln_premultiplied_rgba8_image source,
) => PremultipliedRgba8Image(
  width: source.width,
  height: source.height,
  stride: source.stride,
  pixels: Uint8List.fromList(
    source.pixels.cast<Uint8>().asTypedList(source.byte_length),
  ),
);

Pointer<raw.mln_style_tile_source_options> _writeStyleTileSourceOptions(
  StyleTileSourceOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_style_tile_source_options>();
  result.ref = raw.mln_style_tile_source_options_default();
  if (value.minZoom != null) {
    result.ref.fields |= raw.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
    result.ref.min_zoom = value.minZoom!;
  }
  if (value.maxZoom != null) {
    result.ref.fields |= raw.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
    result.ref.max_zoom = value.maxZoom!;
  }
  if (value.attribution != null) {
    result.ref.fields |= raw.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
    result.ref.attribution = nativeStringView(value.attribution!, arena).value;
  }
  if (value.scheme != null) {
    result.ref.fields |= raw.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
    result.ref.scheme = value.scheme!.rawValue;
  }
  if (value.bounds != null) {
    result.ref.fields |= raw.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
    result.ref.bounds = _writeLatLngBounds(value.bounds!, arena).ref;
  }
  if (value.tileSize != null) {
    result.ref.fields |= raw.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
    result.ref.tile_size = _nativeInteger(value.tileSize!, 0, 4294967295);
  }
  if (value.vectorEncoding != null) {
    result.ref.fields |= raw.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
    result.ref.vector_encoding = value.vectorEncoding!.rawValue;
  }
  if (value.rasterEncoding != null) {
    result.ref.fields |= raw.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
    result.ref.raster_encoding = value.rasterEncoding!.rawValue;
  }
  return result;
}

StyleTileSourceOptions _readStyleTileSourceOptions(
  raw.mln_style_tile_source_options source,
) => StyleTileSourceOptions(
  minZoom: (source.fields & raw.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM) != 0
      ? source.min_zoom
      : null,
  maxZoom: (source.fields & raw.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM) != 0
      ? source.max_zoom
      : null,
  attribution:
      (source.fields & raw.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION) != 0
      ? utf8.decode(_copyBufferView(source.attribution))
      : null,
  scheme: (source.fields & raw.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME) != 0
      ? StyleTileScheme.fromRawValue(source.scheme)
      : null,
  bounds: (source.fields & raw.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS) != 0
      ? _readLatLngBounds(source.bounds)
      : null,
  tileSize: (source.fields & raw.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE) != 0
      ? source.tile_size
      : null,
  vectorEncoding:
      (source.fields & raw.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING) != 0
      ? StyleVectorTileEncoding.fromRawValue(source.vector_encoding)
      : null,
  rasterEncoding:
      (source.fields & raw.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING) != 0
      ? StyleRasterDemEncoding.fromRawValue(source.raster_encoding)
      : null,
);

CameraQueryResult _readCameraQueryResult(raw.mln_camera_query_result source) =>
    CameraQueryResult(
      generation: uint64FromNative(source.generation),
      camera: _readCameraOptions(source.camera),
    );

Pointer<raw.mln_image_stretch> _writeImageStretch(
  ImageStretch value,
  Arena arena,
) {
  final result = arena<raw.mln_image_stretch>();
  result.ref.from = value.from;
  result.ref.to = value.to;
  return result;
}

ImageStretch _readImageStretch(raw.mln_image_stretch source) =>
    ImageStretch(source.from, source.to);

StyleImageStretchesResult _readStyleImageStretchesResult(
  raw.mln_style_image_stretches_result source,
) => StyleImageStretchesResult(
  stretchX: List<ImageStretch>.unmodifiable(
    List.generate(
      source.stretch_x_count,
      (index) => _readImageStretch(source.stretch_x[index]),
    ),
  ),
  stretchY: List<ImageStretch>.unmodifiable(
    List.generate(
      source.stretch_y_count,
      (index) => _readImageStretch(source.stretch_y[index]),
    ),
  ),
);

Pointer<raw.mln_logical_extent> _writeLogicalExtent(
  LogicalExtent value,
  Arena arena,
) {
  final result = arena<raw.mln_logical_extent>();
  result.ref.width = _nativeInteger(value.width, 0, 4294967295);
  result.ref.height = _nativeInteger(value.height, 0, 4294967295);
  result.ref.scale_factor = value.scaleFactor;
  return result;
}

LogicalExtent _readLogicalExtent(raw.mln_logical_extent source) =>
    LogicalExtent(
      width: source.width,
      height: source.height,
      scaleFactor: source.scale_factor,
    );

Pointer<raw.mln_map_options> _writeMapOptions(MapOptions value, Arena arena) {
  final result = arena<raw.mln_map_options>();
  result.ref = raw.mln_map_options_default();
  result.ref.initial_extent = _writeLogicalExtent(
    value.initialExtent,
    arena,
  ).ref;
  result.ref.map_mode = value.mapMode.rawValue;
  result.ref.fast_pfor_enabled = value.fastPforEnabled;
  result.ref.event_mask = value.eventMask.rawValue;
  return result;
}

MapOptions _readMapOptions(raw.mln_map_options source) => MapOptions(
  initialExtent: _readLogicalExtent(source.initial_extent),
  mapMode: MapMode.fromRawValue(source.map_mode),
  fastPforEnabled: source.fast_pfor_enabled,
  eventMask: RuntimeEventMask.fromRawValue(source.event_mask),
);

Pointer<raw.mln_feature_state_selector> _writeFeatureStateSelector(
  FeatureStateSelector value,
  Arena arena,
) {
  final result = arena<raw.mln_feature_state_selector>();
  result.ref.size = sizeOf<raw.mln_feature_state_selector>();
  result.ref.source_id = nativeStringView(value.sourceId, arena).value;
  if (value.sourceLayerId != null) {
    result.ref.fields |= raw.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
    result.ref.source_layer_id = nativeStringView(
      value.sourceLayerId!,
      arena,
    ).value;
  }
  if (value.featureId != null) {
    result.ref.fields |= raw.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
    result.ref.feature_id = nativeStringView(value.featureId!, arena).value;
  }
  if (value.stateKey != null) {
    result.ref.fields |= raw.MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
    result.ref.state_key = nativeStringView(value.stateKey!, arena).value;
  }
  return result;
}

Pointer<raw.mln_image_content> _writeImageContent(
  ImageContent value,
  Arena arena,
) {
  final result = arena<raw.mln_image_content>();
  result.ref.left = value.left;
  result.ref.top = value.top;
  result.ref.right = value.right;
  result.ref.bottom = value.bottom;
  return result;
}

ImageContent _readImageContent(raw.mln_image_content source) => ImageContent(
  left: source.left,
  top: source.top,
  right: source.right,
  bottom: source.bottom,
);

StyleImageInfo _readStyleImageInfo(raw.mln_style_image_info source) =>
    StyleImageInfo(
      width: source.width,
      height: source.height,
      stride: source.stride,
      byteLength: source.byte_length,
      stretchXCount: source.stretch_x_count,
      stretchYCount: source.stretch_y_count,
      content: source.has_content ? _readImageContent(source.content) : null,
      textFitWidth: source.has_text_fit_width
          ? StyleImageTextFit.fromRawValue(source.text_fit_width)
          : null,
      textFitHeight: source.has_text_fit_height
          ? StyleImageTextFit.fromRawValue(source.text_fit_height)
          : null,
      pixelRatio: source.pixel_ratio,
      sdf: source.sdf,
    );

StyleImageResult _readStyleImageResult(raw.mln_style_image_result source) =>
    StyleImageResult(
      info: _readStyleImageInfo(source.info),
      pixels: _copyBufferView(source.pixels),
      stretchX: List<ImageStretch>.unmodifiable(
        List.generate(
          source.stretch_x_count,
          (index) => _readImageStretch(source.stretch_x[index]),
        ),
      ),
      stretchY: List<ImageStretch>.unmodifiable(
        List.generate(
          source.stretch_y_count,
          (index) => _readImageStretch(source.stretch_y[index]),
        ),
      ),
    );

StyleLayerInfo _readStyleLayerInfo(raw.mln_style_layer_info source) =>
    StyleLayerInfo(
      type: utf8.decode(_copyBufferView(source.type)),
      minZoom: source.min_zoom,
      maxZoom: source.max_zoom,
      visibility: StyleLayerVisibility.fromRawValue(source.visibility),
    );

StyleLayerResult _readStyleLayerResult(raw.mln_style_layer_result source) =>
    StyleLayerResult(
      info: _readStyleLayerInfo(source.info),
      sourceId: source.source_id.size == 0
          ? null
          : utf8.decode(_copyBufferView(source.source_id)),
      sourceLayer: source.source_layer.size == 0
          ? null
          : utf8.decode(_copyBufferView(source.source_layer)),
    );

StyleSourceInfo _readStyleSourceInfo(raw.mln_style_source_info source) =>
    StyleSourceInfo(
      type: StyleSourceType.fromRawValue(source.type),
      idSize: source.id_size,
      isVolatile: source.is_volatile,
      attributionSize: source.has_attribution ? source.attribution_size : null,
      urlSize: (source.fields & raw.MLN_STYLE_SOURCE_INFO_URL) != 0
          ? source.url_size
          : null,
      tilejson: (source.fields & raw.MLN_STYLE_SOURCE_INFO_TILEJSON) != 0
          ? StyleSourceTileInfo(
              tileCount: source.tile_count,
              minZoom: source.min_zoom,
              maxZoom: source.max_zoom,
              scheme: StyleTileScheme.fromRawValue(source.scheme),
            )
          : null,
      bounds: (source.fields & raw.MLN_STYLE_SOURCE_INFO_BOUNDS) != 0
          ? _readLatLngBounds(source.bounds)
          : null,
      tileSize: (source.fields & raw.MLN_STYLE_SOURCE_INFO_TILE_SIZE) != 0
          ? source.tile_size
          : null,
      vectorEncoding:
          (source.fields & raw.MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING) != 0
          ? StyleVectorTileEncoding.fromRawValue(source.vector_encoding)
          : null,
      rasterEncoding:
          (source.fields & raw.MLN_STYLE_SOURCE_INFO_RASTER_ENCODING) != 0
          ? StyleRasterDemEncoding.fromRawValue(source.raster_encoding)
          : null,
    );

StyleSourceResult _readStyleSourceResult(raw.mln_style_source_result source) =>
    StyleSourceResult(
      info: _readStyleSourceInfo(source.info),
      attribution: source.info.has_attribution
          ? utf8.decode(_copyBufferView(source.attribution))
          : null,
      url: (source.info.fields & raw.MLN_STYLE_SOURCE_INFO_URL) != 0
          ? utf8.decode(_copyBufferView(source.url))
          : null,
      tileUrls: (source.info.fields & raw.MLN_STYLE_SOURCE_INFO_TILEJSON) != 0
          ? List<String>.unmodifiable(
              List.generate(
                source.tile_url_count,
                (index) =>
                    utf8.decode(_copyBufferView(source.tile_urls[index])),
              ),
            )
          : null,
    );

StyleSourceTileUrlsResult _readStyleSourceTileUrlsResult(
  raw.mln_style_source_tile_urls_result source,
) => StyleSourceTileUrlsResult(
  tileUrls: List<String>.unmodifiable(
    List.generate(
      source.tile_url_count,
      (index) => utf8.decode(_copyBufferView(source.tile_urls[index])),
    ),
  ),
);

Pointer<raw.mln_style_transition_options> _writeStyleTransitionOptions(
  StyleTransitionOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_style_transition_options>();
  result.ref = raw.mln_style_transition_options_default();
  if (value.durationMs != null) {
    result.ref.fields |= raw.MLN_STYLE_TRANSITION_OPTION_DURATION;
    result.ref.duration_ms = value.durationMs!;
  }
  if (value.delayMs != null) {
    result.ref.fields |= raw.MLN_STYLE_TRANSITION_OPTION_DELAY;
    result.ref.delay_ms = value.delayMs!;
  }
  if (value.enablePlacementTransitions != null) {
    result.ref.fields |=
        raw.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
    result.ref.enable_placement_transitions = value.enablePlacementTransitions!;
  }
  return result;
}

StyleTransitionOptions _readStyleTransitionOptions(
  raw.mln_style_transition_options source,
) => StyleTransitionOptions(
  durationMs: (source.fields & raw.MLN_STYLE_TRANSITION_OPTION_DURATION) != 0
      ? source.duration_ms
      : null,
  delayMs: (source.fields & raw.MLN_STYLE_TRANSITION_OPTION_DELAY) != 0
      ? source.delay_ms
      : null,
  enablePlacementTransitions:
      (source.fields &
              raw.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) !=
          0
      ? source.enable_placement_transitions
      : null,
);

StyleLayerEntry _readStyleLayerEntry(raw.mln_style_layer_entry source) =>
    StyleLayerEntry(
      id: utf8.decode(_copyBufferView(source.id)),
      type: utf8.decode(_copyBufferView(source.type)),
      sourceId: source.source_id.size == 0
          ? null
          : utf8.decode(_copyBufferView(source.source_id)),
      sourceLayer: source.source_layer.size == 0
          ? null
          : utf8.decode(_copyBufferView(source.source_layer)),
    );

Pointer<raw.mln_projection_mode> _writeProjectionMode(
  ProjectionMode value,
  Arena arena,
) {
  final result = arena<raw.mln_projection_mode>();
  result.ref = raw.mln_projection_mode_default();
  if (value.axonometric != null) {
    result.ref.fields |= raw.MLN_PROJECTION_MODE_AXONOMETRIC;
    result.ref.axonometric = value.axonometric!;
  }
  if (value.xSkew != null) {
    result.ref.fields |= raw.MLN_PROJECTION_MODE_X_SKEW;
    result.ref.x_skew = value.xSkew!;
  }
  if (value.ySkew != null) {
    result.ref.fields |= raw.MLN_PROJECTION_MODE_Y_SKEW;
    result.ref.y_skew = value.ySkew!;
  }
  return result;
}

ProjectionMode _readProjectionMode(raw.mln_projection_mode source) =>
    ProjectionMode(
      axonometric: (source.fields & raw.MLN_PROJECTION_MODE_AXONOMETRIC) != 0
          ? source.axonometric
          : null,
      xSkew: (source.fields & raw.MLN_PROJECTION_MODE_X_SKEW) != 0
          ? source.x_skew
          : null,
      ySkew: (source.fields & raw.MLN_PROJECTION_MODE_Y_SKEW) != 0
          ? source.y_skew
          : null,
    );

Pointer<raw.mln_style_image_options> _writeStyleImageOptions(
  StyleImageOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_style_image_options>();
  result.ref = raw.mln_style_image_options_default();
  if (value.stretchX != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_STRETCH_X;
    result.ref.stretch_x = arena<raw.mln_image_stretch>(
      value.stretchX!.isEmpty ? 1 : value.stretchX!.length,
    );
    result.ref.stretch_x_count = value.stretchX!.length;
    for (var index = 0; index < value.stretchX!.length; index++) {
      result.ref.stretch_x[index] = _writeImageStretch(
        value.stretchX![index],
        arena,
      ).ref;
    }
  }
  if (value.stretchY != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
    result.ref.stretch_y = arena<raw.mln_image_stretch>(
      value.stretchY!.isEmpty ? 1 : value.stretchY!.length,
    );
    result.ref.stretch_y_count = value.stretchY!.length;
    for (var index = 0; index < value.stretchY!.length; index++) {
      result.ref.stretch_y[index] = _writeImageStretch(
        value.stretchY![index],
        arena,
      ).ref;
    }
  }
  if (value.content != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_CONTENT;
    result.ref.content = _writeImageContent(value.content!, arena).ref;
  }
  if (value.textFitWidth != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
    result.ref.text_fit_width = value.textFitWidth!.rawValue;
  }
  if (value.textFitHeight != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
    result.ref.text_fit_height = value.textFitHeight!.rawValue;
  }
  if (value.pixelRatio != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
    result.ref.pixel_ratio = value.pixelRatio!;
  }
  if (value.sdf != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_SDF;
    result.ref.sdf = value.sdf!;
  }
  return result;
}

StyleImageOptions _readStyleImageOptions(raw.mln_style_image_options source) =>
    StyleImageOptions(
      stretchX: (source.fields & raw.MLN_STYLE_IMAGE_OPTION_STRETCH_X) != 0
          ? List<ImageStretch>.unmodifiable(
              List.generate(
                source.stretch_x_count,
                (index) => _readImageStretch(source.stretch_x[index]),
              ),
            )
          : null,
      stretchY: (source.fields & raw.MLN_STYLE_IMAGE_OPTION_STRETCH_Y) != 0
          ? List<ImageStretch>.unmodifiable(
              List.generate(
                source.stretch_y_count,
                (index) => _readImageStretch(source.stretch_y[index]),
              ),
            )
          : null,
      content: (source.fields & raw.MLN_STYLE_IMAGE_OPTION_CONTENT) != 0
          ? _readImageContent(source.content)
          : null,
      textFitWidth:
          (source.fields & raw.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH) != 0
          ? StyleImageTextFit.fromRawValue(source.text_fit_width)
          : null,
      textFitHeight:
          (source.fields & raw.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT) != 0
          ? StyleImageTextFit.fromRawValue(source.text_fit_height)
          : null,
      pixelRatio: (source.fields & raw.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) != 0
          ? source.pixel_ratio
          : null,
      sdf: (source.fields & raw.MLN_STYLE_IMAGE_OPTION_SDF) != 0
          ? source.sdf
          : null,
    );

Pointer<raw.mln_map_tile_options> _writeMapTileOptions(
  MapTileOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_map_tile_options>();
  result.ref = raw.mln_map_tile_options_default();
  if (value.prefetchZoomDelta != null) {
    result.ref.fields |= raw.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
    result.ref.prefetch_zoom_delta = _nativeInteger(
      value.prefetchZoomDelta!,
      0,
      4294967295,
    );
  }
  if (value.lodMinRadius != null) {
    result.ref.fields |= raw.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
    result.ref.lod_min_radius = value.lodMinRadius!;
  }
  if (value.lodScale != null) {
    result.ref.fields |= raw.MLN_MAP_TILE_OPTION_LOD_SCALE;
    result.ref.lod_scale = value.lodScale!;
  }
  if (value.lodPitchThreshold != null) {
    result.ref.fields |= raw.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
    result.ref.lod_pitch_threshold = value.lodPitchThreshold!;
  }
  if (value.lodZoomShift != null) {
    result.ref.fields |= raw.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
    result.ref.lod_zoom_shift = value.lodZoomShift!;
  }
  if (value.lodMode != null) {
    result.ref.fields |= raw.MLN_MAP_TILE_OPTION_LOD_MODE;
    result.ref.lod_mode = value.lodMode!.rawValue;
  }
  return result;
}

MapTileOptions _readMapTileOptions(
  raw.mln_map_tile_options source,
) => MapTileOptions(
  prefetchZoomDelta:
      (source.fields & raw.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0
      ? source.prefetch_zoom_delta
      : null,
  lodMinRadius: (source.fields & raw.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0
      ? source.lod_min_radius
      : null,
  lodScale: (source.fields & raw.MLN_MAP_TILE_OPTION_LOD_SCALE) != 0
      ? source.lod_scale
      : null,
  lodPitchThreshold:
      (source.fields & raw.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0
      ? source.lod_pitch_threshold
      : null,
  lodZoomShift: (source.fields & raw.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0
      ? source.lod_zoom_shift
      : null,
  lodMode: (source.fields & raw.MLN_MAP_TILE_OPTION_LOD_MODE) != 0
      ? TileLodMode.fromRawValue(source.lod_mode)
      : null,
);

Pointer<raw.mln_map_viewport_options> _writeMapViewportOptions(
  MapViewportOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_map_viewport_options>();
  result.ref = raw.mln_map_viewport_options_default();
  if (value.northOrientation != null) {
    result.ref.fields |= raw.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
    result.ref.north_orientation = value.northOrientation!.rawValue;
  }
  if (value.constrainMode != null) {
    result.ref.fields |= raw.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
    result.ref.constrain_mode = value.constrainMode!.rawValue;
  }
  if (value.viewportMode != null) {
    result.ref.fields |= raw.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
    result.ref.viewport_mode = value.viewportMode!.rawValue;
  }
  if (value.frustumOffset != null) {
    result.ref.fields |= raw.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
    result.ref.frustum_offset = _writeEdgeInsets(
      value.frustumOffset!,
      arena,
    ).ref;
  }
  return result;
}

MapViewportOptions _readMapViewportOptions(
  raw.mln_map_viewport_options source,
) => MapViewportOptions(
  northOrientation:
      (source.fields & raw.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0
      ? NorthOrientation.fromRawValue(source.north_orientation)
      : null,
  constrainMode:
      (source.fields & raw.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0
      ? ConstrainMode.fromRawValue(source.constrain_mode)
      : null,
  viewportMode: (source.fields & raw.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0
      ? ViewportMode.fromRawValue(source.viewport_mode)
      : null,
  frustumOffset:
      (source.fields & raw.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0
      ? _readEdgeInsets(source.frustum_offset)
      : null,
);

MapSnapshot _readMapSnapshot(raw.mln_map_snapshot source) => MapSnapshot(
  debugOptions: MapDebugOption.fromRawValue(source.debug_options),
  generation: uint64FromNative(source.generation),
  camera: _readCameraOptions(source.camera),
  logicalExtent: _readLogicalExtent(source.logical_extent),
  projectionMode: _readProjectionMode(source.projection_mode),
  viewport: _readMapViewportOptions(source.viewport),
  fullyLoaded: source.fully_loaded,
  renderingStatsViewEnabled: source.rendering_stats_view_enabled,
  repaintDemand: source.repaint_demand,
  gestureInProgress: source.gesture_in_progress,
  eventMask: RuntimeEventMask.fromRawValue(source.event_mask),
  latestRenderUpdateGeneration: uint64FromNative(
    source.latest_render_update_generation,
  ),
  tile: _readMapTileOptions(source.tile),
  bounds: _readBoundOptions(source.bounds),
  freeCamera: _readFreeCameraOptions(source.free_camera),
);

Pointer<raw.mln_render_target_extent> _writeRenderTargetExtent(
  RenderTargetExtent value,
  Arena arena,
) {
  final result = arena<raw.mln_render_target_extent>();
  result.ref.size = sizeOf<raw.mln_render_target_extent>();
  result.ref.width = _nativeInteger(value.width, 0, 4294967295);
  result.ref.height = _nativeInteger(value.height, 0, 4294967295);
  result.ref.scale_factor = value.scaleFactor;
  return result;
}

RenderTargetExtent _readRenderTargetExtent(
  raw.mln_render_target_extent source,
) => RenderTargetExtent(
  width: source.width,
  height: source.height,
  scaleFactor: source.scale_factor,
);

Pointer<raw.mln_metal_borrowed_texture_descriptor>
_writeMetalBorrowedTextureDescriptor(
  MetalBorrowedTextureDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_metal_borrowed_texture_descriptor>();
  result.ref = raw.mln_metal_borrowed_texture_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.physical_width = _nativeInteger(
    value.physicalWidth,
    0,
    4294967295,
  );
  result.ref.physical_height = _nativeInteger(
    value.physicalHeight,
    0,
    4294967295,
  );
  result.ref.texture = Pointer<Void>.fromAddress(value.texture.address).cast();
  return result;
}

MetalBorrowedTextureDescriptor _readMetalBorrowedTextureDescriptor(
  raw.mln_metal_borrowed_texture_descriptor source,
) => MetalBorrowedTextureDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  physicalWidth: source.physical_width,
  physicalHeight: source.physical_height,
  texture: NativePointer(source.texture.address),
);

_NativeRegistration<raw.mln_wake> _prepareWake(
  Wake value,
  _NativeCallbackPorts roots,
) {
  final arena = Arena();
  _NativeCallbackPort? port;
  try {
    final result = arena<raw.mln_wake>();
    result.ref.size = sizeOf<raw.mln_wake>();
    if (value.callback == null) {
      return _NativeRegistration(result, () {}, arena.releaseAll);
    }
    port = roots.register({
      if (value.callback != null)
        (raw.MLN_ADAPTER_DART_PORT_WAKE_CALLBACK & 0xffffffff): (message) =>
            value.callback!(),
    });
    result.ref.callback = value.callback == null
        ? nullptr
        : raw
              .mln_adapter_dart_port_function(
                (raw.MLN_ADAPTER_DART_PORT_WAKE_CALLBACK & 0xffffffff),
              )
              .cast();
    result.ref.user_data = port.context;
    result.ref.release_user_data =
        Native.addressOf<
              NativeFunction<raw.mln_runtime_callback_releaseFunction>
            >(raw.mln_adapter_dart_port_release)
            .cast();
    return _NativeRegistration(result, port.reject, arena.releaseAll);
  } catch (_) {
    port?.reject();
    arena.releaseAll();
    rethrow;
  }
}

Pointer<raw.mln_render_session_attach_options> _writeRenderSessionAttachOptions(
  RenderSessionAttachOptions value,
  Arena arena,
  _NativeRegistrations registrations,
) {
  final result = arena<raw.mln_render_session_attach_options>();
  result.ref = raw.mln_render_session_attach_options_default();
  result.ref.driver = value.driver.rawValue;
  result.ref.requested_texture_ring_depth = _nativeInteger(
    value.requestedTextureRingDepth,
    0,
    4294967295,
  );
  result.ref.frame_wake = registrations
      .add(_prepareWake(value.frameWake, registrations.ports))
      .ref;
  result.ref.driver_work_wake = registrations
      .add(_prepareWake(value.driverWorkWake, registrations.ports))
      .ref;
  return result;
}

RenderSessionAttachOptions _readRenderSessionAttachOptions(
  raw.mln_render_session_attach_options source,
) => RenderSessionAttachOptions(
  driver: RenderDriverKind.fromRawValue(source.driver),
  requestedTextureRingDepth: source.requested_texture_ring_depth,
  frameWake: const Wake(),
  driverWorkWake: const Wake(),
);

Pointer<raw.mln_metal_context_descriptor> _writeMetalContextDescriptor(
  MetalContextDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_metal_context_descriptor>();
  result.ref.size = sizeOf<raw.mln_metal_context_descriptor>();
  result.ref.device = Pointer<Void>.fromAddress(value.device.address).cast();
  return result;
}

MetalContextDescriptor _readMetalContextDescriptor(
  raw.mln_metal_context_descriptor source,
) => MetalContextDescriptor(device: NativePointer(source.device.address));

Pointer<raw.mln_metal_owned_texture_descriptor>
_writeMetalOwnedTextureDescriptor(
  MetalOwnedTextureDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_metal_owned_texture_descriptor>();
  result.ref = raw.mln_metal_owned_texture_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.context = _writeMetalContextDescriptor(value.context, arena).ref;
  return result;
}

MetalOwnedTextureDescriptor _readMetalOwnedTextureDescriptor(
  raw.mln_metal_owned_texture_descriptor source,
) => MetalOwnedTextureDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  context: _readMetalContextDescriptor(source.context),
);

Pointer<raw.mln_metal_surface_descriptor> _writeMetalSurfaceDescriptor(
  MetalSurfaceDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_metal_surface_descriptor>();
  result.ref = raw.mln_metal_surface_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.context = _writeMetalContextDescriptor(value.context, arena).ref;
  result.ref.layer = Pointer<Void>.fromAddress(value.layer.address).cast();
  return result;
}

MetalSurfaceDescriptor _readMetalSurfaceDescriptor(
  raw.mln_metal_surface_descriptor source,
) => MetalSurfaceDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  context: _readMetalContextDescriptor(source.context),
  layer: NativePointer(source.layer.address),
);

Pointer<raw.mln_wgl_context_descriptor> _writeWglContextDescriptor(
  WglContextDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_wgl_context_descriptor>();
  result.ref.size = sizeOf<raw.mln_wgl_context_descriptor>();
  result.ref.device_context = Pointer<Void>.fromAddress(
    value.deviceContext.address,
  ).cast();
  result.ref.share_context = Pointer<Void>.fromAddress(
    value.shareContext.address,
  ).cast();
  result.ref.get_proc_address = Pointer<Void>.fromAddress(
    value.getProcAddress.address,
  ).cast();
  return result;
}

WglContextDescriptor _readWglContextDescriptor(
  raw.mln_wgl_context_descriptor source,
) => WglContextDescriptor(
  deviceContext: NativePointer(source.device_context.address),
  shareContext: NativePointer(source.share_context.address),
  getProcAddress: NativePointer(source.get_proc_address.address),
);

Pointer<raw.mln_egl_context_descriptor> _writeEglContextDescriptor(
  EglContextDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_egl_context_descriptor>();
  result.ref.size = sizeOf<raw.mln_egl_context_descriptor>();
  result.ref.display = Pointer<Void>.fromAddress(value.display.address).cast();
  result.ref.config = Pointer<Void>.fromAddress(value.config.address).cast();
  result.ref.share_context = Pointer<Void>.fromAddress(
    value.shareContext.address,
  ).cast();
  result.ref.client_api = value.clientApi.rawValue;
  result.ref.get_proc_address = Pointer<Void>.fromAddress(
    value.getProcAddress.address,
  ).cast();
  return result;
}

EglContextDescriptor _readEglContextDescriptor(
  raw.mln_egl_context_descriptor source,
) => EglContextDescriptor(
  display: NativePointer(source.display.address),
  config: NativePointer(source.config.address),
  shareContext: NativePointer(source.share_context.address),
  clientApi: OpenglClientApi.fromRawValue(source.client_api),
  getProcAddress: NativePointer(source.get_proc_address.address),
);

Pointer<raw.mln_webgl_context_descriptor> _writeWebglContextDescriptor(
  WebglContextDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_webgl_context_descriptor>();
  result.ref.size = sizeOf<raw.mln_webgl_context_descriptor>();
  result.ref.kind = value.kind.rawValue;
  result.ref.context = _nativeInteger(value.context, -2147483648, 2147483647);
  result.ref.canvas_selector = nativeStringView(
    value.canvasSelector,
    arena,
  ).value;
  return result;
}

WebglContextDescriptor _readWebglContextDescriptor(
  raw.mln_webgl_context_descriptor source,
) => WebglContextDescriptor(
  kind: WebglContextKind.fromRawValue(source.kind),
  context: source.context,
  canvasSelector: utf8.decode(_copyBufferView(source.canvas_selector)),
);

Pointer<raw.mln_opengl_context_descriptor> _writeOpenglContextDescriptor(
  OpenglContextDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_opengl_context_descriptor>();
  result.ref.size = sizeOf<raw.mln_opengl_context_descriptor>();
  result.ref.ownership = value.ownership.rawValue;
  switch (value.data) {
    case OpenglContextDescriptorDataWgl(:final value):
      result.ref.platform = 1;
      result.ref.data.wgl = _writeWglContextDescriptor(value, arena).ref;
    case OpenglContextDescriptorDataEgl(:final value):
      result.ref.platform = 2;
      result.ref.data.egl = _writeEglContextDescriptor(value, arena).ref;
    case OpenglContextDescriptorDataWebgl(:final value):
      result.ref.platform = 3;
      result.ref.data.webgl = _writeWebglContextDescriptor(value, arena).ref;
    case OpenglContextDescriptorDataUnknown():
      throwInvalidArgument('unknown native union variant cannot be submitted');
  }
  return result;
}

OpenglContextDescriptor _readOpenglContextDescriptor(
  raw.mln_opengl_context_descriptor source, {
  Uint8List Function()? rawRecord,
}) => OpenglContextDescriptor(
  ownership: OpenglContextOwnership.fromRawValue(source.ownership),
  data: switch (source.platform) {
    1 => OpenglContextDescriptorDataWgl(
      _readWglContextDescriptor(source.data.wgl),
    ),
    2 => OpenglContextDescriptorDataEgl(
      _readEglContextDescriptor(source.data.egl),
    ),
    3 => OpenglContextDescriptorDataWebgl(
      _readWebglContextDescriptor(source.data.webgl),
    ),
    final tag => OpenglContextDescriptorDataUnknown(
      tag,
      rawRecord?.call() ??
          withNativeArena((arena) {
            final copy = arena<raw.mln_opengl_context_descriptor>()
              ..ref = source;
            return Uint8List.fromList(
              copy.cast<Uint8>().asTypedList(
                sizeOf<raw.mln_opengl_context_descriptor>(),
              ),
            );
          }),
    ),
  },
);

Pointer<raw.mln_opengl_borrowed_texture_descriptor>
_writeOpenglBorrowedTextureDescriptor(
  OpenglBorrowedTextureDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_opengl_borrowed_texture_descriptor>();
  result.ref = raw.mln_opengl_borrowed_texture_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.physical_width = _nativeInteger(
    value.physicalWidth,
    0,
    4294967295,
  );
  result.ref.physical_height = _nativeInteger(
    value.physicalHeight,
    0,
    4294967295,
  );
  result.ref.context = _writeOpenglContextDescriptor(value.context, arena).ref;
  result.ref.texture = _nativeInteger(value.texture, 0, 4294967295);
  result.ref.target = _nativeInteger(value.target, 0, 4294967295);
  return result;
}

OpenglBorrowedTextureDescriptor _readOpenglBorrowedTextureDescriptor(
  raw.mln_opengl_borrowed_texture_descriptor source,
) => OpenglBorrowedTextureDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  physicalWidth: source.physical_width,
  physicalHeight: source.physical_height,
  context: _readOpenglContextDescriptor(source.context),
  texture: source.texture,
  target: source.target,
);

Pointer<raw.mln_opengl_owned_texture_descriptor>
_writeOpenglOwnedTextureDescriptor(
  OpenglOwnedTextureDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_opengl_owned_texture_descriptor>();
  result.ref = raw.mln_opengl_owned_texture_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.context = _writeOpenglContextDescriptor(value.context, arena).ref;
  return result;
}

OpenglOwnedTextureDescriptor _readOpenglOwnedTextureDescriptor(
  raw.mln_opengl_owned_texture_descriptor source,
) => OpenglOwnedTextureDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  context: _readOpenglContextDescriptor(source.context),
);

Pointer<raw.mln_opengl_surface_descriptor> _writeOpenglSurfaceDescriptor(
  OpenglSurfaceDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_opengl_surface_descriptor>();
  result.ref = raw.mln_opengl_surface_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.context = _writeOpenglContextDescriptor(value.context, arena).ref;
  result.ref.surface = Pointer<Void>.fromAddress(value.surface.address).cast();
  return result;
}

OpenglSurfaceDescriptor _readOpenglSurfaceDescriptor(
  raw.mln_opengl_surface_descriptor source,
) => OpenglSurfaceDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  context: _readOpenglContextDescriptor(source.context),
  surface: NativePointer(source.surface.address),
);

RenderAbandonResult _readRenderAbandonResult(
  raw.mln_render_abandon_result source,
) => RenderAbandonResult(
  disposition: RenderAbandonDisposition.fromRawValue(source.disposition),
  quarantinedResourceCount: source.quarantined_resource_count,
);

RenderSessionCapabilities _readRenderSessionCapabilities(
  raw.mln_render_session_capabilities source,
) => RenderSessionCapabilities(
  driver: RenderDriverKind.fromRawValue(source.driver),
  textureRingDepth: source.texture_ring_depth,
  flags: RenderSessionCapabilityFlag.fromRawValue(source.flags),
);

RenderSessionSnapshot _readRenderSessionSnapshot(
  raw.mln_render_session_snapshot source,
) => RenderSessionSnapshot(
  state: RenderSessionState.fromRawValue(source.state),
  driver: RenderDriverKind.fromRawValue(source.driver),
  latestResult: RenderResult.fromRawValue(source.latest_result),
  extent: _readRenderTargetExtent(source.extent),
  generation: uint64FromNative(source.generation),
  mapUpdateGeneration: uint64FromNative(source.map_update_generation),
  renderedUpdateGeneration: uint64FromNative(source.rendered_update_generation),
  extentGeneration: uint64FromNative(source.extent_generation),
  frameGeneration: uint64FromNative(source.frame_generation),
  latestDemandToken: uint64FromNative(source.latest_demand_token),
  pendingDemandCount: source.pending_demand_count,
  acquiredFrameCount: source.acquired_frame_count,
  targetReady: source.target_ready,
  pendingChanges: source.pending_changes,
);

Pointer<raw.mln_screen_box> _writeScreenBox(ScreenBox value, Arena arena) {
  final result = arena<raw.mln_screen_box>();
  result.ref.min = _writeScreenPoint(value.min, arena).ref;
  result.ref.max = _writeScreenPoint(value.max, arena).ref;
  return result;
}

ScreenBox _readScreenBox(raw.mln_screen_box source) => ScreenBox(
  min: _readScreenPoint(source.min),
  max: _readScreenPoint(source.max),
);

Pointer<raw.mln_screen_line_string> _writeScreenLineString(
  ScreenLineString value,
  Arena arena,
) {
  final result = arena<raw.mln_screen_line_string>();
  result.ref.points = arena<raw.mln_screen_point>(
    value.points.isEmpty ? 1 : value.points.length,
  );
  result.ref.point_count = value.points.length;
  for (var index = 0; index < value.points.length; index++) {
    result.ref.points[index] = _writeScreenPoint(
      value.points[index],
      arena,
    ).ref;
  }
  return result;
}

ScreenLineString _readScreenLineString(raw.mln_screen_line_string source) =>
    ScreenLineString(
      points: List<ScreenPoint>.unmodifiable(
        List.generate(
          source.point_count,
          (index) => _readScreenPoint(source.points[index]),
        ),
      ),
    );

Pointer<raw.mln_rendered_query_geometry> _writeRenderedQueryGeometry(
  RenderedQueryGeometry value,
  Arena arena,
) {
  final result = arena<raw.mln_rendered_query_geometry>();
  result.ref.size = sizeOf<raw.mln_rendered_query_geometry>();
  switch (value) {
    case RenderedQueryGeometryPoint():
      result.ref.type = 1;
      result.ref.data.point = _writeScreenPoint(value.value, arena).ref;
    case RenderedQueryGeometryBox():
      result.ref.type = 2;
      result.ref.data.box = _writeScreenBox(value.value, arena).ref;
    case RenderedQueryGeometryLineString():
      result.ref.type = 3;
      result.ref.data.line_string = _writeScreenLineString(
        value.value,
        arena,
      ).ref;
  }
  return result;
}

RenderedQueryGeometry _readRenderedQueryGeometry(
  raw.mln_rendered_query_geometry source,
) => switch (source.type) {
  1 => RenderedQueryGeometry.point(_readScreenPoint(source.data.point)),
  2 => RenderedQueryGeometry.box(_readScreenBox(source.data.box)),
  3 => RenderedQueryGeometry.lineString(
    _readScreenLineString(source.data.line_string),
  ),
  _ => throwInvalidState('native union contains an unsupported variant'),
};

Pointer<raw.mln_rendered_feature_query_options>
_writeRenderedFeatureQueryOptions(
  RenderedFeatureQueryOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_rendered_feature_query_options>();
  result.ref = raw.mln_rendered_feature_query_options_default();
  if (value.layerIds != null) {
    result.ref.fields |= raw.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
    result.ref.layer_ids = arena<raw.mln_buffer_view>(
      value.layerIds!.isEmpty ? 1 : value.layerIds!.length,
    );
    result.ref.layer_id_count = value.layerIds!.length;
    for (var index = 0; index < value.layerIds!.length; index++) {
      result.ref.layer_ids[index] = nativeStringView(
        value.layerIds![index],
        arena,
      ).value;
    }
  }
  result.ref.filter = value.filter == null
      ? nullptr
      : (() {
          final storage = arena<raw.mln_buffer_view>();
          storage.ref = nativeBufferView(value.filter!, arena);
          return storage;
        })();
  return result;
}

RenderedFeatureQueryOptions _readRenderedFeatureQueryOptions(
  raw.mln_rendered_feature_query_options source,
) => RenderedFeatureQueryOptions(
  layerIds:
      (source.fields & raw.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0
      ? List<String>.unmodifiable(
          List.generate(
            source.layer_id_count,
            (index) => utf8.decode(_copyBufferView(source.layer_ids[index])),
          ),
        )
      : null,
  filter: source.filter == nullptr ? null : _copyBufferView(source.filter.ref),
);

QueriedFeature _readQueriedFeature(raw.mln_queried_feature source) =>
    QueriedFeature(
      feature: _copyBufferView(source.feature),
      sourceId: (source.fields & raw.MLN_QUERIED_FEATURE_SOURCE_ID) != 0
          ? utf8.decode(_copyBufferView(source.source_id))
          : null,
      sourceLayerId:
          (source.fields & raw.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0
          ? utf8.decode(_copyBufferView(source.source_layer_id))
          : null,
      state: (source.fields & raw.MLN_QUERIED_FEATURE_STATE) != 0
          ? _copyBufferView(source.state)
          : null,
    );

Pointer<raw.mln_source_feature_query_options> _writeSourceFeatureQueryOptions(
  SourceFeatureQueryOptions value,
  Arena arena,
) {
  final result = arena<raw.mln_source_feature_query_options>();
  result.ref = raw.mln_source_feature_query_options_default();
  if (value.sourceLayerIds != null) {
    result.ref.fields |= raw.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
    result.ref.source_layer_ids = arena<raw.mln_buffer_view>(
      value.sourceLayerIds!.isEmpty ? 1 : value.sourceLayerIds!.length,
    );
    result.ref.source_layer_id_count = value.sourceLayerIds!.length;
    for (var index = 0; index < value.sourceLayerIds!.length; index++) {
      result.ref.source_layer_ids[index] = nativeStringView(
        value.sourceLayerIds![index],
        arena,
      ).value;
    }
  }
  result.ref.filter = value.filter == null
      ? nullptr
      : (() {
          final storage = arena<raw.mln_buffer_view>();
          storage.ref = nativeBufferView(value.filter!, arena);
          return storage;
        })();
  return result;
}

SourceFeatureQueryOptions _readSourceFeatureQueryOptions(
  raw.mln_source_feature_query_options source,
) => SourceFeatureQueryOptions(
  sourceLayerIds:
      (source.fields & raw.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) !=
          0
      ? List<String>.unmodifiable(
          List.generate(
            source.source_layer_id_count,
            (index) =>
                utf8.decode(_copyBufferView(source.source_layer_ids[index])),
          ),
        )
      : null,
  filter: source.filter == nullptr ? null : _copyBufferView(source.filter.ref),
);

Pointer<raw.mln_resource_response> _writeResourceResponse(
  ResourceResponse value,
  Arena arena,
) {
  final result = arena<raw.mln_resource_response>();
  result.ref.size = sizeOf<raw.mln_resource_response>();
  result.ref.status = value.status.rawValue;
  result.ref.error_reason = value.errorReason.rawValue;
  final bytesbytes = nativeBufferView(value.bytes, arena);
  result.ref.bytes = bytesbytes.data.cast();
  result.ref.byte_count = bytesbytes.size;
  result.ref.error_message = value.errorMessage == null
      ? nullptr
      : nativeUtf8CString(value.errorMessage!, arena).pointer.cast<Char>();
  result.ref.must_revalidate = value.mustRevalidate;
  if (value.modifiedUnixMs != null) {
    result.ref.has_modified = true;
    result.ref.modified_unix_ms = value.modifiedUnixMs!;
  }
  if (value.expiresUnixMs != null) {
    result.ref.has_expires = true;
    result.ref.expires_unix_ms = value.expiresUnixMs!;
  }
  result.ref.etag = value.etag == null
      ? nullptr
      : nativeUtf8CString(value.etag!, arena).pointer.cast<Char>();
  if (value.retryAfterUnixMs != null) {
    result.ref.has_retry_after = true;
    result.ref.retry_after_unix_ms = value.retryAfterUnixMs!;
  }
  return result;
}

Pointer<raw.mln_runtime_options> _writeRuntimeOptions(
  RuntimeOptions value,
  Arena arena,
  _NativeRegistrations registrations,
) {
  final result = arena<raw.mln_runtime_options>();
  result.ref = raw.mln_runtime_options_default();
  result.ref.flags = _nativeInteger(value.flags, 0, 4294967295);
  result.ref.asset_path = value.assetPath == null
      ? nullptr
      : nativeUtf8CString(value.assetPath!, arena).pointer.cast<Char>();
  result.ref.cache_path = value.cachePath == null
      ? nullptr
      : nativeUtf8CString(value.cachePath!, arena).pointer.cast<Char>();
  result.ref.event_mask = value.eventMask.rawValue;
  result.ref.event_wake = registrations
      .add(_prepareWake(value.eventWake, registrations.ports))
      .ref;
  return result;
}

RuntimeOptions _readRuntimeOptions(raw.mln_runtime_options source) =>
    RuntimeOptions(
      flags: source.flags,
      assetPath: source.asset_path == nullptr
          ? null
          : source.asset_path.cast<Utf8>().toDartString(),
      cachePath: source.cache_path == nullptr
          ? null
          : source.cache_path.cast<Utf8>().toDartString(),
      eventMask: RuntimeEventMask.fromRawValue(source.event_mask),
      eventWake: const Wake(),
    );

Pointer<raw.mln_offline_tile_pyramid_region_definition>
_writeOfflineTilePyramidRegionDefinition(
  OfflineTilePyramidRegionDefinition value,
  Arena arena,
) {
  final result = arena<raw.mln_offline_tile_pyramid_region_definition>();
  result.ref.size = sizeOf<raw.mln_offline_tile_pyramid_region_definition>();
  result.ref.style_url = nativeUtf8CString(
    value.styleUrl,
    arena,
  ).pointer.cast<Char>();
  result.ref.bounds = _writeLatLngBounds(value.bounds, arena).ref;
  result.ref.min_zoom = value.minZoom;
  result.ref.max_zoom = value.maxZoom;
  result.ref.pixel_ratio = value.pixelRatio;
  result.ref.include_ideographs = value.includeIdeographs;
  return result;
}

OfflineTilePyramidRegionDefinition _readOfflineTilePyramidRegionDefinition(
  raw.mln_offline_tile_pyramid_region_definition source,
) => OfflineTilePyramidRegionDefinition(
  styleUrl: source.style_url.cast<Utf8>().toDartString(),
  bounds: _readLatLngBounds(source.bounds),
  minZoom: source.min_zoom,
  maxZoom: source.max_zoom,
  pixelRatio: source.pixel_ratio,
  includeIdeographs: source.include_ideographs,
);

Pointer<raw.mln_offline_geometry_region_definition>
_writeOfflineGeometryRegionDefinition(
  OfflineGeometryRegionDefinition value,
  Arena arena,
) {
  final result = arena<raw.mln_offline_geometry_region_definition>();
  result.ref.size = sizeOf<raw.mln_offline_geometry_region_definition>();
  result.ref.style_url = nativeUtf8CString(
    value.styleUrl,
    arena,
  ).pointer.cast<Char>();
  result.ref.geometry = nativeBufferView(value.geometry, arena);
  result.ref.min_zoom = value.minZoom;
  result.ref.max_zoom = value.maxZoom;
  result.ref.pixel_ratio = value.pixelRatio;
  result.ref.include_ideographs = value.includeIdeographs;
  return result;
}

OfflineGeometryRegionDefinition _readOfflineGeometryRegionDefinition(
  raw.mln_offline_geometry_region_definition source,
) => OfflineGeometryRegionDefinition(
  styleUrl: source.style_url.cast<Utf8>().toDartString(),
  geometry: _copyBufferView(source.geometry),
  minZoom: source.min_zoom,
  maxZoom: source.max_zoom,
  pixelRatio: source.pixel_ratio,
  includeIdeographs: source.include_ideographs,
);

Pointer<raw.mln_offline_region_definition> _writeOfflineRegionDefinition(
  OfflineRegionDefinition value,
  Arena arena,
) {
  final result = arena<raw.mln_offline_region_definition>();
  result.ref.size = sizeOf<raw.mln_offline_region_definition>();
  switch (value) {
    case OfflineRegionDefinitionTilePyramid():
      result.ref.type = 1;
      result.ref.data.tile_pyramid = _writeOfflineTilePyramidRegionDefinition(
        value.value,
        arena,
      ).ref;
    case OfflineRegionDefinitionGeometry():
      result.ref.type = 2;
      result.ref.data.geometry = _writeOfflineGeometryRegionDefinition(
        value.value,
        arena,
      ).ref;
  }
  return result;
}

OfflineRegionDefinition _readOfflineRegionDefinition(
  raw.mln_offline_region_definition source,
) => switch (source.type) {
  1 => OfflineRegionDefinition.tilePyramid(
    _readOfflineTilePyramidRegionDefinition(source.data.tile_pyramid),
  ),
  2 => OfflineRegionDefinition.geometry(
    _readOfflineGeometryRegionDefinition(source.data.geometry),
  ),
  _ => throwInvalidState('native union contains an unsupported variant'),
};

OfflineRegionInfo _readOfflineRegionInfo(raw.mln_offline_region_info source) =>
    OfflineRegionInfo(
      id: source.id,
      definition: _readOfflineRegionDefinition(source.definition),
      metadata: Uint8List.fromList(
        source.metadata.cast<Uint8>().asTypedList(source.metadata_size),
      ),
    );

Pointer<raw.mln_adapter_http_header> _writeAdapterHttpHeader(
  AdapterHttpHeader value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_http_header>();
  result.ref.name = value.name == null
      ? nullptr
      : nativeUtf8CString(value.name!, arena).pointer.cast<Char>();
  result.ref.value = value.value == null
      ? nullptr
      : nativeUtf8CString(value.value!, arena).pointer.cast<Char>();
  return result;
}

Pointer<raw.mln_adapter_http_header_transform_rule>
_writeAdapterHttpHeaderTransformRule(
  AdapterHttpHeaderTransformRule value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_http_header_transform_rule>();
  result.ref.kind = _nativeInteger(value.kind, 0, 4294967295);
  result.ref.flags = value.flags.rawValue;
  result.ref.url = value.url == null
      ? nullptr
      : nativeUtf8CString(value.url!, arena).pointer.cast<Char>();
  result.ref.headers = arena<raw.mln_adapter_http_header>(
    value.headers.isEmpty ? 1 : value.headers.length,
  );
  result.ref.header_count = value.headers.length;
  for (var index = 0; index < value.headers.length; index++) {
    result.ref.headers[index] = _writeAdapterHttpHeader(
      value.headers[index],
      arena,
    ).ref;
  }
  return result;
}

Pointer<raw.mln_adapter_http_header_transform_rules>
_writeAdapterHttpHeaderTransformRules(
  AdapterHttpHeaderTransformRules value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_http_header_transform_rules>();
  result.ref.rules = arena<raw.mln_adapter_http_header_transform_rule>(
    value.rules.isEmpty ? 1 : value.rules.length,
  );
  result.ref.count = value.rules.length;
  for (var index = 0; index < value.rules.length; index++) {
    result.ref.rules[index] = _writeAdapterHttpHeaderTransformRule(
      value.rules[index],
      arena,
    ).ref;
  }
  return result;
}

_NativeRegistration<raw.mln_http_header_transform> _prepareHttpHeaderTransform(
  HttpHeaderTransform value,
  NativeCallbackReleases roots,
) {
  final arena = NativeOwnedArena();
  var transferred = false;
  try {
    switch (value) {
      case HttpHeaderTransformEmpty():
        final descriptor = arena<raw.mln_http_header_transform>();
        descriptor.ref.size = sizeOf<raw.mln_http_header_transform>();
        return _NativeRegistration(
          descriptor,
          arena.releaseAll,
          arena.releaseAll,
        );
      case HttpHeaderTransformHttpHeaderTransformRules():
        final context = _writeAdapterHttpHeaderTransformRules(
          value.value,
          arena,
        );
        final descriptor = arena<raw.mln_http_header_transform>();
        descriptor.ref.size = sizeOf<raw.mln_http_header_transform>();
        descriptor.ref.callback =
            Native.addressOf<
              NativeFunction<raw.mln_http_header_transform_callbackFunction>
            >(raw.mln_adapter_http_header_transform_callback);
        descriptor.ref.user_data = context.cast();
        descriptor.ref.release_user_data =
            Native.addressOf<
              NativeFunction<raw.mln_runtime_callback_releaseFunction>
            >(raw.mln_adapter_dart_release);
        transferred = true;
        roots.register(context.cast(), arena.releaseAll, arena: arena);
        return _NativeRegistration(
          descriptor,
          () => roots.reject(context.cast()),
        );
    }
  } catch (_) {
    if (!transferred) {
      arena.releaseAll();
    }
    rethrow;
  }
}

Pointer<raw.mln_adapter_resource_provider_rule>
_writeAdapterResourceProviderRule(
  AdapterResourceProviderRule value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_resource_provider_rule>();
  result.ref.kind = _nativeInteger(value.kind, 0, 4294967295);
  result.ref.flags = value.flags.rawValue;
  result.ref.requested_url = value.requestedUrl == null
      ? nullptr
      : nativeUtf8CString(value.requestedUrl!, arena).pointer.cast<Char>();
  result.ref.response = _writeResourceResponse(value.response, arena).ref;
  return result;
}

Pointer<raw.mln_adapter_resource_provider_rules>
_writeAdapterResourceProviderRules(
  AdapterResourceProviderRules value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_resource_provider_rules>();
  result.ref.rules = arena<raw.mln_adapter_resource_provider_rule>(
    value.rules.isEmpty ? 1 : value.rules.length,
  );
  result.ref.count = value.rules.length;
  for (var index = 0; index < value.rules.length; index++) {
    result.ref.rules[index] = _writeAdapterResourceProviderRule(
      value.rules[index],
      arena,
    ).ref;
  }
  return result;
}

Pointer<raw.mln_adapter_resource_route> _writeAdapterResourceRoute(
  AdapterResourceRoute value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_resource_route>();
  result.ref.kind = _nativeInteger(value.kind, 0, 4294967295);
  result.ref.flags = value.flags.rawValue;
  result.ref.url = value.url == null
      ? nullptr
      : nativeUtf8CString(value.url!, arena).pointer.cast<Char>();
  return result;
}

ResourceRequest _readResourceRequest(raw.mln_resource_request source) =>
    ResourceRequest(
      requestedUrl: source.requested_url == nullptr
          ? null
          : source.requested_url.cast<Utf8>().toDartString(),
      resolvedUrl: source.resolved_url == nullptr
          ? null
          : source.resolved_url.cast<Utf8>().toDartString(),
      kind: ResourceKind.fromRawValue(source.kind),
      loadingMethod: ResourceLoadingMethod.fromRawValue(source.loading_method),
      priority: ResourcePriority.fromRawValue(source.priority),
      usage: ResourceUsage.fromRawValue(source.usage),
      storagePolicy: ResourceStoragePolicy.fromRawValue(source.storage_policy),
      range: source.has_range
          ? (
              rangeStart: uint64FromNative(source.range_start),
              rangeEnd: uint64FromNative(source.range_end),
            )
          : null,
      priorModifiedUnixMs: source.has_prior_modified
          ? source.prior_modified_unix_ms
          : null,
      priorExpiresUnixMs: source.has_prior_expires
          ? source.prior_expires_unix_ms
          : null,
      priorEtag: source.prior_etag == nullptr
          ? null
          : source.prior_etag.cast<Utf8>().toDartString(),
      priorData: Uint8List.fromList(
        source.prior_data.cast<Uint8>().asTypedList(source.prior_data_size),
      ),
    );

void _deliverResourceProviderCallback(
  ResourceProviderCallback callback,
  List<dynamic> message,
) {
  final record = Pointer<raw.mln_adapter_deferred_call_record>.fromAddress(
    message[1] as int,
  );
  ResourceRequestHandle? owner;
  try {
    final arguments = record.ref.arguments
        .cast<raw.mln_adapter_resource_provider_callback_arguments>()
        .ref;
    final adopted = owner = ResourceRequestHandle._(
      NativeResourceRequest(arguments.handle),
    );
    raw.mln_adapter_deferred_call_record_adopt(record);
    callback(_readResourceRequest(arguments.request.ref), adopted);
  } catch (_) {
    owner?.close();
    rethrow;
  } finally {
    raw.mln_adapter_deferred_call_record_destroy(record);
  }
}

Pointer<raw.mln_adapter_routed_resource_provider>
_writeAdapterRoutedResourceProvider(
  AdapterRoutedResourceProvider value,
  NativeOwnedArena arena,
  _NativeCallbackPorts ports,
) {
  final result = arena<raw.mln_adapter_routed_resource_provider>();
  result.ref.routes = arena<raw.mln_adapter_resource_route>(
    value.routes.isEmpty ? 1 : value.routes.length,
  );
  result.ref.route_count = value.routes.length;
  for (var index = 0; index < value.routes.length; index++) {
    result.ref.routes[index] = _writeAdapterResourceRoute(
      value.routes[index],
      arena,
    ).ref;
  }
  final portCallback = ports.registerDeferred(
    (raw.MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK & 0xffffffff),
    (message) => _deliverResourceProviderCallback(value.callback, message),
  );
  arena.adoptRelease(
    Native.addressOf<NativeFunction<raw.mln_runtime_callback_releaseFunction>>(
      raw.mln_adapter_deferred_callback_release,
    ),
    portCallback.context,
  );
  result.ref.callback = raw
      .mln_adapter_deferred_callback_function(
        (raw.MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK & 0xffffffff),
      )
      .cast();
  result.ref.user_data = portCallback.context;
  return result;
}

_NativeRegistration<raw.mln_resource_provider> _prepareResourceProvider(
  ResourceProvider value,
  NativeCallbackReleases roots,
  _NativeCallbackPorts ports,
) {
  final arena = NativeOwnedArena();
  var transferred = false;
  try {
    switch (value) {
      case ResourceProviderEmpty():
        final descriptor = arena<raw.mln_resource_provider>();
        descriptor.ref.size = sizeOf<raw.mln_resource_provider>();
        return _NativeRegistration(
          descriptor,
          arena.releaseAll,
          arena.releaseAll,
        );
      case ResourceProviderResourceProviderRules():
        final context = _writeAdapterResourceProviderRules(value.value, arena);
        final descriptor = arena<raw.mln_resource_provider>();
        descriptor.ref.size = sizeOf<raw.mln_resource_provider>();
        descriptor.ref.callback =
            Native.addressOf<
              NativeFunction<raw.mln_resource_provider_callbackFunction>
            >(raw.mln_adapter_resource_provider_rules_callback);
        descriptor.ref.user_data = context.cast();
        descriptor.ref.release_user_data =
            Native.addressOf<
              NativeFunction<raw.mln_runtime_callback_releaseFunction>
            >(raw.mln_adapter_dart_release);
        transferred = true;
        roots.register(context.cast(), arena.releaseAll, arena: arena);
        return _NativeRegistration(
          descriptor,
          () => roots.reject(context.cast()),
        );
      case ResourceProviderRoutedResourceProvider():
        final context = _writeAdapterRoutedResourceProvider(
          value.value,
          arena,
          ports,
        );
        final descriptor = arena<raw.mln_resource_provider>();
        descriptor.ref.size = sizeOf<raw.mln_resource_provider>();
        descriptor.ref.callback =
            Native.addressOf<
              NativeFunction<raw.mln_resource_provider_callbackFunction>
            >(raw.mln_adapter_routed_resource_provider_callback);
        descriptor.ref.user_data = context.cast();
        descriptor.ref.release_user_data =
            Native.addressOf<
              NativeFunction<raw.mln_runtime_callback_releaseFunction>
            >(raw.mln_adapter_dart_release);
        transferred = true;
        roots.register(context.cast(), arena.releaseAll, arena: arena);
        return _NativeRegistration(
          descriptor,
          () => roots.reject(context.cast()),
        );
    }
  } catch (_) {
    if (!transferred) {
      arena.releaseAll();
    }
    rethrow;
  }
}

Pointer<raw.mln_adapter_resource_rewrite_rule> _writeAdapterResourceRewriteRule(
  AdapterResourceRewriteRule value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_resource_rewrite_rule>();
  result.ref.kind = _nativeInteger(value.kind, 0, 4294967295);
  result.ref.flags = value.flags.rawValue;
  result.ref.url = value.url == null
      ? nullptr
      : nativeUtf8CString(value.url!, arena).pointer.cast<Char>();
  result.ref.replacement_url = value.replacementUrl == null
      ? nullptr
      : nativeUtf8CString(value.replacementUrl!, arena).pointer.cast<Char>();
  return result;
}

Pointer<raw.mln_adapter_resource_rewrite_rules>
_writeAdapterResourceRewriteRules(
  AdapterResourceRewriteRules value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_resource_rewrite_rules>();
  result.ref.rules = arena<raw.mln_adapter_resource_rewrite_rule>(
    value.rules.isEmpty ? 1 : value.rules.length,
  );
  result.ref.count = value.rules.length;
  for (var index = 0; index < value.rules.length; index++) {
    result.ref.rules[index] = _writeAdapterResourceRewriteRule(
      value.rules[index],
      arena,
    ).ref;
  }
  return result;
}

_NativeRegistration<raw.mln_resource_transform> _prepareResourceTransform(
  ResourceTransform value,
  NativeCallbackReleases roots,
) {
  final arena = NativeOwnedArena();
  var transferred = false;
  try {
    switch (value) {
      case ResourceTransformEmpty():
        final descriptor = arena<raw.mln_resource_transform>();
        descriptor.ref.size = sizeOf<raw.mln_resource_transform>();
        return _NativeRegistration(
          descriptor,
          arena.releaseAll,
          arena.releaseAll,
        );
      case ResourceTransformResourceRewriteRules():
        final context = _writeAdapterResourceRewriteRules(value.value, arena);
        final descriptor = arena<raw.mln_resource_transform>();
        descriptor.ref.size = sizeOf<raw.mln_resource_transform>();
        descriptor.ref.callback =
            Native.addressOf<
              NativeFunction<raw.mln_resource_transform_callbackFunction>
            >(raw.mln_adapter_resource_transform_rewrite_callback);
        descriptor.ref.user_data = context.cast();
        descriptor.ref.release_user_data =
            Native.addressOf<
              NativeFunction<raw.mln_runtime_callback_releaseFunction>
            >(raw.mln_adapter_dart_release);
        transferred = true;
        roots.register(context.cast(), arena.releaseAll, arena: arena);
        return _NativeRegistration(
          descriptor,
          () => roots.reject(context.cast()),
        );
    }
  } catch (_) {
    if (!transferred) {
      arena.releaseAll();
    }
    rethrow;
  }
}

TextureImageInfo _readTextureImageInfo(raw.mln_texture_image_info source) =>
    TextureImageInfo(
      width: source.width,
      height: source.height,
      stride: source.stride,
      byteLength: source.byte_length,
    );

TextureReadbackResult _readTextureReadbackResult(
  raw.mln_texture_readback_result source,
) => TextureReadbackResult(
  data: _copyBufferView(source.data),
  info: _readTextureImageInfo(source.info),
);

Pointer<raw.mln_vulkan_context_descriptor> _writeVulkanContextDescriptor(
  VulkanContextDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_vulkan_context_descriptor>();
  result.ref.size = sizeOf<raw.mln_vulkan_context_descriptor>();
  result.ref.instance = Pointer<Void>.fromAddress(
    value.instance.address,
  ).cast();
  result.ref.physical_device = Pointer<Void>.fromAddress(
    value.physicalDevice.address,
  ).cast();
  result.ref.device = Pointer<Void>.fromAddress(value.device.address).cast();
  result.ref.graphics_queue = Pointer<Void>.fromAddress(
    value.graphicsQueue.address,
  ).cast();
  result.ref.graphics_queue_family_index = _nativeInteger(
    value.graphicsQueueFamilyIndex,
    0,
    4294967295,
  );
  result.ref.get_instance_proc_addr = Pointer<Void>.fromAddress(
    value.getInstanceProcAddr.address,
  ).cast();
  result.ref.get_device_proc_addr = Pointer<Void>.fromAddress(
    value.getDeviceProcAddr.address,
  ).cast();
  return result;
}

VulkanContextDescriptor _readVulkanContextDescriptor(
  raw.mln_vulkan_context_descriptor source,
) => VulkanContextDescriptor(
  instance: NativePointer(source.instance.address),
  physicalDevice: NativePointer(source.physical_device.address),
  device: NativePointer(source.device.address),
  graphicsQueue: NativePointer(source.graphics_queue.address),
  graphicsQueueFamilyIndex: source.graphics_queue_family_index,
  getInstanceProcAddr: NativePointer(source.get_instance_proc_addr.address),
  getDeviceProcAddr: NativePointer(source.get_device_proc_addr.address),
);

Pointer<raw.mln_vulkan_borrowed_texture_descriptor>
_writeVulkanBorrowedTextureDescriptor(
  VulkanBorrowedTextureDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_vulkan_borrowed_texture_descriptor>();
  result.ref = raw.mln_vulkan_borrowed_texture_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.physical_width = _nativeInteger(
    value.physicalWidth,
    0,
    4294967295,
  );
  result.ref.physical_height = _nativeInteger(
    value.physicalHeight,
    0,
    4294967295,
  );
  result.ref.context = _writeVulkanContextDescriptor(value.context, arena).ref;
  result.ref.image = uint64ToNative(
    value.image,
    'mln_vulkan_non_dispatchable_handle',
  );
  result.ref.image_view = uint64ToNative(
    value.imageView,
    'mln_vulkan_non_dispatchable_handle',
  );
  result.ref.format = _nativeInteger(value.format, 0, 4294967295);
  result.ref.initial_layout = _nativeInteger(
    value.initialLayout,
    0,
    4294967295,
  );
  result.ref.final_layout = _nativeInteger(value.finalLayout, 0, 4294967295);
  return result;
}

VulkanBorrowedTextureDescriptor _readVulkanBorrowedTextureDescriptor(
  raw.mln_vulkan_borrowed_texture_descriptor source,
) => VulkanBorrowedTextureDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  physicalWidth: source.physical_width,
  physicalHeight: source.physical_height,
  context: _readVulkanContextDescriptor(source.context),
  image: uint64FromNative(source.image),
  imageView: uint64FromNative(source.image_view),
  format: source.format,
  initialLayout: source.initial_layout,
  finalLayout: source.final_layout,
);

Pointer<raw.mln_vulkan_owned_texture_descriptor>
_writeVulkanOwnedTextureDescriptor(
  VulkanOwnedTextureDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_vulkan_owned_texture_descriptor>();
  result.ref = raw.mln_vulkan_owned_texture_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.context = _writeVulkanContextDescriptor(value.context, arena).ref;
  return result;
}

VulkanOwnedTextureDescriptor _readVulkanOwnedTextureDescriptor(
  raw.mln_vulkan_owned_texture_descriptor source,
) => VulkanOwnedTextureDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  context: _readVulkanContextDescriptor(source.context),
);

Pointer<raw.mln_vulkan_surface_descriptor> _writeVulkanSurfaceDescriptor(
  VulkanSurfaceDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_vulkan_surface_descriptor>();
  result.ref = raw.mln_vulkan_surface_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.context = _writeVulkanContextDescriptor(value.context, arena).ref;
  result.ref.surface = uint64ToNative(
    value.surface,
    'mln_vulkan_non_dispatchable_handle',
  );
  return result;
}

VulkanSurfaceDescriptor _readVulkanSurfaceDescriptor(
  raw.mln_vulkan_surface_descriptor source,
) => VulkanSurfaceDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  context: _readVulkanContextDescriptor(source.context),
  surface: uint64FromNative(source.surface),
);

Pointer<raw.mln_webgpu_context_descriptor> _writeWebgpuContextDescriptor(
  WebgpuContextDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_webgpu_context_descriptor>();
  result.ref.size = sizeOf<raw.mln_webgpu_context_descriptor>();
  result.ref.instance = Pointer<Void>.fromAddress(
    value.instance.address,
  ).cast();
  result.ref.device = Pointer<Void>.fromAddress(value.device.address).cast();
  result.ref.queue = Pointer<Void>.fromAddress(value.queue.address).cast();
  return result;
}

WebgpuContextDescriptor _readWebgpuContextDescriptor(
  raw.mln_webgpu_context_descriptor source,
) => WebgpuContextDescriptor(
  instance: NativePointer(source.instance.address),
  device: NativePointer(source.device.address),
  queue: NativePointer(source.queue.address),
);

Pointer<raw.mln_webgpu_borrowed_texture_descriptor>
_writeWebgpuBorrowedTextureDescriptor(
  WebgpuBorrowedTextureDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_webgpu_borrowed_texture_descriptor>();
  result.ref = raw.mln_webgpu_borrowed_texture_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.physical_width = _nativeInteger(
    value.physicalWidth,
    0,
    4294967295,
  );
  result.ref.physical_height = _nativeInteger(
    value.physicalHeight,
    0,
    4294967295,
  );
  result.ref.context = _writeWebgpuContextDescriptor(value.context, arena).ref;
  result.ref.texture = Pointer<Void>.fromAddress(value.texture.address).cast();
  result.ref.texture_view = Pointer<Void>.fromAddress(
    value.textureView.address,
  ).cast();
  result.ref.format = _nativeInteger(value.format, 0, 4294967295);
  return result;
}

WebgpuBorrowedTextureDescriptor _readWebgpuBorrowedTextureDescriptor(
  raw.mln_webgpu_borrowed_texture_descriptor source,
) => WebgpuBorrowedTextureDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  physicalWidth: source.physical_width,
  physicalHeight: source.physical_height,
  context: _readWebgpuContextDescriptor(source.context),
  texture: NativePointer(source.texture.address),
  textureView: NativePointer(source.texture_view.address),
  format: source.format,
);

Pointer<raw.mln_webgpu_owned_texture_descriptor>
_writeWebgpuOwnedTextureDescriptor(
  WebgpuOwnedTextureDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_webgpu_owned_texture_descriptor>();
  result.ref = raw.mln_webgpu_owned_texture_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.context = _writeWebgpuContextDescriptor(value.context, arena).ref;
  return result;
}

WebgpuOwnedTextureDescriptor _readWebgpuOwnedTextureDescriptor(
  raw.mln_webgpu_owned_texture_descriptor source,
) => WebgpuOwnedTextureDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  context: _readWebgpuContextDescriptor(source.context),
);

Pointer<raw.mln_webgpu_surface_descriptor> _writeWebgpuSurfaceDescriptor(
  WebgpuSurfaceDescriptor value,
  Arena arena,
) {
  final result = arena<raw.mln_webgpu_surface_descriptor>();
  result.ref = raw.mln_webgpu_surface_descriptor_default();
  result.ref.extent = _writeRenderTargetExtent(value.extent, arena).ref;
  result.ref.context = _writeWebgpuContextDescriptor(value.context, arena).ref;
  result.ref.surface = Pointer<Void>.fromAddress(value.surface.address).cast();
  result.ref.format = _nativeInteger(value.format, 0, 4294967295);
  return result;
}

WebgpuSurfaceDescriptor _readWebgpuSurfaceDescriptor(
  raw.mln_webgpu_surface_descriptor source,
) => WebgpuSurfaceDescriptor(
  extent: _readRenderTargetExtent(source.extent),
  context: _readWebgpuContextDescriptor(source.context),
  surface: NativePointer(source.surface.address),
  format: source.format,
);

final _resultCameraOptions = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_CAMERA_OPTIONS,
  sizeOf<raw.mln_camera_options>(),
  (element) => _readCameraOptions(element.cast<raw.mln_camera_options>().ref),
);
final _resultCameraQueryResult = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_CAMERA_QUERY_RESULT,
  sizeOf<raw.mln_camera_query_result>(),
  (element) =>
      _readCameraQueryResult(element.cast<raw.mln_camera_query_result>().ref),
);
final _resultLatLng = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_LAT_LNG,
  sizeOf<raw.mln_lat_lng>(),
  (element) => _readLatLng(element.cast<raw.mln_lat_lng>().ref),
);
final _resultLatLngBounds = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_LAT_LNG_BOUNDS,
  sizeOf<raw.mln_lat_lng_bounds>(),
  (element) => _readLatLngBounds(element.cast<raw.mln_lat_lng_bounds>().ref),
);
final _resultOfflineRegionInfo = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_INFO,
  sizeOf<raw.mln_offline_region_info>(),
  (element) =>
      _readOfflineRegionInfo(element.cast<raw.mln_offline_region_info>().ref),
);
final _resultOfflineRegionStatus = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_STATUS,
  sizeOf<raw.mln_offline_region_status>(),
  (element) => _readOfflineRegionStatus(
    element.cast<raw.mln_offline_region_status>().ref,
  ),
);
final _resultQueriedFeature = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_QUERIED_FEATURE,
  sizeOf<raw.mln_queried_feature>(),
  (element) => _readQueriedFeature(element.cast<raw.mln_queried_feature>().ref),
);
final _resultScreenPoint = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_SCREEN_POINT,
  sizeOf<raw.mln_screen_point>(),
  (element) => _readScreenPoint(element.cast<raw.mln_screen_point>().ref),
);
final _resultString = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
  sizeOf<raw.mln_buffer_view>(),
  (element) =>
      utf8.decode(_copyBufferView(element.cast<raw.mln_buffer_view>().ref)),
);
final _resultStringOrNull = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
  sizeOf<raw.mln_buffer_view>(),
  (element) => element.cast<raw.mln_buffer_view>().ref.data == nullptr
      ? null
      : utf8.decode(_copyBufferView(element.cast<raw.mln_buffer_view>().ref)),
);
final _resultStyleImageResult = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE_RESULT,
  sizeOf<raw.mln_style_image_result>(),
  (element) =>
      _readStyleImageResult(element.cast<raw.mln_style_image_result>().ref),
);
final _resultStyleImageStretchesResult = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE_STRETCHES_RESULT,
  sizeOf<raw.mln_style_image_stretches_result>(),
  (element) => _readStyleImageStretchesResult(
    element.cast<raw.mln_style_image_stretches_result>().ref,
  ),
);
final _resultStyleLayerEntry = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_STYLE_LAYER_ENTRY,
  sizeOf<raw.mln_style_layer_entry>(),
  (element) =>
      _readStyleLayerEntry(element.cast<raw.mln_style_layer_entry>().ref),
);
final _resultStyleLayerResult = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_STYLE_LAYER_RESULT,
  sizeOf<raw.mln_style_layer_result>(),
  (element) =>
      _readStyleLayerResult(element.cast<raw.mln_style_layer_result>().ref),
);
final _resultStyleSourceResult = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_RESULT,
  sizeOf<raw.mln_style_source_result>(),
  (element) =>
      _readStyleSourceResult(element.cast<raw.mln_style_source_result>().ref),
);
final _resultStyleSourceTileUrlsResult = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_TILE_URLS_RESULT,
  sizeOf<raw.mln_style_source_tile_urls_result>(),
  (element) => _readStyleSourceTileUrlsResult(
    element.cast<raw.mln_style_source_tile_urls_result>().ref,
  ),
);
final _resultStyleTransitionOptions = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_STYLE_TRANSITION_OPTIONS,
  sizeOf<raw.mln_style_transition_options>(),
  (element) => _readStyleTransitionOptions(
    element.cast<raw.mln_style_transition_options>().ref,
  ),
);
final _resultTextureReadbackResult = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_TEXTURE_READBACK_RESULT,
  sizeOf<raw.mln_texture_readback_result>(),
  (element) => _readTextureReadbackResult(
    element.cast<raw.mln_texture_readback_result>().ref,
  ),
);
final _resultUint8List = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
  sizeOf<raw.mln_buffer_view>(),
  (element) => _copyBufferView(element.cast<raw.mln_buffer_view>().ref),
);
final _resultUint8ListOrNull = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
  sizeOf<raw.mln_buffer_view>(),
  (element) => element.cast<raw.mln_buffer_view>().ref.data == nullptr
      ? null
      : _copyBufferView(element.cast<raw.mln_buffer_view>().ref),
);
final _resultdouble = _CompletionValue(
  raw.MLN_ADAPTER_COMPLETION_COPY_FLAT,
  sizeOf<Double>(),
  (element) => element.cast<Double>().value,
);

/// Initializes Android platform services.
///
/// See `mln_android_init` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
void androidInit(
  NativePointer jniEnv,
  NativePointer jniClass,
  NativePointer context,
) {
  ensureAbiVersion();
  return _check(
    raw.mln_android_init(
      Pointer<Void>.fromAddress(jniEnv.address).cast(),
      Pointer<Void>.fromAddress(jniClass.address).cast(),
      Pointer<Void>.fromAddress(context.address).cast(),
      nativeDiagnostic,
    ),
  );
}

/// Returns empty animation options initialized for this C API version.
///
/// See `mln_animation_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
AnimationOptions animationOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_animation_options_default();
  return _readAnimationOptions(nativeResult);
}

/// Returns empty map bound options initialized for this C API version.
///
/// See `mln_bound_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
BoundOptions boundOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_bound_options_default();
  return _readBoundOptions(nativeResult);
}

/// Reports the C ABI contract version. The value is 0 while the ABI is
/// unstable, and will increment on each SemVer major release.
///
/// See `mln_c_version` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
int cVersion() {
  ensureAbiVersion();
  final nativeResult = raw.mln_c_version();
  return nativeResult;
}

/// Returns an empty relative camera update initialized for this API version.
///
/// See `mln_camera_delta_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
CameraDelta cameraDeltaDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_camera_delta_default();
  return _readCameraDelta(nativeResult);
}

/// Returns empty camera fitting options initialized for this C API version.
///
/// See `mln_camera_fit_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
CameraFitOptions cameraFitOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_camera_fit_options_default();
  return _readCameraFitOptions(nativeResult);
}

/// Returns empty camera options initialized for this C API version.
///
/// See `mln_camera_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
CameraOptions cameraOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_camera_options_default();
  return _readCameraOptions(nativeResult);
}

/// Returns an empty atomic camera update initialized for this API version.
///
/// See `mln_camera_update_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
CameraUpdate cameraUpdateDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_camera_update_default();
  return _readCameraUpdate(nativeResult);
}

/// Returns default custom geometry source options.
///
/// See `mln_custom_geometry_source_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
CustomGeometrySourceOptions customGeometrySourceOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_custom_geometry_source_options_default();
  return _readCustomGeometrySourceOptions(nativeResult);
}

/// Returns default custom MVT vector source options.
///
/// See `mln_custom_mvt_vector_source_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
CustomMvtVectorSourceOptions customMvtVectorSourceOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_custom_mvt_vector_source_options_default();
  return _readCustomMvtVectorSourceOptions(nativeResult);
}

/// Returns a zero-token, render-if-needed, nonpresenting frame demand.
///
/// See `mln_frame_demand_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
FrameDemand frameDemandDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_frame_demand_default();
  return _readFrameDemand(nativeResult);
}

/// Returns empty free camera options initialized for this C API version.
///
/// See `mln_free_camera_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
FreeCameraOptions freeCameraOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_free_camera_options_default();
  return _readFreeCameraOptions(nativeResult);
}

/// Prepares GeoJSON source data for installation on a map.
///
/// See `mln_geojson_source_data_create` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
GeojsonSourceDataHandle geojsonSourceDataCreate(
  Uint8List data, {
  GeojsonSourceOptions? options,
}) => withNativeArena((arena) {
  ensureAbiVersion();
  final outData = arena<Uint64>();
  _check(
    raw.mln_geojson_source_data_create(
      nativeBufferView(data, arena),
      options == null ? nullptr : _writeGeojsonSourceOptions(options, arena),
      outData,
      nativeDiagnostic,
    ),
  );
  return _adoptOwned(
    outData.value,
    () => GeojsonSourceDataHandle._(NativeGeojsonSourceData(outData.value)),
    (handle) {
      raw.mln_geojson_source_data_destroy(handle);
    },
  );
});

/// Returns default GeoJSON source options.
///
/// See `mln_geojson_source_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
GeojsonSourceOptions geojsonSourceOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_geojson_source_options_default();
  return _readGeojsonSourceOptions(nativeResult);
}

/// Returns CPU-complete synchronization for this C API version.
///
/// See `mln_gpu_sync_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
GpuSync gpuSyncDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_gpu_sync_default();
  return _readGpuSync(nativeResult);
}

/// Converts spherical Mercator projected meters to a geographic coordinate.
///
/// See `mln_lat_lng_for_projected_meters` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
LatLng latLngForProjectedMeters(ProjectedMeters meters) =>
    withNativeArena((arena) {
      ensureAbiVersion();
      final outCoordinate = arena<raw.mln_lat_lng>();
      _check(
        raw.mln_lat_lng_for_projected_meters(
          _writeProjectedMeters(meters, arena).ref,
          outCoordinate,
          nativeDiagnostic,
        ),
      );
      return _readLatLng(outCoordinate.ref);
    });

/// Clears the process-global log callback.
///
/// See `mln_log_clear_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
void logClearCallback() {
  ensureAbiVersion();
  return _check(raw.mln_log_clear_callback(nativeDiagnostic));
}

/// Controls which log severities MapLibre Native may dispatch asynchronously.
///
/// See `mln_log_set_async_severity_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
void logSetAsyncSeverityMask(LogSeverityMask mask) {
  ensureAbiVersion();
  return _check(
    raw.mln_log_set_async_severity_mask(mask.rawValue, nativeDiagnostic),
  );
}

/// Installs a process-global MapLibre Native log callback.
///
/// See `mln_log_set_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
void logSetCallback(LogCallback callback) {
  ensureAbiVersion();
  final port = _globalCallbackPorts.registerDeferred(
    (raw.MLN_ADAPTER_DEFERRED_LOG_CALLBACK & 0xffffffff),
    (message) => _deliverLogCallback(callback, message),
  );
  var accepted = false;
  try {
    _check(
      raw.mln_log_set_callback(
        raw
            .mln_adapter_deferred_callback_function(
              (raw.MLN_ADAPTER_DEFERRED_LOG_CALLBACK & 0xffffffff),
            )
            .cast(),
        port.context,
        Native.addressOf<NativeFunction<raw.mln_log_callback_releaseFunction>>(
          raw.mln_adapter_deferred_callback_release,
        ).cast(),
        nativeDiagnostic,
      ),
    );
    accepted = true;
  } finally {
    if (!accepted) {
      port.reject();
    }
  }
}

/// Returns map options initialized for this C API version.
///
/// See `mln_map_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
MapOptions mapOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_map_options_default();
  return _readMapOptions(nativeResult);
}

/// Returns empty tile tuning options initialized for this C API version.
///
/// See `mln_map_tile_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
MapTileOptions mapTileOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_map_tile_options_default();
  return _readMapTileOptions(nativeResult);
}

/// Returns empty viewport options initialized for this C API version.
///
/// See `mln_map_viewport_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
MapViewportOptions mapViewportOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_map_viewport_options_default();
  return _readMapViewportOptions(nativeResult);
}

/// Returns Metal borrowed-texture descriptor defaults for this C API version.
///
/// See `mln_metal_borrowed_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
MetalBorrowedTextureDescriptor metalBorrowedTextureDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_metal_borrowed_texture_descriptor_default();
  return _readMetalBorrowedTextureDescriptor(nativeResult);
}

/// Returns Metal owned-texture descriptor defaults for this C API version.
///
/// See `mln_metal_owned_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
MetalOwnedTextureDescriptor metalOwnedTextureDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_metal_owned_texture_descriptor_default();
  return _readMetalOwnedTextureDescriptor(nativeResult);
}

/// Returns Metal surface descriptor defaults for this C API version.
///
/// See `mln_metal_surface_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
MetalSurfaceDescriptor metalSurfaceDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_metal_surface_descriptor_default();
  return _readMetalSurfaceDescriptor(nativeResult);
}

/// Reads MapLibre Native's process-global network status.
///
/// See `mln_network_status_get` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
NetworkStatus networkStatusGet() => withNativeArena((arena) {
  ensureAbiVersion();
  final outStatus = arena<Uint32>();
  _check(raw.mln_network_status_get(outStatus, nativeDiagnostic));
  return NetworkStatus.fromRawValue(outStatus.value);
});

/// Sets MapLibre Native's process-global network status.
///
/// See `mln_network_status_set` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
void networkStatusSet(NetworkStatus status) {
  ensureAbiVersion();
  return _check(raw.mln_network_status_set(status.rawValue, nativeDiagnostic));
}

/// Returns OpenGL borrowed-texture descriptor defaults for this C API
/// version.
///
/// See `mln_opengl_borrowed_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
OpenglBorrowedTextureDescriptor openglBorrowedTextureDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_opengl_borrowed_texture_descriptor_default();
  return _readOpenglBorrowedTextureDescriptor(nativeResult);
}

/// Returns OpenGL owned-texture descriptor defaults for this C API version.
///
/// See `mln_opengl_owned_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
OpenglOwnedTextureDescriptor openglOwnedTextureDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_opengl_owned_texture_descriptor_default();
  return _readOpenglOwnedTextureDescriptor(nativeResult);
}

/// Returns OpenGL context providers supported by this build.
///
/// See `mln_opengl_supported_context_provider_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
OpenglContextProviderFlag openglSupportedContextProviderMask() {
  ensureAbiVersion();
  final nativeResult = raw.mln_opengl_supported_context_provider_mask();
  return OpenglContextProviderFlag.fromRawValue(nativeResult);
}

/// Returns OpenGL surface descriptor defaults for this C API version.
///
/// See `mln_opengl_surface_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
OpenglSurfaceDescriptor openglSurfaceDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_opengl_surface_descriptor_default();
  return _readOpenglSurfaceDescriptor(nativeResult);
}

/// Returns the process-wide `mln_plugin_register_v1` entry point; never null.
///
/// See `mln_plugin_get_register_function_v1` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
NativePointer pluginGetRegisterFunctionV1() {
  ensureAbiVersion();
  final nativeResult = raw.mln_plugin_get_register_function_v1();
  return NativePointer(nativeResult.address);
}

/// Returns a default premultiplied RGBA8 image descriptor.
///
/// See `mln_premultiplied_rgba8_image_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
PremultipliedRgba8Image premultipliedRgba8ImageDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_premultiplied_rgba8_image_default();
  return _readPremultipliedRgba8Image(nativeResult);
}

/// Converts a geographic coordinate to spherical Mercator projected meters.
///
/// See `mln_projected_meters_for_lat_lng` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
ProjectedMeters projectedMetersForLatLng(LatLng coordinate) =>
    withNativeArena((arena) {
      ensureAbiVersion();
      final outMeters = arena<raw.mln_projected_meters>();
      _check(
        raw.mln_projected_meters_for_lat_lng(
          _writeLatLng(coordinate, arena).ref,
          outMeters,
          nativeDiagnostic,
        ),
      );
      return _readProjectedMeters(outMeters.ref);
    });

/// Returns empty axonometric rendering options initialized for this C API
/// version.
///
/// See `mln_projection_mode_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
ProjectionMode projectionModeDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_projection_mode_default();
  return _readProjectionMode(nativeResult);
}

/// Returns default caller-graphics-thread attachment policy with no wakes and
/// a one-slot texture ring.
///
/// See `mln_render_session_attach_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
RenderSessionAttachOptions renderSessionAttachOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_render_session_attach_options_default();
  return _readRenderSessionAttachOptions(nativeResult);
}

/// Computes the physical device-pixel size of a logical render target extent.
///
/// See `mln_render_target_extent_physical_size` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
(int, int) renderTargetExtentPhysicalSize(RenderTargetExtent extent) =>
    withNativeArena((arena) {
      ensureAbiVersion();
      final outWidth = arena<Uint32>();
      final outHeight = arena<Uint32>();
      _check(
        raw.mln_render_target_extent_physical_size(
          _writeRenderTargetExtent(extent, arena),
          outWidth,
          outHeight,
          nativeDiagnostic,
        ),
      );
      return (outWidth.value, outHeight.value);
    });

/// Returns default rendered feature query options.
///
/// See `mln_rendered_feature_query_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
RenderedFeatureQueryOptions renderedFeatureQueryOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_rendered_feature_query_options_default();
  return _readRenderedFeatureQueryOptions(nativeResult);
}

/// Returns a rendered box query geometry descriptor.
///
/// See `mln_rendered_query_geometry_box` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
RenderedQueryGeometry renderedQueryGeometryBox(ScreenBox box) =>
    withNativeArena((arena) {
      ensureAbiVersion();
      final nativeResult = raw.mln_rendered_query_geometry_box(
        _writeScreenBox(box, arena).ref,
      );
      return _readRenderedQueryGeometry(nativeResult);
    });

/// Returns a rendered line-string query geometry descriptor.
///
/// See `mln_rendered_query_geometry_line_string` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
RenderedQueryGeometry renderedQueryGeometryLineString(
  List<ScreenPoint> points,
) => withNativeArena((arena) {
  ensureAbiVersion();
  final nativepoints = arena<raw.mln_screen_point>(
    points.isEmpty ? 1 : points.length,
  );
  for (var index = 0; index < points.length; index++) {
    nativepoints[index] = _writeScreenPoint(points[index], arena).ref;
  }
  final nativeResult = raw.mln_rendered_query_geometry_line_string(
    nativepoints,
    points.length,
  );
  return _readRenderedQueryGeometry(nativeResult);
});

/// Returns a rendered point query geometry descriptor.
///
/// See `mln_rendered_query_geometry_point` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
RenderedQueryGeometry renderedQueryGeometryPoint(ScreenPoint point) =>
    withNativeArena((arena) {
      ensureAbiVersion();
      final nativeResult = raw.mln_rendered_query_geometry_point(
        _writeScreenPoint(point, arena).ref,
      );
      return _readRenderedQueryGeometry(nativeResult);
    });

/// Creates a runtime with a new core-owned worker.
///
/// See `mln_runtime_create` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
RuntimeHandle runtimeCreate(RuntimeOptions options) => withNativeArena((arena) {
  ensureAbiVersion();
  final outRuntime = arena<Uint64>();
  final registrations = _NativeRegistrations(_NativeCallbackPorts());
  _check(
    registrations.run(
      () => raw.mln_runtime_create(
        _writeRuntimeOptions(options, arena, registrations),
        outRuntime,
        nativeDiagnostic,
      ),
    ),
  );
  return _adoptOwned(
    outRuntime.value,
    () =>
        (RuntimeHandle._(NativeRuntime(outRuntime.value))
          .._state.retain(registrations)),
    (handle) {
      _check(raw.mln_runtime_dispose(handle, nativeDiagnostic));
    },
  );
});

/// Returns runtime options initialized for this C API version.
///
/// See `mln_runtime_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
RuntimeOptions runtimeOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_runtime_options_default();
  return _readRuntimeOptions(nativeResult);
}

/// Returns default source feature query options.
///
/// See `mln_source_feature_query_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
SourceFeatureQueryOptions sourceFeatureQueryOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_source_feature_query_options_default();
  return _readSourceFeatureQueryOptions(nativeResult);
}

/// Returns default runtime style image metadata.
///
/// See `mln_style_image_info_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
StyleImageInfo styleImageInfoDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_style_image_info_default();
  return _readStyleImageInfo(nativeResult);
}

/// Returns default runtime style image options.
///
/// See `mln_style_image_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
StyleImageOptions styleImageOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_style_image_options_default();
  return _readStyleImageOptions(nativeResult);
}

/// Returns default tile source options.
///
/// See `mln_style_tile_source_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
StyleTileSourceOptions styleTileSourceOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_style_tile_source_options_default();
  return _readStyleTileSourceOptions(nativeResult);
}

/// Returns default global style transition options.
///
/// See `mln_style_transition_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
StyleTransitionOptions styleTransitionOptionsDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_style_transition_options_default();
  return _readStyleTransitionOptions(nativeResult);
}

/// Reports the render backends available in this native library build.
///
/// See `mln_supported_render_backend_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
RenderBackendFlag supportedRenderBackendMask() {
  ensureAbiVersion();
  final nativeResult = raw.mln_supported_render_backend_mask();
  return RenderBackendFlag.fromRawValue(nativeResult);
}

/// Returns texture image info defaults for this C API version.
///
/// See `mln_texture_image_info_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
TextureImageInfo textureImageInfoDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_texture_image_info_default();
  return _readTextureImageInfo(nativeResult);
}

/// Returns Vulkan borrowed-texture descriptor defaults for this C API
/// version.
///
/// See `mln_vulkan_borrowed_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
VulkanBorrowedTextureDescriptor vulkanBorrowedTextureDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_vulkan_borrowed_texture_descriptor_default();
  return _readVulkanBorrowedTextureDescriptor(nativeResult);
}

/// Returns Vulkan owned-texture descriptor defaults for this C API version.
///
/// See `mln_vulkan_owned_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
VulkanOwnedTextureDescriptor vulkanOwnedTextureDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_vulkan_owned_texture_descriptor_default();
  return _readVulkanOwnedTextureDescriptor(nativeResult);
}

/// Returns Vulkan surface descriptor defaults for this C API version.
///
/// See `mln_vulkan_surface_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
VulkanSurfaceDescriptor vulkanSurfaceDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_vulkan_surface_descriptor_default();
  return _readVulkanSurfaceDescriptor(nativeResult);
}

/// Returns WebGPU borrowed-texture descriptor defaults for this C API
/// version.
///
/// See `mln_webgpu_borrowed_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
WebgpuBorrowedTextureDescriptor webgpuBorrowedTextureDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_webgpu_borrowed_texture_descriptor_default();
  return _readWebgpuBorrowedTextureDescriptor(nativeResult);
}

/// Returns WebGPU owned-texture descriptor defaults for this C API version.
///
/// See `mln_webgpu_owned_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
WebgpuOwnedTextureDescriptor webgpuOwnedTextureDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_webgpu_owned_texture_descriptor_default();
  return _readWebgpuOwnedTextureDescriptor(nativeResult);
}

/// Returns WebGPU surface descriptor defaults for this C API version.
///
/// See `mln_webgpu_surface_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
WebgpuSurfaceDescriptor webgpuSurfaceDescriptorDefault() {
  ensureAbiVersion();
  final nativeResult = raw.mln_webgpu_surface_descriptor_default();
  return _readWebgpuSurfaceDescriptor(nativeResult);
}

/// Issued `mln_acquired_frame` handle id.
extension type const NativeAcquiredFrame(int raw) implements NativeHandle {}

/// Owner of one native `mln_acquired_frame` handle.
///
/// A rendered frame that a render session lends until its release.
///
/// See `mln_acquired_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class AcquiredFrameHandle implements Finalizable {
  AcquiredFrameHandle._(this._parent, NativeAcquiredFrame handle)
    : _state = NativeHandleState(handle, 'AcquiredFrameHandle');
  // Keeps the parent owner reachable while this owner lives.
  // ignore: unused_field
  final RenderSessionHandle _parent;
  final NativeHandleState<NativeAcquiredFrame> _state;
  NativeAcquiredFrame get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Consumes an acquired frame and quarantines its slot of the texture ring.
  ///
  /// See `mln_acquired_frame_dispose` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void dispose() => _state.close(
    (handle) => raw.mln_acquired_frame_dispose(handle.raw, nativeDiagnostic),
  );

  /// Copies Metal-native metadata from an acquired frame.
  ///
  /// See `mln_acquired_frame_get_metal_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ScopedMetalOwnedTextureFrame getMetalTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_metal_owned_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_metal_owned_texture_frame>();
    _check(
      raw.mln_acquired_frame_get_metal_texture(
        _handle.raw,
        outFrame,
        nativeDiagnostic,
      ),
    );
    return ScopedMetalOwnedTextureFrame._(
      this,
      _readMetalOwnedTextureFrame(outFrame.ref),
    );
  });

  /// Copies OpenGL-native metadata from an acquired frame.
  ///
  /// See `mln_acquired_frame_get_opengl_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ScopedOpenglOwnedTextureFrame getOpenglTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_opengl_owned_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_opengl_owned_texture_frame>();
    _check(
      raw.mln_acquired_frame_get_opengl_texture(
        _handle.raw,
        outFrame,
        nativeDiagnostic,
      ),
    );
    return ScopedOpenglOwnedTextureFrame._(
      this,
      _readOpenglOwnedTextureFrame(outFrame.ref),
    );
  });

  /// Copies the producer synchronization for an acquired texture frame.
  ///
  /// See `mln_acquired_frame_get_producer_sync` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ScopedGpuSync getProducerSync() => withNativeArena((arena) {
    final outSync = arena<raw.mln_gpu_sync>();
    outSync.ref = raw.mln_gpu_sync_default();
    _check(
      raw.mln_acquired_frame_get_producer_sync(
        _handle.raw,
        outSync,
        nativeDiagnostic,
      ),
    );
    return ScopedGpuSync._(this, _readGpuSync(outSync.ref));
  });

  /// Copies common metadata for an acquired frame.
  ///
  /// See `mln_acquired_frame_get_result` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  RenderFrameResult getResult() => withNativeArena((arena) {
    final outResult = arena<raw.mln_render_frame_result>();
    outResult.ref.size = sizeOf<raw.mln_render_frame_result>();
    _check(
      raw.mln_acquired_frame_get_result(
        _handle.raw,
        outResult,
        nativeDiagnostic,
      ),
    );
    return _readRenderFrameResult(outResult.ref);
  });

  /// Copies Vulkan-native metadata from an acquired frame.
  ///
  /// See `mln_acquired_frame_get_vulkan_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ScopedVulkanOwnedTextureFrame getVulkanTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_vulkan_owned_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_vulkan_owned_texture_frame>();
    _check(
      raw.mln_acquired_frame_get_vulkan_texture(
        _handle.raw,
        outFrame,
        nativeDiagnostic,
      ),
    );
    return ScopedVulkanOwnedTextureFrame._(
      this,
      _readVulkanOwnedTextureFrame(outFrame.ref),
    );
  });

  /// Copies WebGPU-native metadata from an acquired frame.
  ///
  /// See `mln_acquired_frame_get_webgpu_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ScopedWebgpuOwnedTextureFrame getWebgpuTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_webgpu_owned_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_webgpu_owned_texture_frame>();
    _check(
      raw.mln_acquired_frame_get_webgpu_texture(
        _handle.raw,
        outFrame,
        nativeDiagnostic,
      ),
    );
    return ScopedWebgpuOwnedTextureFrame._(
      this,
      _readWebgpuOwnedTextureFrame(outFrame.ref),
    );
  });

  /// Releases an acquired frame after optional consumer GPU work.
  ///
  /// See `mln_acquired_frame_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void release(GpuSync consumerCompletion) => _state.close(
    (handle) => withNativeArena((arena) {
      final receiverPointer = arena<Uint64>()..value = handle.raw;
      return raw.mln_acquired_frame_release(
        receiverPointer,
        _writeGpuSync(consumerCompletion, arena),
        nativeDiagnostic,
      );
    }),
  );
}

/// Issued `mln_buffer` handle id.
extension type const NativeBuffer(int raw) implements NativeHandle {}

/// Owner of one native `mln_buffer` handle.
///
/// An owned buffer of bytes.
///
/// See `mln_buffer` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class BufferHandle implements Finalizable {
  BufferHandle._(NativeBuffer handle)
    : _state = NativeHandleState(handle, 'BufferHandle');
  final NativeHandleState<NativeBuffer> _state;
  NativeBuffer get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Destroys an owned buffer. A null handle is a no-op.
  ///
  /// See `mln_buffer_destroy` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
  void close() => _state.close((handle) {
    raw.mln_buffer_destroy(handle.raw);
    return nativeStatusOk;
  });

  /// Borrows the data stored by an owned buffer.
  ///
  /// See `mln_buffer_get` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
  Uint8List getValue() => withNativeArena((arena) {
    final outView = arena<raw.mln_buffer_view>();
    _check(raw.mln_buffer_get(_handle.raw, outView, nativeDiagnostic));
    return _copyBufferView(outView.ref);
  });
}

/// Issued `mln_event_batch` handle id.
extension type const NativeEventBatch(int raw) implements NativeHandle {}

/// Owner of one native `mln_event_batch` handle.
///
/// An owned batch of runtime events from one drain.
///
/// See `mln_event_batch` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class EventBatchHandle implements Finalizable {
  EventBatchHandle._(NativeEventBatch handle)
    : _state = NativeHandleState(handle, 'EventBatchHandle');
  final NativeHandleState<NativeEventBatch> _state;
  NativeEventBatch get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Borrows the event and message view stored by an owned event batch.
  ///
  /// See `mln_event_batch_get` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  RuntimeEventBatchView getValue() => withNativeArena((arena) {
    final outView = arena<raw.mln_runtime_event_batch_view>();
    outView.ref.size = sizeOf<raw.mln_runtime_event_batch_view>();
    _check(raw.mln_event_batch_get(_handle.raw, outView, nativeDiagnostic));
    return _readRuntimeEventBatchView(outView.ref);
  });

  /// Releases an owned event batch. A null handle is a no-op.
  ///
  /// See `mln_event_batch_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void close() => _state.close((handle) {
    raw.mln_event_batch_release(handle.raw);
    return nativeStatusOk;
  });
}

/// Issued `mln_geojson_source_data` handle id.
extension type const NativeGeojsonSourceData(int raw) implements NativeHandle {}

/// Owner of one native `mln_geojson_source_data` handle.
final class GeojsonSourceDataHandle implements Finalizable {
  GeojsonSourceDataHandle._(NativeGeojsonSourceData handle)
    : _state = NativeHandleState(handle, 'GeojsonSourceDataHandle');
  final NativeHandleState<NativeGeojsonSourceData> _state;
  NativeGeojsonSourceData get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Releases prepared GeoJSON source data.
  ///
  /// See `mln_geojson_source_data_destroy` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  void close() => _state.close((handle) {
    raw.mln_geojson_source_data_destroy(handle.raw);
    return nativeStatusOk;
  });
}

/// Issued `mln_map` handle id.
extension type const NativeMap(int raw) implements NativeHandle {}

/// Owner of one native `mln_map` handle.
///
/// A map, which holds map state independent of any render target.
///
/// See `mln_map` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class MapHandle implements Finalizable, _CallbackPortOwner {
  MapHandle._(this._parent, NativeMap handle)
    : _state = NativeHandleState(handle, 'MapHandle');
  // Keeps the parent owner reachable while this owner lives.
  // ignore: unused_field
  final RuntimeHandle _parent;
  // Roots this owner's port registrations for as long as it lives.
  @override
  final _callbackPorts = _NativeCallbackPorts();
  final NativeHandleState<NativeMap> _state;
  NativeMap get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Adds a color-relief layer for a raster DEM source.
  ///
  /// See `mln_map_add_color_relief_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addColorReliefLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) => _command(
    (arena, completion) => raw.mln_map_add_color_relief_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView(sourceId, arena).value,
      nativeStringView((beforeLayerId ?? ''), arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds a custom geometry source.
  ///
  /// See `mln_map_add_custom_geometry_source` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addCustomGeometrySource(
    String sourceId,
    CustomGeometrySourceOptions options,
  ) => _command((arena, completion) {
    final registrations = _NativeRegistrations(_callbackPorts);
    return registrations.run(
      () => raw.mln_map_add_custom_geometry_source(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        registrations.add(
          _prepareCustomGeometrySourceOptions(options, registrations.ports),
        ),
        completion,
        nativeDiagnostic,
      ),
    );
  });

  /// Adds a custom MVT vector source.
  ///
  /// See `mln_map_add_custom_mvt_vector_source` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addCustomMvtVectorSource(
    String sourceId,
    CustomMvtVectorSourceOptions options,
  ) => _command((arena, completion) {
    final registrations = _NativeRegistrations(_callbackPorts);
    return registrations.run(
      () => raw.mln_map_add_custom_mvt_vector_source(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        registrations.add(
          _prepareCustomMvtVectorSourceOptions(options, registrations.ports),
        ),
        completion,
        nativeDiagnostic,
      ),
    );
  });

  /// Adds a GeoJSON source with prepared inline data.
  ///
  /// See `mln_map_add_geojson_source_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addGeojsonSourceData(
    String sourceId,
    GeojsonSourceDataHandle data,
  ) => _command(
    (arena, completion) => raw.mln_map_add_geojson_source_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      data._handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds a GeoJSON source with URL data.
  ///
  /// See `mln_map_add_geojson_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addGeojsonSourceUrl(
    String sourceId,
    String url, {
    GeojsonSourceOptions? options,
  }) => _command(
    (arena, completion) => raw.mln_map_add_geojson_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeStringView(url, arena).value,
      options == null ? nullptr : _writeGeojsonSourceOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds a hillshade layer for a raster DEM source.
  ///
  /// See `mln_map_add_hillshade_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addHillshadeLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) => _command(
    (arena, completion) => raw.mln_map_add_hillshade_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView(sourceId, arena).value,
      nativeStringView((beforeLayerId ?? ''), arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds an image source with inline image pixels.
  ///
  /// See `mln_map_add_image_source_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addImageSourceImage(
    String sourceId,
    List<LatLng> coordinates,
    PremultipliedRgba8Image image,
  ) => _command((arena, completion) {
    final nativecoordinates = arena<raw.mln_lat_lng>(
      coordinates.isEmpty ? 1 : coordinates.length,
    );
    for (var index = 0; index < coordinates.length; index++) {
      nativecoordinates[index] = _writeLatLng(coordinates[index], arena).ref;
    }
    return raw.mln_map_add_image_source_image(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativecoordinates,
      coordinates.length,
      _writePremultipliedRgba8Image(image, arena),
      completion,
      nativeDiagnostic,
    );
  });

  /// Adds an image source that loads its image from a URL.
  ///
  /// See `mln_map_add_image_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addImageSourceUrl(
    String sourceId,
    List<LatLng> coordinates,
    String url,
  ) => _command((arena, completion) {
    final nativecoordinates = arena<raw.mln_lat_lng>(
      coordinates.isEmpty ? 1 : coordinates.length,
    );
    for (var index = 0; index < coordinates.length; index++) {
      nativecoordinates[index] = _writeLatLng(coordinates[index], arena).ref;
    }
    return raw.mln_map_add_image_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativecoordinates,
      coordinates.length,
      nativeStringView(url, arena).value,
      completion,
      nativeDiagnostic,
    );
  });

  /// Adds a source-free location indicator layer.
  ///
  /// See `mln_map_add_location_indicator_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addLocationIndicatorLayer(
    String layerId, {
    String? beforeLayerId,
  }) => _command(
    (arena, completion) => raw.mln_map_add_location_indicator_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView((beforeLayerId ?? ''), arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds a raster DEM source with inline tile URLs.
  ///
  /// See `mln_map_add_raster_dem_source_tiles` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addRasterDemSourceTiles(
    String sourceId,
    List<String> tiles, {
    StyleTileSourceOptions? options,
  }) => _command((arena, completion) {
    final nativetiles = arena<raw.mln_buffer_view>(
      tiles.isEmpty ? 1 : tiles.length,
    );
    for (var index = 0; index < tiles.length; index++) {
      nativetiles[index] = nativeStringView(tiles[index], arena).value;
    }
    return raw.mln_map_add_raster_dem_source_tiles(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativetiles,
      tiles.length,
      options == null ? nullptr : _writeStyleTileSourceOptions(options, arena),
      completion,
      nativeDiagnostic,
    );
  });

  /// Adds a raster DEM source with a TileJSON URL.
  ///
  /// See `mln_map_add_raster_dem_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addRasterDemSourceUrl(
    String sourceId,
    String url, {
    StyleTileSourceOptions? options,
  }) => _command(
    (arena, completion) => raw.mln_map_add_raster_dem_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeStringView(url, arena).value,
      options == null ? nullptr : _writeStyleTileSourceOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds a raster source with inline tile URLs.
  ///
  /// See `mln_map_add_raster_source_tiles` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addRasterSourceTiles(
    String sourceId,
    List<String> tiles, {
    StyleTileSourceOptions? options,
  }) => _command((arena, completion) {
    final nativetiles = arena<raw.mln_buffer_view>(
      tiles.isEmpty ? 1 : tiles.length,
    );
    for (var index = 0; index < tiles.length; index++) {
      nativetiles[index] = nativeStringView(tiles[index], arena).value;
    }
    return raw.mln_map_add_raster_source_tiles(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativetiles,
      tiles.length,
      options == null ? nullptr : _writeStyleTileSourceOptions(options, arena),
      completion,
      nativeDiagnostic,
    );
  });

  /// Adds a raster source with a TileJSON URL.
  ///
  /// See `mln_map_add_raster_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addRasterSourceUrl(
    String sourceId,
    String url, {
    StyleTileSourceOptions? options,
  }) => _command(
    (arena, completion) => raw.mln_map_add_raster_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeStringView(url, arena).value,
      options == null ? nullptr : _writeStyleTileSourceOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds one style layer from a full style-spec layer JSON object.
  ///
  /// See `mln_map_add_style_layer_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addStyleLayerJson(
    Uint8List layerJson, {
    String? beforeLayerId,
  }) => _command(
    (arena, completion) => raw.mln_map_add_style_layer_json(
      _handle.raw,
      nativeBufferView(layerJson, arena),
      nativeStringView((beforeLayerId ?? ''), arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds one style source from a style-spec source JSON object.
  ///
  /// See `mln_map_add_style_source_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addStyleSourceJson(
    String sourceId,
    Uint8List sourceJson,
  ) => _command(
    (arena, completion) => raw.mln_map_add_style_source_json(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeBufferView(sourceJson, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds a vector source with inline tile URLs.
  ///
  /// See `mln_map_add_vector_source_tiles` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addVectorSourceTiles(
    String sourceId,
    List<String> tiles, {
    StyleTileSourceOptions? options,
  }) => _command((arena, completion) {
    final nativetiles = arena<raw.mln_buffer_view>(
      tiles.isEmpty ? 1 : tiles.length,
    );
    for (var index = 0; index < tiles.length; index++) {
      nativetiles[index] = nativeStringView(tiles[index], arena).value;
    }
    return raw.mln_map_add_vector_source_tiles(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativetiles,
      tiles.length,
      options == null ? nullptr : _writeStyleTileSourceOptions(options, arena),
      completion,
      nativeDiagnostic,
    );
  });

  /// Adds a vector source with a TileJSON URL.
  ///
  /// See `mln_map_add_vector_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addVectorSourceUrl(
    String sourceId,
    String url, {
    StyleTileSourceOptions? options,
  }) => _command(
    (arena, completion) => raw.mln_map_add_vector_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeStringView(url, arena).value,
      options == null ? nullptr : _writeStyleTileSourceOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits one copied relative camera update.
  ///
  /// See `mln_map_apply_camera_delta` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> applyCameraDelta(CameraDelta delta) => _command(
    (arena, completion) => raw.mln_map_apply_camera_delta(
      _handle.raw,
      _writeCameraDelta(delta, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered query for a camera that fits a GeoJSON geometry.
  ///
  /// See `mln_map_camera_for_geometry` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CameraOptions> cameraForGeometry(
    Uint8List geometry, {
    CameraFitOptions? fitOptions,
  }) => _query(
    _resultCameraOptions,
    (arena, completion) => raw.mln_map_camera_for_geometry(
      _handle.raw,
      nativeBufferView(geometry, arena),
      fitOptions == null ? nullptr : _writeCameraFitOptions(fitOptions, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered query for a camera that fits geographic bounds.
  ///
  /// See `mln_map_camera_for_lat_lng_bounds` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CameraOptions> cameraForLatLngBounds(
    LatLngBounds bounds, {
    CameraFitOptions? fitOptions,
  }) => _query(
    _resultCameraOptions,
    (arena, completion) => raw.mln_map_camera_for_lat_lng_bounds(
      _handle.raw,
      _writeLatLngBounds(bounds, arena).ref,
      fitOptions == null ? nullptr : _writeCameraFitOptions(fitOptions, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered query for a camera that fits geographic coordinates.
  ///
  /// See `mln_map_camera_for_lat_lngs` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CameraOptions> cameraForLatLngs(
    List<LatLng> coordinates, {
    CameraFitOptions? fitOptions,
  }) => _query(_resultCameraOptions, (arena, completion) {
    final nativecoordinates = arena<raw.mln_lat_lng>(
      coordinates.isEmpty ? 1 : coordinates.length,
    );
    for (var index = 0; index < coordinates.length; index++) {
      nativecoordinates[index] = _writeLatLng(coordinates[index], arena).ref;
    }
    return raw.mln_map_camera_for_lat_lngs(
      _handle.raw,
      nativecoordinates,
      coordinates.length,
      fitOptions == null ? nullptr : _writeCameraFitOptions(fitOptions, arena),
      completion,
      nativeDiagnostic,
    );
  });

  /// Starts an ordered camera read.
  ///
  /// See `mln_map_camera_query` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CameraQueryResult> cameraQuery() => _query(
    _resultCameraQueryResult,
    (arena, completion) =>
        raw.mln_map_camera_query(_handle.raw, completion, nativeDiagnostic),
  );

  /// Copies the camera from the latest immutable map snapshot.
  ///
  /// See `mln_map_camera_snapshot_get` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  (CameraOptions, BigInt) cameraSnapshotGet() => withNativeArena((arena) {
    final outCamera = arena<raw.mln_camera_options>();
    outCamera.ref = raw.mln_camera_options_default();
    final outGeneration = arena<Uint64>();
    _check(
      raw.mln_map_camera_snapshot_get(
        _handle.raw,
        outCamera,
        outGeneration,
        nativeDiagnostic,
      ),
    );
    return (
      _readCameraOptions(outCamera.ref),
      uint64FromNative(outGeneration.value),
    );
  });

  /// Cancels the camera transitions running when this command commits.
  ///
  /// See `mln_map_cancel_transitions` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> cancelTransitions() => _command(
    (arena, completion) => raw.mln_map_cancel_transitions(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Copies one layer's source ID.
  ///
  /// See `mln_map_copy_layer_source_id` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<String?> copyLayerSourceId(String layerId) => _query(
    _resultStringOrNull,
    (arena, completion) => raw.mln_map_copy_layer_source_id(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Copies one layer's source-layer ID.
  ///
  /// See `mln_map_copy_layer_source_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<String?> copyLayerSourceLayer(String layerId) => _query(
    _resultStringOrNull,
    (arena, completion) => raw.mln_map_copy_layer_source_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Copies one runtime style image as tightly packed premultiplied RGBA8
  /// pixels.
  ///
  /// See `mln_map_copy_style_image_premultiplied_rgba8` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<Uint8List?> copyStyleImagePremultipliedRgba8(String imageId) =>
      _queryOptional(
        _resultUint8ListOrNull,
        (arena, completion) => raw.mln_map_copy_style_image_premultiplied_rgba8(
          _handle.raw,
          nativeStringView(imageId, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Copies one runtime style image's stretchable intervals.
  ///
  /// See `mln_map_copy_style_image_stretches` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<StyleImageStretchesResult?> copyStyleImageStretches(String imageId) =>
      _queryOptional(
        _resultStyleImageStretchesResult,
        (arena, completion) => raw.mln_map_copy_style_image_stretches(
          _handle.raw,
          nativeStringView(imageId, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Copies one style source attribution string.
  ///
  /// See `mln_map_copy_style_source_attribution` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<String?> copyStyleSourceAttribution(String sourceId) => _queryOptional(
    _resultStringOrNull,
    (arena, completion) => raw.mln_map_copy_style_source_attribution(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Copies one style source URL.
  ///
  /// See `mln_map_copy_style_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<String?> copyStyleSourceUrl(String sourceId) => _queryOptional(
    _resultStringOrNull,
    (arena, completion) => raw.mln_map_copy_style_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Consumes a map handle without observing its asynchronous retirement.
  ///
  /// See `mln_map_dispose` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  void dispose() => _state.close(
    (handle) => raw.mln_map_dispose(handle.raw, nativeDiagnostic),
  );

  /// Submits an ordered debug-log command.
  ///
  /// See `mln_map_dump_debug_logs` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> dumpDebugLogs() => _command(
    (arena, completion) =>
        raw.mln_map_dump_debug_logs(_handle.raw, completion, nativeDiagnostic),
  );

  /// Starts an ordered read of per-feature state from this map.
  ///
  /// See `mln_map_get_feature_state` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<Uint8List> getFeatureState(FeatureStateSelector selector) => _query(
    _resultUint8List,
    (arena, completion) => raw.mln_map_get_feature_state(
      _handle.raw,
      _writeFeatureStateSelector(selector, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Queries the global-state JSON object, including style defaults. Completion
  /// borrows one `mln_buffer_view` for the duration of the callback.
  ///
  /// See `mln_map_get_global_state` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<Uint8List> getGlobalState() => _query(
    _resultUint8List,
    (arena, completion) =>
        raw.mln_map_get_global_state(_handle.raw, completion, nativeDiagnostic),
  );

  /// Copies image source coordinates.
  ///
  /// See `mln_map_get_image_source_coordinates` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<List<LatLng>?> getImageSourceCoordinates(String sourceId) =>
      _queryOptionalList(
        _resultLatLng,
        (arena, completion) => raw.mln_map_get_image_source_coordinates(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Serializes one layer filter as a style-spec JSON value.
  ///
  /// See `mln_map_get_layer_filter` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<Uint8List?> getLayerFilter(String layerId) => _queryOptional(
    _resultUint8ListOrNull,
    (arena, completion) => raw.mln_map_get_layer_filter(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Serializes one layer property as a style-spec JSON value.
  ///
  /// See `mln_map_get_layer_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<Uint8List?> getLayerProperty(String layerId, String propertyName) =>
      _queryOptional(
        _resultUint8ListOrNull,
        (arena, completion) => raw.mln_map_get_layer_property(
          _handle.raw,
          nativeStringView(layerId, arena).value,
          nativeStringView(propertyName, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Copies one complete runtime style image.
  ///
  /// See `mln_map_get_style_image_info` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<StyleImageResult?> getStyleImageInfo(String imageId) => _queryOptional(
    _resultStyleImageResult,
    (arena, completion) => raw.mln_map_get_style_image_info(
      _handle.raw,
      nativeStringView(imageId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Copies complete metadata for one style layer.
  ///
  /// See `mln_map_get_style_layer_info` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<StyleLayerResult?> getStyleLayerInfo(String layerId) => _queryOptional(
    _resultStyleLayerResult,
    (arena, completion) => raw.mln_map_get_style_layer_info(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Serializes one style layer as a full style-spec layer JSON object.
  ///
  /// See `mln_map_get_style_layer_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<Uint8List?> getStyleLayerJson(String layerId) => _queryOptional(
    _resultUint8ListOrNull,
    (arena, completion) => raw.mln_map_get_style_layer_json(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Serializes one style light property as a style-spec JSON value.
  ///
  /// See `mln_map_get_style_light_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<Uint8List?> getStyleLightProperty(String propertyName) =>
      _queryOptional(
        _resultUint8ListOrNull,
        (arena, completion) => raw.mln_map_get_style_light_property(
          _handle.raw,
          nativeStringView(propertyName, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Copies complete metadata for one style source.
  ///
  /// See `mln_map_get_style_source_info` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<StyleSourceResult?> getStyleSourceInfo(String sourceId) =>
      _queryOptional(
        _resultStyleSourceResult,
        (arena, completion) => raw.mln_map_get_style_source_info(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Copies one style source's inline TileJSON tile URLs.
  ///
  /// See `mln_map_get_style_source_tile_urls` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<StyleSourceTileUrlsResult?> getStyleSourceTileUrls(String sourceId) =>
      _queryOptional(
        _resultStyleSourceTileUrlsResult,
        (arena, completion) => raw.mln_map_get_style_source_tile_urls(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Reads the style's global transition options.
  ///
  /// See `mln_map_get_style_transition_options` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<StyleTransitionOptions> getStyleTransitionOptions() => _query(
    _resultStyleTransitionOptions,
    (arena, completion) => raw.mln_map_get_style_transition_options(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Invalidates custom geometry source data inside one geographic region.
  ///
  /// See `mln_map_invalidate_custom_geometry_source_region` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> invalidateCustomGeometrySourceRegion(
    String sourceId,
    LatLngBounds bounds,
  ) => _command(
    (arena, completion) => raw.mln_map_invalidate_custom_geometry_source_region(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeLatLngBounds(bounds, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Invalidates custom geometry source data for one canonical tile.
  ///
  /// See `mln_map_invalidate_custom_geometry_source_tile` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> invalidateCustomGeometrySourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) => _command(
    (arena, completion) => raw.mln_map_invalidate_custom_geometry_source_tile(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Invalidates custom MVT vector source data for one canonical tile.
  ///
  /// See `mln_map_invalidate_custom_mvt_vector_source_tile` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> invalidateCustomMvtVectorSourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) => _command(
    (arena, completion) => raw.mln_map_invalidate_custom_mvt_vector_source_tile(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered wrapped-bounds query for a copied camera.
  ///
  /// See `mln_map_lat_lng_bounds_for_camera` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<LatLngBounds> latLngBoundsForCamera(CameraOptions camera) => _query(
    _resultLatLngBounds,
    (arena, completion) => raw.mln_map_lat_lng_bounds_for_camera(
      _handle.raw,
      _writeCameraOptions(camera, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered unwrapped-bounds query for a copied camera.
  ///
  /// See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<LatLngBounds> latLngBoundsForCameraUnwrapped(CameraOptions camera) =>
      _query(
        _resultLatLngBounds,
        (arena, completion) => raw.mln_map_lat_lng_bounds_for_camera_unwrapped(
          _handle.raw,
          _writeCameraOptions(camera, arena),
          completion,
          nativeDiagnostic,
        ),
      );

  /// Starts an ordered conversion from a screen point to a geographic
  /// coordinate.
  ///
  /// See `mln_map_lat_lng_for_pixel` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<LatLng> latLngForPixel(ScreenPoint point) => _query(
    _resultLatLng,
    (arena, completion) => raw.mln_map_lat_lng_for_pixel(
      _handle.raw,
      _writeScreenPoint(point, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered conversion from a screen point to an unwrapped
  /// geographic coordinate.
  ///
  /// See `mln_map_lat_lng_for_pixel_unwrapped` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<LatLng> latLngForPixelUnwrapped(ScreenPoint point) => _query(
    _resultLatLng,
    (arena, completion) => raw.mln_map_lat_lng_for_pixel_unwrapped(
      _handle.raw,
      _writeScreenPoint(point, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered conversion of copied screen points to coordinates.
  ///
  /// See `mln_map_lat_lngs_for_pixels` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<List<LatLng>> latLngsForPixels(List<ScreenPoint> points) =>
      _queryList(_resultLatLng, (arena, completion) {
        final nativepoints = arena<raw.mln_screen_point>(
          points.isEmpty ? 1 : points.length,
        );
        for (var index = 0; index < points.length; index++) {
          nativepoints[index] = _writeScreenPoint(points[index], arena).ref;
        }
        return raw.mln_map_lat_lngs_for_pixels(
          _handle.raw,
          nativepoints,
          points.length,
          completion,
          nativeDiagnostic,
        );
      });

  /// Starts an ordered conversion of copied screen points to unwrapped
  /// coordinates.
  ///
  /// See `mln_map_lat_lngs_for_pixels_unwrapped` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<List<LatLng>> latLngsForPixelsUnwrapped(List<ScreenPoint> points) =>
      _queryList(_resultLatLng, (arena, completion) {
        final nativepoints = arena<raw.mln_screen_point>(
          points.isEmpty ? 1 : points.length,
        );
        for (var index = 0; index < points.length; index++) {
          nativepoints[index] = _writeScreenPoint(points[index], arena).ref;
        }
        return raw.mln_map_lat_lngs_for_pixels_unwrapped(
          _handle.raw,
          nativepoints,
          points.length,
          completion,
          nativeDiagnostic,
        );
      });

  /// Copies style layer IDs in style order.
  ///
  /// See `mln_map_list_style_layer_ids` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<List<String>> listStyleLayerIds() => _queryList(
    _resultString,
    (arena, completion) => raw.mln_map_list_style_layer_ids(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered query of every style layer in style order.
  ///
  /// See `mln_map_list_style_layers` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<List<StyleLayerEntry>> listStyleLayers() => _queryList(
    _resultStyleLayerEntry,
    (arena, completion) => raw.mln_map_list_style_layers(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Copies style source IDs in style order.
  ///
  /// See `mln_map_list_style_source_ids` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<List<String>> listStyleSourceIds() => _queryList(
    _resultString,
    (arena, completion) => raw.mln_map_list_style_source_ids(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered copy of the last successfully parsed style document.
  ///
  /// See `mln_map_loaded_style_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<Uint8List> loadedStyleJson() => _query(
    _resultUint8List,
    (arena, completion) => raw.mln_map_loaded_style_json(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered query of meters per logical pixel at a latitude and the
  /// current map zoom. The completion borrows one double.
  ///
  /// See `mln_map_meters_per_pixel_at_latitude` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<double> metersPerPixelAtLatitude(double latitude) => _query(
    _resultdouble,
    (arena, completion) => raw.mln_map_meters_per_pixel_at_latitude(
      _handle.raw,
      latitude,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Moves one style layer before another layer or to the top.
  ///
  /// See `mln_map_move_style_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> moveStyleLayer(
    String layerId, {
    String? beforeLayerId,
  }) => _command(
    (arena, completion) => raw.mln_map_move_style_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView((beforeLayerId ?? ''), arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered conversion from a geographic coordinate to a screen
  /// point.
  ///
  /// See `mln_map_pixel_for_lat_lng` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<ScreenPoint> pixelForLatLng(LatLng coordinate) => _query(
    _resultScreenPoint,
    (arena, completion) => raw.mln_map_pixel_for_lat_lng(
      _handle.raw,
      _writeLatLng(coordinate, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered conversion of copied coordinates to screen points.
  ///
  /// See `mln_map_pixels_for_lat_lngs` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<List<ScreenPoint>> pixelsForLatLngs(List<LatLng> coordinates) =>
      _queryList(_resultScreenPoint, (arena, completion) {
        final nativecoordinates = arena<raw.mln_lat_lng>(
          coordinates.isEmpty ? 1 : coordinates.length,
        );
        for (var index = 0; index < coordinates.length; index++) {
          nativecoordinates[index] = _writeLatLng(
            coordinates[index],
            arena,
          ).ref;
        }
        return raw.mln_map_pixels_for_lat_lngs(
          _handle.raw,
          nativecoordinates,
          coordinates.length,
          completion,
          nativeDiagnostic,
        );
      });

  /// Starts creation of a standalone projection from the map's ordered
  /// transform state.
  ///
  /// See `mln_map_projection_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  Future<MapProjectionHandle> projectionCreate() => _queryOwned(
    raw.MLN_ADAPTER_COMPLETION_COPY_MAP_PROJECTION,
    (arena, completion) => raw.mln_map_projection_create(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
    (handle) => _adoptOwned(
      handle,
      () => MapProjectionHandle._(NativeMapProjection(handle)),
      (handle) {
        _check(raw.mln_map_projection_close(handle, nativeDiagnostic));
      },
    ),
  );

  /// Releases a map after synchronous state preflight.
  ///
  /// See `mln_map_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> close() => _state.closeAsync(
    (handle) => _run(
      (arena, completion) =>
          raw.mln_map_release(handle.raw, completion, nativeDiagnostic),
    ),
  );

  /// Removes per-feature state from this map.
  ///
  /// See `mln_map_remove_feature_state` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> removeFeatureState(FeatureStateSelector selector) =>
      _command(
        (arena, completion) => raw.mln_map_remove_feature_state(
          _handle.raw,
          _writeFeatureStateSelector(selector, arena),
          completion,
          nativeDiagnostic,
        ),
      );

  /// Removes one runtime style image by ID.
  ///
  /// See `mln_map_remove_style_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> removeStyleImage(String imageId) => _command(
    (arena, completion) => raw.mln_map_remove_style_image(
      _handle.raw,
      nativeStringView(imageId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Removes one style layer by ID.
  ///
  /// See `mln_map_remove_style_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> removeStyleLayer(String layerId) => _command(
    (arena, completion) => raw.mln_map_remove_style_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Removes one style source by ID.
  ///
  /// See `mln_map_remove_style_source` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> removeStyleSource(String sourceId) => _command(
    (arena, completion) => raw.mln_map_remove_style_source(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Requests a repaint for a continuous map.
  ///
  /// See `mln_map_request_repaint` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> requestRepaint() => _command(
    (arena, completion) =>
        raw.mln_map_request_repaint(_handle.raw, completion, nativeDiagnostic),
  );

  /// Requests one still image for a static or tile map.
  ///
  /// See `mln_map_request_still_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> requestStillImage() => _run(
    (arena, completion) => raw.mln_map_request_still_image(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits the sole post-creation logical extent update.
  ///
  /// See `mln_map_resize` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> resize(LogicalExtent extent) => _command(
    (arena, completion) => raw.mln_map_resize(
      _handle.raw,
      _writeLogicalExtent(extent, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits a copied camera-constraint command.
  ///
  /// See `mln_map_set_bounds` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> setBounds(BoundOptions options) => _command(
    (arena, completion) => raw.mln_map_set_bounds(
      _handle.raw,
      _writeBoundOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets custom geometry source data for one canonical tile.
  ///
  /// See `mln_map_set_custom_geometry_source_tile_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setCustomGeometrySourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Uint8List data,
  ) => _command(
    (arena, completion) => raw.mln_map_set_custom_geometry_source_tile_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      nativeBufferView(data, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets custom MVT vector source data for one canonical tile.
  ///
  /// See `mln_map_set_custom_mvt_vector_source_tile_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setCustomMvtVectorSourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Uint8List data,
  ) => _command(
    (arena, completion) => raw.mln_map_set_custom_mvt_vector_source_tile_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      nativeBufferView(data, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Reports a custom MVT vector source error for one canonical tile.
  ///
  /// See `mln_map_set_custom_mvt_vector_source_tile_error` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setCustomMvtVectorSourceTileError(
    String sourceId,
    CanonicalTileId tileId,
    String message,
  ) => _command(
    (arena, completion) => raw.mln_map_set_custom_mvt_vector_source_tile_error(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      nativeStringView(message, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits a debug-overlay command.
  ///
  /// See `mln_map_set_debug_options` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> setDebugOptions(MapDebugOption options) => _command(
    (arena, completion) => raw.mln_map_set_debug_options(
      _handle.raw,
      options.rawValue,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Selects which map-originated event types this map queues.
  ///
  /// See `mln_map_set_event_mask` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> setEventMask(RuntimeEventMask mask) => _command(
    (arena, completion) => raw.mln_map_set_event_mask(
      _handle.raw,
      mask.rawValue,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits a copied per-feature-state command.
  ///
  /// See `mln_map_set_feature_state` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> setFeatureState(
    FeatureStateSelector selector,
    Uint8List state,
  ) => _command(
    (arena, completion) => raw.mln_map_set_feature_state(
      _handle.raw,
      _writeFeatureStateSelector(selector, arena),
      nativeBufferView(state, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits a copied free-camera command.
  ///
  /// See `mln_map_set_free_camera_options` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> setFreeCameraOptions(FreeCameraOptions options) =>
      _command(
        (arena, completion) => raw.mln_map_set_free_camera_options(
          _handle.raw,
          _writeFreeCameraOptions(options, arena),
          completion,
          nativeDiagnostic,
        ),
      );

  /// Updates one GeoJSON source with prepared inline data.
  ///
  /// See `mln_map_set_geojson_source_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setGeojsonSourceData(
    String sourceId,
    GeojsonSourceDataHandle data,
  ) => _command(
    (arena, completion) => raw.mln_map_set_geojson_source_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      data._handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Overrides one GeoJSON source's synchronous tiling at runtime.
  ///
  /// See `mln_map_set_geojson_source_synchronous_tiling` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setGeojsonSourceSynchronousTiling(
    String sourceId,
    bool enabled,
  ) => _command(
    (arena, completion) => raw.mln_map_set_geojson_source_synchronous_tiling(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      enabled,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Updates one GeoJSON source to load data from a URL.
  ///
  /// See `mln_map_set_geojson_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setGeojsonSourceUrl(String sourceId, String url) =>
      _command(
        (arena, completion) => raw.mln_map_set_geojson_source_url(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          nativeStringView(url, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Submits a global-state JSON value. JSON null restores the style default.
  /// Input is copied before return.
  ///
  /// See `mln_map_set_global_state_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setGlobalStateProperty(
    String propertyName,
    Uint8List value,
  ) => _command(
    (arena, completion) => raw.mln_map_set_global_state_property(
      _handle.raw,
      nativeStringView(propertyName, arena).value,
      nativeBufferView(value, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Updates image source coordinates.
  ///
  /// See `mln_map_set_image_source_coordinates` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setImageSourceCoordinates(
    String sourceId,
    List<LatLng> coordinates,
  ) => _command((arena, completion) {
    final nativecoordinates = arena<raw.mln_lat_lng>(
      coordinates.isEmpty ? 1 : coordinates.length,
    );
    for (var index = 0; index < coordinates.length; index++) {
      nativecoordinates[index] = _writeLatLng(coordinates[index], arena).ref;
    }
    return raw.mln_map_set_image_source_coordinates(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativecoordinates,
      coordinates.length,
      completion,
      nativeDiagnostic,
    );
  });

  /// Updates an image source with inline image pixels.
  ///
  /// See `mln_map_set_image_source_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setImageSourceImage(
    String sourceId,
    PremultipliedRgba8Image image,
  ) => _command(
    (arena, completion) => raw.mln_map_set_image_source_image(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writePremultipliedRgba8Image(image, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Updates an image source to load its image from a URL.
  ///
  /// See `mln_map_set_image_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setImageSourceUrl(String sourceId, String url) =>
      _command(
        (arena, completion) => raw.mln_map_set_image_source_url(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          nativeStringView(url, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Sets or clears one layer filter.
  ///
  /// See `mln_map_set_layer_filter` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLayerFilter(
    String layerId, {
    Uint8List? filter,
  }) => _command(
    (arena, completion) => raw.mln_map_set_layer_filter(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      filter == null
          ? nullptr
          : (() {
              final storage = arena<raw.mln_buffer_view>();
              storage.ref = nativeBufferView(filter, arena);
              return storage;
            })(),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets the highest zoom at which one layer draws.
  ///
  /// See `mln_map_set_layer_max_zoom` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLayerMaxZoom(String layerId, double maxZoom) =>
      _command(
        (arena, completion) => raw.mln_map_set_layer_max_zoom(
          _handle.raw,
          nativeStringView(layerId, arena).value,
          maxZoom,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Sets the lowest zoom at which one layer draws.
  ///
  /// See `mln_map_set_layer_min_zoom` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLayerMinZoom(String layerId, double minZoom) =>
      _command(
        (arena, completion) => raw.mln_map_set_layer_min_zoom(
          _handle.raw,
          nativeStringView(layerId, arena).value,
          minZoom,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Sets one layer property using its MapLibre style-spec property name.
  ///
  /// See `mln_map_set_layer_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLayerProperty(
    String layerId,
    String propertyName,
    Uint8List value,
  ) => _command(
    (arena, completion) => raw.mln_map_set_layer_property(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView(propertyName, arena).value,
      nativeBufferView(value, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets one layer's source ID.
  ///
  /// See `mln_map_set_layer_source_id` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLayerSourceId(String layerId, String sourceId) =>
      _command(
        (arena, completion) => raw.mln_map_set_layer_source_id(
          _handle.raw,
          nativeStringView(layerId, arena).value,
          nativeStringView(sourceId, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Sets one layer's source-layer ID.
  ///
  /// See `mln_map_set_layer_source_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLayerSourceLayer(
    String layerId, {
    String? sourceLayer,
  }) => _command(
    (arena, completion) => raw.mln_map_set_layer_source_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView((sourceLayer ?? ''), arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets whether one layer draws.
  ///
  /// See `mln_map_set_layer_visibility` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLayerVisibility(
    String layerId,
    StyleLayerVisibility visibility,
  ) => _command(
    (arena, completion) => raw.mln_map_set_layer_visibility(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      visibility.rawValue,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets a location indicator layer accuracy radius in meters.
  ///
  /// See `mln_map_set_location_indicator_accuracy_radius` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLocationIndicatorAccuracyRadius(
    String layerId,
    double radius,
  ) => _command(
    (arena, completion) => raw.mln_map_set_location_indicator_accuracy_radius(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      radius,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets a location indicator layer bearing in degrees.
  ///
  /// See `mln_map_set_location_indicator_bearing` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLocationIndicatorBearing(
    String layerId,
    double bearing,
  ) => _command(
    (arena, completion) => raw.mln_map_set_location_indicator_bearing(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      bearing,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets one location indicator image-name property.
  ///
  /// See `mln_map_set_location_indicator_image_name` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLocationIndicatorImageName(
    String layerId,
    LocationIndicatorImageKind imageKind,
    String imageId,
  ) => _command(
    (arena, completion) => raw.mln_map_set_location_indicator_image_name(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      imageKind.rawValue,
      nativeStringView(imageId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets a location indicator layer location.
  ///
  /// See `mln_map_set_location_indicator_location` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setLocationIndicatorLocation(
    String layerId,
    LatLng coordinate,
    double altitude,
  ) => _command(
    (arena, completion) => raw.mln_map_set_location_indicator_location(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      _writeLatLng(coordinate, arena).ref,
      altitude,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits copied axonometric rendering option fields.
  ///
  /// See `mln_map_set_projection_mode` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> setProjectionMode(ProjectionMode mode) => _command(
    (arena, completion) => raw.mln_map_set_projection_mode(
      _handle.raw,
      _writeProjectionMode(mode, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits a rendering-stats visibility command.
  ///
  /// See `mln_map_set_rendering_stats_view_enabled` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> setRenderingStatsViewEnabled(bool enabled) =>
      _command(
        (arena, completion) => raw.mln_map_set_rendering_stats_view_enabled(
          _handle.raw,
          enabled,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Sets one runtime style image.
  ///
  /// See `mln_map_set_style_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setStyleImage(
    String imageId,
    PremultipliedRgba8Image image, {
    StyleImageOptions? options,
  }) => _command(
    (arena, completion) => raw.mln_map_set_style_image(
      _handle.raw,
      nativeStringView(imageId, arena).value,
      _writePremultipliedRgba8Image(image, arena),
      options == null ? nullptr : _writeStyleImageOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Queues an inline style JSON command.
  ///
  /// See `mln_map_set_style_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> setStyleJson(Uint8List json) => _command(
    (arena, completion) => raw.mln_map_set_style_json(
      _handle.raw,
      nativeBufferView(json, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets the style light from a style-spec light JSON object.
  ///
  /// See `mln_map_set_style_light_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setStyleLightJson(Uint8List lightJson) => _command(
    (arena, completion) => raw.mln_map_set_style_light_json(
      _handle.raw,
      nativeBufferView(lightJson, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets one style light property using its MapLibre style-spec property name.
  ///
  /// See `mln_map_set_style_light_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setStyleLightProperty(
    String propertyName,
    Uint8List value,
  ) => _command(
    (arena, completion) => raw.mln_map_set_style_light_property(
      _handle.raw,
      nativeStringView(propertyName, arena).value,
      nativeBufferView(value, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets whether one style source stores fetched tiles in the persistent
  /// cache.
  ///
  /// See `mln_map_set_style_source_volatile` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setStyleSourceVolatile(
    String sourceId,
    bool isVolatile,
  ) => _command(
    (arena, completion) => raw.mln_map_set_style_source_volatile(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      isVolatile,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets the style's global transition options.
  ///
  /// See `mln_map_set_style_transition_options` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setStyleTransitionOptions(
    StyleTransitionOptions options,
  ) => _command(
    (arena, completion) => raw.mln_map_set_style_transition_options(
      _handle.raw,
      _writeStyleTransitionOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Queues a style URL command.
  ///
  /// See `mln_map_set_style_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> setStyleUrl(String url) => _command(
    (arena, completion) => raw.mln_map_set_style_url(
      _handle.raw,
      nativeUtf8CString(url, arena).pointer.cast<Char>(),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits a copied tile-options command.
  ///
  /// See `mln_map_set_tile_options` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> setTileOptions(MapTileOptions options) => _command(
    (arena, completion) => raw.mln_map_set_tile_options(
      _handle.raw,
      _writeMapTileOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits a copied viewport-options command.
  ///
  /// See `mln_map_set_viewport_options` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> setViewportOptions(MapViewportOptions options) =>
      _command(
        (arena, completion) => raw.mln_map_set_viewport_options(
          _handle.raw,
          _writeMapViewportOptions(options, arena),
          completion,
          nativeDiagnostic,
        ),
      );

  /// Copies the latest immutable state published by the map worker.
  ///
  /// See `mln_map_snapshot_get` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  MapSnapshot snapshotGet() => withNativeArena((arena) {
    final outSnapshot = arena<raw.mln_map_snapshot>();
    outSnapshot.ref.size = sizeOf<raw.mln_map_snapshot>();
    _check(
      raw.mln_map_snapshot_get(_handle.raw, outSnapshot, nativeDiagnostic),
    );
    return _readMapSnapshot(outSnapshot.ref);
  });

  /// Starts an ordered copy of the last requested style URL.
  ///
  /// See `mln_map_style_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<String> styleUrl() => _query(
    _resultString,
    (arena, completion) =>
        raw.mln_map_style_url(_handle.raw, completion, nativeDiagnostic),
  );

  /// Submits one atomic camera update.
  ///
  /// See `mln_map_update_camera` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> updateCamera(CameraUpdate update) => _command(
    (arena, completion) => raw.mln_map_update_camera(
      _handle.raw,
      _writeCameraUpdate(update, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts attachment of a caller-owned Metal texture target.
  ///
  /// See `mln_metal_borrowed_texture_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  RenderSessionAttachment metalBorrowedTextureAttach(
    MetalBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_metal_borrowed_texture_attach(
          _handle.raw,
          _writeMetalBorrowedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a session-owned Metal texture ring.
  ///
  /// See `mln_metal_owned_texture_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  RenderSessionAttachment metalOwnedTextureAttach(
    MetalOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_metal_owned_texture_attach(
          _handle.raw,
          _writeMetalOwnedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a Metal surface target.
  ///
  /// See `mln_metal_surface_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  RenderSessionAttachment metalSurfaceAttach(
    MetalSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_metal_surface_attach(
          _handle.raw,
          _writeMetalSurfaceDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a caller-owned OpenGL texture target.
  ///
  /// See `mln_opengl_borrowed_texture_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  RenderSessionAttachment openglBorrowedTextureAttach(
    OpenglBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_opengl_borrowed_texture_attach(
          _handle.raw,
          _writeOpenglBorrowedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a session-owned OpenGL texture ring.
  ///
  /// See `mln_opengl_owned_texture_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  RenderSessionAttachment openglOwnedTextureAttach(
    OpenglOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_opengl_owned_texture_attach(
          _handle.raw,
          _writeOpenglOwnedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of an OpenGL surface target.
  ///
  /// See `mln_opengl_surface_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  RenderSessionAttachment openglSurfaceAttach(
    OpenglSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_opengl_surface_attach(
          _handle.raw,
          _writeOpenglSurfaceDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a caller-owned Vulkan texture target.
  ///
  /// See `mln_vulkan_borrowed_texture_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  RenderSessionAttachment vulkanBorrowedTextureAttach(
    VulkanBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_vulkan_borrowed_texture_attach(
          _handle.raw,
          _writeVulkanBorrowedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a session-owned Vulkan texture ring.
  ///
  /// See `mln_vulkan_owned_texture_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  RenderSessionAttachment vulkanOwnedTextureAttach(
    VulkanOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_vulkan_owned_texture_attach(
          _handle.raw,
          _writeVulkanOwnedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a Vulkan surface target.
  ///
  /// See `mln_vulkan_surface_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  RenderSessionAttachment vulkanSurfaceAttach(
    VulkanSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_vulkan_surface_attach(
          _handle.raw,
          _writeVulkanSurfaceDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a caller-owned WebGPU texture target.
  ///
  /// See `mln_webgpu_borrowed_texture_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  RenderSessionAttachment webgpuBorrowedTextureAttach(
    WebgpuBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_webgpu_borrowed_texture_attach(
          _handle.raw,
          _writeWebgpuBorrowedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a session-owned WebGPU texture ring.
  ///
  /// See `mln_webgpu_owned_texture_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  RenderSessionAttachment webgpuOwnedTextureAttach(
    WebgpuOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_webgpu_owned_texture_attach(
          _handle.raw,
          _writeWebgpuOwnedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }

  /// Starts attachment of a WebGPU surface target.
  ///
  /// See `mln_webgpu_surface_attach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  RenderSessionAttachment webgpuSurfaceAttach(
    WebgpuSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_NativeCallbackPorts());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_webgpu_surface_attach(
          _handle.raw,
          _writeWebgpuSurfaceDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(raw.mln_render_session_dispose(handle, nativeDiagnostic));
        },
      ),
      RenderSessionAttachment.new,
    );
  }
}

/// Issued `mln_map_projection` handle id.
extension type const NativeMapProjection(int raw) implements NativeHandle {}

/// Owner of one native `mln_map_projection` handle.
///
/// A standalone projection of a map's transform state at its creation.
///
/// See `mln_map_projection` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class MapProjectionHandle implements Finalizable {
  MapProjectionHandle._(NativeMapProjection handle)
    : _state = NativeHandleState(handle, 'MapProjectionHandle');
  final NativeHandleState<NativeMapProjection> _state;
  NativeMapProjection get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Closes a standalone projection.
  ///
  /// See `mln_map_projection_close` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  void close() => _state.close(
    (handle) => raw.mln_map_projection_close(handle.raw, nativeDiagnostic),
  );

  /// Copies the projection camera into out_camera.
  ///
  /// See `mln_map_projection_get_camera` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  CameraOptions getCamera() => withNativeArena((arena) {
    final outCamera = arena<raw.mln_camera_options>();
    outCamera.ref = raw.mln_camera_options_default();
    _check(
      raw.mln_map_projection_get_camera(
        _handle.raw,
        outCamera,
        nativeDiagnostic,
      ),
    );
    return _readCameraOptions(outCamera.ref);
  });

  /// Converts a screen point to a geographic coordinate.
  ///
  /// See `mln_map_projection_lat_lng_for_pixel` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  LatLng latLngForPixel(ScreenPoint point) => withNativeArena((arena) {
    final outCoordinate = arena<raw.mln_lat_lng>();
    _check(
      raw.mln_map_projection_lat_lng_for_pixel(
        _handle.raw,
        _writeScreenPoint(point, arena).ref,
        outCoordinate,
        nativeDiagnostic,
      ),
    );
    return _readLatLng(outCoordinate.ref);
  });

  /// Converts a screen point to an unwrapped geographic coordinate.
  ///
  /// See `mln_map_projection_lat_lng_for_pixel_unwrapped` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  LatLng latLngForPixelUnwrapped(ScreenPoint point) => withNativeArena((arena) {
    final outCoordinate = arena<raw.mln_lat_lng>();
    _check(
      raw.mln_map_projection_lat_lng_for_pixel_unwrapped(
        _handle.raw,
        _writeScreenPoint(point, arena).ref,
        outCoordinate,
        nativeDiagnostic,
      ),
    );
    return _readLatLng(outCoordinate.ref);
  });

  /// Reads the ground distance covered by one logical map pixel at a latitude
  /// for the helper camera zoom.
  ///
  /// See `mln_map_projection_meters_per_pixel_at_latitude` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  double metersPerPixelAtLatitude(double latitude) => withNativeArena((arena) {
    final outMetersPerPixel = arena<Double>();
    _check(
      raw.mln_map_projection_meters_per_pixel_at_latitude(
        _handle.raw,
        latitude,
        outMetersPerPixel,
        nativeDiagnostic,
      ),
    );
    return outMetersPerPixel.value;
  });

  /// Converts a geographic coordinate to a screen point.
  ///
  /// See `mln_map_projection_pixel_for_lat_lng` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  ScreenPoint pixelForLatLng(LatLng coordinate) => withNativeArena((arena) {
    final outPoint = arena<raw.mln_screen_point>();
    _check(
      raw.mln_map_projection_pixel_for_lat_lng(
        _handle.raw,
        _writeLatLng(coordinate, arena).ref,
        outPoint,
        nativeDiagnostic,
      ),
    );
    return _readScreenPoint(outPoint.ref);
  });

  /// Applies a camera update to a standalone projection.
  ///
  /// See `mln_map_projection_set_camera` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  void setCamera(CameraOptions camera) => withNativeArena((arena) {
    _check(
      raw.mln_map_projection_set_camera(
        _handle.raw,
        _writeCameraOptions(camera, arena),
        nativeDiagnostic,
      ),
    );
  });

  /// Applies a camera fit for geographic coordinates.
  ///
  /// See `mln_map_projection_set_visible_coordinates` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  void setVisibleCoordinates(List<LatLng> coordinates, EdgeInsets padding) =>
      withNativeArena((arena) {
        final nativecoordinates = arena<raw.mln_lat_lng>(
          coordinates.isEmpty ? 1 : coordinates.length,
        );
        for (var index = 0; index < coordinates.length; index++) {
          nativecoordinates[index] = _writeLatLng(
            coordinates[index],
            arena,
          ).ref;
        }
        _check(
          raw.mln_map_projection_set_visible_coordinates(
            _handle.raw,
            nativecoordinates,
            coordinates.length,
            _writeEdgeInsets(padding, arena).ref,
            nativeDiagnostic,
          ),
        );
      });

  /// Applies a camera fit for GeoJSON Geometry bytes.
  ///
  /// See `mln_map_projection_set_visible_geometry` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  void setVisibleGeometry(Uint8List geometry, EdgeInsets padding) =>
      withNativeArena((arena) {
        _check(
          raw.mln_map_projection_set_visible_geometry(
            _handle.raw,
            nativeBufferView(geometry, arena),
            _writeEdgeInsets(padding, arena).ref,
            nativeDiagnostic,
          ),
        );
      });
}

/// Issued `mln_render_frame_batch` handle id.
extension type const NativeRenderFrameBatch(int raw) implements NativeHandle {}

/// Owner of one native `mln_render_frame_batch` handle.
///
/// An owned batch of frame results from one drain.
///
/// See `mln_render_frame_batch` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class RenderFrameBatchHandle implements Finalizable {
  RenderFrameBatchHandle._(NativeRenderFrameBatch handle)
    : _state = NativeHandleState(handle, 'RenderFrameBatchHandle');
  final NativeHandleState<NativeRenderFrameBatch> _state;
  NativeRenderFrameBatch get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Returns the number of records in an owned frame-result batch.
  ///
  /// See `mln_render_frame_batch_count` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  int count() => withNativeArena((arena) {
    final outCount = arena<Size>();
    _check(
      raw.mln_render_frame_batch_count(_handle.raw, outCount, nativeDiagnostic),
    );
    return outCount.value;
  });

  /// Copies one frame-result record.
  ///
  /// See `mln_render_frame_batch_get` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  RenderFrameResult getValue(int indexValue) => withNativeArena((arena) {
    final outResult = arena<raw.mln_render_frame_result>();
    outResult.ref.size = sizeOf<raw.mln_render_frame_result>();
    _check(
      raw.mln_render_frame_batch_get(
        _handle.raw,
        _nativeInteger(
          indexValue,
          0,
          sizeOf<Size>() == 4 ? 4294967295 : 0x7fffffffffffffff,
        ),
        outResult,
        nativeDiagnostic,
      ),
    );
    return _readRenderFrameResult(outResult.ref);
  });

  /// Releases a frame-result batch.
  ///
  /// See `mln_render_frame_batch_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void close() => _state.close((handle) {
    raw.mln_render_frame_batch_release(handle.raw);
    return nativeStatusOk;
  });
}

/// Issued `mln_render_session` handle id.
extension type const NativeRenderSession(int raw) implements NativeHandle {}

/// Owner of one native `mln_render_session` handle.
///
/// A render session, which renders one map to one render target.
///
/// See `mln_render_session` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class RenderSessionHandle implements Finalizable {
  RenderSessionHandle._(this._parent, NativeRenderSession handle)
    : _state = NativeHandleState(handle, 'RenderSessionHandle');
  // Keeps the parent owner reachable while this owner lives.
  // ignore: unused_field
  final MapHandle _parent;
  final NativeHandleState<NativeRenderSession> _state;
  NativeRenderSession get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Starts an ordered caller-owned Metal texture replacement.
  ///
  /// See `mln_metal_borrowed_texture_set_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<void> metalBorrowedTextureSetTarget(
    MetalBorrowedTextureDescriptor descriptor,
  ) => _run(
    (arena, completion) => raw.mln_metal_borrowed_texture_set_target(
      _handle.raw,
      _writeMetalBorrowedTextureDescriptor(descriptor, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered Metal surface replacement.
  ///
  /// See `mln_metal_surface_set_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  Future<void> metalSurfaceSetTarget(MetalSurfaceDescriptor descriptor) => _run(
    (arena, completion) => raw.mln_metal_surface_set_target(
      _handle.raw,
      _writeMetalSurfaceDescriptor(descriptor, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered caller-owned OpenGL texture replacement.
  ///
  /// See `mln_opengl_borrowed_texture_set_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<void> openglBorrowedTextureSetTarget(
    OpenglBorrowedTextureDescriptor descriptor,
  ) => _run(
    (arena, completion) => raw.mln_opengl_borrowed_texture_set_target(
      _handle.raw,
      _writeOpenglBorrowedTextureDescriptor(descriptor, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered OpenGL surface replacement.
  ///
  /// See `mln_opengl_surface_set_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  Future<void> openglSurfaceSetTarget(OpenglSurfaceDescriptor descriptor) =>
      _run(
        (arena, completion) => raw.mln_opengl_surface_set_target(
          _handle.raw,
          _writeOpenglSurfaceDescriptor(descriptor, arena),
          completion,
          nativeDiagnostic,
        ),
      );

  /// Irreversibly closes control and mailboxes and disposes of the session's
  /// graphics objects.
  ///
  /// See `mln_render_session_abandon` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  RenderAbandonResult abandon() => withNativeArena((arena) {
    final outResult = arena<raw.mln_render_abandon_result>();
    outResult.ref.size = sizeOf<raw.mln_render_abandon_result>();
    _check(
      raw.mln_render_session_abandon(_handle.raw, outResult, nativeDiagnostic),
    );
    return _readRenderAbandonResult(outResult.ref);
  });

  /// Acquires the oldest rendered frame that is not already acquired. The frame
  /// owns its slot until release. The call is nonblocking.
  ///
  /// See `mln_render_session_acquire_frame` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  AcquiredFrameHandle? acquireFrame() => withNativeArena((arena) {
    final outFrame = arena<Uint64>();
    if (!_present(
      raw.mln_render_session_acquire_frame(
        _handle.raw,
        outFrame,
        nativeDiagnostic,
      ),
      raw.MLN_STATUS_NOT_READY,
    )) {
      return null;
    }
    return _adoptOwned(
      outFrame.value,
      () => AcquiredFrameHandle._(this, NativeAcquiredFrame(outFrame.value)),
      (handle) {
        _check(raw.mln_acquired_frame_dispose(handle, nativeDiagnostic));
      },
    );
  });

  /// Starts a barrier that completes after all render work accepted before it
  /// has a terminal result. A barrier does not request a frame.
  ///
  /// See `mln_render_session_barrier` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  Future<void> barrier() => _run(
    (arena, completion) => raw.mln_render_session_barrier(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts asynchronous renderer-data clearing.
  ///
  /// See `mln_render_session_clear_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  Future<void> clearData() => _run(
    (arena, completion) => raw.mln_render_session_clear_data(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Retires a detached or abandoned session handle. The call is CPU-only and
  /// may run on any native thread, including from one of the session's own
  /// completions. If an abandonment is still in progress on another thread,
  /// this waits for it to finish before consuming the session owner.
  ///
  /// See `mln_render_session_destroy` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void close() => _state.close(
    (handle) => raw.mln_render_session_destroy(handle.raw, nativeDiagnostic),
  );

  /// Starts normal graphics-owner teardown and map detachment.
  ///
  /// See `mln_render_session_detach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  Future<void> detach() => _run(
    (arena, completion) => raw.mln_render_session_detach(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Consumes a session and schedules its retirement and destruction.
  ///
  /// See `mln_render_session_dispose` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void dispose() => _state.close(
    (handle) => raw.mln_render_session_dispose(handle.raw, nativeDiagnostic),
  );

  /// Drains every currently queued terminal frame result into an independently
  /// owned batch. The records remain stable until the batch is released.
  ///
  /// See `mln_render_session_drain_frame_results` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  RenderFrameBatchHandle? drainFrameResults() => withNativeArena((arena) {
    final outBatch = arena<Uint64>();
    if (!_present(
      raw.mln_render_session_drain_frame_results(
        _handle.raw,
        outBatch,
        nativeDiagnostic,
      ),
      raw.MLN_STATUS_NOT_READY,
    )) {
      return null;
    }
    return _adoptOwned(
      outBatch.value,
      () => RenderFrameBatchHandle._(NativeRenderFrameBatch(outBatch.value)),
      (handle) {
        raw.mln_render_frame_batch_release(handle);
      },
    );
  });

  /// Starts asynchronous renderer diagnostic-log emission.
  ///
  /// See `mln_render_session_dump_debug_logs` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  Future<void> dumpDebugLogs() => _run(
    (arena, completion) => raw.mln_render_session_dump_debug_logs(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Returns the immutable capabilities fixed during attachment.
  ///
  /// See `mln_render_session_get_capabilities` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  RenderSessionCapabilities getCapabilities() => withNativeArena((arena) {
    final outCapabilities = arena<raw.mln_render_session_capabilities>();
    outCapabilities.ref.size = sizeOf<raw.mln_render_session_capabilities>();
    _check(
      raw.mln_render_session_get_capabilities(
        _handle.raw,
        outCapabilities,
        nativeDiagnostic,
      ),
    );
    return _readRenderSessionCapabilities(outCapabilities.ref);
  });

  /// Copies the latest render-session snapshot from any native thread.
  ///
  /// See `mln_render_session_get_snapshot` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  RenderSessionSnapshot getSnapshot() => withNativeArena((arena) {
    final outSnapshot = arena<raw.mln_render_session_snapshot>();
    outSnapshot.ref.size = sizeOf<raw.mln_render_session_snapshot>();
    _check(
      raw.mln_render_session_get_snapshot(
        _handle.raw,
        outSnapshot,
        nativeDiagnostic,
      ),
    );
    return _readRenderSessionSnapshot(outSnapshot.ref);
  });

  /// Copies the last completed rendered transform into an independent
  /// projection. Callable from any thread. Returns invalid state before a
  /// completed render, after an extent or target change, or after detachment.
  /// The caller owns the returned projection, which remains usable after the
  /// session is released. out_projection must point to a null handle.
  ///
  /// See `mln_render_session_projection_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  MapProjectionHandle projectionCreate() => withNativeArena((arena) {
    final outProjection = arena<Uint64>();
    _check(
      raw.mln_render_session_projection_create(
        _handle.raw,
        outProjection,
        nativeDiagnostic,
      ),
    );
    return _adoptOwned(
      outProjection.value,
      () => MapProjectionHandle._(NativeMapProjection(outProjection.value)),
      (handle) {
        _check(raw.mln_map_projection_close(handle, nativeDiagnostic));
      },
    );
  });

  /// Starts a feature-extension query against the latest driver state. The
  /// completion borrows one `mln_buffer_view` holding UTF-8 JSON (value_count
  /// 1), valid only for the callback.
  ///
  /// See `mln_render_session_query_feature_extensions` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
  Future<Uint8List> queryFeatureExtensions(
    String sourceId,
    Uint8List feature,
    String extensionValue,
    String extensionField, {
    Uint8List? arguments,
  }) => _query(
    _resultUint8List,
    (arena, completion) => raw.mln_render_session_query_feature_extensions(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeBufferView(feature, arena),
      nativeStringView(extensionValue, arena).value,
      nativeStringView(extensionField, arena).value,
      arguments == null
          ? nullptr
          : (() {
              final storage = arena<raw.mln_buffer_view>();
              storage.ref = nativeBufferView(arguments, arena);
              return storage;
            })(),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts a rendered-feature query against the session's latest driver state.
  ///
  /// See `mln_render_session_query_rendered_features` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
  Future<List<QueriedFeature>> queryRenderedFeatures(
    RenderedQueryGeometry geometry, {
    RenderedFeatureQueryOptions? options,
  }) => _queryList(
    _resultQueriedFeature,
    (arena, completion) => raw.mln_render_session_query_rendered_features(
      _handle.raw,
      _writeRenderedQueryGeometry(geometry, arena),
      options == null
          ? nullptr
          : _writeRenderedFeatureQueryOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts a source-feature query against the session's latest driver state.
  /// The completion borrows an array of `mln_queried_feature` values
  /// (value_count entries), valid only for the callback.
  ///
  /// See `mln_render_session_query_source_features` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
  Future<List<QueriedFeature>> querySourceFeatures(
    String sourceId, {
    SourceFeatureQueryOptions? options,
  }) => _queryList(
    _resultQueriedFeature,
    (arena, completion) => raw.mln_render_session_query_source_features(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      options == null
          ? nullptr
          : _writeSourceFeatureQueryOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts best-effort release of renderer caches.
  ///
  /// See `mln_render_session_reduce_memory_use` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  Future<void> reduceMemoryUse() => _run(
    (arena, completion) => raw.mln_render_session_reduce_memory_use(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Requests a frame without waiting. Every accepted demand produces one
  /// terminal result record. A core worker wakes itself; a caller driver
  /// publishes its driver-work endpoint.
  ///
  /// See `mln_render_session_request_frame` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void requestFrame(FrameDemand demand) => withNativeArena((arena) {
    _check(
      raw.mln_render_session_request_frame(
        _handle.raw,
        _writeFrameDemand(demand, arena),
        nativeDiagnostic,
      ),
    );
  });

  /// Starts an ordered logical resize. The completion runs after the selected
  /// driver applies the extent and updates the map viewport.
  ///
  /// See `mln_render_session_resize` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  Future<CommandCompletion> resize(RenderTargetExtent extent) => _command(
    (arena, completion) => raw.mln_render_session_resize(
      _handle.raw,
      _writeRenderTargetExtent(extent, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Services up to max_work items for a caller-graphics-thread driver; zero
  /// services every item currently queued. The first successful service call
  /// fixes the session's graphics-thread identity; later calls from another
  /// native thread return `MLN_STATUS_WRONG_THREAD`. The target context must be
  /// current. Core-worker sessions return `MLN_STATUS_INVALID_STATE`.
  ///
  /// See `mln_render_session_service_driver_work` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  int serviceDriverWork(int maxWork) => withNativeArena((arena) {
    final outServiced = arena<Size>();
    _check(
      raw.mln_render_session_service_driver_work(
        _handle.raw,
        _nativeInteger(
          maxWork,
          0,
          sizeOf<Size>() == 4 ? 4294967295 : 0x7fffffffffffffff,
        ),
        outServiced,
        nativeDiagnostic,
      ),
    );
    return outServiced.value;
  });

  /// Starts readback of the latest rendered texture frame.
  ///
  /// See `mln_texture_read_premultiplied_rgba8` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<TextureReadbackResult> textureReadPremultipliedRgba8() => _query(
    _resultTextureReadbackResult,
    (arena, completion) => raw.mln_texture_read_premultiplied_rgba8(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered caller-owned Vulkan texture replacement.
  ///
  /// See `mln_vulkan_borrowed_texture_set_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<void> vulkanBorrowedTextureSetTarget(
    VulkanBorrowedTextureDescriptor descriptor,
  ) => _run(
    (arena, completion) => raw.mln_vulkan_borrowed_texture_set_target(
      _handle.raw,
      _writeVulkanBorrowedTextureDescriptor(descriptor, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered Vulkan surface replacement.
  ///
  /// See `mln_vulkan_surface_set_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  Future<void> vulkanSurfaceSetTarget(VulkanSurfaceDescriptor descriptor) =>
      _run(
        (arena, completion) => raw.mln_vulkan_surface_set_target(
          _handle.raw,
          _writeVulkanSurfaceDescriptor(descriptor, arena),
          completion,
          nativeDiagnostic,
        ),
      );

  /// Starts an ordered caller-owned WebGPU texture replacement.
  ///
  /// See `mln_webgpu_borrowed_texture_set_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<void> webgpuBorrowedTextureSetTarget(
    WebgpuBorrowedTextureDescriptor descriptor,
  ) => _run(
    (arena, completion) => raw.mln_webgpu_borrowed_texture_set_target(
      _handle.raw,
      _writeWebgpuBorrowedTextureDescriptor(descriptor, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered WebGPU surface replacement.
  ///
  /// See `mln_webgpu_surface_set_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  Future<void> webgpuSurfaceSetTarget(WebgpuSurfaceDescriptor descriptor) =>
      _run(
        (arena, completion) => raw.mln_webgpu_surface_set_target(
          _handle.raw,
          _writeWebgpuSurfaceDescriptor(descriptor, arena),
          completion,
          nativeDiagnostic,
        ),
      );
}

/// Issued `mln_resource_request_handle` handle id.
extension type const NativeResourceRequest(int raw) implements NativeHandle {}

/// Owner of one native `mln_resource_request_handle` handle.
///
/// A resource request that a resource provider handles.
///
/// See `mln_resource_request_handle` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class ResourceRequestHandle implements Finalizable, _CallbackPortOwner {
  ResourceRequestHandle._(NativeResourceRequest handle)
    : _state = NativeHandleState(handle, 'ResourceRequestHandle');
  // Roots this owner's port registrations for as long as it lives.
  @override
  final _callbackPorts = _NativeCallbackPorts();
  final NativeHandleState<NativeResourceRequest> _state;
  NativeResourceRequest get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Reports whether MapLibre has cancelled a C API resource provider request.
  ///
  /// See `mln_resource_request_cancelled` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  bool cancelled() => withNativeArena((arena) {
    final outCancelled = arena<Bool>();
    _check(
      raw.mln_resource_request_cancelled(
        _handle.raw,
        outCancelled,
        nativeDiagnostic,
      ),
    );
    return outCancelled.value;
  });

  /// Completes a C API resource provider request.
  ///
  /// See `mln_resource_request_complete` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void complete(ResourceResponse response) => withNativeArena((arena) {
    _check(
      raw.mln_resource_request_complete(
        _handle.raw,
        _writeResourceResponse(response, arena),
        nativeDiagnostic,
      ),
    );
  });

  /// Releases the provider's reference to a resource request handle.
  ///
  /// See `mln_resource_request_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void close() => _state.close((handle) {
    raw.mln_resource_request_release(handle.raw);
    return nativeStatusOk;
  });

  /// Registers a callback that runs when MapLibre cancels a C API resource
  /// provider request.
  ///
  /// See `mln_resource_request_set_cancel_callback` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  bool setCancelCallback(
    ResourceRequestCancelCallback callback,
  ) => withNativeArena((arena) {
    final handle = _handle;
    final declined = arena<Bool>();
    final port = _callbackPorts.register({
      (raw.MLN_ADAPTER_DART_PORT_RESOURCE_REQUEST_SET_CANCEL_CALLBACK_CALLBACK &
          0xffffffff): (message) {
        if (!isClosed) {
          callback();
        }
      },
    });
    var accepted = false;
    try {
      _check(
        raw.mln_resource_request_set_cancel_callback(
          handle.raw,
          raw
              .mln_adapter_dart_port_function(
                (raw.MLN_ADAPTER_DART_PORT_RESOURCE_REQUEST_SET_CANCEL_CALLBACK_CALLBACK &
                    0xffffffff),
              )
              .cast(),
          port.context,
          Native.addressOf<
                NativeFunction<raw.mln_runtime_callback_releaseFunction>
              >(raw.mln_adapter_dart_port_release)
              .cast(),
          declined,
          nativeDiagnostic,
        ),
      );
      accepted = !declined.value;
      return declined.value;
    } finally {
      if (!accepted) {
        port.reject();
      }
    }
  });

  /// Blocks until a resource request is released and its cancel callback
  /// registration has retired: the callback, if it ran, and release_user_data
  /// have both returned. Completing a request does not release its owner.
  ///
  /// See `mln_resource_request_wait_until_retired` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void waitUntilRetired() => _check(
    raw.mln_resource_request_wait_until_retired(
      _state.handleId,
      nativeDiagnostic,
    ),
  );
}

/// Issued `mln_runtime` handle id.
extension type const NativeRuntime(int raw) implements NativeHandle {}

/// Owner of one native `mln_runtime` handle.
///
/// A runtime: the native scheduler thread and event store for its maps.
///
/// See `mln_runtime` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class RuntimeHandle implements Finalizable, _CallbackPortOwner {
  RuntimeHandle._(NativeRuntime handle)
    : _state = NativeHandleState(handle, 'RuntimeHandle');
  // Roots this owner's port registrations for as long as it lives.
  @override
  final _callbackPorts = _NativeCallbackPorts();
  final NativeHandleState<NativeRuntime> _state;
  NativeRuntime get _handle => _state.handle;

  /// Whether this binding object has released its native handle.
  bool get isClosed => _state.isClosed;

  /// The issued native handle id.
  BigInt get identity => uint64FromNative(_state.handleId);

  /// Creates a map on the runtime worker.
  ///
  /// See `mln_map_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<MapHandle> mapCreate(MapOptions options) => _queryOwned(
    raw.MLN_ADAPTER_COMPLETION_COPY_MAP,
    (arena, completion) => raw.mln_map_create(
      _handle.raw,
      _writeMapOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
    (handle) => _adoptOwned(
      handle,
      () => MapHandle._(this, NativeMap(handle)),
      (handle) {
        _check(raw.mln_map_dispose(handle, nativeDiagnostic));
      },
    ),
  );

  /// Starts an ordered runtime barrier.
  ///
  /// See `mln_runtime_barrier` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> barrier() => _run(
    (arena, completion) =>
        raw.mln_runtime_barrier(_handle.raw, completion, nativeDiagnostic),
  );

  /// Clears the runtime-scoped outgoing HTTP header transform.
  ///
  /// See `mln_runtime_clear_http_header_transform` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> clearHttpHeaderTransform() => _run(
    (arena, completion) => raw.mln_runtime_clear_http_header_transform(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Clears the runtime-scoped network resource provider.
  ///
  /// See `mln_runtime_clear_resource_provider` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> clearResourceProvider() => _run(
    (arena, completion) => raw.mln_runtime_clear_resource_provider(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Clears the runtime-scoped URL transform for network resources.
  ///
  /// See `mln_runtime_clear_resource_transform` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> clearResourceTransform() => _run(
    (arena, completion) => raw.mln_runtime_clear_resource_transform(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Consumes a runtime handle without observing its asynchronous retirement.
  ///
  /// See `mln_runtime_dispose` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void dispose() => _state.close(
    (handle) => raw.mln_runtime_dispose(handle.raw, nativeDiagnostic),
  );

  /// Drains this runtime's queued events into a new owned batch.
  ///
  /// See `mln_runtime_drain_events` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  EventBatchHandle drainEvents() => withNativeArena((arena) {
    final outBatch = arena<Uint64>();
    _check(
      raw.mln_runtime_drain_events(_handle.raw, outBatch, nativeDiagnostic),
    );
    return _adoptOwned(
      outBatch.value,
      () => EventBatchHandle._(NativeEventBatch(outBatch.value)),
      (handle) {
        raw.mln_event_batch_release(handle);
      },
    );
  });

  /// Reports which runtime-scoped event types this runtime queues.
  ///
  /// See `mln_runtime_get_event_mask` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  RuntimeEventMask getEventMask() => withNativeArena((arena) {
    final outMask = arena<Uint64>();
    _check(
      raw.mln_runtime_get_event_mask(_handle.raw, outMask, nativeDiagnostic),
    );
    return RuntimeEventMask.fromRawValue(outMask.value);
  });

  /// Starts creating an offline region.
  ///
  /// See `mln_runtime_offline_region_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<OfflineRegionInfo> offlineRegionCreate(
    OfflineRegionDefinition definition,
    Uint8List metadata,
  ) => _query(_resultOfflineRegionInfo, (arena, completion) {
    final bytesmetadata = nativeBufferView(metadata, arena);
    return raw.mln_runtime_offline_region_create(
      _handle.raw,
      _writeOfflineRegionDefinition(definition, arena),
      bytesmetadata.data.cast(),
      bytesmetadata.size,
      completion,
      nativeDiagnostic,
    );
  });

  /// Deletes an offline region.
  ///
  /// See `mln_runtime_offline_region_delete` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> offlineRegionDelete(int regionId) => _run(
    (arena, completion) => raw.mln_runtime_offline_region_delete(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts getting one offline region by ID.
  ///
  /// See `mln_runtime_offline_region_get` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<OfflineRegionInfo?> offlineRegionGet(int regionId) => _queryOptional(
    _resultOfflineRegionInfo,
    (arena, completion) => raw.mln_runtime_offline_region_get(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts getting the current download status for an offline region.
  ///
  /// See `mln_runtime_offline_region_get_status` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<OfflineRegionStatus> offlineRegionGetStatus(int regionId) => _query(
    _resultOfflineRegionStatus,
    (arena, completion) => raw.mln_runtime_offline_region_get_status(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Invalidates cached resources for an offline region.
  ///
  /// See `mln_runtime_offline_region_invalidate` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> offlineRegionInvalidate(int regionId) => _run(
    (arena, completion) => raw.mln_runtime_offline_region_invalidate(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets an offline region's native download state.
  ///
  /// See `mln_runtime_offline_region_set_download_state` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> offlineRegionSetDownloadState(
    int regionId,
    OfflineRegionDownloadState state,
  ) => _run(
    (arena, completion) => raw.mln_runtime_offline_region_set_download_state(
      _handle.raw,
      regionId,
      state.rawValue,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Enables or disables runtime events for an offline region.
  ///
  /// See `mln_runtime_offline_region_set_observed` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> offlineRegionSetObserved(int regionId, bool observed) => _run(
    (arena, completion) => raw.mln_runtime_offline_region_set_observed(
      _handle.raw,
      regionId,
      observed,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts updating opaque binary metadata for an offline region.
  ///
  /// See `mln_runtime_offline_region_update_metadata` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<OfflineRegionInfo> offlineRegionUpdateMetadata(
    int regionId,
    Uint8List metadata,
  ) => _query(_resultOfflineRegionInfo, (arena, completion) {
    final bytesmetadata = nativeBufferView(metadata, arena);
    return raw.mln_runtime_offline_region_update_metadata(
      _handle.raw,
      regionId,
      bytesmetadata.data.cast(),
      bytesmetadata.size,
      completion,
      nativeDiagnostic,
    );
  });

  /// Starts listing the offline regions in the runtime database.
  ///
  /// See `mln_runtime_offline_regions_list` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<List<OfflineRegionInfo>> offlineRegionsList() => _queryList(
    _resultOfflineRegionInfo,
    (arena, completion) => raw.mln_runtime_offline_regions_list(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts merging offline regions from another MapLibre offline database.
  ///
  /// See `mln_runtime_offline_regions_merge_database` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<List<OfflineRegionInfo>> offlineRegionsMergeDatabase(
    String sideDatabasePath,
  ) => _queryList(
    _resultOfflineRegionInfo,
    (arena, completion) => raw.mln_runtime_offline_regions_merge_database(
      _handle.raw,
      nativeUtf8CString(sideDatabasePath, arena).pointer.cast<Char>(),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Releases a runtime after synchronous child preflight.
  ///
  /// See `mln_runtime_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> close() => _state.closeAsync(
    (handle) => _run(
      (arena, completion) =>
          raw.mln_runtime_release(handle.raw, completion, nativeDiagnostic),
    ),
  );

  /// Starts a MapLibre ambient cache maintenance operation for this runtime.
  ///
  /// See `mln_runtime_run_ambient_cache_operation` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> runAmbientCacheOperation(AmbientCacheOperation operation) =>
      _run(
        (arena, completion) => raw.mln_runtime_run_ambient_cache_operation(
          _handle.raw,
          operation.rawValue,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Selects which runtime-scoped event types this runtime queues.
  ///
  /// See `mln_runtime_set_event_mask` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void setEventMask(RuntimeEventMask mask) => _check(
    raw.mln_runtime_set_event_mask(
      _handle.raw,
      mask.rawValue,
      nativeDiagnostic,
    ),
  );

  /// Registers or replaces the runtime-scoped outgoing HTTP header transform.
  ///
  /// See `mln_runtime_set_http_header_transform` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> setHttpHeaderTransform(HttpHeaderTransform transform) =>
      _run((arena, completion) {
        final registrations = _NativeRegistrations(_callbackPorts);
        return registrations.run(
          () => raw.mln_runtime_set_http_header_transform(
            _handle.raw,
            registrations.add(
              _prepareHttpHeaderTransform(transform, _callbackReleases),
            ),
            completion,
            nativeDiagnostic,
          ),
        );
      });

  /// Starts a change to this runtime's maximum ambient cache size.
  ///
  /// See `mln_runtime_set_maximum_ambient_cache_size` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> setMaximumAmbientCacheSize(BigInt size) => _run(
    (arena, completion) => raw.mln_runtime_set_maximum_ambient_cache_size(
      _handle.raw,
      uint64ToNative(size, 'uint64_t'),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Registers or replaces a runtime-scoped network resource provider.
  ///
  /// See `mln_runtime_set_resource_provider` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> setResourceProvider(ResourceProvider provider) =>
      _run((arena, completion) {
        final registrations = _NativeRegistrations(_callbackPorts);
        return registrations.run(
          () => raw.mln_runtime_set_resource_provider(
            _handle.raw,
            registrations.add(
              _prepareResourceProvider(
                provider,
                _callbackReleases,
                registrations.ports,
              ),
            ),
            completion,
            nativeDiagnostic,
          ),
        );
      });

  /// Registers or updates a runtime-scoped URL transform for network resources.
  ///
  /// See `mln_runtime_set_resource_transform` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> setResourceTransform(ResourceTransform transform) =>
      _run((arena, completion) {
        final registrations = _NativeRegistrations(_callbackPorts);
        return registrations.run(
          () => raw.mln_runtime_set_resource_transform(
            _handle.raw,
            registrations.add(
              _prepareResourceTransform(transform, _callbackReleases),
            ),
            completion,
            nativeDiagnostic,
          ),
        );
      });
}

/// A new session and the completion of the attachment that created it.
final class RenderSessionAttachment {
  const RenderSessionAttachment(this.session, this.completed);

  /// The session, usable at once while attachment completes.
  final RenderSessionHandle session;

  /// Completes after native attachment finishes.
  final Future<void> completed;
}

final class ScopedMetalOwnedTextureFrame {
  ScopedMetalOwnedTextureFrame._(AcquiredFrameHandle owner, this._value)
    : _scope = _NativeViewScope(
        owner._state,
        raw.mln_acquired_frame_view_begin,
        raw.mln_acquired_frame_view_end,
      );
  final MetalOwnedTextureFrame _value;
  final _NativeViewScope _scope;
  T withView<T>(T Function(ScopedMetalOwnedTextureFrame) use) =>
      _scope.use(() => use(this));
  BigInt get generation => _scope.active(_value).generation;
  int get width => _scope.active(_value).width;
  int get height => _scope.active(_value).height;
  double get scaleFactor => _scope.active(_value).scaleFactor;
  BigInt get frameId => _scope.active(_value).frameId;
  ScopedNativePointer get texture => ScopedNativePointer(
    _value.texture.address,
    checkValid: _scope.checkActive,
    debugName: 'MetalOwnedTextureFrame.texture',
  );
  ScopedNativePointer get device => ScopedNativePointer(
    _value.device.address,
    checkValid: _scope.checkActive,
    debugName: 'MetalOwnedTextureFrame.device',
  );
  BigInt get pixelFormat => _scope.active(_value).pixelFormat;
}

final class ScopedOpenglOwnedTextureFrame {
  ScopedOpenglOwnedTextureFrame._(AcquiredFrameHandle owner, this._value)
    : _scope = _NativeViewScope(
        owner._state,
        raw.mln_acquired_frame_view_begin,
        raw.mln_acquired_frame_view_end,
      );
  final OpenglOwnedTextureFrame _value;
  final _NativeViewScope _scope;
  T withView<T>(T Function(ScopedOpenglOwnedTextureFrame) use) =>
      _scope.use(() => use(this));
  BigInt get generation => _scope.active(_value).generation;
  int get width => _scope.active(_value).width;
  int get height => _scope.active(_value).height;
  double get scaleFactor => _scope.active(_value).scaleFactor;
  BigInt get frameId => _scope.active(_value).frameId;
  int get texture => _scope.active(_value).texture;
  int get target => _scope.active(_value).target;
  int get internalFormat => _scope.active(_value).internalFormat;
  int get format => _scope.active(_value).format;
  int get type => _scope.active(_value).type;
}

final class ScopedGpuSync {
  ScopedGpuSync._(AcquiredFrameHandle owner, this._value)
    : _scope = _NativeViewScope(
        owner._state,
        raw.mln_acquired_frame_view_begin,
        raw.mln_acquired_frame_view_end,
      );
  final GpuSync _value;
  final _NativeViewScope _scope;
  T withView<T>(T Function(ScopedGpuSync) use) => _scope.use(() => use(this));
  GpuSyncKind get kind => _scope.active(_value).kind;
  BigInt get object => _scope.active(_value).object;
  BigInt get value => _scope.active(_value).value;
}

final class ScopedVulkanOwnedTextureFrame {
  ScopedVulkanOwnedTextureFrame._(AcquiredFrameHandle owner, this._value)
    : _scope = _NativeViewScope(
        owner._state,
        raw.mln_acquired_frame_view_begin,
        raw.mln_acquired_frame_view_end,
      );
  final VulkanOwnedTextureFrame _value;
  final _NativeViewScope _scope;
  T withView<T>(T Function(ScopedVulkanOwnedTextureFrame) use) =>
      _scope.use(() => use(this));
  BigInt get generation => _scope.active(_value).generation;
  int get width => _scope.active(_value).width;
  int get height => _scope.active(_value).height;
  double get scaleFactor => _scope.active(_value).scaleFactor;
  BigInt get frameId => _scope.active(_value).frameId;
  BigInt get image => _scope.active(_value).image;
  BigInt get imageView => _scope.active(_value).imageView;
  ScopedNativePointer get device => ScopedNativePointer(
    _value.device.address,
    checkValid: _scope.checkActive,
    debugName: 'VulkanOwnedTextureFrame.device',
  );
  int get format => _scope.active(_value).format;
  int get layout => _scope.active(_value).layout;
}

final class ScopedWebgpuOwnedTextureFrame {
  ScopedWebgpuOwnedTextureFrame._(AcquiredFrameHandle owner, this._value)
    : _scope = _NativeViewScope(
        owner._state,
        raw.mln_acquired_frame_view_begin,
        raw.mln_acquired_frame_view_end,
      );
  final WebgpuOwnedTextureFrame _value;
  final _NativeViewScope _scope;
  T withView<T>(T Function(ScopedWebgpuOwnedTextureFrame) use) =>
      _scope.use(() => use(this));
  BigInt get generation => _scope.active(_value).generation;
  int get width => _scope.active(_value).width;
  int get height => _scope.active(_value).height;
  double get scaleFactor => _scope.active(_value).scaleFactor;
  BigInt get frameId => _scope.active(_value).frameId;
  ScopedNativePointer get texture => ScopedNativePointer(
    _value.texture.address,
    checkValid: _scope.checkActive,
    debugName: 'WebgpuOwnedTextureFrame.texture',
  );
  ScopedNativePointer get textureView => ScopedNativePointer(
    _value.textureView.address,
    checkValid: _scope.checkActive,
    debugName: 'WebgpuOwnedTextureFrame.textureView',
  );
  ScopedNativePointer get device => ScopedNativePointer(
    _value.device.address,
    checkValid: _scope.checkActive,
    debugName: 'WebgpuOwnedTextureFrame.device',
  );
  int get format => _scope.active(_value).format;
}
