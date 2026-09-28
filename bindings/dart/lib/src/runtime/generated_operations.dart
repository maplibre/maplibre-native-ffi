// Generated from the C headers by tools/bindgen. Do not edit.
part of 'runtime.dart';

final class _NativeRegistration<T extends Struct> {
  const _NativeRegistration(this.pointer, this.reject, [this.releaseMemory]);
  final Pointer<T> pointer;
  final void Function() reject;
  final void Function()? releaseMemory;
}

final class _NativeRegistrations {
  _NativeRegistrations(this.ports);
  final _NativeCallbackPorts ports;
  final _pending = <_NativeRegistration>[];
  bool _accepted = false;
  void accept() {
    _accepted = true;
  }

  void close() {
    for (final registration in _pending.reversed) {
      if (!_accepted) {
        registration.reject();
      }
      registration.releaseMemory?.call();
    }
    _pending.clear();
  }

  Pointer<raw.mln_custom_geometry_source_options>
  prepareCustomGeometrySourceOptions(CustomGeometrySourceOptions value) {
    final registration = _prepareCustomGeometrySourceOptions(value, ports);
    _pending.add(registration);
    return registration.pointer;
  }

  Pointer<raw.mln_custom_mvt_vector_source_options>
  prepareCustomMvtVectorSourceOptions(CustomMvtVectorSourceOptions value) {
    final registration = _prepareCustomMvtVectorSourceOptions(value, ports);
    _pending.add(registration);
    return registration.pointer;
  }

  Pointer<raw.mln_wake> prepareWake(Wake value) {
    final registration = _prepareWake(value, ports);
    _pending.add(registration);
    return registration.pointer;
  }

  Pointer<raw.mln_http_header_transform> prepareHttpHeaderTransform(
    HttpHeaderTransform value,
  ) {
    final registration = _prepareHttpHeaderTransform(value, _callbackReleases);
    _pending.add(registration);
    return registration.pointer;
  }

  Pointer<raw.mln_resource_provider> prepareResourceProvider(
    ResourceProvider value,
  ) {
    final registration = _prepareResourceProvider(value, _callbackReleases);
    _pending.add(registration);
    return registration.pointer;
  }

  Pointer<raw.mln_resource_transform> prepareResourceTransform(
    ResourceTransform value,
  ) {
    final registration = _prepareResourceTransform(value, _callbackReleases);
    _pending.add(registration);
    return registration.pointer;
  }
}

ResourceRequest _readAdapterQueuedResourceRequest(
  raw.mln_adapter_queued_resource_request source,
) => ResourceRequest(
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
    result.ref.fields |=
        raw.mln_animation_option_field.MLN_ANIMATION_OPTION_DURATION;
    result.ref.duration_ms = value.durationMs!;
  }
  if (value.velocity != null) {
    result.ref.fields |=
        raw.mln_animation_option_field.MLN_ANIMATION_OPTION_VELOCITY;
    result.ref.velocity = value.velocity!;
  }
  if (value.minZoom != null) {
    result.ref.fields |=
        raw.mln_animation_option_field.MLN_ANIMATION_OPTION_MIN_ZOOM;
    result.ref.min_zoom = value.minZoom!;
  }
  if (value.easing != null) {
    result.ref.fields |=
        raw.mln_animation_option_field.MLN_ANIMATION_OPTION_EASING;
    result.ref.easing = _writeUnitBezier(value.easing!, arena).ref;
  }
  if (value.transitionId != null) {
    result.ref.fields |=
        raw.mln_animation_option_field.MLN_ANIMATION_OPTION_TRANSITION_ID;
    result.ref.transition_id = uint64ToNative(value.transitionId!, 'uint64_t');
  }
  return result;
}

AnimationOptions _readAnimationOptions(
  raw.mln_animation_options source,
) => AnimationOptions(
  durationMs:
      (source.fields &
              raw.mln_animation_option_field.MLN_ANIMATION_OPTION_DURATION) !=
          0
      ? source.duration_ms
      : null,
  velocity:
      (source.fields &
              raw.mln_animation_option_field.MLN_ANIMATION_OPTION_VELOCITY) !=
          0
      ? source.velocity
      : null,
  minZoom:
      (source.fields &
              raw.mln_animation_option_field.MLN_ANIMATION_OPTION_MIN_ZOOM) !=
          0
      ? source.min_zoom
      : null,
  easing:
      (source.fields &
              raw.mln_animation_option_field.MLN_ANIMATION_OPTION_EASING) !=
          0
      ? _readUnitBezier(source.easing)
      : null,
  transitionId:
      (source.fields &
              raw
                  .mln_animation_option_field
                  .MLN_ANIMATION_OPTION_TRANSITION_ID) !=
          0
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
    result.ref.fields |= raw.mln_bound_option_field.MLN_BOUND_OPTION_UNBOUNDED;
  }
  if (value.bounds != null) {
    result.ref.fields |= raw.mln_bound_option_field.MLN_BOUND_OPTION_BOUNDS;
    result.ref.bounds = _writeLatLngBounds(value.bounds!, arena).ref;
  }
  if (value.minZoom != null) {
    result.ref.fields |= raw.mln_bound_option_field.MLN_BOUND_OPTION_MIN_ZOOM;
    result.ref.min_zoom = value.minZoom!;
  }
  if (value.maxZoom != null) {
    result.ref.fields |= raw.mln_bound_option_field.MLN_BOUND_OPTION_MAX_ZOOM;
    result.ref.max_zoom = value.maxZoom!;
  }
  if (value.minPitch != null) {
    result.ref.fields |= raw.mln_bound_option_field.MLN_BOUND_OPTION_MIN_PITCH;
    result.ref.min_pitch = value.minPitch!;
  }
  if (value.maxPitch != null) {
    result.ref.fields |= raw.mln_bound_option_field.MLN_BOUND_OPTION_MAX_PITCH;
    result.ref.max_pitch = value.maxPitch!;
  }
  return result;
}

BoundOptions _readBoundOptions(raw.mln_bound_options source) => BoundOptions(
  unbounded:
      (source.fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_UNBOUNDED) !=
      0,
  bounds:
      (source.fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_BOUNDS) != 0
      ? _readLatLngBounds(source.bounds)
      : null,
  minZoom:
      (source.fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_MIN_ZOOM) !=
          0
      ? source.min_zoom
      : null,
  maxZoom:
      (source.fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_MAX_ZOOM) !=
          0
      ? source.max_zoom
      : null,
  minPitch:
      (source.fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_MIN_PITCH) !=
          0
      ? source.min_pitch
      : null,
  maxPitch:
      (source.fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_MAX_PITCH) !=
          0
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
    result.ref.fields |=
        raw.mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_PADDING;
    result.ref.padding = _writeEdgeInsets(value.padding!, arena).ref;
  }
  if (value.bearing != null) {
    result.ref.fields |=
        raw.mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_BEARING;
    result.ref.bearing = value.bearing!;
  }
  if (value.pitch != null) {
    result.ref.fields |=
        raw.mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_PITCH;
    result.ref.pitch = value.pitch!;
  }
  return result;
}

CameraFitOptions _readCameraFitOptions(
  raw.mln_camera_fit_options source,
) => CameraFitOptions(
  padding:
      (source.fields &
              raw.mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_PADDING) !=
          0
      ? _readEdgeInsets(source.padding)
      : null,
  bearing:
      (source.fields &
              raw.mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_BEARING) !=
          0
      ? source.bearing
      : null,
  pitch:
      (source.fields &
              raw.mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_PITCH) !=
          0
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
    result.ref.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_CENTER;
    result.ref.latitude = value.center!.latitude;
    result.ref.longitude = value.center!.longitude;
  }
  if (value.centerAltitude != null) {
    result.ref.fields |=
        raw.mln_camera_option_field.MLN_CAMERA_OPTION_CENTER_ALTITUDE;
    result.ref.center_altitude = value.centerAltitude!;
  }
  if (value.padding != null) {
    result.ref.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_PADDING;
    result.ref.padding = _writeEdgeInsets(value.padding!, arena).ref;
  }
  if (value.anchor != null) {
    result.ref.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_ANCHOR;
    result.ref.anchor = _writeScreenPoint(value.anchor!, arena).ref;
  }
  if (value.zoom != null) {
    result.ref.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_ZOOM;
    result.ref.zoom = value.zoom!;
  }
  if (value.bearing != null) {
    result.ref.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_BEARING;
    result.ref.bearing = value.bearing!;
  }
  if (value.pitch != null) {
    result.ref.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_PITCH;
    result.ref.pitch = value.pitch!;
  }
  if (value.roll != null) {
    result.ref.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_ROLL;
    result.ref.roll = value.roll!;
  }
  if (value.fieldOfView != null) {
    result.ref.fields |= raw.mln_camera_option_field.MLN_CAMERA_OPTION_FOV;
    result.ref.field_of_view = value.fieldOfView!;
  }
  return result;
}

CameraOptions _readCameraOptions(
  raw.mln_camera_options source,
) => CameraOptions(
  center:
      (source.fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_CENTER) !=
          0
      ? LatLng(source.latitude, source.longitude)
      : null,
  centerAltitude:
      (source.fields &
              raw.mln_camera_option_field.MLN_CAMERA_OPTION_CENTER_ALTITUDE) !=
          0
      ? source.center_altitude
      : null,
  padding:
      (source.fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_PADDING) !=
          0
      ? _readEdgeInsets(source.padding)
      : null,
  anchor:
      (source.fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_ANCHOR) !=
          0
      ? _readScreenPoint(source.anchor)
      : null,
  zoom:
      (source.fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_ZOOM) != 0
      ? source.zoom
      : null,
  bearing:
      (source.fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_BEARING) !=
          0
      ? source.bearing
      : null,
  pitch:
      (source.fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_PITCH) != 0
      ? source.pitch
      : null,
  roll:
      (source.fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_ROLL) != 0
      ? source.roll
      : null,
  fieldOfView:
      (source.fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_FOV) != 0
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
  result.ref.z = _generatedInteger(value.z, 0, 4294967295);
  result.ref.x = _generatedInteger(value.x, 0, 4294967295);
  result.ref.y = _generatedInteger(value.y, 0, 4294967295);
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
        (raw
                .mln_adapter_dart_port_callback
                .MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE &
            0xffffffff): (message) => value.fetchTile!(
          CanonicalTileId(
            z: message[1] as int,
            x: message[2] as int,
            y: message[3] as int,
          ),
        ),
      if (value.cancelTile != null)
        (raw
                .mln_adapter_dart_port_callback
                .MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_CANCEL_TILE &
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
                (raw
                        .mln_adapter_dart_port_callback
                        .MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE &
                    0xffffffff),
              )
              .cast();
    result.ref.cancel_tile = value.cancelTile == null
        ? nullptr
        : raw
              .mln_adapter_dart_port_function(
                (raw
                        .mln_adapter_dart_port_callback
                        .MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_CANCEL_TILE &
                    0xffffffff),
              )
              .cast();
    if (value.minZoom != null) {
      result.ref.fields |= raw
          .mln_custom_geometry_source_option_field
          .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
      result.ref.min_zoom = value.minZoom!;
    }
    if (value.maxZoom != null) {
      result.ref.fields |= raw
          .mln_custom_geometry_source_option_field
          .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
      result.ref.max_zoom = value.maxZoom!;
    }
    if (value.tolerance != null) {
      result.ref.fields |= raw
          .mln_custom_geometry_source_option_field
          .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
      result.ref.tolerance = value.tolerance!;
    }
    if (value.tileSize != null) {
      result.ref.fields |= raw
          .mln_custom_geometry_source_option_field
          .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
      result.ref.tile_size = _generatedInteger(value.tileSize!, 0, 4294967295);
    }
    if (value.buffer != null) {
      result.ref.fields |= raw
          .mln_custom_geometry_source_option_field
          .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
      result.ref.buffer = _generatedInteger(value.buffer!, 0, 4294967295);
    }
    if (value.clip != null) {
      result.ref.fields |= raw
          .mln_custom_geometry_source_option_field
          .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
      result.ref.clip = value.clip!;
    }
    if (value.wrap != null) {
      result.ref.fields |= raw
          .mln_custom_geometry_source_option_field
          .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
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
) {
  if (source.fetch_tile != nullptr) {
    throwInvalidState('cannot copy a registered native callback');
  }
  if (source.cancel_tile != nullptr) {
    throwInvalidState('cannot copy a registered native callback');
  }
  return CustomGeometrySourceOptions(
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
}

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
        (raw
                .mln_adapter_dart_port_callback
                .MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_FETCH_TILE &
            0xffffffff): (message) => value.fetchTile!(
          CanonicalTileId(
            z: message[1] as int,
            x: message[2] as int,
            y: message[3] as int,
          ),
        ),
      if (value.cancelTile != null)
        (raw
                .mln_adapter_dart_port_callback
                .MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_CANCEL_TILE &
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
                (raw
                        .mln_adapter_dart_port_callback
                        .MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_FETCH_TILE &
                    0xffffffff),
              )
              .cast();
    result.ref.cancel_tile = value.cancelTile == null
        ? nullptr
        : raw
              .mln_adapter_dart_port_function(
                (raw
                        .mln_adapter_dart_port_callback
                        .MLN_ADAPTER_DART_PORT_CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_CANCEL_TILE &
                    0xffffffff),
              )
              .cast();
    if (value.minZoom != null) {
      result.ref.fields |= raw
          .mln_custom_mvt_vector_source_option_field
          .MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
      result.ref.min_zoom = value.minZoom!;
    }
    if (value.maxZoom != null) {
      result.ref.fields |= raw
          .mln_custom_mvt_vector_source_option_field
          .MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
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
) {
  if (source.fetch_tile != nullptr) {
    throwInvalidState('cannot copy a registered native callback');
  }
  if (source.cancel_tile != nullptr) {
    throwInvalidState('cannot copy a registered native callback');
  }
  return CustomMvtVectorSourceOptions(
    fetchTile: null,
    cancelTile: null,
    minZoom: source.min_zoom,
    maxZoom: source.max_zoom,
  );
}

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
          message: _generatedArenaUtf8(
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
    result.ref.fields |=
        raw.mln_free_camera_option_field.MLN_FREE_CAMERA_OPTION_POSITION;
    result.ref.position = _writeVec3(value.position!, arena).ref;
  }
  if (value.orientation != null) {
    result.ref.fields |=
        raw.mln_free_camera_option_field.MLN_FREE_CAMERA_OPTION_ORIENTATION;
    result.ref.orientation = _writeQuaternion(value.orientation!, arena).ref;
  }
  return result;
}

FreeCameraOptions _readFreeCameraOptions(raw.mln_free_camera_options source) =>
    FreeCameraOptions(
      position:
          (source.fields &
                  raw
                      .mln_free_camera_option_field
                      .MLN_FREE_CAMERA_OPTION_POSITION) !=
              0
          ? _readVec3(source.position)
          : null,
      orientation:
          (source.fields &
                  raw
                      .mln_free_camera_option_field
                      .MLN_FREE_CAMERA_OPTION_ORIENTATION) !=
              0
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
    result.ref.fields |=
        raw.mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
    result.ref.min_zoom = value.minZoom!;
  }
  if (value.maxZoom != null) {
    result.ref.fields |=
        raw.mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
    result.ref.max_zoom = value.maxZoom!;
  }
  if (value.tolerance != null) {
    result.ref.fields |=
        raw.mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
    result.ref.tolerance = value.tolerance!;
  }
  if (value.clusterMaxZoom != null) {
    result.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
    result.ref.cluster_max_zoom = value.clusterMaxZoom!;
  }
  if (value.clusterProperties != null) {
    result.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
    result.ref.cluster_properties = nativeBufferView(
      value.clusterProperties!,
      arena,
    );
  }
  if (value.tileSize != null) {
    result.ref.fields |=
        raw.mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE;
    result.ref.tile_size = _generatedInteger(value.tileSize!, 0, 4294967295);
  }
  if (value.buffer != null) {
    result.ref.fields |=
        raw.mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_BUFFER;
    result.ref.buffer = _generatedInteger(value.buffer!, 0, 4294967295);
  }
  if (value.clusterRadius != null) {
    result.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
    result.ref.cluster_radius = _generatedInteger(
      value.clusterRadius!,
      0,
      4294967295,
    );
  }
  if (value.clusterMinPoints != null) {
    result.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
    result.ref.cluster_min_points = _generatedInteger(
      value.clusterMinPoints!,
      0,
      4294967295,
    );
  }
  if (value.lineMetrics != null) {
    result.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
    result.ref.line_metrics = value.lineMetrics!;
  }
  if (value.cluster != null) {
    result.ref.fields |=
        raw.mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
    result.ref.cluster = value.cluster!;
  }
  if (value.synchronousTiling != null) {
    result.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
    result.ref.synchronous_tiling = value.synchronousTiling!;
  }
  return result;
}

GeojsonSourceOptions _readGeojsonSourceOptions(
  raw.mln_geojson_source_options source,
) => GeojsonSourceOptions(
  minZoom:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM) !=
          0
      ? source.min_zoom
      : null,
  maxZoom:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM) !=
          0
      ? source.max_zoom
      : null,
  tolerance:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_TOLERANCE) !=
          0
      ? source.tolerance
      : null,
  clusterMaxZoom:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM) !=
          0
      ? source.cluster_max_zoom
      : null,
  clusterProperties:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES) !=
          0
      ? _copyBufferView(source.cluster_properties)
      : null,
  tileSize:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE) !=
          0
      ? source.tile_size
      : null,
  buffer:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_BUFFER) !=
          0
      ? source.buffer
      : null,
  clusterRadius:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS) !=
          0
      ? source.cluster_radius
      : null,
  clusterMinPoints:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS) !=
          0
      ? source.cluster_min_points
      : null,
  lineMetrics:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS) !=
          0
      ? source.line_metrics
      : null,
  cluster:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_CLUSTER) !=
          0
      ? source.cluster
      : null,
  synchronousTiling:
      (source.fields &
              raw
                  .mln_geojson_source_option_field
                  .MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING) !=
          0
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

Pointer<raw.mln_premultiplied_rgba8_image> _writePremultipliedRgba8Image(
  PremultipliedRgba8Image value,
  Arena arena,
) {
  final result = arena<raw.mln_premultiplied_rgba8_image>();
  result.ref = raw.mln_premultiplied_rgba8_image_default();
  result.ref.width = _generatedInteger(value.width, 0, 4294967295);
  result.ref.height = _generatedInteger(value.height, 0, 4294967295);
  result.ref.stride = _generatedInteger(value.stride, 0, 4294967295);
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
    result.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
    result.ref.min_zoom = value.minZoom!;
  }
  if (value.maxZoom != null) {
    result.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
    result.ref.max_zoom = value.maxZoom!;
  }
  if (value.attribution != null) {
    result.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
    result.ref.attribution = nativeStringView(value.attribution!, arena).value;
  }
  if (value.scheme != null) {
    result.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
    result.ref.scheme = value.scheme!.rawValue;
  }
  if (value.bounds != null) {
    result.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
    result.ref.bounds = _writeLatLngBounds(value.bounds!, arena).ref;
  }
  if (value.tileSize != null) {
    result.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
    result.ref.tile_size = _generatedInteger(value.tileSize!, 0, 4294967295);
  }
  if (value.vectorEncoding != null) {
    result.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
    result.ref.vector_encoding = value.vectorEncoding!.rawValue;
  }
  if (value.rasterEncoding != null) {
    result.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
    result.ref.raster_encoding = value.rasterEncoding!.rawValue;
  }
  return result;
}

StyleTileSourceOptions _readStyleTileSourceOptions(
  raw.mln_style_tile_source_options source,
) => StyleTileSourceOptions(
  minZoom:
      (source.fields &
              raw
                  .mln_style_tile_source_option_field
                  .MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM) !=
          0
      ? source.min_zoom
      : null,
  maxZoom:
      (source.fields &
              raw
                  .mln_style_tile_source_option_field
                  .MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM) !=
          0
      ? source.max_zoom
      : null,
  attribution:
      (source.fields &
              raw
                  .mln_style_tile_source_option_field
                  .MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION) !=
          0
      ? utf8.decode(_copyBufferView(source.attribution))
      : null,
  scheme:
      (source.fields &
              raw
                  .mln_style_tile_source_option_field
                  .MLN_STYLE_TILE_SOURCE_OPTION_SCHEME) !=
          0
      ? StyleTileScheme.fromRawValue(source.scheme)
      : null,
  bounds:
      (source.fields &
              raw
                  .mln_style_tile_source_option_field
                  .MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS) !=
          0
      ? _readLatLngBounds(source.bounds)
      : null,
  tileSize:
      (source.fields &
              raw
                  .mln_style_tile_source_option_field
                  .MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE) !=
          0
      ? source.tile_size
      : null,
  vectorEncoding:
      (source.fields &
              raw
                  .mln_style_tile_source_option_field
                  .MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING) !=
          0
      ? StyleVectorTileEncoding.fromRawValue(source.vector_encoding)
      : null,
  rasterEncoding:
      (source.fields &
              raw
                  .mln_style_tile_source_option_field
                  .MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING) !=
          0
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
  result.ref.width = _generatedInteger(value.width, 0, 4294967295);
  result.ref.height = _generatedInteger(value.height, 0, 4294967295);
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
    result.ref.fields |= raw
        .mln_feature_state_selector_field
        .MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
    result.ref.source_layer_id = nativeStringView(
      value.sourceLayerId!,
      arena,
    ).value;
  }
  if (value.featureId != null) {
    result.ref.fields |= raw
        .mln_feature_state_selector_field
        .MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
    result.ref.feature_id = nativeStringView(value.featureId!, arena).value;
  }
  if (value.stateKey != null) {
    result.ref.fields |= raw
        .mln_feature_state_selector_field
        .MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
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

StyleSourceInfo _readStyleSourceInfo(
  raw.mln_style_source_info source,
) => StyleSourceInfo(
  type: StyleSourceType.fromRawValue(source.type),
  idSize: source.id_size,
  isVolatile: source.is_volatile,
  attributionSize: source.has_attribution ? source.attribution_size : null,
  urlSize:
      (source.fields &
              raw.mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_URL) !=
          0
      ? source.url_size
      : null,
  tilejson:
      (source.fields &
              raw.mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_TILEJSON) !=
          0
      ? StyleSourceTileInfo(
          tileCount: source.tile_count,
          minZoom: source.min_zoom,
          maxZoom: source.max_zoom,
          scheme: StyleTileScheme.fromRawValue(source.scheme),
        )
      : null,
  bounds:
      (source.fields &
              raw.mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_BOUNDS) !=
          0
      ? _readLatLngBounds(source.bounds)
      : null,
  tileSize:
      (source.fields &
              raw
                  .mln_style_source_info_field
                  .MLN_STYLE_SOURCE_INFO_TILE_SIZE) !=
          0
      ? source.tile_size
      : null,
  vectorEncoding:
      (source.fields &
              raw
                  .mln_style_source_info_field
                  .MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING) !=
          0
      ? StyleVectorTileEncoding.fromRawValue(source.vector_encoding)
      : null,
  rasterEncoding:
      (source.fields &
              raw
                  .mln_style_source_info_field
                  .MLN_STYLE_SOURCE_INFO_RASTER_ENCODING) !=
          0
      ? StyleRasterDemEncoding.fromRawValue(source.raster_encoding)
      : null,
);

StyleSourceResult _readStyleSourceResult(
  raw.mln_style_source_result source,
) => StyleSourceResult(
  info: _readStyleSourceInfo(source.info),
  attribution: source.info.has_attribution
      ? utf8.decode(_copyBufferView(source.attribution))
      : null,
  url:
      (source.info.fields &
              raw.mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_URL) !=
          0
      ? utf8.decode(_copyBufferView(source.url))
      : null,
  tileUrls:
      (source.info.fields &
              raw.mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_TILEJSON) !=
          0
      ? List<String>.unmodifiable(
          List.generate(
            source.tile_url_count,
            (index) => utf8.decode(_copyBufferView(source.tile_urls[index])),
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
    result.ref.fields |= raw
        .mln_style_transition_option_field
        .MLN_STYLE_TRANSITION_OPTION_DURATION;
    result.ref.duration_ms = value.durationMs!;
  }
  if (value.delayMs != null) {
    result.ref.fields |=
        raw.mln_style_transition_option_field.MLN_STYLE_TRANSITION_OPTION_DELAY;
    result.ref.delay_ms = value.delayMs!;
  }
  if (value.enablePlacementTransitions != null) {
    result.ref.fields |= raw
        .mln_style_transition_option_field
        .MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
    result.ref.enable_placement_transitions = value.enablePlacementTransitions!;
  }
  return result;
}

StyleTransitionOptions _readStyleTransitionOptions(
  raw.mln_style_transition_options source,
) => StyleTransitionOptions(
  durationMs:
      (source.fields &
              raw
                  .mln_style_transition_option_field
                  .MLN_STYLE_TRANSITION_OPTION_DURATION) !=
          0
      ? source.duration_ms
      : null,
  delayMs:
      (source.fields &
              raw
                  .mln_style_transition_option_field
                  .MLN_STYLE_TRANSITION_OPTION_DELAY) !=
          0
      ? source.delay_ms
      : null,
  enablePlacementTransitions:
      (source.fields &
              raw
                  .mln_style_transition_option_field
                  .MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) !=
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
    result.ref.fields |=
        raw.mln_projection_mode_field.MLN_PROJECTION_MODE_AXONOMETRIC;
    result.ref.axonometric = value.axonometric!;
  }
  if (value.xSkew != null) {
    result.ref.fields |=
        raw.mln_projection_mode_field.MLN_PROJECTION_MODE_X_SKEW;
    result.ref.x_skew = value.xSkew!;
  }
  if (value.ySkew != null) {
    result.ref.fields |=
        raw.mln_projection_mode_field.MLN_PROJECTION_MODE_Y_SKEW;
    result.ref.y_skew = value.ySkew!;
  }
  return result;
}

ProjectionMode _readProjectionMode(
  raw.mln_projection_mode source,
) => ProjectionMode(
  axonometric:
      (source.fields &
              raw.mln_projection_mode_field.MLN_PROJECTION_MODE_AXONOMETRIC) !=
          0
      ? source.axonometric
      : null,
  xSkew:
      (source.fields &
              raw.mln_projection_mode_field.MLN_PROJECTION_MODE_X_SKEW) !=
          0
      ? source.x_skew
      : null,
  ySkew:
      (source.fields &
              raw.mln_projection_mode_field.MLN_PROJECTION_MODE_Y_SKEW) !=
          0
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
    result.ref.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_STRETCH_X;
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
    result.ref.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
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
    result.ref.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_CONTENT;
    result.ref.content = _writeImageContent(value.content!, arena).ref;
  }
  if (value.textFitWidth != null) {
    result.ref.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
    result.ref.text_fit_width = value.textFitWidth!.rawValue;
  }
  if (value.textFitHeight != null) {
    result.ref.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
    result.ref.text_fit_height = value.textFitHeight!.rawValue;
  }
  if (value.pixelRatio != null) {
    result.ref.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
    result.ref.pixel_ratio = value.pixelRatio!;
  }
  if (value.sdf != null) {
    result.ref.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_SDF;
    result.ref.sdf = value.sdf!;
  }
  return result;
}

StyleImageOptions _readStyleImageOptions(
  raw.mln_style_image_options source,
) => StyleImageOptions(
  stretchX:
      (source.fields &
              raw
                  .mln_style_image_option_field
                  .MLN_STYLE_IMAGE_OPTION_STRETCH_X) !=
          0
      ? List<ImageStretch>.unmodifiable(
          List.generate(
            source.stretch_x_count,
            (index) => _readImageStretch(source.stretch_x[index]),
          ),
        )
      : null,
  stretchY:
      (source.fields &
              raw
                  .mln_style_image_option_field
                  .MLN_STYLE_IMAGE_OPTION_STRETCH_Y) !=
          0
      ? List<ImageStretch>.unmodifiable(
          List.generate(
            source.stretch_y_count,
            (index) => _readImageStretch(source.stretch_y[index]),
          ),
        )
      : null,
  content:
      (source.fields &
              raw
                  .mln_style_image_option_field
                  .MLN_STYLE_IMAGE_OPTION_CONTENT) !=
          0
      ? _readImageContent(source.content)
      : null,
  textFitWidth:
      (source.fields &
              raw
                  .mln_style_image_option_field
                  .MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH) !=
          0
      ? StyleImageTextFit.fromRawValue(source.text_fit_width)
      : null,
  textFitHeight:
      (source.fields &
              raw
                  .mln_style_image_option_field
                  .MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT) !=
          0
      ? StyleImageTextFit.fromRawValue(source.text_fit_height)
      : null,
  pixelRatio:
      (source.fields &
              raw
                  .mln_style_image_option_field
                  .MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) !=
          0
      ? source.pixel_ratio
      : null,
  sdf:
      (source.fields &
              raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_SDF) !=
          0
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
    result.ref.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
    result.ref.prefetch_zoom_delta = _generatedInteger(
      value.prefetchZoomDelta!,
      0,
      4294967295,
    );
  }
  if (value.lodMinRadius != null) {
    result.ref.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
    result.ref.lod_min_radius = value.lodMinRadius!;
  }
  if (value.lodScale != null) {
    result.ref.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_SCALE;
    result.ref.lod_scale = value.lodScale!;
  }
  if (value.lodPitchThreshold != null) {
    result.ref.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
    result.ref.lod_pitch_threshold = value.lodPitchThreshold!;
  }
  if (value.lodZoomShift != null) {
    result.ref.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
    result.ref.lod_zoom_shift = value.lodZoomShift!;
  }
  if (value.lodMode != null) {
    result.ref.fields |=
        raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_MODE;
    result.ref.lod_mode = value.lodMode!.rawValue;
  }
  return result;
}

MapTileOptions _readMapTileOptions(
  raw.mln_map_tile_options source,
) => MapTileOptions(
  prefetchZoomDelta:
      (source.fields &
              raw
                  .mln_map_tile_option_field
                  .MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) !=
          0
      ? source.prefetch_zoom_delta
      : null,
  lodMinRadius:
      (source.fields &
              raw
                  .mln_map_tile_option_field
                  .MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) !=
          0
      ? source.lod_min_radius
      : null,
  lodScale:
      (source.fields &
              raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_SCALE) !=
          0
      ? source.lod_scale
      : null,
  lodPitchThreshold:
      (source.fields &
              raw
                  .mln_map_tile_option_field
                  .MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) !=
          0
      ? source.lod_pitch_threshold
      : null,
  lodZoomShift:
      (source.fields &
              raw
                  .mln_map_tile_option_field
                  .MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) !=
          0
      ? source.lod_zoom_shift
      : null,
  lodMode:
      (source.fields &
              raw.mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_MODE) !=
          0
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
    result.ref.fields |= raw
        .mln_map_viewport_option_field
        .MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
    result.ref.north_orientation = value.northOrientation!.rawValue;
  }
  if (value.constrainMode != null) {
    result.ref.fields |= raw
        .mln_map_viewport_option_field
        .MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
    result.ref.constrain_mode = value.constrainMode!.rawValue;
  }
  if (value.viewportMode != null) {
    result.ref.fields |=
        raw.mln_map_viewport_option_field.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
    result.ref.viewport_mode = value.viewportMode!.rawValue;
  }
  if (value.frustumOffset != null) {
    result.ref.fields |= raw
        .mln_map_viewport_option_field
        .MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
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
      (source.fields &
              raw
                  .mln_map_viewport_option_field
                  .MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) !=
          0
      ? NorthOrientation.fromRawValue(source.north_orientation)
      : null,
  constrainMode:
      (source.fields &
              raw
                  .mln_map_viewport_option_field
                  .MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) !=
          0
      ? ConstrainMode.fromRawValue(source.constrain_mode)
      : null,
  viewportMode:
      (source.fields &
              raw
                  .mln_map_viewport_option_field
                  .MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) !=
          0
      ? ViewportMode.fromRawValue(source.viewport_mode)
      : null,
  frustumOffset:
      (source.fields &
              raw
                  .mln_map_viewport_option_field
                  .MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) !=
          0
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
  result.ref.width = _generatedInteger(value.width, 0, 4294967295);
  result.ref.height = _generatedInteger(value.height, 0, 4294967295);
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
  result.ref.physical_width = _generatedInteger(
    value.physicalWidth,
    0,
    4294967295,
  );
  result.ref.physical_height = _generatedInteger(
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
        (raw
                .mln_adapter_dart_port_callback
                .MLN_ADAPTER_DART_PORT_WAKE_CALLBACK &
            0xffffffff): (message) =>
            value.callback!(),
    });
    result.ref.callback = value.callback == null
        ? nullptr
        : raw
              .mln_adapter_dart_port_function(
                (raw
                        .mln_adapter_dart_port_callback
                        .MLN_ADAPTER_DART_PORT_WAKE_CALLBACK &
                    0xffffffff),
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

Wake _readWake(raw.mln_wake source) {
  if (source.callback != nullptr) {
    throwInvalidState('cannot copy a registered native callback');
  }
  return Wake(callback: null);
}

Pointer<raw.mln_render_session_attach_options> _writeRenderSessionAttachOptions(
  RenderSessionAttachOptions value,
  Arena arena,
  _NativeRegistrations registrations,
) {
  final result = arena<raw.mln_render_session_attach_options>();
  result.ref = raw.mln_render_session_attach_options_default();
  result.ref.driver = value.driver.rawValue;
  result.ref.requested_texture_ring_depth = _generatedInteger(
    value.requestedTextureRingDepth,
    0,
    4294967295,
  );
  result.ref.frame_wake = registrations.prepareWake(value.frameWake).ref;
  result.ref.driver_work_wake = registrations
      .prepareWake(value.driverWorkWake)
      .ref;
  return result;
}

RenderSessionAttachOptions _readRenderSessionAttachOptions(
  raw.mln_render_session_attach_options source,
) => RenderSessionAttachOptions(
  driver: RenderDriverKind.fromRawValue(source.driver),
  requestedTextureRingDepth: source.requested_texture_ring_depth,
  frameWake: _readWake(source.frame_wake),
  driverWorkWake: _readWake(source.driver_work_wake),
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
  result.ref.context = _generatedInteger(
    value.context,
    -2147483648,
    2147483647,
  );
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
  result.ref.physical_width = _generatedInteger(
    value.physicalWidth,
    0,
    4294967295,
  );
  result.ref.physical_height = _generatedInteger(
    value.physicalHeight,
    0,
    4294967295,
  );
  result.ref.context = _writeOpenglContextDescriptor(value.context, arena).ref;
  result.ref.texture = _generatedInteger(value.texture, 0, 4294967295);
  result.ref.target = _generatedInteger(value.target, 0, 4294967295);
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
    result.ref.fields |= raw
        .mln_rendered_feature_query_option_field
        .MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
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
      (source.fields &
              raw
                  .mln_rendered_feature_query_option_field
                  .MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) !=
          0
      ? List<String>.unmodifiable(
          List.generate(
            source.layer_id_count,
            (index) => utf8.decode(_copyBufferView(source.layer_ids[index])),
          ),
        )
      : null,
  filter: source.filter == nullptr ? null : _copyBufferView(source.filter.ref),
);

QueriedFeature _readQueriedFeature(
  raw.mln_queried_feature source,
) => QueriedFeature(
  feature: _copyBufferView(source.feature),
  sourceId:
      (source.fields &
              raw.mln_queried_feature_field.MLN_QUERIED_FEATURE_SOURCE_ID) !=
          0
      ? utf8.decode(_copyBufferView(source.source_id))
      : null,
  sourceLayerId:
      (source.fields &
              raw
                  .mln_queried_feature_field
                  .MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) !=
          0
      ? utf8.decode(_copyBufferView(source.source_layer_id))
      : null,
  state:
      (source.fields &
              raw.mln_queried_feature_field.MLN_QUERIED_FEATURE_STATE) !=
          0
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
    result.ref.fields |= raw
        .mln_source_feature_query_option_field
        .MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
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
      (source.fields &
              raw
                  .mln_source_feature_query_option_field
                  .MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) !=
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
  result.ref.flags = _generatedInteger(value.flags, 0, 4294967295);
  result.ref.asset_path = value.assetPath == null
      ? nullptr
      : nativeUtf8CString(value.assetPath!, arena).pointer.cast<Char>();
  result.ref.cache_path = value.cachePath == null
      ? nullptr
      : nativeUtf8CString(value.cachePath!, arena).pointer.cast<Char>();
  result.ref.event_mask = value.eventMask.rawValue;
  result.ref.event_wake = registrations.prepareWake(value.eventWake).ref;
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
      eventWake: _readWake(source.event_wake),
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
  result.ref.kind = _generatedInteger(value.kind, 0, 4294967295);
  result.ref.flags = _generatedInteger(value.flags, 0, 4294967295);
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

Pointer<raw.mln_adapter_queued_resource_provider_route>
_writeAdapterQueuedResourceProviderRoute(
  AdapterQueuedResourceProviderRoute value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_queued_resource_provider_route>();
  result.ref.kind = _generatedInteger(value.kind, 0, 4294967295);
  result.ref.flags = _generatedInteger(value.flags, 0, 4294967295);
  result.ref.url = value.url == null
      ? nullptr
      : nativeUtf8CString(value.url!, arena).pointer.cast<Char>();
  return result;
}

Pointer<raw.mln_adapter_resource_provider_rule>
_writeAdapterResourceProviderRule(
  AdapterResourceProviderRule value,
  Arena arena,
) {
  final result = arena<raw.mln_adapter_resource_provider_rule>();
  result.ref.kind = _generatedInteger(value.kind, 0, 4294967295);
  result.ref.flags = _generatedInteger(value.flags, 0, 4294967295);
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

_NativeRegistration<raw.mln_resource_provider> _prepareResourceProvider(
  ResourceProvider value,
  NativeCallbackReleases roots,
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
  result.ref.kind = _generatedInteger(value.kind, 0, 4294967295);
  result.ref.flags = _generatedInteger(value.flags, 0, 4294967295);
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
  result.ref.graphics_queue_family_index = _generatedInteger(
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
  result.ref.physical_width = _generatedInteger(
    value.physicalWidth,
    0,
    4294967295,
  );
  result.ref.physical_height = _generatedInteger(
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
  result.ref.format = _generatedInteger(value.format, 0, 4294967295);
  result.ref.initial_layout = _generatedInteger(
    value.initialLayout,
    0,
    4294967295,
  );
  result.ref.final_layout = _generatedInteger(value.finalLayout, 0, 4294967295);
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
  result.ref.physical_width = _generatedInteger(
    value.physicalWidth,
    0,
    4294967295,
  );
  result.ref.physical_height = _generatedInteger(
    value.physicalHeight,
    0,
    4294967295,
  );
  result.ref.context = _writeWebgpuContextDescriptor(value.context, arena).ref;
  result.ref.texture = Pointer<Void>.fromAddress(value.texture.address).cast();
  result.ref.texture_view = Pointer<Void>.fromAddress(
    value.textureView.address,
  ).cast();
  result.ref.format = _generatedInteger(value.format, 0, 4294967295);
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
  result.ref.format = _generatedInteger(value.format, 0, 4294967295);
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
String _generatedArenaUtf8(
  Pointer<Uint8> data,
  int size,
  int offset,
  int length,
) {
  if (offset < 0 || length < 0 || offset > size || length > size - offset) {
    throwInvalidState('native message slice exceeds its arena');
  }
  return length == 0 ? '' : utf8.decode((data + offset).asTypedList(length));
}

int _generatedInteger(int value, int minimum, int maximum) {
  if (value < minimum || value > maximum) {
    throwInvalidArgument('integer is outside its native range');
  }
  return value;
}

mixin _GeneratedAcquiredFrameOperations implements Finalizable {
  NativeAcquiredFrame get _handle;

  NativeHandleState<NativeAcquiredFrame> get _state;

  void dispose() => _state.close(
    (handle) => withNativeArena((arena) {
      return raw.mln_acquired_frame_dispose(handle.raw);
    }),
    threadLastErrorMessage,
  );

  ScopedMetalOwnedTextureFrame getMetalTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_metal_owned_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_metal_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_metal_texture(_handle.raw, outFrame));
    return ScopedMetalOwnedTextureFrame._(
      this as AcquiredFrame,
      _readMetalOwnedTextureFrame(outFrame.ref),
    );
  });

  ScopedOpenglOwnedTextureFrame getOpenglTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_opengl_owned_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_opengl_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_opengl_texture(_handle.raw, outFrame));
    return ScopedOpenglOwnedTextureFrame._(
      this as AcquiredFrame,
      _readOpenglOwnedTextureFrame(outFrame.ref),
    );
  });

  ScopedGpuSync getProducerSync() => withNativeArena((arena) {
    final outSync = arena<raw.mln_gpu_sync>();
    outSync.ref = raw.mln_gpu_sync_default();
    _check(raw.mln_acquired_frame_get_producer_sync(_handle.raw, outSync));
    return ScopedGpuSync._(this as AcquiredFrame, _readGpuSync(outSync.ref));
  });

  RenderFrameResult getResult() => withNativeArena((arena) {
    final outResult = arena<raw.mln_render_frame_result>();
    outResult.ref.size = sizeOf<raw.mln_render_frame_result>();
    _check(raw.mln_acquired_frame_get_result(_handle.raw, outResult));
    return _readRenderFrameResult(outResult.ref);
  });

  ScopedVulkanOwnedTextureFrame getVulkanTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_vulkan_owned_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_vulkan_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_vulkan_texture(_handle.raw, outFrame));
    return ScopedVulkanOwnedTextureFrame._(
      this as AcquiredFrame,
      _readVulkanOwnedTextureFrame(outFrame.ref),
    );
  });

  ScopedWebgpuOwnedTextureFrame getWebgpuTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_webgpu_owned_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_webgpu_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_webgpu_texture(_handle.raw, outFrame));
    return ScopedWebgpuOwnedTextureFrame._(
      this as AcquiredFrame,
      _readWebgpuOwnedTextureFrame(outFrame.ref),
    );
  });

  void release(GpuSync consumerCompletion) => _state.close(
    (handle) => withNativeArena((arena) {
      final receiverPointer = arena<Uint64>()..value = handle.raw;
      return raw.mln_acquired_frame_release(
        receiverPointer,
        _writeGpuSync(consumerCompletion, arena),
      );
    }),
    threadLastErrorMessage,
  );
}

mixin _GeneratedBufferOperations implements Finalizable {
  NativeBuffer get _handle;

  NativeHandleState<NativeBuffer> get _state;

  void close() => _state.close(
    (handle) => withNativeArena((arena) {
      raw.mln_buffer_destroy(handle.raw);
      return nativeStatusOk;
    }),
    threadLastErrorMessage,
  );

  Uint8List getValue() => withNativeArena((arena) {
    final outView = arena<raw.mln_buffer_view>();
    _check(raw.mln_buffer_get(_handle.raw, outView));
    return _copyBufferView(outView.ref);
  });
}

/// Buffer handle id.
extension type const NativeBuffer(int raw) implements NativeHandle {}

final class BufferHandle with _GeneratedBufferOperations {
  BufferHandle._(NativeBuffer handle)
    : _state = NativeHandleState(handle, 'BufferHandle');
  @override
  final NativeHandleState<NativeBuffer> _state;
  @override
  NativeBuffer get _handle => _state.handle;
  bool get isClosed => _state.isClosed;
}

mixin _GeneratedEventBatchOperations implements Finalizable {
  NativeEventBatch get _handle;

  NativeHandleState<NativeEventBatch> get _state;

  RuntimeEventBatchView getValue() => withNativeArena((arena) {
    final outView = arena<raw.mln_runtime_event_batch_view>();
    outView.ref.size = sizeOf<raw.mln_runtime_event_batch_view>();
    _check(raw.mln_event_batch_get(_handle.raw, outView));
    return _readRuntimeEventBatchView(outView.ref);
  });

  void close() => _state.close(
    (handle) => withNativeArena((arena) {
      raw.mln_event_batch_release(handle.raw);
      return nativeStatusOk;
    }),
    threadLastErrorMessage,
  );
}

/// EventBatch handle id.
extension type const NativeEventBatch(int raw) implements NativeHandle {}

final class EventBatchHandle with _GeneratedEventBatchOperations {
  EventBatchHandle._(NativeEventBatch handle)
    : _state = NativeHandleState(handle, 'EventBatchHandle');
  @override
  final NativeHandleState<NativeEventBatch> _state;
  @override
  NativeEventBatch get _handle => _state.handle;
  bool get isClosed => _state.isClosed;
}

mixin _GeneratedGeoJsonSourceDataOperations implements Finalizable {
  NativeHandleState<NativeGeoJsonSourceData> get _state;

  void close() => _state.close(
    (handle) => withNativeArena((arena) {
      raw.mln_geojson_source_data_destroy(handle.raw);
      return nativeStatusOk;
    }),
    threadLastErrorMessage,
  );
}

void androidInit(
  NativePointer jniEnv,
  NativePointer jniClass,
  NativePointer context,
) => withNativeArena((arena) {
  _check(
    raw.mln_android_init(
      Pointer<Void>.fromAddress(jniEnv.address).cast(),
      Pointer<Void>.fromAddress(jniClass.address).cast(),
      Pointer<Void>.fromAddress(context.address).cast(),
    ),
  );
});

AnimationOptions animationOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_animation_options_default();
  return _readAnimationOptions(nativeResult);
});

BoundOptions boundOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_bound_options_default();
  return _readBoundOptions(nativeResult);
});

int cVersion() => withNativeArena((arena) {
  final nativeResult = raw.mln_c_version();
  return nativeResult;
});

CameraDelta cameraDeltaDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_camera_delta_default();
  return _readCameraDelta(nativeResult);
});

CameraFitOptions cameraFitOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_camera_fit_options_default();
  return _readCameraFitOptions(nativeResult);
});

CameraOptions cameraOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_camera_options_default();
  return _readCameraOptions(nativeResult);
});

CameraUpdate cameraUpdateDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_camera_update_default();
  return _readCameraUpdate(nativeResult);
});

CustomGeometrySourceOptions customGeometrySourceOptionsDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_custom_geometry_source_options_default();
      return _readCustomGeometrySourceOptions(nativeResult);
    });

CustomMvtVectorSourceOptions customMvtVectorSourceOptionsDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_custom_mvt_vector_source_options_default();
      return _readCustomMvtVectorSourceOptions(nativeResult);
    });

FrameDemand frameDemandDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_frame_demand_default();
  return _readFrameDemand(nativeResult);
});

FreeCameraOptions freeCameraOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_free_camera_options_default();
  return _readFreeCameraOptions(nativeResult);
});

GeoJsonSourceDataHandle geojsonSourceDataCreate(
  Uint8List data, {
  GeojsonSourceOptions? options,
}) => withNativeArena((arena) {
  final outData = arena<Uint64>();
  _check(
    raw.mln_geojson_source_data_create(
      nativeBufferView(data, arena),
      options == null ? nullptr : _writeGeojsonSourceOptions(options, arena),
      outData,
    ),
  );
  return _adoptOwned(
    outData.value,
    () => GeoJsonSourceDataHandle._(NativeGeoJsonSourceData(outData.value)),
    (handle) {
      raw.mln_geojson_source_data_destroy(handle);
    },
  );
});

GeojsonSourceOptions geojsonSourceOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_geojson_source_options_default();
  return _readGeojsonSourceOptions(nativeResult);
});

GpuSync gpuSyncDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_gpu_sync_default();
  return _readGpuSync(nativeResult);
});

LatLng latLngForProjectedMeters(ProjectedMeters meters) =>
    withNativeArena((arena) {
      final outCoordinate = arena<raw.mln_lat_lng>();
      _check(
        raw.mln_lat_lng_for_projected_meters(
          _writeProjectedMeters(meters, arena).ref,
          outCoordinate,
        ),
      );
      return _readLatLng(outCoordinate.ref);
    });

void logClearCallback() => withNativeArena((arena) {
  _check(raw.mln_log_clear_callback());
});

void logSetAsyncSeverityMask(LogSeverityMask mask) => withNativeArena((arena) {
  _check(raw.mln_log_set_async_severity_mask(mask.rawValue));
});

void logSetCallback(LogCallback callback, {bool consume = false}) {
  final state = _LogCallbackState(callback, consume: consume);
  _callbackReleases.register(
    state.pointer.cast(),
    state.close,
    arena: state.arena,
  );
  try {
    _check(
      raw.mln_log_set_callback(
        Native.addressOf<NativeFunction<raw.mln_log_callbackFunction>>(
          raw.mln_adapter_log_callback,
        ),
        state.pointer.cast(),
        Native.addressOf<NativeFunction<raw.mln_log_callback_releaseFunction>>(
          raw.mln_adapter_dart_release,
        ),
      ),
    );
    _logCallbackState = state;
  } catch (_) {
    _callbackReleases.reject(state.pointer.cast());
    rethrow;
  }
}

MapOptions mapOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_map_options_default();
  return _readMapOptions(nativeResult);
});

MapTileOptions mapTileOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_map_tile_options_default();
  return _readMapTileOptions(nativeResult);
});

MapViewportOptions mapViewportOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_map_viewport_options_default();
  return _readMapViewportOptions(nativeResult);
});

MetalBorrowedTextureDescriptor metalBorrowedTextureDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_metal_borrowed_texture_descriptor_default();
      return _readMetalBorrowedTextureDescriptor(nativeResult);
    });

MetalOwnedTextureDescriptor metalOwnedTextureDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_metal_owned_texture_descriptor_default();
      return _readMetalOwnedTextureDescriptor(nativeResult);
    });

MetalSurfaceDescriptor metalSurfaceDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_metal_surface_descriptor_default();
      return _readMetalSurfaceDescriptor(nativeResult);
    });

NetworkStatus networkStatusGet() => withNativeArena((arena) {
  final outStatus = arena<Uint32>();
  _check(raw.mln_network_status_get(outStatus));
  return NetworkStatus.fromRawValue(outStatus.value);
});

void networkStatusSet(NetworkStatus status) => withNativeArena((arena) {
  _check(raw.mln_network_status_set(status.rawValue));
});

OpenglBorrowedTextureDescriptor openglBorrowedTextureDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_opengl_borrowed_texture_descriptor_default();
      return _readOpenglBorrowedTextureDescriptor(nativeResult);
    });

OpenglOwnedTextureDescriptor openglOwnedTextureDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_opengl_owned_texture_descriptor_default();
      return _readOpenglOwnedTextureDescriptor(nativeResult);
    });

OpenglContextProviderFlag openglSupportedContextProviderMask() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_opengl_supported_context_provider_mask();
      return OpenglContextProviderFlag.fromRawValue(nativeResult);
    });

OpenglSurfaceDescriptor openglSurfaceDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_opengl_surface_descriptor_default();
      return _readOpenglSurfaceDescriptor(nativeResult);
    });

NativePointer pluginGetRegisterFunctionV1() => withNativeArena((arena) {
  final nativeResult = raw.mln_plugin_get_register_function_v1();
  return NativePointer(nativeResult.address);
});

PremultipliedRgba8Image premultipliedRgba8ImageDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_premultiplied_rgba8_image_default();
      return _readPremultipliedRgba8Image(nativeResult);
    });

ProjectedMeters projectedMetersForLatLng(LatLng coordinate) =>
    withNativeArena((arena) {
      final outMeters = arena<raw.mln_projected_meters>();
      _check(
        raw.mln_projected_meters_for_lat_lng(
          _writeLatLng(coordinate, arena).ref,
          outMeters,
        ),
      );
      return _readProjectedMeters(outMeters.ref);
    });

ProjectionMode projectionModeDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_projection_mode_default();
  return _readProjectionMode(nativeResult);
});

RenderSessionAttachOptions renderSessionAttachOptionsDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_render_session_attach_options_default();
      return _readRenderSessionAttachOptions(nativeResult);
    });

(int, int) renderTargetExtentPhysicalSize(RenderTargetExtent extent) =>
    withNativeArena((arena) {
      final outWidth = arena<Uint32>();
      final outHeight = arena<Uint32>();
      _check(
        raw.mln_render_target_extent_physical_size(
          _writeRenderTargetExtent(extent, arena),
          outWidth,
          outHeight,
        ),
      );
      return (outWidth.value, outHeight.value);
    });

RenderedFeatureQueryOptions renderedFeatureQueryOptionsDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_rendered_feature_query_options_default();
      return _readRenderedFeatureQueryOptions(nativeResult);
    });

RenderedQueryGeometry renderedQueryGeometryBox(ScreenBox box) =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_rendered_query_geometry_box(
        _writeScreenBox(box, arena).ref,
      );
      return _readRenderedQueryGeometry(nativeResult);
    });

RenderedQueryGeometry renderedQueryGeometryLineString(
  List<ScreenPoint> points,
) => withNativeArena((arena) {
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

RenderedQueryGeometry renderedQueryGeometryPoint(ScreenPoint point) =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_rendered_query_geometry_point(
        _writeScreenPoint(point, arena).ref,
      );
      return _readRenderedQueryGeometry(nativeResult);
    });

RuntimeHandle runtimeCreate(RuntimeOptions options) => withNativeArena((arena) {
  final registrations = _NativeRegistrations(_NativeCallbackPorts());
  try {
    final outRuntime = arena<Uint64>();
    _check(
      raw.mln_runtime_create(
        _writeRuntimeOptions(options, arena, registrations),
        outRuntime,
      ),
    );
    registrations.accept();
    return _adoptOwned(
      outRuntime.value,
      () =>
          (RuntimeHandle._(NativeRuntime(outRuntime.value))
            .._state.retain(registrations)),
      (handle) {
        _check(raw.mln_runtime_dispose(handle));
      },
    );
  } finally {
    registrations.close();
  }
});

RuntimeOptions runtimeOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_runtime_options_default();
  return _readRuntimeOptions(nativeResult);
});

SourceFeatureQueryOptions sourceFeatureQueryOptionsDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_source_feature_query_options_default();
      return _readSourceFeatureQueryOptions(nativeResult);
    });

StyleImageInfo styleImageInfoDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_style_image_info_default();
  return _readStyleImageInfo(nativeResult);
});

StyleImageOptions styleImageOptionsDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_style_image_options_default();
  return _readStyleImageOptions(nativeResult);
});

StyleTileSourceOptions styleTileSourceOptionsDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_style_tile_source_options_default();
      return _readStyleTileSourceOptions(nativeResult);
    });

StyleTransitionOptions styleTransitionOptionsDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_style_transition_options_default();
      return _readStyleTransitionOptions(nativeResult);
    });

RenderBackendFlag supportedRenderBackendMask() => withNativeArena((arena) {
  final nativeResult = raw.mln_supported_render_backend_mask();
  return RenderBackendFlag.fromRawValue(nativeResult);
});

TextureImageInfo textureImageInfoDefault() => withNativeArena((arena) {
  final nativeResult = raw.mln_texture_image_info_default();
  return _readTextureImageInfo(nativeResult);
});

String threadLastErrorMessage() => withNativeArena((arena) {
  final nativeResult = raw.mln_thread_last_error_message();
  return nativeResult.cast<Utf8>().toDartString();
});

VulkanBorrowedTextureDescriptor vulkanBorrowedTextureDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_vulkan_borrowed_texture_descriptor_default();
      return _readVulkanBorrowedTextureDescriptor(nativeResult);
    });

VulkanOwnedTextureDescriptor vulkanOwnedTextureDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_vulkan_owned_texture_descriptor_default();
      return _readVulkanOwnedTextureDescriptor(nativeResult);
    });

VulkanSurfaceDescriptor vulkanSurfaceDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_vulkan_surface_descriptor_default();
      return _readVulkanSurfaceDescriptor(nativeResult);
    });

WebgpuBorrowedTextureDescriptor webgpuBorrowedTextureDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_webgpu_borrowed_texture_descriptor_default();
      return _readWebgpuBorrowedTextureDescriptor(nativeResult);
    });

WebgpuOwnedTextureDescriptor webgpuOwnedTextureDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_webgpu_owned_texture_descriptor_default();
      return _readWebgpuOwnedTextureDescriptor(nativeResult);
    });

WebgpuSurfaceDescriptor webgpuSurfaceDescriptorDefault() =>
    withNativeArena((arena) {
      final nativeResult = raw.mln_webgpu_surface_descriptor_default();
      return _readWebgpuSurfaceDescriptor(nativeResult);
    });

mixin _GeneratedMapOperations implements Finalizable {
  NativeMap get _handle;

  NativeHandleState<NativeMap> get _state;

  _NativeCallbackPorts get _callbackPorts;

  Future<CommandCompletion> addColorReliefLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_color_relief_layer(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        nativeStringView(sourceId, arena).value,
        nativeStringView((beforeLayerId ?? ''), arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> addCustomGeometrySource(
    String sourceId,
    CustomGeometrySourceOptions options,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      final registrations = _NativeRegistrations(_callbackPorts);
      try {
        final status = raw.mln_map_add_custom_geometry_source(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          registrations.prepareCustomGeometrySourceOptions(options),
          completion,
        );
        if (status == nativeStatusOk) {
          registrations.accept();
        }
        return status;
      } finally {
        registrations.close();
      }
    }),
  );

  Future<CommandCompletion> addCustomMvtVectorSource(
    String sourceId,
    CustomMvtVectorSourceOptions options,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      final registrations = _NativeRegistrations(_callbackPorts);
      try {
        final status = raw.mln_map_add_custom_mvt_vector_source(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          registrations.prepareCustomMvtVectorSourceOptions(options),
          completion,
        );
        if (status == nativeStatusOk) {
          registrations.accept();
        }
        return status;
      } finally {
        registrations.close();
      }
    }),
  );

  Future<CommandCompletion> addGeojsonSourceData(
    String sourceId,
    GeoJsonSourceDataHandle data,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_geojson_source_data(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        data._handle.raw,
        completion,
      );
    }),
  );

  Future<CommandCompletion> addGeojsonSourceUrl(
    String sourceId,
    String url, {
    GeojsonSourceOptions? options,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_geojson_source_url(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        nativeStringView(url, arena).value,
        options == null ? nullptr : _writeGeojsonSourceOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> addHillshadeLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_hillshade_layer(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        nativeStringView(sourceId, arena).value,
        nativeStringView((beforeLayerId ?? ''), arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> addImageSourceImage(
    String sourceId,
    List<LatLng> coordinates,
    PremultipliedRgba8Image image,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
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
      );
    }),
  );

  Future<CommandCompletion> addImageSourceUrl(
    String sourceId,
    List<LatLng> coordinates,
    String url,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
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
      );
    }),
  );

  Future<CommandCompletion> addLocationIndicatorLayer(
    String layerId, {
    String? beforeLayerId,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_location_indicator_layer(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        nativeStringView((beforeLayerId ?? ''), arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> addRasterDemSourceTiles(
    String sourceId,
    List<String> tiles, {
    StyleTileSourceOptions? options,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
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
        options == null
            ? nullptr
            : _writeStyleTileSourceOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> addRasterDemSourceUrl(
    String sourceId,
    String url, {
    StyleTileSourceOptions? options,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_raster_dem_source_url(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        nativeStringView(url, arena).value,
        options == null
            ? nullptr
            : _writeStyleTileSourceOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> addRasterSourceTiles(
    String sourceId,
    List<String> tiles, {
    StyleTileSourceOptions? options,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
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
        options == null
            ? nullptr
            : _writeStyleTileSourceOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> addRasterSourceUrl(
    String sourceId,
    String url, {
    StyleTileSourceOptions? options,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_raster_source_url(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        nativeStringView(url, arena).value,
        options == null
            ? nullptr
            : _writeStyleTileSourceOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> addStyleLayerJson(
    Uint8List layerJson, {
    String? beforeLayerId,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_style_layer_json(
        _handle.raw,
        nativeBufferView(layerJson, arena),
        nativeStringView((beforeLayerId ?? ''), arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> addStyleSourceJson(
    String sourceId,
    Uint8List sourceJson,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_style_source_json(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        nativeBufferView(sourceJson, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> addVectorSourceTiles(
    String sourceId,
    List<String> tiles, {
    StyleTileSourceOptions? options,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
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
        options == null
            ? nullptr
            : _writeStyleTileSourceOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> addVectorSourceUrl(
    String sourceId,
    String url, {
    StyleTileSourceOptions? options,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_add_vector_source_url(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        nativeStringView(url, arena).value,
        options == null
            ? nullptr
            : _writeStyleTileSourceOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> applyCameraDelta(CameraDelta delta) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_apply_camera_delta(
            _handle.raw,
            _writeCameraDelta(delta, arena),
            completion,
          );
        }),
      );

  Future<CameraOptions> cameraForGeometry(
    Uint8List geometry, {
    CameraFitOptions? fitOptions,
  }) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_CAMERA_OPTIONS,
    elementSize: sizeOf<raw.mln_camera_options>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_camera_for_geometry(
        _handle.raw,
        nativeBufferView(geometry, arena),
        fitOptions == null
            ? nullptr
            : _writeCameraFitOptions(fitOptions, arena),
        completion,
      );
    }),
    decode: (result) =>
        _readCameraOptions(result.value.cast<raw.mln_camera_options>().ref),
    claimBeforeDecode: false,
  );

  Future<CameraOptions> cameraForLatLngBounds(
    LatLngBounds bounds, {
    CameraFitOptions? fitOptions,
  }) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_CAMERA_OPTIONS,
    elementSize: sizeOf<raw.mln_camera_options>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_camera_for_lat_lng_bounds(
        _handle.raw,
        _writeLatLngBounds(bounds, arena).ref,
        fitOptions == null
            ? nullptr
            : _writeCameraFitOptions(fitOptions, arena),
        completion,
      );
    }),
    decode: (result) =>
        _readCameraOptions(result.value.cast<raw.mln_camera_options>().ref),
    claimBeforeDecode: false,
  );

  Future<CameraOptions> cameraForLatLngs(
    List<LatLng> coordinates, {
    CameraFitOptions? fitOptions,
  }) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_CAMERA_OPTIONS,
    elementSize: sizeOf<raw.mln_camera_options>(),
    start: (completion) => withNativeArena((arena) {
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
        fitOptions == null
            ? nullptr
            : _writeCameraFitOptions(fitOptions, arena),
        completion,
      );
    }),
    decode: (result) =>
        _readCameraOptions(result.value.cast<raw.mln_camera_options>().ref),
    claimBeforeDecode: false,
  );

  Future<CameraQueryResult> cameraQuery() => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_CAMERA_QUERY_RESULT,
    elementSize: sizeOf<raw.mln_camera_query_result>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_camera_query(_handle.raw, completion);
    }),
    decode: (result) => _readCameraQueryResult(
      result.value.cast<raw.mln_camera_query_result>().ref,
    ),
    claimBeforeDecode: false,
  );

  (CameraOptions, BigInt) cameraSnapshotGet() => withNativeArena((arena) {
    final outCamera = arena<raw.mln_camera_options>();
    outCamera.ref = raw.mln_camera_options_default();
    final outGeneration = arena<Uint64>();
    _check(
      raw.mln_map_camera_snapshot_get(_handle.raw, outCamera, outGeneration),
    );
    return (
      _readCameraOptions(outCamera.ref),
      uint64FromNative(outGeneration.value),
    );
  });

  Future<CommandCompletion> cancelTransitions() => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_cancel_transitions(_handle.raw, completion);
    }),
  );

  Future<String?> copyLayerSourceId(String layerId) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_copy_layer_source_id(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        completion,
      );
    }),
    decode: (result) => result.value.cast<raw.mln_buffer_view>().ref.size == 0
        ? null
        : utf8.decode(
            _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
          ),
    claimBeforeDecode: false,
  );

  Future<String?> copyLayerSourceLayer(String layerId) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_copy_layer_source_layer(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        completion,
      );
    }),
    decode: (result) => result.value.cast<raw.mln_buffer_view>().ref.size == 0
        ? null
        : utf8.decode(
            _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
          ),
    claimBeforeDecode: false,
  );

  Future<Uint8List?> copyStyleImagePremultipliedRgba8(String imageId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
        elementSize: sizeOf<raw.mln_buffer_view>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_copy_style_image_premultiplied_rgba8(
            _handle.raw,
            nativeStringView(imageId, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : result.value.cast<raw.mln_buffer_view>().ref.data == nullptr
            ? null
            : _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
        claimBeforeDecode: false,
      );

  Future<StyleImageStretchesResult?> copyStyleImageStretches(String imageId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE_STRETCHES_RESULT,
        elementSize: sizeOf<raw.mln_style_image_stretches_result>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_copy_style_image_stretches(
            _handle.raw,
            nativeStringView(imageId, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : _readStyleImageStretchesResult(
                result.value.cast<raw.mln_style_image_stretches_result>().ref,
              ),
        claimBeforeDecode: false,
      );

  Future<String?> copyStyleSourceAttribution(String sourceId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
        elementSize: sizeOf<raw.mln_buffer_view>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_copy_style_source_attribution(
            _handle.raw,
            nativeStringView(sourceId, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : result.value.cast<raw.mln_buffer_view>().ref.data == nullptr
            ? null
            : utf8.decode(
                _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
              ),
        claimBeforeDecode: false,
      );

  Future<String?> copyStyleSourceUrl(String sourceId) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_copy_style_source_url(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        completion,
      );
    }),
    decode: (result) => result.value_count == 0
        ? null
        : result.value.cast<raw.mln_buffer_view>().ref.data == nullptr
        ? null
        : utf8.decode(
            _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
          ),
    claimBeforeDecode: false,
  );

  void dispose() => _state.close(
    (handle) => withNativeArena((arena) {
      return raw.mln_map_dispose(handle.raw);
    }),
    threadLastErrorMessage,
  );

  Future<CommandCompletion> dumpDebugLogs() => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_dump_debug_logs(_handle.raw, completion);
    }),
  );

  Future<Uint8List> getFeatureState(FeatureStateSelector selector) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
        elementSize: sizeOf<raw.mln_buffer_view>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_get_feature_state(
            _handle.raw,
            _writeFeatureStateSelector(selector, arena),
            completion,
          );
        }),
        decode: (result) =>
            _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
        claimBeforeDecode: false,
      );

  Future<Uint8List> getGlobalState() => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_get_global_state(_handle.raw, completion);
    }),
    decode: (result) =>
        _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    claimBeforeDecode: false,
  );

  Future<List<LatLng>?> getImageSourceCoordinates(String sourceId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_LAT_LNG,
        elementSize: sizeOf<raw.mln_lat_lng>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_get_image_source_coordinates(
            _handle.raw,
            nativeStringView(sourceId, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value == nullptr
            ? null
            : List<LatLng>.unmodifiable(
                List.generate(
                  result.value_count,
                  (index) =>
                      _readLatLng(result.value.cast<raw.mln_lat_lng>()[index]),
                ),
              ),
        claimBeforeDecode: false,
      );

  Future<Uint8List?> getLayerFilter(String layerId) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_get_layer_filter(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        completion,
      );
    }),
    decode: (result) => result.value_count == 0
        ? null
        : result.value.cast<raw.mln_buffer_view>().ref.data == nullptr
        ? null
        : _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    claimBeforeDecode: false,
  );

  Future<Uint8List?> getLayerProperty(String layerId, String propertyName) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
        elementSize: sizeOf<raw.mln_buffer_view>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_get_layer_property(
            _handle.raw,
            nativeStringView(layerId, arena).value,
            nativeStringView(propertyName, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : result.value.cast<raw.mln_buffer_view>().ref.data == nullptr
            ? null
            : _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
        claimBeforeDecode: false,
      );

  Future<StyleImageResult?> getStyleImageInfo(String imageId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE_RESULT,
        elementSize: sizeOf<raw.mln_style_image_result>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_get_style_image_info(
            _handle.raw,
            nativeStringView(imageId, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : _readStyleImageResult(
                result.value.cast<raw.mln_style_image_result>().ref,
              ),
        claimBeforeDecode: false,
      );

  Future<StyleLayerResult?> getStyleLayerInfo(String layerId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_STYLE_LAYER_RESULT,
        elementSize: sizeOf<raw.mln_style_layer_result>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_get_style_layer_info(
            _handle.raw,
            nativeStringView(layerId, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : _readStyleLayerResult(
                result.value.cast<raw.mln_style_layer_result>().ref,
              ),
        claimBeforeDecode: false,
      );

  Future<Uint8List?> getStyleLayerJson(String layerId) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_get_style_layer_json(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        completion,
      );
    }),
    decode: (result) => result.value_count == 0
        ? null
        : result.value.cast<raw.mln_buffer_view>().ref.data == nullptr
        ? null
        : _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    claimBeforeDecode: false,
  );

  Future<Uint8List?> getStyleLightProperty(String propertyName) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
        elementSize: sizeOf<raw.mln_buffer_view>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_get_style_light_property(
            _handle.raw,
            nativeStringView(propertyName, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : result.value.cast<raw.mln_buffer_view>().ref.data == nullptr
            ? null
            : _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
        claimBeforeDecode: false,
      );

  Future<StyleSourceResult?> getStyleSourceInfo(String sourceId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_RESULT,
        elementSize: sizeOf<raw.mln_style_source_result>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_get_style_source_info(
            _handle.raw,
            nativeStringView(sourceId, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : _readStyleSourceResult(
                result.value.cast<raw.mln_style_source_result>().ref,
              ),
        claimBeforeDecode: false,
      );

  Future<StyleSourceTileUrlsResult?> getStyleSourceTileUrls(String sourceId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_TILE_URLS_RESULT,
        elementSize: sizeOf<raw.mln_style_source_tile_urls_result>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_get_style_source_tile_urls(
            _handle.raw,
            nativeStringView(sourceId, arena).value,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : _readStyleSourceTileUrlsResult(
                result.value.cast<raw.mln_style_source_tile_urls_result>().ref,
              ),
        claimBeforeDecode: false,
      );

  Future<StyleTransitionOptions> getStyleTransitionOptions() =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_STYLE_TRANSITION_OPTIONS,
        elementSize: sizeOf<raw.mln_style_transition_options>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_get_style_transition_options(
            _handle.raw,
            completion,
          );
        }),
        decode: (result) => _readStyleTransitionOptions(
          result.value.cast<raw.mln_style_transition_options>().ref,
        ),
        claimBeforeDecode: false,
      );

  Future<CommandCompletion> invalidateCustomGeometrySourceRegion(
    String sourceId,
    LatLngBounds bounds,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_invalidate_custom_geometry_source_region(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        _writeLatLngBounds(bounds, arena).ref,
        completion,
      );
    }),
  );

  Future<CommandCompletion> invalidateCustomGeometrySourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_invalidate_custom_geometry_source_tile(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        _writeCanonicalTileId(tileId, arena).ref,
        completion,
      );
    }),
  );

  Future<CommandCompletion> invalidateCustomMvtVectorSourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_invalidate_custom_mvt_vector_source_tile(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        _writeCanonicalTileId(tileId, arena).ref,
        completion,
      );
    }),
  );

  Future<LatLngBounds> latLngBoundsForCamera(CameraOptions camera) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_LAT_LNG_BOUNDS,
        elementSize: sizeOf<raw.mln_lat_lng_bounds>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_lat_lng_bounds_for_camera(
            _handle.raw,
            _writeCameraOptions(camera, arena),
            completion,
          );
        }),
        decode: (result) =>
            _readLatLngBounds(result.value.cast<raw.mln_lat_lng_bounds>().ref),
        claimBeforeDecode: false,
      );

  Future<LatLngBounds> latLngBoundsForCameraUnwrapped(CameraOptions camera) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_LAT_LNG_BOUNDS,
        elementSize: sizeOf<raw.mln_lat_lng_bounds>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_lat_lng_bounds_for_camera_unwrapped(
            _handle.raw,
            _writeCameraOptions(camera, arena),
            completion,
          );
        }),
        decode: (result) =>
            _readLatLngBounds(result.value.cast<raw.mln_lat_lng_bounds>().ref),
        claimBeforeDecode: false,
      );

  Future<LatLng> latLngForPixel(ScreenPoint point) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_LAT_LNG,
    elementSize: sizeOf<raw.mln_lat_lng>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_lat_lng_for_pixel(
        _handle.raw,
        _writeScreenPoint(point, arena).ref,
        completion,
      );
    }),
    decode: (result) => _readLatLng(result.value.cast<raw.mln_lat_lng>().ref),
    claimBeforeDecode: false,
  );

  Future<LatLng> latLngForPixelUnwrapped(ScreenPoint point) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_LAT_LNG,
        elementSize: sizeOf<raw.mln_lat_lng>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_lat_lng_for_pixel_unwrapped(
            _handle.raw,
            _writeScreenPoint(point, arena).ref,
            completion,
          );
        }),
        decode: (result) =>
            _readLatLng(result.value.cast<raw.mln_lat_lng>().ref),
        claimBeforeDecode: false,
      );

  Future<List<LatLng>> latLngsForPixels(List<ScreenPoint> points) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_LAT_LNG,
        elementSize: sizeOf<raw.mln_lat_lng>(),
        start: (completion) => withNativeArena((arena) {
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
          );
        }),
        decode: (result) => List<LatLng>.unmodifiable(
          List.generate(
            result.value_count,
            (index) => _readLatLng(result.value.cast<raw.mln_lat_lng>()[index]),
          ),
        ),
        claimBeforeDecode: false,
      );

  Future<List<LatLng>> latLngsForPixelsUnwrapped(List<ScreenPoint> points) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_LAT_LNG,
        elementSize: sizeOf<raw.mln_lat_lng>(),
        start: (completion) => withNativeArena((arena) {
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
          );
        }),
        decode: (result) => List<LatLng>.unmodifiable(
          List.generate(
            result.value_count,
            (index) => _readLatLng(result.value.cast<raw.mln_lat_lng>()[index]),
          ),
        ),
        claimBeforeDecode: false,
      );

  Future<List<String>> listStyleLayerIds() => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_list_style_layer_ids(_handle.raw, completion);
    }),
    decode: (result) => List<String>.unmodifiable(
      List.generate(
        result.value_count,
        (index) => utf8.decode(
          _copyBufferView(result.value.cast<raw.mln_buffer_view>()[index]),
        ),
      ),
    ),
    claimBeforeDecode: false,
  );

  Future<List<StyleLayerEntry>> listStyleLayers() => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_STYLE_LAYER_ENTRY,
    elementSize: sizeOf<raw.mln_style_layer_entry>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_list_style_layers(_handle.raw, completion);
    }),
    decode: (result) => List<StyleLayerEntry>.unmodifiable(
      List.generate(
        result.value_count,
        (index) => _readStyleLayerEntry(
          result.value.cast<raw.mln_style_layer_entry>()[index],
        ),
      ),
    ),
    claimBeforeDecode: false,
  );

  Future<List<String>> listStyleSourceIds() => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_list_style_source_ids(_handle.raw, completion);
    }),
    decode: (result) => List<String>.unmodifiable(
      List.generate(
        result.value_count,
        (index) => utf8.decode(
          _copyBufferView(result.value.cast<raw.mln_buffer_view>()[index]),
        ),
      ),
    ),
    claimBeforeDecode: false,
  );

  Future<Uint8List> loadedStyleJson() => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_loaded_style_json(_handle.raw, completion);
    }),
    decode: (result) =>
        _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    claimBeforeDecode: false,
  );

  Future<double> metersPerPixelAtLatitude(double latitude) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: sizeOf<Double>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_meters_per_pixel_at_latitude(
            _handle.raw,
            latitude,
            completion,
          );
        }),
        decode: (result) => result.value.cast<Double>().value,
        claimBeforeDecode: false,
      );

  Future<CommandCompletion> moveStyleLayer(
    String layerId, {
    String? beforeLayerId,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_move_style_layer(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        nativeStringView((beforeLayerId ?? ''), arena).value,
        completion,
      );
    }),
  );

  Future<ScreenPoint> pixelForLatLng(LatLng coordinate) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_SCREEN_POINT,
        elementSize: sizeOf<raw.mln_screen_point>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_map_pixel_for_lat_lng(
            _handle.raw,
            _writeLatLng(coordinate, arena).ref,
            completion,
          );
        }),
        decode: (result) =>
            _readScreenPoint(result.value.cast<raw.mln_screen_point>().ref),
        claimBeforeDecode: false,
      );

  Future<List<ScreenPoint>> pixelsForLatLngs(
    List<LatLng> coordinates,
  ) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_SCREEN_POINT,
    elementSize: sizeOf<raw.mln_screen_point>(),
    start: (completion) => withNativeArena((arena) {
      final nativecoordinates = arena<raw.mln_lat_lng>(
        coordinates.isEmpty ? 1 : coordinates.length,
      );
      for (var index = 0; index < coordinates.length; index++) {
        nativecoordinates[index] = _writeLatLng(coordinates[index], arena).ref;
      }
      return raw.mln_map_pixels_for_lat_lngs(
        _handle.raw,
        nativecoordinates,
        coordinates.length,
        completion,
      );
    }),
    decode: (result) => List<ScreenPoint>.unmodifiable(
      List.generate(
        result.value_count,
        (index) =>
            _readScreenPoint(result.value.cast<raw.mln_screen_point>()[index]),
      ),
    ),
    claimBeforeDecode: false,
  );

  Future<MapProjectionHandle> projectionCreate() => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_MAP_PROJECTION,
    elementSize: sizeOf<Uint64>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_projection_create(_handle.raw, completion);
    }),
    decode: (result) => _adoptOwned(
      result.value.cast<Uint64>().value,
      () => MapProjectionHandle._(
        NativeMapProjection(result.value.cast<Uint64>().value),
      ),
      (handle) {
        _check(raw.mln_map_projection_close(handle));
      },
    ),
    claimBeforeDecode: true,
  );

  Future<void> close() => _state.closeAsync(
    (handle) => startNativeCompletion(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        return raw.mln_map_release(handle.raw, completion);
      }),
      decode: (result) {},
    ),
  );

  Future<CommandCompletion> removeFeatureState(FeatureStateSelector selector) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_remove_feature_state(
            _handle.raw,
            _writeFeatureStateSelector(selector, arena),
            completion,
          );
        }),
      );

  Future<CommandCompletion> removeStyleImage(String imageId) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_remove_style_image(
        _handle.raw,
        nativeStringView(imageId, arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> removeStyleLayer(String layerId) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_remove_style_layer(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> removeStyleSource(String sourceId) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_remove_style_source(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> requestRepaint() => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_request_repaint(_handle.raw, completion);
    }),
  );

  Future<void> requestStillImage() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_request_still_image(_handle.raw, completion);
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<CommandCompletion> resize(LogicalExtent extent) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_resize(
        _handle.raw,
        _writeLogicalExtent(extent, arena).ref,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setBounds(BoundOptions options) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_bounds(
        _handle.raw,
        _writeBoundOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setCustomGeometrySourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Uint8List data,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_custom_geometry_source_tile_data(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        _writeCanonicalTileId(tileId, arena).ref,
        nativeBufferView(data, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setCustomMvtVectorSourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Uint8List data,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_custom_mvt_vector_source_tile_data(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        _writeCanonicalTileId(tileId, arena).ref,
        nativeBufferView(data, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setCustomMvtVectorSourceTileError(
    String sourceId,
    CanonicalTileId tileId,
    String message,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_custom_mvt_vector_source_tile_error(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        _writeCanonicalTileId(tileId, arena).ref,
        nativeStringView(message, arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setDebugOptions(MapDebugOption options) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_debug_options(
            _handle.raw,
            options.rawValue,
            completion,
          );
        }),
      );

  Future<CommandCompletion> setEventMask(RuntimeEventMask mask) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_event_mask(
            _handle.raw,
            mask.rawValue,
            completion,
          );
        }),
      );

  Future<CommandCompletion> setFeatureState(
    FeatureStateSelector selector,
    Uint8List state,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_feature_state(
        _handle.raw,
        _writeFeatureStateSelector(selector, arena),
        nativeBufferView(state, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setFreeCameraOptions(FreeCameraOptions options) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_free_camera_options(
            _handle.raw,
            _writeFreeCameraOptions(options, arena),
            completion,
          );
        }),
      );

  Future<CommandCompletion> setGeojsonSourceData(
    String sourceId,
    GeoJsonSourceDataHandle data,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_geojson_source_data(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        data._handle.raw,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setGeojsonSourceSynchronousTiling(
    String sourceId,
    bool enabled,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_geojson_source_synchronous_tiling(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        enabled,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setGeojsonSourceUrl(String sourceId, String url) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_geojson_source_url(
            _handle.raw,
            nativeStringView(sourceId, arena).value,
            nativeStringView(url, arena).value,
            completion,
          );
        }),
      );

  Future<CommandCompletion> setGlobalStateProperty(
    String propertyName,
    Uint8List value,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_global_state_property(
        _handle.raw,
        nativeStringView(propertyName, arena).value,
        nativeBufferView(value, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setImageSourceCoordinates(
    String sourceId,
    List<LatLng> coordinates,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
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
      );
    }),
  );

  Future<CommandCompletion> setImageSourceImage(
    String sourceId,
    PremultipliedRgba8Image image,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_image_source_image(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        _writePremultipliedRgba8Image(image, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setImageSourceUrl(String sourceId, String url) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_image_source_url(
            _handle.raw,
            nativeStringView(sourceId, arena).value,
            nativeStringView(url, arena).value,
            completion,
          );
        }),
      );

  Future<CommandCompletion> setLayerFilter(
    String layerId, {
    Uint8List? filter,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_layer_filter(
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
      );
    }),
  );

  Future<CommandCompletion> setLayerMaxZoom(String layerId, double maxZoom) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_layer_max_zoom(
            _handle.raw,
            nativeStringView(layerId, arena).value,
            maxZoom,
            completion,
          );
        }),
      );

  Future<CommandCompletion> setLayerMinZoom(String layerId, double minZoom) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_layer_min_zoom(
            _handle.raw,
            nativeStringView(layerId, arena).value,
            minZoom,
            completion,
          );
        }),
      );

  Future<CommandCompletion> setLayerProperty(
    String layerId,
    String propertyName,
    Uint8List value,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_layer_property(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        nativeStringView(propertyName, arena).value,
        nativeBufferView(value, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setLayerSourceId(String layerId, String sourceId) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_layer_source_id(
            _handle.raw,
            nativeStringView(layerId, arena).value,
            nativeStringView(sourceId, arena).value,
            completion,
          );
        }),
      );

  Future<CommandCompletion> setLayerSourceLayer(
    String layerId, {
    String? sourceLayer,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_layer_source_layer(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        nativeStringView((sourceLayer ?? ''), arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setLayerVisibility(
    String layerId,
    StyleLayerVisibility visibility,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_layer_visibility(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        visibility.rawValue,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setLocationIndicatorAccuracyRadius(
    String layerId,
    double radius,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_location_indicator_accuracy_radius(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        radius,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setLocationIndicatorBearing(
    String layerId,
    double bearing,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_location_indicator_bearing(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        bearing,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setLocationIndicatorImageName(
    String layerId,
    LocationIndicatorImageKind imageKind,
    String imageId,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_location_indicator_image_name(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        imageKind.rawValue,
        nativeStringView(imageId, arena).value,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setLocationIndicatorLocation(
    String layerId,
    LatLng coordinate,
    double altitude,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_location_indicator_location(
        _handle.raw,
        nativeStringView(layerId, arena).value,
        _writeLatLng(coordinate, arena).ref,
        altitude,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setProjectionMode(ProjectionMode mode) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_projection_mode(
            _handle.raw,
            _writeProjectionMode(mode, arena),
            completion,
          );
        }),
      );

  Future<CommandCompletion> setRenderingStatsViewEnabled(bool enabled) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_rendering_stats_view_enabled(
            _handle.raw,
            enabled,
            completion,
          );
        }),
      );

  Future<CommandCompletion> setStyleImage(
    String imageId,
    PremultipliedRgba8Image image, {
    StyleImageOptions? options,
  }) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_style_image(
        _handle.raw,
        nativeStringView(imageId, arena).value,
        _writePremultipliedRgba8Image(image, arena),
        options == null ? nullptr : _writeStyleImageOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setStyleJson(Uint8List json) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_style_json(
        _handle.raw,
        nativeBufferView(json, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setStyleLightJson(Uint8List lightJson) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_style_light_json(
            _handle.raw,
            nativeBufferView(lightJson, arena),
            completion,
          );
        }),
      );

  Future<CommandCompletion> setStyleLightProperty(
    String propertyName,
    Uint8List value,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_style_light_property(
        _handle.raw,
        nativeStringView(propertyName, arena).value,
        nativeBufferView(value, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setStyleSourceVolatile(
    String sourceId,
    bool isVolatile,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_style_source_volatile(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        isVolatile,
        completion,
      );
    }),
  );

  Future<CommandCompletion> setStyleTransitionOptions(
    StyleTransitionOptions options,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_style_transition_options(
        _handle.raw,
        _writeStyleTransitionOptions(options, arena),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setStyleUrl(String url) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_set_style_url(
        _handle.raw,
        nativeUtf8CString(url, arena).pointer.cast<Char>(),
        completion,
      );
    }),
  );

  Future<CommandCompletion> setTileOptions(MapTileOptions options) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_tile_options(
            _handle.raw,
            _writeMapTileOptions(options, arena),
            completion,
          );
        }),
      );

  Future<CommandCompletion> setViewportOptions(MapViewportOptions options) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          return raw.mln_map_set_viewport_options(
            _handle.raw,
            _writeMapViewportOptions(options, arena),
            completion,
          );
        }),
      );

  MapSnapshot snapshotGet() => withNativeArena((arena) {
    final outSnapshot = arena<raw.mln_map_snapshot>();
    outSnapshot.ref.size = sizeOf<raw.mln_map_snapshot>();
    _check(raw.mln_map_snapshot_get(_handle.raw, outSnapshot));
    return _readMapSnapshot(outSnapshot.ref);
  });

  Future<String> styleUrl() => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_style_url(_handle.raw, completion);
    }),
    decode: (result) => utf8.decode(
      _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    ),
    claimBeforeDecode: false,
  );

  Future<CommandCompletion> updateCamera(CameraUpdate update) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_map_update_camera(
        _handle.raw,
        _writeCameraUpdate(update, arena),
        completion,
      );
    }),
  );

  RenderSessionAttachment metalBorrowedTextureAttach(
    MetalBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_metal_borrowed_texture_attach(
            _handle.raw,
            _writeMetalBorrowedTextureDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment metalOwnedTextureAttach(
    MetalOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_metal_owned_texture_attach(
            _handle.raw,
            _writeMetalOwnedTextureDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment metalSurfaceAttach(
    MetalSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_metal_surface_attach(
            _handle.raw,
            _writeMetalSurfaceDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment openglBorrowedTextureAttach(
    OpenglBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_opengl_borrowed_texture_attach(
            _handle.raw,
            _writeOpenglBorrowedTextureDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment openglOwnedTextureAttach(
    OpenglOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_opengl_owned_texture_attach(
            _handle.raw,
            _writeOpenglOwnedTextureDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment openglSurfaceAttach(
    OpenglSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_opengl_surface_attach(
            _handle.raw,
            _writeOpenglSurfaceDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment vulkanBorrowedTextureAttach(
    VulkanBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_vulkan_borrowed_texture_attach(
            _handle.raw,
            _writeVulkanBorrowedTextureDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment vulkanOwnedTextureAttach(
    VulkanOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_vulkan_owned_texture_attach(
            _handle.raw,
            _writeVulkanOwnedTextureDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment vulkanSurfaceAttach(
    VulkanSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_vulkan_surface_attach(
            _handle.raw,
            _writeVulkanSurfaceDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment webgpuBorrowedTextureAttach(
    WebgpuBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_webgpu_borrowed_texture_attach(
            _handle.raw,
            _writeWebgpuBorrowedTextureDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment webgpuOwnedTextureAttach(
    WebgpuOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_webgpu_owned_texture_attach(
            _handle.raw,
            _writeWebgpuOwnedTextureDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }

  RenderSessionAttachment webgpuSurfaceAttach(
    WebgpuSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    RenderSessionHandle? created;
    Object? adoptionError;
    StackTrace? adoptionStack;
    final completed = startNativeCompletion<void>(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        final registrations = _NativeRegistrations(_NativeCallbackPorts());
        try {
          final outSession = arena<Uint64>();
          final status = raw.mln_webgpu_surface_attach(
            _handle.raw,
            _writeWebgpuSurfaceDescriptor(descriptor, arena),
            _writeRenderSessionAttachOptions(options, arena, registrations),
            outSession,
            completion,
          );
          if (status == nativeStatusOk) {
            registrations.accept();
            try {
              created = _adoptOwned(
                outSession.value,
                () => (RenderSessionHandle._(
                  this as MapHandle,
                  NativeRenderSession(outSession.value),
                ).._state.retain(registrations)),
                (handle) {
                  _check(raw.mln_render_session_dispose(handle));
                },
              );
            } catch (error, stack) {
              adoptionError = error;
              adoptionStack = stack;
            }
          }
          return status;
        } finally {
          registrations.close();
        }
      }),
      decode: (_) {},
    );
    if (adoptionError != null) {
      completed.ignore();
      Error.throwWithStackTrace(adoptionError!, adoptionStack!);
    }
    final session = created!;
    return RenderSessionAttachment(
      session,
      completed.whenComplete(() {
        session.isClosed;
      }),
    );
  }
}

mixin _GeneratedProjectionOperations implements Finalizable {
  NativeMapProjection get _handle;

  NativeHandleState<NativeMapProjection> get _state;

  void close() => _state.close(
    (handle) => withNativeArena((arena) {
      return raw.mln_map_projection_close(handle.raw);
    }),
    threadLastErrorMessage,
  );

  CameraOptions getCamera() => withNativeArena((arena) {
    final outCamera = arena<raw.mln_camera_options>();
    outCamera.ref = raw.mln_camera_options_default();
    _check(raw.mln_map_projection_get_camera(_handle.raw, outCamera));
    return _readCameraOptions(outCamera.ref);
  });

  LatLng latLngForPixel(ScreenPoint point) => withNativeArena((arena) {
    final outCoordinate = arena<raw.mln_lat_lng>();
    _check(
      raw.mln_map_projection_lat_lng_for_pixel(
        _handle.raw,
        _writeScreenPoint(point, arena).ref,
        outCoordinate,
      ),
    );
    return _readLatLng(outCoordinate.ref);
  });

  LatLng latLngForPixelUnwrapped(ScreenPoint point) => withNativeArena((arena) {
    final outCoordinate = arena<raw.mln_lat_lng>();
    _check(
      raw.mln_map_projection_lat_lng_for_pixel_unwrapped(
        _handle.raw,
        _writeScreenPoint(point, arena).ref,
        outCoordinate,
      ),
    );
    return _readLatLng(outCoordinate.ref);
  });

  double metersPerPixelAtLatitude(double latitude) => withNativeArena((arena) {
    final outMetersPerPixel = arena<Double>();
    _check(
      raw.mln_map_projection_meters_per_pixel_at_latitude(
        _handle.raw,
        latitude,
        outMetersPerPixel,
      ),
    );
    return outMetersPerPixel.value;
  });

  ScreenPoint pixelForLatLng(LatLng coordinate) => withNativeArena((arena) {
    final outPoint = arena<raw.mln_screen_point>();
    _check(
      raw.mln_map_projection_pixel_for_lat_lng(
        _handle.raw,
        _writeLatLng(coordinate, arena).ref,
        outPoint,
      ),
    );
    return _readScreenPoint(outPoint.ref);
  });

  void setCamera(CameraOptions camera) => withNativeArena((arena) {
    _check(
      raw.mln_map_projection_set_camera(
        _handle.raw,
        _writeCameraOptions(camera, arena),
      ),
    );
  });

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
          ),
        );
      });

  void setVisibleGeometry(Uint8List geometry, EdgeInsets padding) =>
      withNativeArena((arena) {
        _check(
          raw.mln_map_projection_set_visible_geometry(
            _handle.raw,
            nativeBufferView(geometry, arena),
            _writeEdgeInsets(padding, arena).ref,
          ),
        );
      });
}

mixin _GeneratedRenderFrameBatchOperations implements Finalizable {
  NativeRenderFrameBatch get _handle;

  NativeHandleState<NativeRenderFrameBatch> get _state;

  int count() => withNativeArena((arena) {
    final outCount = arena<Size>();
    _check(raw.mln_render_frame_batch_count(_handle.raw, outCount));
    return outCount.value;
  });

  RenderFrameResult getValue(int indexValue) => withNativeArena((arena) {
    final outResult = arena<raw.mln_render_frame_result>();
    outResult.ref.size = sizeOf<raw.mln_render_frame_result>();
    _check(
      raw.mln_render_frame_batch_get(
        _handle.raw,
        _generatedInteger(
          indexValue,
          0,
          sizeOf<Size>() == 4 ? 4294967295 : 0x7fffffffffffffff,
        ),
        outResult,
      ),
    );
    return _readRenderFrameResult(outResult.ref);
  });

  void close() => _state.close(
    (handle) => withNativeArena((arena) {
      raw.mln_render_frame_batch_release(handle.raw);
      return nativeStatusOk;
    }),
    threadLastErrorMessage,
  );
}

/// RenderFrameBatch handle id.
extension type const NativeRenderFrameBatch(int raw) implements NativeHandle {}

final class RenderFrameBatchHandle with _GeneratedRenderFrameBatchOperations {
  RenderFrameBatchHandle._(NativeRenderFrameBatch handle)
    : _state = NativeHandleState(handle, 'RenderFrameBatchHandle');
  @override
  final NativeHandleState<NativeRenderFrameBatch> _state;
  @override
  NativeRenderFrameBatch get _handle => _state.handle;
  bool get isClosed => _state.isClosed;
}

mixin _GeneratedRenderSessionOperations implements Finalizable {
  NativeRenderSession get _handle;

  NativeHandleState<NativeRenderSession> get _state;

  Future<void> metalBorrowedTextureSetTarget(
    MetalBorrowedTextureDescriptor descriptor,
  ) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_metal_borrowed_texture_set_target(
        _handle.raw,
        _writeMetalBorrowedTextureDescriptor(descriptor, arena),
        completion,
      );
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> metalSurfaceSetTarget(MetalSurfaceDescriptor descriptor) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => withNativeArena((arena) {
          return raw.mln_metal_surface_set_target(
            _handle.raw,
            _writeMetalSurfaceDescriptor(descriptor, arena),
            completion,
          );
        }),
        decode: (result) {},
        claimBeforeDecode: false,
      );

  Future<void> openglBorrowedTextureSetTarget(
    OpenglBorrowedTextureDescriptor descriptor,
  ) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_opengl_borrowed_texture_set_target(
        _handle.raw,
        _writeOpenglBorrowedTextureDescriptor(descriptor, arena),
        completion,
      );
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> openglSurfaceSetTarget(OpenglSurfaceDescriptor descriptor) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => withNativeArena((arena) {
          return raw.mln_opengl_surface_set_target(
            _handle.raw,
            _writeOpenglSurfaceDescriptor(descriptor, arena),
            completion,
          );
        }),
        decode: (result) {},
        claimBeforeDecode: false,
      );

  RenderAbandonResult abandon() => withNativeArena((arena) {
    final outResult = arena<raw.mln_render_abandon_result>();
    outResult.ref.size = sizeOf<raw.mln_render_abandon_result>();
    _check(raw.mln_render_session_abandon(_handle.raw, outResult));
    return _readRenderAbandonResult(outResult.ref);
  });

  AcquiredFrame acquireFrame() => withNativeArena((arena) {
    final outFrame = arena<Uint64>();
    _check(raw.mln_render_session_acquire_frame(_handle.raw, outFrame));
    return _adoptOwned(
      outFrame.value,
      () => AcquiredFrame._(
        this as RenderSessionHandle,
        NativeAcquiredFrame(outFrame.value),
      ),
      (handle) {
        _check(raw.mln_acquired_frame_dispose(handle));
      },
    );
  });

  Future<void> barrier() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_render_session_barrier(_handle.raw, completion);
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> clearData() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_render_session_clear_data(_handle.raw, completion);
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  void close() => _state.close(
    (handle) => withNativeArena((arena) {
      return raw.mln_render_session_destroy(handle.raw);
    }),
    threadLastErrorMessage,
  );

  Future<void> detach() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_render_session_detach(_handle.raw, completion);
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  void dispose() => _state.close(
    (handle) => withNativeArena((arena) {
      return raw.mln_render_session_dispose(handle.raw);
    }),
    threadLastErrorMessage,
  );

  RenderFrameBatchHandle drainFrameResults() => withNativeArena((arena) {
    final outBatch = arena<Uint64>();
    _check(raw.mln_render_session_drain_frame_results(_handle.raw, outBatch));
    return _adoptOwned(
      outBatch.value,
      () => RenderFrameBatchHandle._(NativeRenderFrameBatch(outBatch.value)),
      (handle) {
        raw.mln_render_frame_batch_release(handle);
      },
    );
  });

  Future<void> dumpDebugLogs() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_render_session_dump_debug_logs(_handle.raw, completion);
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  RenderSessionCapabilities getCapabilities() => withNativeArena((arena) {
    final outCapabilities = arena<raw.mln_render_session_capabilities>();
    outCapabilities.ref.size = sizeOf<raw.mln_render_session_capabilities>();
    _check(
      raw.mln_render_session_get_capabilities(_handle.raw, outCapabilities),
    );
    return _readRenderSessionCapabilities(outCapabilities.ref);
  });

  RenderSessionSnapshot getSnapshot() => withNativeArena((arena) {
    final outSnapshot = arena<raw.mln_render_session_snapshot>();
    outSnapshot.ref.size = sizeOf<raw.mln_render_session_snapshot>();
    _check(raw.mln_render_session_get_snapshot(_handle.raw, outSnapshot));
    return _readRenderSessionSnapshot(outSnapshot.ref);
  });

  MapProjectionHandle projectionCreate() => withNativeArena((arena) {
    final outProjection = arena<Uint64>();
    _check(
      raw.mln_render_session_projection_create(_handle.raw, outProjection),
    );
    return _adoptOwned(
      outProjection.value,
      () => MapProjectionHandle._(NativeMapProjection(outProjection.value)),
      (handle) {
        _check(raw.mln_map_projection_close(handle));
      },
    );
  });

  Future<Uint8List> queryFeatureExtensions(
    String sourceId,
    Uint8List feature,
    String extensionValue,
    String extensionField, {
    Uint8List? arguments,
  }) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEW,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_render_session_query_feature_extensions(
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
      );
    }),
    decode: (result) =>
        _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    claimBeforeDecode: false,
  );

  Future<List<QueriedFeature>> queryRenderedFeatures(
    RenderedQueryGeometry geometry, {
    RenderedFeatureQueryOptions? options,
  }) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_QUERIED_FEATURE,
    elementSize: sizeOf<raw.mln_queried_feature>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_render_session_query_rendered_features(
        _handle.raw,
        _writeRenderedQueryGeometry(geometry, arena),
        options == null
            ? nullptr
            : _writeRenderedFeatureQueryOptions(options, arena),
        completion,
      );
    }),
    decode: (result) => List<QueriedFeature>.unmodifiable(
      List.generate(
        result.value_count,
        (index) => _readQueriedFeature(
          result.value.cast<raw.mln_queried_feature>()[index],
        ),
      ),
    ),
    claimBeforeDecode: false,
  );

  Future<List<QueriedFeature>> querySourceFeatures(
    String sourceId, {
    SourceFeatureQueryOptions? options,
  }) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_QUERIED_FEATURE,
    elementSize: sizeOf<raw.mln_queried_feature>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_render_session_query_source_features(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        options == null
            ? nullptr
            : _writeSourceFeatureQueryOptions(options, arena),
        completion,
      );
    }),
    decode: (result) => List<QueriedFeature>.unmodifiable(
      List.generate(
        result.value_count,
        (index) => _readQueriedFeature(
          result.value.cast<raw.mln_queried_feature>()[index],
        ),
      ),
    ),
    claimBeforeDecode: false,
  );

  Future<void> reduceMemoryUse() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_render_session_reduce_memory_use(_handle.raw, completion);
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  void requestFrame(FrameDemand demand) => withNativeArena((arena) {
    _check(
      raw.mln_render_session_request_frame(
        _handle.raw,
        _writeFrameDemand(demand, arena),
      ),
    );
  });

  Future<CommandCompletion> resize(RenderTargetExtent extent) => _startCommand(
    (completion) => withNativeArena((arena) {
      return raw.mln_render_session_resize(
        _handle.raw,
        _writeRenderTargetExtent(extent, arena),
        completion,
      );
    }),
  );

  int serviceDriverWork(int maxWork) => withNativeArena((arena) {
    final outServiced = arena<Size>();
    _check(
      raw.mln_render_session_service_driver_work(
        _handle.raw,
        _generatedInteger(
          maxWork,
          0,
          sizeOf<Size>() == 4 ? 4294967295 : 0x7fffffffffffffff,
        ),
        outServiced,
      ),
    );
    return outServiced.value;
  });

  Future<TextureReadbackResult> textureReadPremultipliedRgba8() =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_TEXTURE_READBACK_RESULT,
        elementSize: sizeOf<raw.mln_texture_readback_result>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_texture_read_premultiplied_rgba8(
            _handle.raw,
            completion,
          );
        }),
        decode: (result) => _readTextureReadbackResult(
          result.value.cast<raw.mln_texture_readback_result>().ref,
        ),
        claimBeforeDecode: false,
      );

  Future<void> vulkanBorrowedTextureSetTarget(
    VulkanBorrowedTextureDescriptor descriptor,
  ) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_vulkan_borrowed_texture_set_target(
        _handle.raw,
        _writeVulkanBorrowedTextureDescriptor(descriptor, arena),
        completion,
      );
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> vulkanSurfaceSetTarget(VulkanSurfaceDescriptor descriptor) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => withNativeArena((arena) {
          return raw.mln_vulkan_surface_set_target(
            _handle.raw,
            _writeVulkanSurfaceDescriptor(descriptor, arena),
            completion,
          );
        }),
        decode: (result) {},
        claimBeforeDecode: false,
      );

  Future<void> webgpuBorrowedTextureSetTarget(
    WebgpuBorrowedTextureDescriptor descriptor,
  ) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_webgpu_borrowed_texture_set_target(
        _handle.raw,
        _writeWebgpuBorrowedTextureDescriptor(descriptor, arena),
        completion,
      );
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> webgpuSurfaceSetTarget(WebgpuSurfaceDescriptor descriptor) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => withNativeArena((arena) {
          return raw.mln_webgpu_surface_set_target(
            _handle.raw,
            _writeWebgpuSurfaceDescriptor(descriptor, arena),
            completion,
          );
        }),
        decode: (result) {},
        claimBeforeDecode: false,
      );
}

mixin _GeneratedResourceRequestOperations implements Finalizable {
  NativeResourceRequest get _handle;

  NativeHandleState<NativeResourceRequest> get _state;

  bool cancelled() => withNativeArena((arena) {
    final outCancelled = arena<Bool>();
    _check(raw.mln_resource_request_cancelled(_handle.raw, outCancelled));
    return outCancelled.value;
  });

  void complete(ResourceResponse response) => withNativeArena((arena) {
    _check(
      raw.mln_resource_request_complete(
        _handle.raw,
        _writeResourceResponse(response, arena),
      ),
    );
  });

  void close() => _state.close(
    (handle) => withNativeArena((arena) {
      raw.mln_resource_request_release(handle.raw);
      return nativeStatusOk;
    }),
    threadLastErrorMessage,
  );

  bool setCancelCallback(void Function() callback) =>
      _registerResourceCancellation(this as ResourceRequestHandle, callback);

  void waitUntilRetired() => withNativeArena((arena) {
    _check(raw.mln_resource_request_wait_until_retired(_state.handleId));
  });
}

mixin _GeneratedRuntimeOperations implements Finalizable {
  NativeRuntime get _handle;

  NativeHandleState<NativeRuntime> get _state;

  _NativeCallbackPorts get _callbackPorts;

  Future<MapHandle> mapCreate(MapOptions options) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_MAP,
    elementSize: sizeOf<Uint64>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_map_create(
        _handle.raw,
        _writeMapOptions(options, arena),
        completion,
      );
    }),
    decode: (result) => _adoptOwned(
      result.value.cast<Uint64>().value,
      () => MapHandle._(
        this as RuntimeHandle,
        NativeMap(result.value.cast<Uint64>().value),
      ),
      (handle) {
        _check(raw.mln_map_dispose(handle));
      },
    ),
    claimBeforeDecode: true,
  );

  Future<void> barrier() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_barrier(_handle.raw, completion);
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> clearHttpHeaderTransform() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_clear_http_header_transform(
        _handle.raw,
        completion,
      );
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> clearResourceProvider() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_clear_resource_provider(_handle.raw, completion);
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> clearResourceTransform() => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_clear_resource_transform(_handle.raw, completion);
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  void dispose() => _state.close(
    (handle) => withNativeArena((arena) {
      return raw.mln_runtime_dispose(handle.raw);
    }),
    threadLastErrorMessage,
  );

  EventBatchHandle drainEvents() => withNativeArena((arena) {
    final outBatch = arena<Uint64>();
    _check(raw.mln_runtime_drain_events(_handle.raw, outBatch));
    return _adoptOwned(
      outBatch.value,
      () => EventBatchHandle._(NativeEventBatch(outBatch.value)),
      (handle) {
        raw.mln_event_batch_release(handle);
      },
    );
  });

  RuntimeEventMask getEventMask() => withNativeArena((arena) {
    final outMask = arena<Uint64>();
    _check(raw.mln_runtime_get_event_mask(_handle.raw, outMask));
    return RuntimeEventMask.fromRawValue(outMask.value);
  });

  Future<OfflineRegionInfo> offlineRegionCreate(
    OfflineRegionDefinition definition,
    Uint8List metadata,
  ) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_INFO,
    elementSize: sizeOf<raw.mln_offline_region_info>(),
    start: (completion) => withNativeArena((arena) {
      final bytesmetadata = nativeBufferView(metadata, arena);
      return raw.mln_runtime_offline_region_create(
        _handle.raw,
        _writeOfflineRegionDefinition(definition, arena),
        bytesmetadata.data.cast(),
        bytesmetadata.size,
        completion,
      );
    }),
    decode: (result) => _readOfflineRegionInfo(
      result.value.cast<raw.mln_offline_region_info>().ref,
    ),
    claimBeforeDecode: false,
  );

  Future<void> offlineRegionDelete(int regionId) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_offline_region_delete(
        _handle.raw,
        regionId,
        completion,
      );
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<OfflineRegionInfo?> offlineRegionGet(int regionId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_INFO,
        elementSize: sizeOf<raw.mln_offline_region_info>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_runtime_offline_region_get(
            _handle.raw,
            regionId,
            completion,
          );
        }),
        decode: (result) => result.value_count == 0
            ? null
            : _readOfflineRegionInfo(
                result.value.cast<raw.mln_offline_region_info>().ref,
              ),
        claimBeforeDecode: false,
      );

  Future<OfflineRegionStatus> offlineRegionGetStatus(int regionId) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_STATUS,
        elementSize: sizeOf<raw.mln_offline_region_status>(),
        start: (completion) => withNativeArena((arena) {
          return raw.mln_runtime_offline_region_get_status(
            _handle.raw,
            regionId,
            completion,
          );
        }),
        decode: (result) => _readOfflineRegionStatus(
          result.value.cast<raw.mln_offline_region_status>().ref,
        ),
        claimBeforeDecode: false,
      );

  Future<void> offlineRegionInvalidate(int regionId) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_offline_region_invalidate(
        _handle.raw,
        regionId,
        completion,
      );
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> offlineRegionSetDownloadState(
    int regionId,
    OfflineRegionDownloadState state,
  ) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_offline_region_set_download_state(
        _handle.raw,
        regionId,
        state.rawValue,
        completion,
      );
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> offlineRegionSetObserved(int regionId, bool observed) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => withNativeArena((arena) {
          return raw.mln_runtime_offline_region_set_observed(
            _handle.raw,
            regionId,
            observed,
            completion,
          );
        }),
        decode: (result) {},
        claimBeforeDecode: false,
      );

  Future<OfflineRegionInfo> offlineRegionUpdateMetadata(
    int regionId,
    Uint8List metadata,
  ) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_INFO,
    elementSize: sizeOf<raw.mln_offline_region_info>(),
    start: (completion) => withNativeArena((arena) {
      final bytesmetadata = nativeBufferView(metadata, arena);
      return raw.mln_runtime_offline_region_update_metadata(
        _handle.raw,
        regionId,
        bytesmetadata.data.cast(),
        bytesmetadata.size,
        completion,
      );
    }),
    decode: (result) => _readOfflineRegionInfo(
      result.value.cast<raw.mln_offline_region_info>().ref,
    ),
    claimBeforeDecode: false,
  );

  Future<List<OfflineRegionInfo>> offlineRegionsList() => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_INFO,
    elementSize: sizeOf<raw.mln_offline_region_info>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_offline_regions_list(_handle.raw, completion);
    }),
    decode: (result) => List<OfflineRegionInfo>.unmodifiable(
      List.generate(
        result.value_count,
        (index) => _readOfflineRegionInfo(
          result.value.cast<raw.mln_offline_region_info>()[index],
        ),
      ),
    ),
    claimBeforeDecode: false,
  );

  Future<List<OfflineRegionInfo>> offlineRegionsMergeDatabase(
    String sideDatabasePath,
  ) => startNativeCompletion(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_INFO,
    elementSize: sizeOf<raw.mln_offline_region_info>(),
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_offline_regions_merge_database(
        _handle.raw,
        nativeUtf8CString(sideDatabasePath, arena).pointer.cast<Char>(),
        completion,
      );
    }),
    decode: (result) => List<OfflineRegionInfo>.unmodifiable(
      List.generate(
        result.value_count,
        (index) => _readOfflineRegionInfo(
          result.value.cast<raw.mln_offline_region_info>()[index],
        ),
      ),
    ),
    claimBeforeDecode: false,
  );

  Future<void> close() => _state.closeAsync(
    (handle) => startNativeCompletion(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: 0,
      start: (completion) => withNativeArena((arena) {
        return raw.mln_runtime_release(handle.raw, completion);
      }),
      decode: (result) {},
    ),
  );

  Future<void> runAmbientCacheOperation(AmbientCacheOperation operation) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => withNativeArena((arena) {
          return raw.mln_runtime_run_ambient_cache_operation(
            _handle.raw,
            operation.rawValue,
            completion,
          );
        }),
        decode: (result) {},
        claimBeforeDecode: false,
      );

  void setEventMask(RuntimeEventMask mask) => withNativeArena((arena) {
    _check(raw.mln_runtime_set_event_mask(_handle.raw, mask.rawValue));
  });

  Future<void> setHttpHeaderTransform(HttpHeaderTransform transform) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => withNativeArena((arena) {
          final registrations = _NativeRegistrations(_callbackPorts);
          try {
            final status = raw.mln_runtime_set_http_header_transform(
              _handle.raw,
              registrations.prepareHttpHeaderTransform(transform),
              completion,
            );
            if (status == nativeStatusOk) {
              registrations.accept();
            }
            return status;
          } finally {
            registrations.close();
          }
        }),
        decode: (result) {},
        claimBeforeDecode: false,
      );

  Future<void> setMaximumAmbientCacheSize(BigInt size) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      return raw.mln_runtime_set_maximum_ambient_cache_size(
        _handle.raw,
        uint64ToNative(size, 'uint64_t'),
        completion,
      );
    }),
    decode: (result) {},
    claimBeforeDecode: false,
  );

  Future<void> setResourceProvider(ResourceProvider provider) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => withNativeArena((arena) {
          final registrations = _NativeRegistrations(_callbackPorts);
          try {
            final status = raw.mln_runtime_set_resource_provider(
              _handle.raw,
              registrations.prepareResourceProvider(provider),
              completion,
            );
            if (status == nativeStatusOk) {
              registrations.accept();
            }
            return status;
          } finally {
            registrations.close();
          }
        }),
        decode: (result) {},
        claimBeforeDecode: false,
      );

  Future<void> setResourceTransform(ResourceTransform transform) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => withNativeArena((arena) {
          final registrations = _NativeRegistrations(_callbackPorts);
          try {
            final status = raw.mln_runtime_set_resource_transform(
              _handle.raw,
              registrations.prepareResourceTransform(transform),
              completion,
            );
            if (status == nativeStatusOk) {
              registrations.accept();
            }
            return status;
          } finally {
            registrations.close();
          }
        }),
        decode: (result) {},
        claimBeforeDecode: false,
      );
}

final class ScopedMetalOwnedTextureFrame {
  ScopedMetalOwnedTextureFrame._(AcquiredFrame owner, this._value)
    : _scope = _GeneratedNativeViewScope(
        owner,
        raw.mln_adapter_acquired_frame_view_begin,
        raw.mln_adapter_acquired_frame_view_end,
      );
  final MetalOwnedTextureFrame _value;
  final _GeneratedNativeViewScope _scope;
  T withView<T>(T Function(ScopedMetalOwnedTextureFrame) use) =>
      _scope.use(() => use(this));
  BigInt get generation {
    _scope.checkActive();
    return _value.generation;
  }

  int get width {
    _scope.checkActive();
    return _value.width;
  }

  int get height {
    _scope.checkActive();
    return _value.height;
  }

  double get scaleFactor {
    _scope.checkActive();
    return _value.scaleFactor;
  }

  BigInt get frameId {
    _scope.checkActive();
    return _value.frameId;
  }

  ScopedNativePointer get texture {
    _scope.checkActive();
    return ScopedNativePointer(
      _value.texture.address,
      checkValid: _scope.checkActive,
      debugName: 'MetalOwnedTextureFrame.texture',
    );
  }

  ScopedNativePointer get device {
    _scope.checkActive();
    return ScopedNativePointer(
      _value.device.address,
      checkValid: _scope.checkActive,
      debugName: 'MetalOwnedTextureFrame.device',
    );
  }

  BigInt get pixelFormat {
    _scope.checkActive();
    return _value.pixelFormat;
  }
}

final class ScopedOpenglOwnedTextureFrame {
  ScopedOpenglOwnedTextureFrame._(AcquiredFrame owner, this._value)
    : _scope = _GeneratedNativeViewScope(
        owner,
        raw.mln_adapter_acquired_frame_view_begin,
        raw.mln_adapter_acquired_frame_view_end,
      );
  final OpenglOwnedTextureFrame _value;
  final _GeneratedNativeViewScope _scope;
  T withView<T>(T Function(ScopedOpenglOwnedTextureFrame) use) =>
      _scope.use(() => use(this));
  BigInt get generation {
    _scope.checkActive();
    return _value.generation;
  }

  int get width {
    _scope.checkActive();
    return _value.width;
  }

  int get height {
    _scope.checkActive();
    return _value.height;
  }

  double get scaleFactor {
    _scope.checkActive();
    return _value.scaleFactor;
  }

  BigInt get frameId {
    _scope.checkActive();
    return _value.frameId;
  }

  int get texture {
    _scope.checkActive();
    return _value.texture;
  }

  int get target {
    _scope.checkActive();
    return _value.target;
  }

  int get internalFormat {
    _scope.checkActive();
    return _value.internalFormat;
  }

  int get format {
    _scope.checkActive();
    return _value.format;
  }

  int get type {
    _scope.checkActive();
    return _value.type;
  }
}

final class ScopedGpuSync {
  ScopedGpuSync._(AcquiredFrame owner, this._value)
    : _scope = _GeneratedNativeViewScope(
        owner,
        raw.mln_adapter_acquired_frame_view_begin,
        raw.mln_adapter_acquired_frame_view_end,
      );
  final GpuSync _value;
  final _GeneratedNativeViewScope _scope;
  T withView<T>(T Function(ScopedGpuSync) use) => _scope.use(() => use(this));
  GpuSyncKind get kind {
    _scope.checkActive();
    return _value.kind;
  }

  BigInt get object {
    _scope.checkActive();
    return _value.object;
  }

  BigInt get value {
    _scope.checkActive();
    return _value.value;
  }
}

final class ScopedVulkanOwnedTextureFrame {
  ScopedVulkanOwnedTextureFrame._(AcquiredFrame owner, this._value)
    : _scope = _GeneratedNativeViewScope(
        owner,
        raw.mln_adapter_acquired_frame_view_begin,
        raw.mln_adapter_acquired_frame_view_end,
      );
  final VulkanOwnedTextureFrame _value;
  final _GeneratedNativeViewScope _scope;
  T withView<T>(T Function(ScopedVulkanOwnedTextureFrame) use) =>
      _scope.use(() => use(this));
  BigInt get generation {
    _scope.checkActive();
    return _value.generation;
  }

  int get width {
    _scope.checkActive();
    return _value.width;
  }

  int get height {
    _scope.checkActive();
    return _value.height;
  }

  double get scaleFactor {
    _scope.checkActive();
    return _value.scaleFactor;
  }

  BigInt get frameId {
    _scope.checkActive();
    return _value.frameId;
  }

  BigInt get image {
    _scope.checkActive();
    return _value.image;
  }

  BigInt get imageView {
    _scope.checkActive();
    return _value.imageView;
  }

  ScopedNativePointer get device {
    _scope.checkActive();
    return ScopedNativePointer(
      _value.device.address,
      checkValid: _scope.checkActive,
      debugName: 'VulkanOwnedTextureFrame.device',
    );
  }

  int get format {
    _scope.checkActive();
    return _value.format;
  }

  int get layout {
    _scope.checkActive();
    return _value.layout;
  }
}

final class ScopedWebgpuOwnedTextureFrame {
  ScopedWebgpuOwnedTextureFrame._(AcquiredFrame owner, this._value)
    : _scope = _GeneratedNativeViewScope(
        owner,
        raw.mln_adapter_acquired_frame_view_begin,
        raw.mln_adapter_acquired_frame_view_end,
      );
  final WebgpuOwnedTextureFrame _value;
  final _GeneratedNativeViewScope _scope;
  T withView<T>(T Function(ScopedWebgpuOwnedTextureFrame) use) =>
      _scope.use(() => use(this));
  BigInt get generation {
    _scope.checkActive();
    return _value.generation;
  }

  int get width {
    _scope.checkActive();
    return _value.width;
  }

  int get height {
    _scope.checkActive();
    return _value.height;
  }

  double get scaleFactor {
    _scope.checkActive();
    return _value.scaleFactor;
  }

  BigInt get frameId {
    _scope.checkActive();
    return _value.frameId;
  }

  ScopedNativePointer get texture {
    _scope.checkActive();
    return ScopedNativePointer(
      _value.texture.address,
      checkValid: _scope.checkActive,
      debugName: 'WebgpuOwnedTextureFrame.texture',
    );
  }

  ScopedNativePointer get textureView {
    _scope.checkActive();
    return ScopedNativePointer(
      _value.textureView.address,
      checkValid: _scope.checkActive,
      debugName: 'WebgpuOwnedTextureFrame.textureView',
    );
  }

  ScopedNativePointer get device {
    _scope.checkActive();
    return ScopedNativePointer(
      _value.device.address,
      checkValid: _scope.checkActive,
      debugName: 'WebgpuOwnedTextureFrame.device',
    );
  }

  int get format {
    _scope.checkActive();
    return _value.format;
  }
}

final class _GeneratedNativeViewScope {
  _GeneratedNativeViewScope(this.owner, this.begin, this.end);
  final AcquiredFrame owner;
  final int Function(int, Pointer<Pointer<Void>>) begin;
  final void Function(Pointer<Void>) end;
  int _active = 0;
  void checkActive() {
    if (_active == 0) {
      throwInvalidState(
        'borrowed native value requires an active withView callback',
      );
    }
  }

  T use<T>(T Function() callback) => withNativeArena((arena) {
    final token = arena<Pointer<Void>>();
    _check(begin(owner._handle.raw, token));
    _active++;
    try {
      final result = callback();
      if (result is Future) {
        throwInvalidArgument('withView callback must complete synchronously');
      }
      return result;
    } finally {
      _active--;
      end(token.value);
    }
  });
}
