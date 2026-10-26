// Generated from the C headers by tools/bindgen. Do not edit.
part of 'runtime.dart';

MetalTextureFrame _readMetalTextureFrame(raw.mln_metal_texture_frame source) =>
    MetalTextureFrame(
      generation: source.generation,
      width: source.width,
      height: source.height,
      scaleFactor: source.scale_factor,
      frameId: source.frame_id,
      slot: source.slot,
      texture: NativePointer(source.texture.address),
      device: NativePointer(source.device.address),
      pixelFormat: source.pixel_format,
    );

OpenGlTextureFrame _readOpenGlTextureFrame(
  raw.mln_open_gl_texture_frame source,
) => OpenGlTextureFrame(
  generation: source.generation,
  width: source.width,
  height: source.height,
  scaleFactor: source.scale_factor,
  frameId: source.frame_id,
  slot: source.slot,
  texture: source.texture,
  target: source.target,
  internalFormat: source.internal_format,
  format: source.format,
  type: source.type,
);

Pointer<raw.mln_gpu_sync> _writeGpuSync(
  GpuSync value,
  Allocator arena, {
  Pointer<raw.mln_gpu_sync>? into,
}) {
  final result = into ?? arena<raw.mln_gpu_sync>();
  result.ref.size = sizeOf<raw.mln_gpu_sync>();
  result.ref.kind = value.kind.rawValue;
  result.ref.handle = value.handle;
  return result;
}

GpuSync _readGpuSync(raw.mln_gpu_sync source) =>
    GpuSync(kind: GpuSyncKind.fromRawValue(source.kind), handle: source.handle);

Pointer<raw.mln_lat_lng> _writeLatLng(
  LatLng value,
  Allocator arena, {
  Pointer<raw.mln_lat_lng>? into,
}) {
  final result = into ?? arena<raw.mln_lat_lng>();
  result.ref.latitude = value.latitude;
  result.ref.longitude = value.longitude;
  return result;
}

LatLng _readLatLng(raw.mln_lat_lng source) =>
    LatLng(latitude: source.latitude, longitude: source.longitude);

Pointer<raw.mln_edge_insets> _writeEdgeInsets(
  EdgeInsets value,
  Allocator arena, {
  Pointer<raw.mln_edge_insets>? into,
}) {
  final result = into ?? arena<raw.mln_edge_insets>();
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

Pointer<raw.mln_camera> _writeCamera(
  Camera value,
  Allocator arena, {
  Pointer<raw.mln_camera>? into,
}) {
  final result = into ?? arena<raw.mln_camera>();
  result.ref.center = _writeLatLng(value.center, arena).ref;
  result.ref.center_altitude = value.centerAltitude;
  result.ref.padding = _writeEdgeInsets(value.padding, arena).ref;
  result.ref.zoom = value.zoom;
  result.ref.bearing = value.bearing;
  result.ref.pitch = value.pitch;
  result.ref.roll = value.roll;
  result.ref.field_of_view = value.fieldOfView;
  return result;
}

Camera _readCamera(raw.mln_camera source) => Camera(
  center: _readLatLng(source.center),
  centerAltitude: source.center_altitude,
  padding: _readEdgeInsets(source.padding),
  zoom: source.zoom,
  bearing: source.bearing,
  pitch: source.pitch,
  roll: source.roll,
  fieldOfView: source.field_of_view,
);

Pointer<raw.mln_logical_extent> _writeLogicalExtent(
  LogicalExtent value,
  Allocator arena, {
  Pointer<raw.mln_logical_extent>? into,
}) {
  final result = into ?? arena<raw.mln_logical_extent>();
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

Pointer<raw.mln_projection_mode> _writeProjectionMode(
  ProjectionMode value,
  Allocator arena, {
  Pointer<raw.mln_projection_mode>? into,
}) {
  final result = into ?? arena<raw.mln_projection_mode>();
  result.ref.size = sizeOf<raw.mln_projection_mode>();
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

Pointer<raw.mln_map_viewport_options> _writeMapViewportOptions(
  MapViewportOptions value,
  Allocator arena, {
  Pointer<raw.mln_map_viewport_options>? into,
}) {
  final result = into ?? arena<raw.mln_map_viewport_options>();
  result.ref.size = sizeOf<raw.mln_map_viewport_options>();
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

Pointer<raw.mln_map_view> _writeMapView(
  MapView value,
  Allocator arena, {
  Pointer<raw.mln_map_view>? into,
}) {
  final result = into ?? arena<raw.mln_map_view>();
  result.ref.size = sizeOf<raw.mln_map_view>();
  result.ref.camera = _writeCamera(value.camera, arena).ref;
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
  result.ref.projection_mode = _writeProjectionMode(
    value.projectionMode,
    arena,
  ).ref;
  result.ref.viewport = _writeMapViewportOptions(value.viewport, arena).ref;
  return result;
}

MapView _readMapView(raw.mln_map_view source) => MapView(
  camera: _readCamera(source.camera),
  extent: _readLogicalExtent(source.extent),
  projectionMode: _readProjectionMode(source.projection_mode),
  viewport: _readMapViewportOptions(source.viewport),
);

RenderFrameResult _readRenderFrameResult(raw.mln_render_frame_result source) =>
    RenderFrameResult(
      disposition: RenderResult.fromRawValue(source.disposition),
      token: source.token,
      mapUpdateGeneration: source.map_update_generation,
      generation: source.generation,
      frameGeneration: source.frame_generation,
      needsRepaint: source.needs_repaint,
      view: _readMapView(source.view),
    );

VulkanTextureFrame _readVulkanTextureFrame(
  raw.mln_vulkan_texture_frame source,
) => VulkanTextureFrame(
  generation: source.generation,
  width: source.width,
  height: source.height,
  scaleFactor: source.scale_factor,
  frameId: source.frame_id,
  slot: source.slot,
  image: source.image,
  imageView: source.image_view,
  device: NativePointer(source.device.address),
  format: source.format,
  layout: source.layout,
);

Pointer<raw.mln_geo_json_source_options> _writeGeoJsonSourceOptions(
  GeoJsonSourceOptions value,
  Allocator arena, {
  Pointer<raw.mln_geo_json_source_options>? into,
}) {
  final result = into ?? arena<raw.mln_geo_json_source_options>();
  result.ref.size = sizeOf<raw.mln_geo_json_source_options>();
  if (value.minZoom != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM;
    result.ref.min_zoom = value.minZoom!;
  }
  if (value.maxZoom != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM;
    result.ref.max_zoom = value.maxZoom!;
  }
  if (value.tolerance != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE;
    result.ref.tolerance = value.tolerance!;
  }
  if (value.clusterMaxZoom != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
    result.ref.cluster_max_zoom = value.clusterMaxZoom!;
  }
  result.ref.cluster_properties = _nativeJsonView(
    value.clusterProperties,
    arena,
    member: true,
  );
  if (value.buffer != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_BUFFER;
    result.ref.buffer = _nativeInteger(value.buffer!, 0, 4294967295);
  }
  if (value.clusterRadius != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS;
    result.ref.cluster_radius = _nativeInteger(
      value.clusterRadius!,
      0,
      4294967295,
    );
  }
  if (value.clusterMinPoints != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
    result.ref.cluster_min_points = _nativeInteger(
      value.clusterMinPoints!,
      0,
      4294967295,
    );
  }
  if (value.lineMetrics != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS;
    result.ref.line_metrics = value.lineMetrics!;
  }
  if (value.cluster != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_CLUSTER;
    result.ref.cluster = value.cluster!;
  }
  if (value.synchronousTiling != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
    result.ref.synchronous_tiling = value.synchronousTiling!;
  }
  return result;
}

Pointer<raw.mln_projected_meters> _writeProjectedMeters(
  ProjectedMeters value,
  Allocator arena, {
  Pointer<raw.mln_projected_meters>? into,
}) {
  final result = into ?? arena<raw.mln_projected_meters>();
  result.ref.northing = value.northing;
  result.ref.easting = value.easting;
  return result;
}

ProjectedMeters _readProjectedMeters(raw.mln_projected_meters source) =>
    ProjectedMeters(northing: source.northing, easting: source.easting);

void _deliverLogCallback(
  LogCallback callback,
  int batch,
  int index,
  Pointer<raw.mln_delivery> delivery,
) {
  final payload = delivery.ref.payload.log_record;
  callback(
    LogSeverity.fromRawValue(payload.severity),
    LogEvent.fromRawValue(payload.event),
    payload.code,
    utf8.decode(_copyBufferView(payload.message)),
  );
}

_NativeRegistration<raw.mln_log_handler> _prepareLogHandler(
  LogCallback value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_log_handler>();
    result.ref.size = sizeOf<raw.mln_log_handler>();
    final registered = registration = roots.registerQueued({
      raw.MLN_DELIVERY_TYPE_LOG_RECORD: (batch, index, delivery) =>
          _deliverLogCallback(value, batch, index, delivery),
    });
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = Pointer.fromAddress(registered.token);
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

Pointer<raw.mln_canonical_tile_id> _writeCanonicalTileId(
  CanonicalTileId value,
  Allocator arena, {
  Pointer<raw.mln_canonical_tile_id>? into,
}) {
  final result = into ?? arena<raw.mln_canonical_tile_id>();
  result.ref.z = _nativeInteger(value.z, 0, 4294967295);
  result.ref.x = _nativeInteger(value.x, 0, 4294967295);
  result.ref.y = _nativeInteger(value.y, 0, 4294967295);
  return result;
}

CanonicalTileId _readCanonicalTileId(raw.mln_canonical_tile_id source) =>
    CanonicalTileId(z: source.z, x: source.x, y: source.y);

void _deliverCustomSourceFetchTileCallback(
  CustomSourceFetchTileCallback callback,
  int batch,
  int index,
  Pointer<raw.mln_delivery> delivery,
) {
  final payload = delivery.ref.payload.custom_source_fetch_tile;
  callback(_readCanonicalTileId(payload.tile_id));
}

void _deliverCustomSourceCancelTileCallback(
  CustomSourceCancelTileCallback callback,
  int batch,
  int index,
  Pointer<raw.mln_delivery> delivery,
) {
  final payload = delivery.ref.payload.custom_source_cancel_tile;
  callback(_readCanonicalTileId(payload.tile_id));
}

_NativeRegistration<raw.mln_custom_geometry_source_options>
_prepareCustomGeometrySourceOptions(
  CustomGeometrySourceOptions value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_custom_geometry_source_options>();
    result.ref.size = sizeOf<raw.mln_custom_geometry_source_options>();
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
    final registered = registration = roots.registerQueued({
      raw.MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE:
          (batch, index, delivery) => _deliverCustomSourceFetchTileCallback(
            value.fetchTile,
            batch,
            index,
            delivery,
          ),
      if (value.cancelTile != null)
        raw.MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE:
            (batch, index, delivery) => _deliverCustomSourceCancelTileCallback(
              value.cancelTile!,
              batch,
              index,
              delivery,
            ),
    });
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = Pointer.fromAddress(registered.token);
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

_NativeRegistration<raw.mln_custom_mvt_vector_source_options>
_prepareCustomMvtVectorSourceOptions(
  CustomMvtVectorSourceOptions value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_custom_mvt_vector_source_options>();
    result.ref.size = sizeOf<raw.mln_custom_mvt_vector_source_options>();
    if (value.minZoom != null) {
      result.ref.fields |= raw.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
      result.ref.min_zoom = value.minZoom!;
    }
    if (value.maxZoom != null) {
      result.ref.fields |= raw.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
      result.ref.max_zoom = value.maxZoom!;
    }
    final registered = registration = roots.registerQueued({
      raw.MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE:
          (batch, index, delivery) => _deliverCustomSourceFetchTileCallback(
            value.fetchTile,
            batch,
            index,
            delivery,
          ),
      if (value.cancelTile != null)
        raw.MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE:
            (batch, index, delivery) => _deliverCustomSourceCancelTileCallback(
              value.cancelTile!,
              batch,
              index,
              delivery,
            ),
    });
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = Pointer.fromAddress(registered.token);
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

Pointer<raw.mln_premultiplied_rgba8_image> _writePremultipliedRgba8Image(
  PremultipliedRgba8Image value,
  Allocator arena, {
  Pointer<raw.mln_premultiplied_rgba8_image>? into,
}) {
  final result = into ?? arena<raw.mln_premultiplied_rgba8_image>();
  result.ref.size = sizeOf<raw.mln_premultiplied_rgba8_image>();
  result.ref.width = _nativeInteger(value.width, 0, 4294967295);
  result.ref.height = _nativeInteger(value.height, 0, 4294967295);
  result.ref.stride = _nativeInteger(value.stride, 0, 4294967295);
  final bytespixels = nativeBufferView(value.pixels, arena);
  result.ref.pixels = bytespixels.data.cast();
  result.ref.byte_length = bytespixels.size;
  return result;
}

Pointer<raw.mln_screen_point> _writeScreenPoint(
  ScreenPoint value,
  Allocator arena, {
  Pointer<raw.mln_screen_point>? into,
}) {
  final result = into ?? arena<raw.mln_screen_point>();
  result.ref.x = value.x;
  result.ref.y = value.y;
  return result;
}

ScreenPoint _readScreenPoint(raw.mln_screen_point source) =>
    ScreenPoint(x: source.x, y: source.y);

Pointer<raw.mln_unit_bezier> _writeUnitBezier(
  UnitBezier value,
  Allocator arena, {
  Pointer<raw.mln_unit_bezier>? into,
}) {
  final result = into ?? arena<raw.mln_unit_bezier>();
  result.ref.x1 = value.x1;
  result.ref.y1 = value.y1;
  result.ref.x2 = value.x2;
  result.ref.y2 = value.y2;
  return result;
}

CameraTransitionEnd _readCameraTransitionEnd(
  raw.mln_camera_transition_end source,
) => CameraTransitionEnd(
  outcome: CameraTransitionOutcome.fromRawValue(source.outcome),
  generation: source.generation,
);

void _deliverCameraTransitionEndCallback(
  CameraTransitionEndCallback callback,
  int batch,
  int index,
  Pointer<raw.mln_delivery> delivery,
) {
  final payload = delivery.ref.payload.camera_transition_end;
  callback(_readCameraTransitionEnd(payload.end.ref));
}

_NativeRegistration<raw.mln_camera_transition_handler>
_prepareCameraTransitionHandler(
  CameraTransitionEndCallback? value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_camera_transition_handler>();
    if (value == null) {
      return _NativeRegistration(result, () {}, arena.releaseAll);
    }
    final registered = registration = roots.registerQueued({
      raw.MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END: (batch, index, delivery) =>
          _deliverCameraTransitionEndCallback(value, batch, index, delivery),
    });
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = Pointer.fromAddress(registered.token);
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

Pointer<raw.mln_animation_options> _writeAnimationOptions(
  AnimationOptions value,
  Allocator arena,
  _NativeRegistrations registrations, {
  Pointer<raw.mln_animation_options>? into,
}) {
  final result = into ?? arena<raw.mln_animation_options>();
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
    result.ref.transition_id = value.transitionId!;
  }
  if (value.endHandler != null) {
    result.ref.end_handler = registrations
        .add(
          _prepareCameraTransitionHandler(
            value.endHandler!,
            registrations.roots,
          ),
        )
        .ref;
  }
  return result;
}

Pointer<raw.mln_camera_delta> _writeCameraDelta(
  CameraDelta value,
  Allocator arena,
  _NativeRegistrations registrations, {
  Pointer<raw.mln_camera_delta>? into,
}) {
  final result = into ?? arena<raw.mln_camera_delta>();
  result.ref.size = sizeOf<raw.mln_camera_delta>();
  if (value.offset != null) {
    result.ref.fields |= raw.MLN_CAMERA_DELTA_OFFSET;
    result.ref.offset = _writeScreenPoint(value.offset!, arena).ref;
  }
  if (value.scale != null) {
    result.ref.fields |= raw.MLN_CAMERA_DELTA_SCALE;
    result.ref.scale = value.scale!;
  }
  if (value.bearing != null) {
    result.ref.fields |= raw.MLN_CAMERA_DELTA_BEARING;
    result.ref.bearing = value.bearing!;
  }
  if (value.pitch != null) {
    result.ref.fields |= raw.MLN_CAMERA_DELTA_PITCH;
    result.ref.pitch = value.pitch!;
  }
  if (value.anchor != null) {
    result.ref.fields |= raw.MLN_CAMERA_DELTA_ANCHOR;
    result.ref.anchor = _writeScreenPoint(value.anchor!, arena).ref;
  }
  result.ref.animation = _writeAnimationOptions(
    value.animation,
    arena,
    registrations,
  ).ref;
  result.ref.gesture_phase = value.gesturePhase.rawValue;
  return result;
}

Pointer<raw.mln_metal_borrowed_texture> _writeMetalBorrowedTexture(
  MetalBorrowedTexture value,
  Allocator arena, {
  Pointer<raw.mln_metal_borrowed_texture>? into,
}) {
  final result = into ?? arena<raw.mln_metal_borrowed_texture>();
  result.ref.texture = Pointer<Void>.fromAddress(value.texture.address).cast();
  return result;
}

Pointer<raw.mln_metal_borrowed_texture_descriptor>
_writeMetalBorrowedTextureDescriptor(
  MetalBorrowedTextureDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_metal_borrowed_texture_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_metal_borrowed_texture_descriptor>();
  result.ref.size = sizeOf<raw.mln_metal_borrowed_texture_descriptor>();
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
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
  result.ref.textures = arena<raw.mln_metal_borrowed_texture>(
    value.textures.isEmpty ? 1 : value.textures.length,
  );
  result.ref.texture_count = value.textures.length;
  for (var index = 0; index < value.textures.length; index++) {
    _writeMetalBorrowedTexture(
      value.textures[index],
      arena,
      into: result.ref.textures + index,
    );
  }
  return result;
}

_NativeRegistration<raw.mln_wake> _prepareWake(
  Wake value,
  _CallbackRoots roots,
) {
  final native = _takeNativeRegistration(value);
  if (native == null) return roots.wake(value.callback);
  final arena = Arena();
  final result = arena<raw.mln_wake>();
  result.ref.callback = Pointer.fromAddress(native.callbacks[0].address);
  result.ref.user_data = Pointer.fromAddress(native.userData.address);
  result.ref.release_user_data = Pointer.fromAddress(
    native.releaseUserData.address,
  );
  return _NativeRegistration(result, native.release, arena.releaseAll);
}

_NativeRegistration<raw.mln_queue_lock> _prepareQueueLock(QueueLock value) {
  final native = _takeNativeRegistration(value)!;
  final arena = Arena();
  final result = arena<raw.mln_queue_lock>();
  result.ref.lock = Pointer.fromAddress(native.callbacks[0].address);
  result.ref.unlock = Pointer.fromAddress(native.callbacks[1].address);
  result.ref.user_data = Pointer.fromAddress(native.userData.address);
  result.ref.release_user_data = Pointer.fromAddress(
    native.releaseUserData.address,
  );
  return _NativeRegistration(result, native.release, arena.releaseAll);
}

Pointer<raw.mln_render_session_attach_options> _writeRenderSessionAttachOptions(
  RenderSessionAttachOptions value,
  Allocator arena,
  _NativeRegistrations registrations, {
  Pointer<raw.mln_render_session_attach_options>? into,
}) {
  final result = into ?? arena<raw.mln_render_session_attach_options>();
  result.ref.size = sizeOf<raw.mln_render_session_attach_options>();
  result.ref.driver = value.driver.rawValue;
  result.ref.requested_texture_ring_depth = _nativeInteger(
    value.requestedTextureRingDepth,
    0,
    4294967295,
  );
  result.ref.producer_sync_kind = value.producerSyncKind.rawValue;
  result.ref.frame_wake = registrations
      .add(_prepareWake(value.frameWake, registrations.roots))
      .ref;
  result.ref.driver_work_wake = registrations
      .add(_prepareWake(value.driverWorkWake, registrations.roots))
      .ref;
  if (value.queueLock != null) {
    result.ref.queue_lock = registrations
        .add(_prepareQueueLock(value.queueLock!))
        .ref;
  }
  return result;
}

Pointer<raw.mln_metal_context_descriptor> _writeMetalContextDescriptor(
  MetalContextDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_metal_context_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_metal_context_descriptor>();
  result.ref.device = Pointer<Void>.fromAddress(value.device.address).cast();
  return result;
}

Pointer<raw.mln_metal_owned_texture_descriptor>
_writeMetalOwnedTextureDescriptor(
  MetalOwnedTextureDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_metal_owned_texture_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_metal_owned_texture_descriptor>();
  result.ref.size = sizeOf<raw.mln_metal_owned_texture_descriptor>();
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
  result.ref.context = _writeMetalContextDescriptor(value.context, arena).ref;
  result.ref.pixel_format = value.pixelFormat;
  return result;
}

Pointer<raw.mln_metal_surface_descriptor> _writeMetalSurfaceDescriptor(
  MetalSurfaceDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_metal_surface_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_metal_surface_descriptor>();
  result.ref.size = sizeOf<raw.mln_metal_surface_descriptor>();
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
  result.ref.context = _writeMetalContextDescriptor(value.context, arena).ref;
  result.ref.layer = Pointer<Void>.fromAddress(value.layer.address).cast();
  return result;
}

Pointer<raw.mln_wgl_context_descriptor> _writeWglContextDescriptor(
  WglContextDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_wgl_context_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_wgl_context_descriptor>();
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

Pointer<raw.mln_egl_context_descriptor> _writeEglContextDescriptor(
  EglContextDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_egl_context_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_egl_context_descriptor>();
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

Pointer<raw.mln_web_gl_context_descriptor> _writeWebGlContextDescriptor(
  WebGlContextDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_web_gl_context_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_web_gl_context_descriptor>();
  result.ref.kind = value.kind.rawValue;
  result.ref.context = _nativeInteger(value.context, -2147483648, 2147483647);
  result.ref.canvas_selector = nativeStringView(
    value.canvasSelector,
    arena,
  ).value;
  return result;
}

Pointer<raw.mln_open_gl_context_descriptor> _writeOpenGlContextDescriptor(
  OpenGlContextDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_open_gl_context_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_open_gl_context_descriptor>();

  result.ref.ownership = value.ownership.rawValue;
  switch (value.data) {
    case OpenGlContextDescriptorDataWgl(:final value):
      result.ref.platform = 1;
      result.ref.data.wgl = _writeWglContextDescriptor(value, arena).ref;
    case OpenGlContextDescriptorDataEgl(:final value):
      result.ref.platform = 2;
      result.ref.data.egl = _writeEglContextDescriptor(value, arena).ref;
    case OpenGlContextDescriptorDataWebGl(:final value):
      result.ref.platform = 3;
      result.ref.data.web_gl = _writeWebGlContextDescriptor(value, arena).ref;
  }
  return result;
}

Pointer<raw.mln_open_gl_borrowed_texture> _writeOpenGlBorrowedTexture(
  OpenGlBorrowedTexture value,
  Allocator arena, {
  Pointer<raw.mln_open_gl_borrowed_texture>? into,
}) {
  final result = into ?? arena<raw.mln_open_gl_borrowed_texture>();
  result.ref.texture = _nativeInteger(value.texture, 0, 4294967295);
  return result;
}

Pointer<raw.mln_open_gl_borrowed_texture_descriptor>
_writeOpenGlBorrowedTextureDescriptor(
  OpenGlBorrowedTextureDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_open_gl_borrowed_texture_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_open_gl_borrowed_texture_descriptor>();
  result.ref.size = sizeOf<raw.mln_open_gl_borrowed_texture_descriptor>();
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
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
  result.ref.context = _writeOpenGlContextDescriptor(value.context, arena).ref;
  result.ref.textures = arena<raw.mln_open_gl_borrowed_texture>(
    value.textures.isEmpty ? 1 : value.textures.length,
  );
  result.ref.texture_count = value.textures.length;
  for (var index = 0; index < value.textures.length; index++) {
    _writeOpenGlBorrowedTexture(
      value.textures[index],
      arena,
      into: result.ref.textures + index,
    );
  }
  result.ref.target = _nativeInteger(value.target, 0, 4294967295);
  return result;
}

Pointer<raw.mln_open_gl_owned_texture_descriptor>
_writeOpenGlOwnedTextureDescriptor(
  OpenGlOwnedTextureDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_open_gl_owned_texture_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_open_gl_owned_texture_descriptor>();
  result.ref.size = sizeOf<raw.mln_open_gl_owned_texture_descriptor>();
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
  result.ref.context = _writeOpenGlContextDescriptor(value.context, arena).ref;
  return result;
}

Pointer<raw.mln_open_gl_surface_descriptor> _writeOpenGlSurfaceDescriptor(
  OpenGlSurfaceDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_open_gl_surface_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_open_gl_surface_descriptor>();
  result.ref.size = sizeOf<raw.mln_open_gl_surface_descriptor>();
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
  result.ref.context = _writeOpenGlContextDescriptor(value.context, arena).ref;
  result.ref.surface = Pointer<Void>.fromAddress(value.surface.address).cast();
  return result;
}

Pointer<raw.mln_vulkan_context_descriptor> _writeVulkanContextDescriptor(
  VulkanContextDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_vulkan_context_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_vulkan_context_descriptor>();
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

Pointer<raw.mln_vulkan_borrowed_texture> _writeVulkanBorrowedTexture(
  VulkanBorrowedTexture value,
  Allocator arena, {
  Pointer<raw.mln_vulkan_borrowed_texture>? into,
}) {
  final result = into ?? arena<raw.mln_vulkan_borrowed_texture>();
  result.ref.image = value.image;
  result.ref.image_view = value.imageView;
  return result;
}

Pointer<raw.mln_vulkan_borrowed_texture_descriptor>
_writeVulkanBorrowedTextureDescriptor(
  VulkanBorrowedTextureDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_vulkan_borrowed_texture_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_vulkan_borrowed_texture_descriptor>();
  result.ref.size = sizeOf<raw.mln_vulkan_borrowed_texture_descriptor>();
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
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
  result.ref.textures = arena<raw.mln_vulkan_borrowed_texture>(
    value.textures.isEmpty ? 1 : value.textures.length,
  );
  result.ref.texture_count = value.textures.length;
  for (var index = 0; index < value.textures.length; index++) {
    _writeVulkanBorrowedTexture(
      value.textures[index],
      arena,
      into: result.ref.textures + index,
    );
  }
  result.ref.format = _nativeInteger(value.format, 0, 4294967295);
  result.ref.initial_layout = _nativeInteger(
    value.initialLayout,
    0,
    4294967295,
  );
  result.ref.final_layout = _nativeInteger(value.finalLayout, 0, 4294967295);
  return result;
}

Pointer<raw.mln_vulkan_owned_texture_descriptor>
_writeVulkanOwnedTextureDescriptor(
  VulkanOwnedTextureDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_vulkan_owned_texture_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_vulkan_owned_texture_descriptor>();
  result.ref.size = sizeOf<raw.mln_vulkan_owned_texture_descriptor>();
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
  result.ref.context = _writeVulkanContextDescriptor(value.context, arena).ref;
  return result;
}

Pointer<raw.mln_vulkan_surface_descriptor> _writeVulkanSurfaceDescriptor(
  VulkanSurfaceDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_vulkan_surface_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_vulkan_surface_descriptor>();
  result.ref.size = sizeOf<raw.mln_vulkan_surface_descriptor>();
  result.ref.extent = _writeLogicalExtent(value.extent, arena).ref;
  result.ref.context = _writeVulkanContextDescriptor(value.context, arena).ref;
  result.ref.surface = value.surface;
  return result;
}

Pointer<raw.mln_camera_fit_options> _writeCameraFitOptions(
  CameraFitOptions value,
  Allocator arena, {
  Pointer<raw.mln_camera_fit_options>? into,
}) {
  final result = into ?? arena<raw.mln_camera_fit_options>();
  result.ref.size = sizeOf<raw.mln_camera_fit_options>();
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

Pointer<raw.mln_camera_options> _writeCameraOptions(
  CameraOptions value,
  Allocator arena, {
  Pointer<raw.mln_camera_options>? into,
}) {
  final result = into ?? arena<raw.mln_camera_options>();
  result.ref.size = sizeOf<raw.mln_camera_options>();
  if (value.center != null) {
    result.ref.fields |= raw.MLN_CAMERA_OPTION_CENTER;
    result.ref.center = _writeLatLng(value.center!, arena).ref;
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
      ? _readLatLng(source.center)
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

Pointer<raw.mln_lat_lng_bounds> _writeLatLngBounds(
  LatLngBounds value,
  Allocator arena, {
  Pointer<raw.mln_lat_lng_bounds>? into,
}) {
  final result = into ?? arena<raw.mln_lat_lng_bounds>();
  result.ref.southwest = _writeLatLng(value.southwest, arena).ref;
  result.ref.northeast = _writeLatLng(value.northeast, arena).ref;
  return result;
}

LatLngBounds _readLatLngBounds(raw.mln_lat_lng_bounds source) => LatLngBounds(
  southwest: _readLatLng(source.southwest),
  northeast: _readLatLng(source.northeast),
);

CameraQueryResult _readCameraQueryResult(raw.mln_camera_query_result source) =>
    CameraQueryResult(
      generation: source.generation,
      camera: _readCamera(source.camera),
    );

Pointer<raw.mln_feature_state_selector> _writeFeatureStateSelector(
  FeatureStateSelector value,
  Allocator arena, {
  Pointer<raw.mln_feature_state_selector>? into,
}) {
  final result = into ?? arena<raw.mln_feature_state_selector>();
  result.ref.size = sizeOf<raw.mln_feature_state_selector>();
  result.ref.source_id = nativeStringView(value.sourceId, arena).value;
  result.ref.source_layer_id = nativeStringView(
    value.sourceLayerId,
    arena,
  ).value;
  if (value.featureId != null) {
    result.ref.fields |= raw.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
    result.ref.feature_id = nativeStringView(value.featureId!, arena).value;
  }
  result.ref.state_key = nativeStringView(value.stateKey, arena).value;
  return result;
}

Pointer<raw.mln_map_tile_options> _writeMapTileOptions(
  MapTileOptions value,
  Allocator arena, {
  Pointer<raw.mln_map_tile_options>? into,
}) {
  final result = into ?? arena<raw.mln_map_tile_options>();
  result.ref.size = sizeOf<raw.mln_map_tile_options>();
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

Pointer<raw.mln_bound_options> _writeBoundOptions(
  BoundOptions value,
  Allocator arena, {
  Pointer<raw.mln_bound_options>? into,
}) {
  final result = into ?? arena<raw.mln_bound_options>();
  result.ref.size = sizeOf<raw.mln_bound_options>();
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

Pointer<raw.mln_vec3> _writeVec3(
  Vec3 value,
  Allocator arena, {
  Pointer<raw.mln_vec3>? into,
}) {
  final result = into ?? arena<raw.mln_vec3>();
  result.ref.x = value.x;
  result.ref.y = value.y;
  result.ref.z = value.z;
  return result;
}

Vec3 _readVec3(raw.mln_vec3 source) =>
    Vec3(x: source.x, y: source.y, z: source.z);

Pointer<raw.mln_quaternion> _writeQuaternion(
  Quaternion value,
  Allocator arena, {
  Pointer<raw.mln_quaternion>? into,
}) {
  final result = into ?? arena<raw.mln_quaternion>();
  result.ref.x = value.x;
  result.ref.y = value.y;
  result.ref.z = value.z;
  result.ref.w = value.w;
  return result;
}

Quaternion _readQuaternion(raw.mln_quaternion source) =>
    Quaternion(x: source.x, y: source.y, z: source.z, w: source.w);

Pointer<raw.mln_free_camera_options> _writeFreeCameraOptions(
  FreeCameraOptions value,
  Allocator arena, {
  Pointer<raw.mln_free_camera_options>? into,
}) {
  final result = into ?? arena<raw.mln_free_camera_options>();
  result.ref.size = sizeOf<raw.mln_free_camera_options>();
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

MapSnapshot _readMapSnapshot(raw.mln_map_snapshot source) => MapSnapshot(
  debugOptions: MapDebugFlag.fromRawValue(source.debug_options),
  generation: source.generation,
  fullyLoaded: source.fully_loaded,
  renderingStatsViewEnabled: source.rendering_stats_view_enabled,
  gestureInProgress: source.gesture_in_progress,
  transitionInProgress: source.transition_in_progress,
  eventMask: RuntimeEventFlag.fromRawValue(source.event_mask),
  latestRenderUpdateGeneration: source.latest_render_update_generation,
  tile: _readMapTileOptions(source.tile),
  bounds: _readBoundOptions(source.bounds),
  freeCamera: _readFreeCameraOptions(source.free_camera),
  view: _readMapView(source.view),
);

Pointer<raw.mln_image_stretch> _writeImageStretch(
  ImageStretch value,
  Allocator arena, {
  Pointer<raw.mln_image_stretch>? into,
}) {
  final result = into ?? arena<raw.mln_image_stretch>();
  result.ref.from = value.from;
  result.ref.to = value.to;
  return result;
}

ImageStretch _readImageStretch(raw.mln_image_stretch source) =>
    ImageStretch(from: source.from, to: source.to);

Pointer<raw.mln_image_content> _writeImageContent(
  ImageContent value,
  Allocator arena, {
  Pointer<raw.mln_image_content>? into,
}) {
  final result = into ?? arena<raw.mln_image_content>();
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

StyleImageInfo _readStyleImageInfo(
  raw.mln_style_image_info source,
) => StyleImageInfo(
  width: source.width,
  height: source.height,
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
  content: (source.fields & raw.MLN_STYLE_IMAGE_INFO_CONTENT) != 0
      ? _readImageContent(source.content)
      : null,
  textFitWidth: (source.fields & raw.MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH) != 0
      ? StyleImageTextFit.fromRawValue(source.text_fit_width)
      : null,
  textFitHeight: (source.fields & raw.MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT) != 0
      ? StyleImageTextFit.fromRawValue(source.text_fit_height)
      : null,
  pixelRatio: source.pixel_ratio,
  sdf: source.sdf,
);

StyleLayerInfo _readStyleLayerInfo(raw.mln_style_layer_info source) =>
    StyleLayerInfo(
      id: utf8.decode(_copyBufferView(source.id)),
      type: utf8.decode(_copyBufferView(source.type)),
      sourceId: utf8.decode(_copyBufferView(source.source_id)),
      sourceLayer: utf8.decode(_copyBufferView(source.source_layer)),
      minZoom: source.min_zoom,
      maxZoom: source.max_zoom,
      visibility: StyleLayerVisibility.fromRawValue(source.visibility),
    );

StyleSourceTileInfo _readStyleSourceTileInfo(
  raw.mln_style_source_tile_info source,
) => StyleSourceTileInfo(
  tileUrls: List<String>.unmodifiable(
    List.generate(
      source.tile_url_count,
      (index) => utf8.decode(_copyBufferView(source.tile_urls[index])),
    ),
  ),
  minZoom: source.min_zoom,
  maxZoom: source.max_zoom,
  scheme: StyleTileScheme.fromRawValue(source.scheme),
);

StyleSourceInfo _readStyleSourceInfo(raw.mln_style_source_info source) =>
    StyleSourceInfo(
      id: utf8.decode(_copyBufferView(source.id)),
      type: StyleSourceType.fromRawValue(source.type),
      isVolatile: source.is_volatile,
      attribution: utf8.decode(_copyBufferView(source.attribution)),
      url: utf8.decode(_copyBufferView(source.url)),
      tileJson: (source.fields & raw.MLN_STYLE_SOURCE_INFO_TILE_JSON) != 0
          ? _readStyleSourceTileInfo(source.tile_json)
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

Pointer<raw.mln_style_transition_options> _writeStyleTransitionOptions(
  StyleTransitionOptions value,
  Allocator arena, {
  Pointer<raw.mln_style_transition_options>? into,
}) {
  final result = into ?? arena<raw.mln_style_transition_options>();
  result.ref.size = sizeOf<raw.mln_style_transition_options>();
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

Pointer<raw.mln_style_image_options> _writeStyleImageOptions(
  StyleImageOptions value,
  Allocator arena, {
  Pointer<raw.mln_style_image_options>? into,
}) {
  final result = into ?? arena<raw.mln_style_image_options>();
  result.ref.size = sizeOf<raw.mln_style_image_options>();
  if (value.stretchX != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_STRETCH_X;
    result.ref.stretch_x = arena<raw.mln_image_stretch>(
      value.stretchX!.isEmpty ? 1 : value.stretchX!.length,
    );
    result.ref.stretch_x_count = value.stretchX!.length;
    for (var index = 0; index < value.stretchX!.length; index++) {
      _writeImageStretch(
        value.stretchX![index],
        arena,
        into: result.ref.stretch_x + index,
      );
    }
  }
  if (value.stretchY != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
    result.ref.stretch_y = arena<raw.mln_image_stretch>(
      value.stretchY!.isEmpty ? 1 : value.stretchY!.length,
    );
    result.ref.stretch_y_count = value.stretchY!.length;
    for (var index = 0; index < value.stretchY!.length; index++) {
      _writeImageStretch(
        value.stretchY![index],
        arena,
        into: result.ref.stretch_y + index,
      );
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

Pointer<raw.mln_camera_update> _writeCameraUpdate(
  CameraUpdate value,
  Allocator arena,
  _NativeRegistrations registrations, {
  Pointer<raw.mln_camera_update>? into,
}) {
  final result = into ?? arena<raw.mln_camera_update>();
  result.ref.size = sizeOf<raw.mln_camera_update>();
  result.ref.mode = value.mode.rawValue;
  result.ref.camera = _writeCameraOptions(value.camera, arena).ref;
  result.ref.animation = _writeAnimationOptions(
    value.animation,
    arena,
    registrations,
  ).ref;
  result.ref.gesture_phase = value.gesturePhase.rawValue;
  return result;
}

RenderAbandonResult _readRenderAbandonResult(
  raw.mln_render_abandon_result source,
) => RenderAbandonResult(
  disposition: RenderAbandonDisposition.fromRawValue(source.disposition),
  quarantinedResourceCount: source.quarantined_resource_count,
);

List<RenderFrameResult> _readRenderFrameBatchView(
  raw.mln_render_frame_batch_view source,
) => (() {
  if (source.result_size < sizeOf<raw.mln_render_frame_result>()) {
    throwInvalidState('native record stride is too small');
  }
  return List<RenderFrameResult>.unmodifiable(
    List.generate(
      source.result_count,
      (index) => _readRenderFrameResult(
        (source.results.cast<Uint8>() + index * source.result_size)
            .cast<raw.mln_render_frame_result>()
            .ref,
      ),
    ),
  );
})();

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
  extent: _readLogicalExtent(source.extent),
  generation: source.generation,
  mapUpdateGeneration: source.map_update_generation,
  renderedUpdateGeneration: source.rendered_update_generation,
  frameGeneration: source.frame_generation,
  acquiredFrameCount: source.acquired_frame_count,
  pendingChanges: source.pending_changes,
);

Pointer<raw.mln_screen_box> _writeScreenBox(
  ScreenBox value,
  Allocator arena, {
  Pointer<raw.mln_screen_box>? into,
}) {
  final result = into ?? arena<raw.mln_screen_box>();
  result.ref.min = _writeScreenPoint(value.min, arena).ref;
  result.ref.max = _writeScreenPoint(value.max, arena).ref;
  return result;
}

Pointer<raw.mln_screen_line_string> _writeScreenLineString(
  ScreenLineString value,
  Allocator arena, {
  Pointer<raw.mln_screen_line_string>? into,
}) {
  final result = into ?? arena<raw.mln_screen_line_string>();
  result.ref.points = arena<raw.mln_screen_point>(
    value.points.isEmpty ? 1 : value.points.length,
  );
  result.ref.point_count = value.points.length;
  for (var index = 0; index < value.points.length; index++) {
    _writeScreenPoint(
      value.points[index],
      arena,
      into: result.ref.points + index,
    );
  }
  return result;
}

Pointer<raw.mln_rendered_query_geometry> _writeRenderedQueryGeometry(
  RenderedQueryGeometry value,
  Allocator arena, {
  Pointer<raw.mln_rendered_query_geometry>? into,
}) {
  final result = into ?? arena<raw.mln_rendered_query_geometry>();
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

Pointer<raw.mln_rendered_feature_query_options>
_writeRenderedFeatureQueryOptions(
  RenderedFeatureQueryOptions value,
  Allocator arena, {
  Pointer<raw.mln_rendered_feature_query_options>? into,
}) {
  final result = into ?? arena<raw.mln_rendered_feature_query_options>();
  result.ref.size = sizeOf<raw.mln_rendered_feature_query_options>();
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
  result.ref.filter = _nativeJsonView(value.filter, arena, member: true);
  return result;
}

RenderedFeature _readRenderedFeature(raw.mln_rendered_feature source) =>
    RenderedFeature(
      feature: _decodeJsonView(source.feature),
      layerId: utf8.decode(_copyBufferView(source.layer_id)),
      sourceId: utf8.decode(_copyBufferView(source.source_id)),
      sourceLayerId: utf8.decode(_copyBufferView(source.source_layer_id)),
      state: _decodeJsonView(source.state),
    );

Pointer<raw.mln_source_feature_query_options> _writeSourceFeatureQueryOptions(
  SourceFeatureQueryOptions value,
  Allocator arena, {
  Pointer<raw.mln_source_feature_query_options>? into,
}) {
  final result = into ?? arena<raw.mln_source_feature_query_options>();
  result.ref.size = sizeOf<raw.mln_source_feature_query_options>();
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
  result.ref.filter = _nativeJsonView(value.filter, arena, member: true);
  return result;
}

SourceFeature _readSourceFeature(raw.mln_source_feature source) =>
    SourceFeature(
      feature: _decodeJsonView(source.feature),
      sourceLayerId: utf8.decode(_copyBufferView(source.source_layer_id)),
    );

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

Pointer<raw.mln_frame_demand> _writeFrameDemand(
  FrameDemand value,
  Allocator arena, {
  Pointer<raw.mln_frame_demand>? into,
}) {
  final result = into ?? arena<raw.mln_frame_demand>();
  result.ref.size = sizeOf<raw.mln_frame_demand>();
  result.ref.flags = value.flags.rawValue;
  result.ref.token = value.token;
  return result;
}

Pointer<raw.mln_resource_response> _writeResourceResponse(
  ResourceResponse value,
  Allocator arena, {
  Pointer<raw.mln_resource_response>? into,
}) {
  final result = into ?? arena<raw.mln_resource_response>();
  result.ref.size = sizeOf<raw.mln_resource_response>();
  result.ref.status = value.status.rawValue;
  result.ref.error_reason = value.errorReason.rawValue;
  final bytesbytes = nativeBufferView(value.bytes, arena);
  result.ref.bytes = bytesbytes.data.cast();
  result.ref.byte_count = bytesbytes.size;
  result.ref.error_message = nativeStringView(value.errorMessage, arena).value;
  result.ref.must_revalidate = value.mustRevalidate;
  if (value.modifiedUnixMs != null) {
    result.ref.fields |= raw.MLN_RESOURCE_RESPONSE_MODIFIED;
    result.ref.modified_unix_ms = value.modifiedUnixMs!;
  }
  if (value.expiresUnixMs != null) {
    result.ref.fields |= raw.MLN_RESOURCE_RESPONSE_EXPIRES;
    result.ref.expires_unix_ms = value.expiresUnixMs!;
  }
  result.ref.etag = nativeStringView(value.etag, arena).value;
  if (value.retryAfterUnixMs != null) {
    result.ref.fields |= raw.MLN_RESOURCE_RESPONSE_RETRY_AFTER;
    result.ref.retry_after_unix_ms = value.retryAfterUnixMs!;
  }
  return result;
}

void _deliverResourceRequestCancelCallback(
  ResourceRequestCancelCallback callback,
  int batch,
  int index,
  Pointer<raw.mln_delivery> delivery,
) {
  callback();
}

_NativeRegistration<raw.mln_resource_request_cancel_handler>
_prepareResourceRequestCancelHandler(
  ResourceRequestCancelCallback value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_resource_request_cancel_handler>();
    result.ref.size = sizeOf<raw.mln_resource_request_cancel_handler>();
    final registered = registration = roots.registerQueued({
      raw.MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL: (batch, index, delivery) =>
          _deliverResourceRequestCancelCallback(value, batch, index, delivery),
    });
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = Pointer.fromAddress(registered.token);
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

Pointer<raw.mln_runtime_options> _writeRuntimeOptions(
  RuntimeOptions value,
  Allocator arena, {
  Pointer<raw.mln_runtime_options>? into,
}) {
  final result = into ?? arena<raw.mln_runtime_options>();
  result.ref.size = sizeOf<raw.mln_runtime_options>();
  result.ref.asset_path = nativeStringView(value.assetPath, arena).value;
  result.ref.cache_path = nativeStringView(value.cachePath, arena).value;
  return result;
}

Pointer<raw.mln_map_options> _writeMapOptions(
  MapOptions value,
  Allocator arena, {
  Pointer<raw.mln_map_options>? into,
}) {
  final result = into ?? arena<raw.mln_map_options>();
  result.ref.size = sizeOf<raw.mln_map_options>();
  result.ref.initial_extent = _writeLogicalExtent(
    value.initialExtent,
    arena,
  ).ref;
  result.ref.map_mode = value.mapMode.rawValue;
  result.ref.fast_pfor_enabled = value.fastPforEnabled;
  if (value.eventMask != null) {
    result.ref.fields |= raw.MLN_MAP_OPTION_EVENT_MASK;
    result.ref.event_mask = value.eventMask!.rawValue;
  }
  return result;
}

Pointer<raw.mln_offline_tile_pyramid_region_definition>
_writeOfflineTilePyramidRegionDefinition(
  OfflineTilePyramidRegionDefinition value,
  Allocator arena, {
  Pointer<raw.mln_offline_tile_pyramid_region_definition>? into,
}) {
  final result =
      into ?? arena<raw.mln_offline_tile_pyramid_region_definition>();
  result.ref.style_url = nativeStringView(value.styleUrl, arena).value;
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
  styleUrl: utf8.decode(_copyBufferView(source.style_url)),
  bounds: _readLatLngBounds(source.bounds),
  minZoom: source.min_zoom,
  maxZoom: source.max_zoom,
  pixelRatio: source.pixel_ratio,
  includeIdeographs: source.include_ideographs,
);

Pointer<raw.mln_offline_geometry_region_definition>
_writeOfflineGeometryRegionDefinition(
  OfflineGeometryRegionDefinition value,
  Allocator arena, {
  Pointer<raw.mln_offline_geometry_region_definition>? into,
}) {
  final result = into ?? arena<raw.mln_offline_geometry_region_definition>();
  result.ref.style_url = nativeStringView(value.styleUrl, arena).value;
  result.ref.geometry = _nativeJsonView(value.geometry, arena, member: true);
  result.ref.min_zoom = value.minZoom;
  result.ref.max_zoom = value.maxZoom;
  result.ref.pixel_ratio = value.pixelRatio;
  result.ref.include_ideographs = value.includeIdeographs;
  return result;
}

OfflineGeometryRegionDefinition _readOfflineGeometryRegionDefinition(
  raw.mln_offline_geometry_region_definition source,
) => OfflineGeometryRegionDefinition(
  styleUrl: utf8.decode(_copyBufferView(source.style_url)),
  geometry: _decodeJsonView(source.geometry),
  minZoom: source.min_zoom,
  maxZoom: source.max_zoom,
  pixelRatio: source.pixel_ratio,
  includeIdeographs: source.include_ideographs,
);

Pointer<raw.mln_offline_region_definition> _writeOfflineRegionDefinition(
  OfflineRegionDefinition value,
  Allocator arena, {
  Pointer<raw.mln_offline_region_definition>? into,
}) {
  final result = into ?? arena<raw.mln_offline_region_definition>();
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
    case OfflineRegionDefinitionUnknown():
      throwInvalidArgument('unknown native union variant cannot be submitted');
  }
  return result;
}

OfflineRegionDefinition _readOfflineRegionDefinition(
  raw.mln_offline_region_definition source, {
  Uint8List Function()? rawRecord,
}) => switch (source.type) {
  1 => OfflineRegionDefinition.tilePyramid(
    _readOfflineTilePyramidRegionDefinition(source.data.tile_pyramid),
  ),
  2 => OfflineRegionDefinition.geometry(
    _readOfflineGeometryRegionDefinition(source.data.geometry),
  ),
  final tag => OfflineRegionDefinitionUnknown(
    tag,
    rawRecord?.call() ??
        withNativeArena((arena) {
          final copy = arena<raw.mln_offline_region_definition>()..ref = source;
          return Uint8List.fromList(
            copy.cast<Uint8>().asTypedList(
              sizeOf<raw.mln_offline_region_definition>(),
            ),
          );
        }),
  ),
};

OfflineRegionInfo _readOfflineRegionInfo(raw.mln_offline_region_info source) =>
    OfflineRegionInfo(
      id: source.id,
      definition: _readOfflineRegionDefinition(source.definition),
      metadata: Uint8List.fromList(
        source.metadata.cast<Uint8>().asTypedList(source.metadata_size),
      ),
    );

OfflineRegionStatus _readOfflineRegionStatus(
  raw.mln_offline_region_status source,
) => OfflineRegionStatus(
  downloadState: OfflineRegionDownloadState.fromRawValue(source.download_state),
  completedResourceCount: source.completed_resource_count,
  completedResourceSize: source.completed_resource_size,
  completedTileCount: source.completed_tile_count,
  requiredTileCount: source.required_tile_count,
  completedTileSize: source.completed_tile_size,
  requiredResourceCount: source.required_resource_count,
  requiredResourceCountIsPrecise: source.required_resource_count_is_precise,
  complete: source.complete,
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
  limit: source.limit,
);

RuntimeEvent _readRuntimeEvent(
  raw.mln_runtime_event source, {
  Uint8List Function()? rawRecord,
}) => RuntimeEvent(
  type: RuntimeEventType.fromRawValue(source.type),
  sourceType: RuntimeEventSourceType.fromRawValue(source.source_type),
  source: source.source,
  generation: source.generation,
  code: source.code,
  message: utf8.decode(_copyBufferView(source.message)),
  payload: switch (source.payload_type) {
    1 => RuntimeEventPayloadRenderFrame(
      _readRuntimeEventRenderFrame(source.payload.render_frame),
    ),
    2 => RuntimeEventPayloadRenderMap(
      _readRuntimeEventRenderMap(source.payload.render_map),
    ),
    3 => RuntimeEventPayloadTileAction(
      _readRuntimeEventTileAction(source.payload.tile_action),
    ),
    4 => RuntimeEventPayloadOfflineRegionStatus(
      _readRuntimeEventOfflineRegionStatus(
        source.payload.offline_region_status,
      ),
    ),
    5 => RuntimeEventPayloadOfflineRegionResponseError(
      _readRuntimeEventOfflineRegionResponseError(
        source.payload.offline_region_response_error,
      ),
    ),
    6 => RuntimeEventPayloadOfflineRegionTileCountLimit(
      _readRuntimeEventOfflineRegionTileCountLimit(
        source.payload.offline_region_tile_count_limit,
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

List<RuntimeEvent> _readEventBatchView(raw.mln_event_batch_view source) => (() {
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
      ),
    ),
  );
})();

void _deliverRuntimeEventCallback(
  RuntimeEventCallback callback,
  int batch,
  int index,
  Pointer<raw.mln_delivery> delivery,
) {
  final payload = delivery.ref.payload.runtime_events;
  callback(_readEventBatchView(payload.events.ref));
}

_NativeRegistration<raw.mln_runtime_event_handler> _prepareRuntimeEventHandler(
  RuntimeEventCallback value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_runtime_event_handler>();
    result.ref.size = sizeOf<raw.mln_runtime_event_handler>();
    final registered = registration = roots.registerQueued({
      raw.MLN_DELIVERY_TYPE_RUNTIME_EVENTS: (batch, index, delivery) =>
          _deliverRuntimeEventCallback(value, batch, index, delivery),
    });
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = Pointer.fromAddress(registered.token);
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

Pointer<raw.mln_http_header> _writeHttpHeader(
  HttpHeader value,
  Allocator arena, {
  Pointer<raw.mln_http_header>? into,
}) {
  final result = into ?? arena<raw.mln_http_header>();
  result.ref.name = nativeStringView(value.name, arena).value;
  result.ref.value = nativeStringView(value.value, arena).value;
  return result;
}

Pointer<raw.mln_http_header_rule> _writeHttpHeaderRule(
  HttpHeaderRule value,
  Allocator arena, {
  Pointer<raw.mln_http_header_rule>? into,
}) {
  final result = into ?? arena<raw.mln_http_header_rule>();
  result.ref.size = sizeOf<raw.mln_http_header_rule>();
  result.ref.kinds = value.kinds.rawValue;
  result.ref.flags = value.flags.rawValue;
  result.ref.url = nativeStringView(value.url, arena).value;
  result.ref.headers = arena<raw.mln_http_header>(
    value.headers.isEmpty ? 1 : value.headers.length,
  );
  result.ref.header_count = value.headers.length;
  for (var index = 0; index < value.headers.length; index++) {
    _writeHttpHeader(
      value.headers[index],
      arena,
      into: result.ref.headers + index,
    );
  }
  return result;
}

_NativeRegistration<raw.mln_http_header_transform> _prepareHttpHeaderTransform(
  HttpHeaderTransform value,
) {
  final arena = Arena();
  try {
    final result = arena<raw.mln_http_header_transform>();
    result.ref.size = sizeOf<raw.mln_http_header_transform>();
    result.ref.rules = arena<raw.mln_http_header_rule>(
      value.rules.isEmpty ? 1 : value.rules.length,
    );
    result.ref.rule_count = value.rules.length;
    for (var index = 0; index < value.rules.length; index++) {
      _writeHttpHeaderRule(
        value.rules[index],
        arena,
        into: result.ref.rules + index,
      );
    }
    return _NativeRegistration(result, () {}, arena.releaseAll);
  } catch (_) {
    arena.releaseAll();
    rethrow;
  }
}

ResourceRange _readResourceRange(raw.mln_resource_range source) =>
    ResourceRange(start: source.start, end: source.end);

ResourceRequestInfo _readResourceRequestInfo(
  raw.mln_resource_request_info source,
) => ResourceRequestInfo(
  requestedUrl: utf8.decode(_copyBufferView(source.requested_url)),
  resolvedUrl: utf8.decode(_copyBufferView(source.resolved_url)),
  kind: ResourceKind.fromRawValue(source.kind),
  loadingMethod: ResourceLoadingMethod.fromRawValue(source.loading_method),
  priority: ResourcePriority.fromRawValue(source.priority),
  usage: ResourceUsage.fromRawValue(source.usage),
  storagePolicy: ResourceStoragePolicy.fromRawValue(source.storage_policy),
  range: (source.fields & raw.MLN_RESOURCE_REQUEST_RANGE) != 0
      ? _readResourceRange(source.range)
      : null,
  priorModifiedUnixMs:
      (source.fields & raw.MLN_RESOURCE_REQUEST_PRIOR_MODIFIED) != 0
      ? source.prior_modified_unix_ms
      : null,
  priorExpiresUnixMs:
      (source.fields & raw.MLN_RESOURCE_REQUEST_PRIOR_EXPIRES) != 0
      ? source.prior_expires_unix_ms
      : null,
  priorEtag: utf8.decode(_copyBufferView(source.prior_etag)),
  priorData: Uint8List.fromList(
    source.prior_data.cast<Uint8>().asTypedList(source.prior_data_size),
  ),
);

void _deliverResourceProviderCallback(
  ResourceProviderCallback callback,
  int batch,
  int index,
  Pointer<raw.mln_delivery> delivery,
) {
  final payload = delivery.ref.payload.resource_request;
  ResourceRequestHandle? owner;
  try {
    _check(raw.mln_delivery_batch_take(batch, index, nativeDiagnostic));
    final adopted = owner = ResourceRequestHandle._(
      _NativeResourceRequest(payload.handle),
    );
    callback(_readResourceRequestInfo(payload.request.ref), adopted);
  } catch (_) {
    owner?.close();
    rethrow;
  }
}

Pointer<raw.mln_resource_route> _writeResourceRoute(
  ResourceRoute value,
  Allocator arena, {
  Pointer<raw.mln_resource_route>? into,
}) {
  final result = into ?? arena<raw.mln_resource_route>();
  result.ref.size = sizeOf<raw.mln_resource_route>();
  result.ref.kinds = value.kinds.rawValue;
  result.ref.flags = value.flags.rawValue;
  result.ref.url = nativeStringView(value.url, arena).value;
  return result;
}

_NativeRegistration<raw.mln_resource_provider> _prepareResourceProvider(
  ResourceProvider value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_resource_provider>();
    result.ref.size = sizeOf<raw.mln_resource_provider>();
    result.ref.routes = arena<raw.mln_resource_route>(
      value.routes.isEmpty ? 1 : value.routes.length,
    );
    result.ref.route_count = value.routes.length;
    for (var index = 0; index < value.routes.length; index++) {
      _writeResourceRoute(
        value.routes[index],
        arena,
        into: result.ref.routes + index,
      );
    }
    final registered = registration = roots.registerQueued({
      raw.MLN_DELIVERY_TYPE_RESOURCE_REQUEST: (batch, index, delivery) =>
          _deliverResourceProviderCallback(
            value.callback,
            batch,
            index,
            delivery,
          ),
    });
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = Pointer.fromAddress(registered.token);
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

void _deliverResourceTransformCallback(
  ResourceTransformCallback callback,
  int batch,
  int index,
  Pointer<raw.mln_delivery> delivery,
) {
  final payload = delivery.ref.payload.resource_transform;
  ResourceTransformRequestHandle? owner;
  try {
    _check(raw.mln_delivery_batch_take(batch, index, nativeDiagnostic));
    final adopted = owner = ResourceTransformRequestHandle._(
      _NativeResourceTransformRequest(payload.request),
    );
    callback(
      ResourceKind.fromRawValue(payload.kind),
      utf8.decode(_copyBufferView(payload.url)),
      adopted,
    );
  } catch (_) {
    owner?.close();
    rethrow;
  }
}

_NativeRegistration<raw.mln_resource_transform> _prepareResourceTransform(
  ResourceTransformCallback value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_resource_transform>();
    result.ref.size = sizeOf<raw.mln_resource_transform>();
    final registered = registration = roots.registerQueued({
      raw.MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM: (batch, index, delivery) =>
          _deliverResourceTransformCallback(value, batch, index, delivery),
    });
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = Pointer.fromAddress(registered.token);
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

Pointer<raw.mln_style_expression_type> _writeStyleExpressionType(
  StyleExpressionType value,
  Allocator arena, {
  Pointer<raw.mln_style_expression_type>? into,
}) {
  final result = into ?? arena<raw.mln_style_expression_type>();
  result.ref.kind = value.kind.rawValue;
  result.ref.item_kind = value.itemKind.rawValue;
  result.ref.item_count = _nativeInteger(value.itemCount, 0, 4294967295);
  return result;
}

Pointer<raw.mln_style_expression_options> _writeStyleExpressionOptions(
  StyleExpressionOptions value,
  Allocator arena, {
  Pointer<raw.mln_style_expression_options>? into,
}) {
  final result = into ?? arena<raw.mln_style_expression_options>();
  result.ref.size = sizeOf<raw.mln_style_expression_options>();
  if (value.expectedType != null) {
    result.ref.fields |= raw.MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE;
    result.ref.expected_type = _writeStyleExpressionType(
      value.expectedType!,
      arena,
    ).ref;
  }
  return result;
}

Pointer<raw.mln_style_expression_inputs> _writeStyleExpressionInputs(
  StyleExpressionInputs value,
  Allocator arena, {
  Pointer<raw.mln_style_expression_inputs>? into,
}) {
  final result = into ?? arena<raw.mln_style_expression_inputs>();
  result.ref.size = sizeOf<raw.mln_style_expression_inputs>();
  if (value.zoom != null) {
    result.ref.fields |= raw.MLN_STYLE_EXPRESSION_INPUT_ZOOM;
    result.ref.zoom = value.zoom!;
  }
  if (value.colorRampParameter != null) {
    result.ref.fields |= raw.MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER;
    result.ref.color_ramp_parameter = value.colorRampParameter!;
  }
  if (value.elevation != null) {
    result.ref.fields |= raw.MLN_STYLE_EXPRESSION_INPUT_ELEVATION;
    result.ref.elevation = value.elevation!;
  }
  result.ref.feature = _nativeJsonView(value.feature, arena, member: true);
  result.ref.feature_state = _nativeJsonView(
    value.featureState,
    arena,
    member: true,
  );
  result.ref.global_state = _nativeJsonView(
    value.globalState,
    arena,
    member: true,
  );
  if (value.canonicalTileId != null) {
    result.ref.fields |= raw.MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
    result.ref.canonical_tile_id = _writeCanonicalTileId(
      value.canonicalTileId!,
      arena,
    ).ref;
  }
  result.ref.available_images = arena<raw.mln_buffer_view>(
    value.availableImages.isEmpty ? 1 : value.availableImages.length,
  );
  result.ref.available_image_count = value.availableImages.length;
  for (var index = 0; index < value.availableImages.length; index++) {
    result.ref.available_images[index] = nativeStringView(
      value.availableImages[index],
      arena,
    ).value;
  }
  return result;
}

StyleExpressionInfo _readStyleExpressionInfo(
  raw.mln_style_expression_info source,
) => StyleExpressionInfo(
  isFeatureConstant: source.is_feature_constant,
  isZoomConstant: source.is_zoom_constant,
);

final _resultCameraOptions = _CompletionValue(
  sizeOf<raw.mln_camera_options>(),
  (element) => _readCameraOptions(element.cast<raw.mln_camera_options>().ref),
);
final _resultCameraQueryResult = _CompletionValue(
  sizeOf<raw.mln_camera_query_result>(),
  (element) =>
      _readCameraQueryResult(element.cast<raw.mln_camera_query_result>().ref),
);
final _resultLatLng = _CompletionValue(
  sizeOf<raw.mln_lat_lng>(),
  (element) => _readLatLng(element.cast<raw.mln_lat_lng>().ref),
);
final _resultLatLngBounds = _CompletionValue(
  sizeOf<raw.mln_lat_lng_bounds>(),
  (element) => _readLatLngBounds(element.cast<raw.mln_lat_lng_bounds>().ref),
);
final _resultObjectOrNull = _CompletionValue(
  sizeOf<raw.mln_buffer_view>(),
  (element) => _decodeJsonView(element.cast<raw.mln_buffer_view>().ref),
);
final _resultOfflineRegionInfo = _CompletionValue(
  sizeOf<raw.mln_offline_region_info>(),
  (element) =>
      _readOfflineRegionInfo(element.cast<raw.mln_offline_region_info>().ref),
);
final _resultOfflineRegionStatus = _CompletionValue(
  sizeOf<raw.mln_offline_region_status>(),
  (element) => _readOfflineRegionStatus(
    element.cast<raw.mln_offline_region_status>().ref,
  ),
);
final _resultRenderedFeature = _CompletionValue(
  sizeOf<raw.mln_rendered_feature>(),
  (element) =>
      _readRenderedFeature(element.cast<raw.mln_rendered_feature>().ref),
);
final _resultSourceFeature = _CompletionValue(
  sizeOf<raw.mln_source_feature>(),
  (element) => _readSourceFeature(element.cast<raw.mln_source_feature>().ref),
);
final _resultString = _CompletionValue(
  sizeOf<raw.mln_buffer_view>(),
  (element) =>
      utf8.decode(_copyBufferView(element.cast<raw.mln_buffer_view>().ref)),
);
final _resultStyleImageInfo = _CompletionValue(
  sizeOf<raw.mln_style_image_info>(),
  (element) =>
      _readStyleImageInfo(element.cast<raw.mln_style_image_info>().ref),
);
final _resultStyleLayerInfo = _CompletionValue(
  sizeOf<raw.mln_style_layer_info>(),
  (element) =>
      _readStyleLayerInfo(element.cast<raw.mln_style_layer_info>().ref),
);
final _resultStyleSourceInfo = _CompletionValue(
  sizeOf<raw.mln_style_source_info>(),
  (element) =>
      _readStyleSourceInfo(element.cast<raw.mln_style_source_info>().ref),
);
final _resultStyleTransitionOptions = _CompletionValue(
  sizeOf<raw.mln_style_transition_options>(),
  (element) => _readStyleTransitionOptions(
    element.cast<raw.mln_style_transition_options>().ref,
  ),
);
final _resultTextureReadbackResult = _CompletionValue(
  sizeOf<raw.mln_texture_readback_result>(),
  (element) => _readTextureReadbackResult(
    element.cast<raw.mln_texture_readback_result>().ref,
  ),
);

/// Reports the C ABI contract version. The value is 0 while the ABI is
/// unstable, and will increment on each SemVer major release.
///
/// See `mln_c_version` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
int cVersion() {
  final nativeResult = raw.mln_c_version();
  return nativeResult;
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
void clearLogCallback() {
  ensureAbiVersion();
  return _check(raw.mln_log_clear_callback(nativeDiagnostic));
}

/// Controls which log severities MapLibre Native may dispatch asynchronously.
///
/// See `mln_log_set_async_severity_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
void setLogAsyncSeverityMask(LogSeverityFlag mask) {
  ensureAbiVersion();
  return _check(
    raw.mln_log_set_async_severity_mask(mask.rawValue, nativeDiagnostic),
  );
}

/// Installs a process-global MapLibre Native log callback.
///
/// See `mln_log_set_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
void setLogCallback(LogCallback handler) {
  ensureAbiVersion();
  final registrations = _NativeRegistrations(_globalCallbackRoots);
  _check(
    registrations.run(
      () => raw.mln_log_set_callback(
        registrations.add(_prepareLogHandler(handler, registrations.roots)),
        nativeDiagnostic,
      ),
    ),
  );
}

/// Computes the physical device-pixel size of a logical extent.
///
/// See `mln_logical_extent_physical_size` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
(int, int) logicalExtentPhysicalSize(LogicalExtent extent) =>
    withNativeArena((arena) {
      ensureAbiVersion();
      final outWidth = arena<Uint32>();
      final outHeight = arena<Uint32>();
      _check(
        raw.mln_logical_extent_physical_size(
          _writeLogicalExtent(extent, arena).ref,
          outWidth,
          outHeight,
          nativeDiagnostic,
        ),
      );
      return (outWidth.value, outHeight.value);
    });

/// Reads MapLibre Native's process-global network status.
///
/// See `mln_network_get_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
NetworkStatus getNetworkStatus() => withNativeArena((arena) {
  ensureAbiVersion();
  final outStatus = arena<Uint32>();
  _check(raw.mln_network_get_status(outStatus, nativeDiagnostic));
  return NetworkStatus.fromRawValue(outStatus.value);
});

/// Sets MapLibre Native's process-global network status.
///
/// See `mln_network_set_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
void setNetworkStatus(NetworkStatus status) {
  ensureAbiVersion();
  return _check(raw.mln_network_set_status(status.rawValue, nativeDiagnostic));
}

/// Returns OpenGL context providers supported by this build.
///
/// See `mln_open_gl_supported_context_provider_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
OpenGlContextProviderFlag openGlSupportedContextProviderMask() {
  ensureAbiVersion();
  final nativeResult = raw.mln_open_gl_supported_context_provider_mask();
  return OpenGlContextProviderFlag.fromRawValue(nativeResult);
}

/// Returns the process-wide `mln_plugin_register_v1` entry point; never null.
///
/// See `mln_plugin_get_register_function_v1` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
NativePointer getPluginRegisterFunctionV1() {
  ensureAbiVersion();
  final nativeResult = raw.mln_plugin_get_register_function_v1();
  return NativePointer(nativeResult.address);
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

/// Reports the render backends available in this native library build.
///
/// See `mln_supported_render_backend_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
RenderBackendFlag supportedRenderBackendMask() {
  ensureAbiVersion();
  final nativeResult = raw.mln_supported_render_backend_mask();
  return RenderBackendFlag.fromRawValue(nativeResult);
}

/// Issued `mln_acquired_frame` handle id.
extension type const _NativeAcquiredFrame(int raw) implements NativeHandle {}

/// Owner of one native `mln_acquired_frame` handle.
///
/// A rendered frame that a render session lends until its release.
///
/// See `mln_acquired_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class AcquiredFrameHandle implements Finalizable {
  AcquiredFrameHandle._(this._parent, _NativeAcquiredFrame handle)
    : _state = NativeHandleState(handle, 'AcquiredFrameHandle');
  // Keeps the parent owner reachable while this owner lives.
  // ignore: unused_field
  final RenderSessionHandle _parent;
  final NativeHandleState<_NativeAcquiredFrame> _state;
  _NativeAcquiredFrame get _handle => _state.handle;

  /// The native handle id.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'AcquiredFrameHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Consumes an acquired frame without consumer GPU synchronization, so it
  /// quarantines the frame's slot of the texture ring.
  ///
  /// See `mln_acquired_frame_discard` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void discard() => _state.close(
    (handle) => raw.mln_acquired_frame_discard(handle.raw, nativeDiagnostic),
  );

  /// Copies Metal-native metadata from an acquired frame.
  ///
  /// See `mln_acquired_frame_get_metal_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  T withMetalTexture<T>(T Function(ScopedMetalTextureFrame view) use) =>
      _NativeViewScope(
        _state,
        raw.mln_acquired_frame_view_begin,
        raw.mln_acquired_frame_view_end,
      ).use(
        (scope) => use(ScopedMetalTextureFrame._(scope, _getMetalTexture())),
      );

  MetalTextureFrame _getMetalTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_metal_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_metal_texture_frame>();
    _check(
      raw.mln_acquired_frame_get_metal_texture(
        _handle.raw,
        outFrame,
        nativeDiagnostic,
      ),
    );
    return _readMetalTextureFrame(outFrame.ref);
  });

  /// Copies OpenGL-native metadata from an acquired frame.
  ///
  /// See `mln_acquired_frame_get_open_gl_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  T withOpenGlTexture<T>(T Function(ScopedOpenGlTextureFrame view) use) =>
      _NativeViewScope(
        _state,
        raw.mln_acquired_frame_view_begin,
        raw.mln_acquired_frame_view_end,
      ).use(
        (scope) => use(ScopedOpenGlTextureFrame._(scope, _getOpenGlTexture())),
      );

  OpenGlTextureFrame _getOpenGlTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_open_gl_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_open_gl_texture_frame>();
    _check(
      raw.mln_acquired_frame_get_open_gl_texture(
        _handle.raw,
        outFrame,
        nativeDiagnostic,
      ),
    );
    return _readOpenGlTextureFrame(outFrame.ref);
  });

  /// Copies the producer synchronization for an acquired texture frame.
  ///
  /// See `mln_acquired_frame_get_producer_sync` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  T withProducerSync<T>(T Function(ScopedGpuSync view) use) => _NativeViewScope(
    _state,
    raw.mln_acquired_frame_view_begin,
    raw.mln_acquired_frame_view_end,
  ).use((scope) => use(ScopedGpuSync._(scope, _getProducerSync())));

  GpuSync _getProducerSync() => withNativeArena((arena) {
    final outSync = arena<raw.mln_gpu_sync>();
    outSync.ref.size = sizeOf<raw.mln_gpu_sync>();
    _check(
      raw.mln_acquired_frame_get_producer_sync(
        _handle.raw,
        outSync,
        nativeDiagnostic,
      ),
    );
    return _readGpuSync(outSync.ref);
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
  T withVulkanTexture<T>(T Function(ScopedVulkanTextureFrame view) use) =>
      _NativeViewScope(
        _state,
        raw.mln_acquired_frame_view_begin,
        raw.mln_acquired_frame_view_end,
      ).use(
        (scope) => use(ScopedVulkanTextureFrame._(scope, _getVulkanTexture())),
      );

  VulkanTextureFrame _getVulkanTexture() => withNativeArena((arena) {
    final outFrame = arena<raw.mln_vulkan_texture_frame>();
    outFrame.ref.size = sizeOf<raw.mln_vulkan_texture_frame>();
    _check(
      raw.mln_acquired_frame_get_vulkan_texture(
        _handle.raw,
        outFrame,
        nativeDiagnostic,
      ),
    );
    return _readVulkanTextureFrame(outFrame.ref);
  });

  /// Releases an acquired frame after the host's GPU work that reads it.
  ///
  /// See `mln_acquired_frame_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void release({GpuSync? consumerCompletion}) => _state.close(
    (handle) => withNativeArena(
      (arena) => raw.mln_acquired_frame_release(
        handle.raw,
        consumerCompletion == null
            ? nullptr
            : _writeGpuSync(consumerCompletion, arena),
        nativeDiagnostic,
      ),
    ),
  );
}

/// Issued `mln_geo_json_source_data` handle id.
extension type const _NativeGeoJsonSourceData(int raw)
    implements NativeHandle {}

/// Owner of one native `mln_geo_json_source_data` handle.
///
/// Prepared GeoJSON data that sources retain independently of its handle.
///
/// See `mln_geo_json_source_data` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class GeoJsonSourceDataHandle implements Finalizable {
  GeoJsonSourceDataHandle._(_NativeGeoJsonSourceData handle)
    : _state = NativeHandleState(handle, 'GeoJsonSourceDataHandle');
  final NativeHandleState<_NativeGeoJsonSourceData> _state;
  // ignore: unused_element
  _NativeGeoJsonSourceData get _handle => _state.handle;

  /// The native handle id.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'GeoJsonSourceDataHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Prepares GeoJSON source data for installation on a map.
  ///
  /// See `mln_geo_json_source_data_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  static GeoJsonSourceDataHandle create(
    Object? data, {
    GeoJsonSourceOptions? options,
  }) => withNativeArena((arena) {
    ensureAbiVersion();
    final outData = arena<Uint64>();
    _check(
      raw.mln_geo_json_source_data_create(
        _nativeJsonView(data, arena),
        options == null ? nullptr : _writeGeoJsonSourceOptions(options, arena),
        outData,
        nativeDiagnostic,
      ),
    );
    return _adoptOwned(
      outData.value,
      () => GeoJsonSourceDataHandle._(_NativeGeoJsonSourceData(outData.value)),
      (handle) {
        raw.mln_geo_json_source_data_release(handle);
      },
    );
  });

  /// Releases prepared GeoJSON source data.
  ///
  /// See `mln_geo_json_source_data_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  void close() => _state.close((handle) {
    raw.mln_geo_json_source_data_release(handle.raw);
    return nativeStatusOk;
  });
}

extension type MapHandleDiscarding._(MapHandle _owner) {
  _NativeMap get _handle => _owner._handle;
  _CallbackRoots get _callbackRoots => _owner._callbackRoots;
  void addCustomGeometrySource(
    String sourceId,
    CustomGeometrySourceOptions options,
  ) => _discard((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_add_custom_geometry_source(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        registrations.add(
          _prepareCustomGeometrySourceOptions(options, registrations.roots),
        ),
        completion,
        nativeDiagnostic,
      ),
    );
  });
  void addCustomMvtVectorSource(
    String sourceId,
    CustomMvtVectorSourceOptions options,
  ) => _discard((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_add_custom_mvt_vector_source(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        registrations.add(
          _prepareCustomMvtVectorSourceOptions(options, registrations.roots),
        ),
        completion,
        nativeDiagnostic,
      ),
    );
  });
  void addGeoJsonSourceData(String sourceId, GeoJsonSourceDataHandle data) =>
      _discard(
        (arena, completion) => raw.mln_map_add_geo_json_source_data(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          data._handle.raw,
          completion,
          nativeDiagnostic,
        ),
      );
  void addGeoJsonSourceUrl(
    String sourceId,
    String url, {
    GeoJsonSourceOptions? options,
  }) => _discard(
    (arena, completion) => raw.mln_map_add_geo_json_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeStringView(url, arena).value,
      options == null ? nullptr : _writeGeoJsonSourceOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void addImageSourceImage(
    String sourceId,
    List<LatLng> coordinates,
    PremultipliedRgba8Image image,
  ) => _discard((arena, completion) {
    final nativecoordinates = arena<raw.mln_lat_lng>(
      coordinates.isEmpty ? 1 : coordinates.length,
    );
    for (var index = 0; index < coordinates.length; index++) {
      _writeLatLng(coordinates[index], arena, into: nativecoordinates + index);
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
  void addImageSourceUrl(
    String sourceId,
    List<LatLng> coordinates,
    String url,
  ) => _discard((arena, completion) {
    final nativecoordinates = arena<raw.mln_lat_lng>(
      coordinates.isEmpty ? 1 : coordinates.length,
    );
    for (var index = 0; index < coordinates.length; index++) {
      _writeLatLng(coordinates[index], arena, into: nativecoordinates + index);
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
  void addStyleLayerJson(Object? layerJson, String beforeLayerId) => _discard(
    (arena, completion) => raw.mln_map_add_style_layer_json(
      _handle.raw,
      _nativeJsonView(layerJson, arena),
      nativeStringView(beforeLayerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void addStyleSourceJson(String sourceId, Object? sourceJson) => _discard(
    (arena, completion) => raw.mln_map_add_style_source_json(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _nativeJsonView(sourceJson, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void applyCameraDelta(CameraDelta delta) => _discard((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_apply_camera_delta(
        _handle.raw,
        _writeCameraDelta(delta, arena, registrations),
        completion,
        nativeDiagnostic,
      ),
    );
  });
  void beginCommandGroup() => _discard(
    (arena, completion) => raw.mln_map_begin_command_group(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );
  void cancelCameraTransition(int transitionId) => _discard(
    (arena, completion) => raw.mln_map_cancel_camera_transition(
      _handle.raw,
      transitionId,
      completion,
      nativeDiagnostic,
    ),
  );
  void cancelTransitions() => _discard(
    (arena, completion) => raw.mln_map_cancel_transitions(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );
  void dumpDebugLogs() => _discard(
    (arena, completion) =>
        raw.mln_map_dump_debug_logs(_handle.raw, completion, nativeDiagnostic),
  );
  void endCommandGroup() => _discard(
    (arena, completion) => raw.mln_map_end_command_group(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );
  void invalidateCustomGeometrySourceRegion(
    String sourceId,
    LatLngBounds bounds,
  ) => _discard(
    (arena, completion) => raw.mln_map_invalidate_custom_geometry_source_region(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeLatLngBounds(bounds, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );
  void invalidateCustomGeometrySourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) => _discard(
    (arena, completion) => raw.mln_map_invalidate_custom_geometry_source_tile(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );
  void invalidateCustomMvtVectorSourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) => _discard(
    (arena, completion) => raw.mln_map_invalidate_custom_mvt_vector_source_tile(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );
  void moveStyleLayer(String layerId, String beforeLayerId) => _discard(
    (arena, completion) => raw.mln_map_move_style_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView(beforeLayerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void removeFeatureState(FeatureStateSelector selector) => _discard(
    (arena, completion) => raw.mln_map_remove_feature_state(
      _handle.raw,
      _writeFeatureStateSelector(selector, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void removeStyleImage(String imageId) => _discard(
    (arena, completion) => raw.mln_map_remove_style_image(
      _handle.raw,
      nativeStringView(imageId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void removeStyleLayer(String layerId) => _discard(
    (arena, completion) => raw.mln_map_remove_style_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void removeStyleSource(String sourceId) => _discard(
    (arena, completion) => raw.mln_map_remove_style_source(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void requestRepaint() => _discard(
    (arena, completion) =>
        raw.mln_map_request_repaint(_handle.raw, completion, nativeDiagnostic),
  );
  void resize(LogicalExtent extent) => _discard(
    (arena, completion) => raw.mln_map_resize(
      _handle.raw,
      _writeLogicalExtent(extent, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );
  void setBounds(BoundOptions options) => _discard(
    (arena, completion) => raw.mln_map_set_bounds(
      _handle.raw,
      _writeBoundOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setCustomGeometrySourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Object? data,
  ) => _discard(
    (arena, completion) => raw.mln_map_set_custom_geometry_source_tile_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      _nativeJsonView(data, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setCustomGeometrySourceTileError(
    String sourceId,
    CanonicalTileId tileId,
    String message,
  ) => _discard(
    (arena, completion) => raw.mln_map_set_custom_geometry_source_tile_error(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      nativeStringView(message, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void setCustomMvtVectorSourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Uint8List data,
  ) => _discard(
    (arena, completion) => raw.mln_map_set_custom_mvt_vector_source_tile_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      nativeBufferView(data, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setCustomMvtVectorSourceTileError(
    String sourceId,
    CanonicalTileId tileId,
    String message,
  ) => _discard(
    (arena, completion) => raw.mln_map_set_custom_mvt_vector_source_tile_error(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      nativeStringView(message, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void setDebugOptions(MapDebugFlag options) => _discard(
    (arena, completion) => raw.mln_map_set_debug_options(
      _handle.raw,
      options.rawValue,
      completion,
      nativeDiagnostic,
    ),
  );
  void setEventMask(RuntimeEventFlag mask) => _discard(
    (arena, completion) => raw.mln_map_set_event_mask(
      _handle.raw,
      mask.rawValue,
      completion,
      nativeDiagnostic,
    ),
  );
  void setFeatureState(FeatureStateSelector selector, Object? state) =>
      _discard(
        (arena, completion) => raw.mln_map_set_feature_state(
          _handle.raw,
          _writeFeatureStateSelector(selector, arena),
          _nativeJsonView(state, arena),
          completion,
          nativeDiagnostic,
        ),
      );
  void setFreeCameraOptions(FreeCameraOptions options) => _discard(
    (arena, completion) => raw.mln_map_set_free_camera_options(
      _handle.raw,
      _writeFreeCameraOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setGeoJsonSourceData(String sourceId, GeoJsonSourceDataHandle data) =>
      _discard(
        (arena, completion) => raw.mln_map_set_geo_json_source_data(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          data._handle.raw,
          completion,
          nativeDiagnostic,
        ),
      );
  void setGeoJsonSourceSynchronousTiling(String sourceId, bool enabled) =>
      _discard(
        (arena, completion) =>
            raw.mln_map_set_geo_json_source_synchronous_tiling(
              _handle.raw,
              nativeStringView(sourceId, arena).value,
              enabled,
              completion,
              nativeDiagnostic,
            ),
      );
  void setGeoJsonSourceUrl(String sourceId, String url) => _discard(
    (arena, completion) => raw.mln_map_set_geo_json_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeStringView(url, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void setGlobalStateProperty(String propertyName, Object? value) => _discard(
    (arena, completion) => raw.mln_map_set_global_state_property(
      _handle.raw,
      nativeStringView(propertyName, arena).value,
      _nativeJsonView(value, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setImageSourceCoordinates(String sourceId, List<LatLng> coordinates) =>
      _discard((arena, completion) {
        final nativecoordinates = arena<raw.mln_lat_lng>(
          coordinates.isEmpty ? 1 : coordinates.length,
        );
        for (var index = 0; index < coordinates.length; index++) {
          _writeLatLng(
            coordinates[index],
            arena,
            into: nativecoordinates + index,
          );
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
  void setImageSourceImage(String sourceId, PremultipliedRgba8Image image) =>
      _discard(
        (arena, completion) => raw.mln_map_set_image_source_image(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          _writePremultipliedRgba8Image(image, arena),
          completion,
          nativeDiagnostic,
        ),
      );
  void setImageSourceUrl(String sourceId, String url) => _discard(
    (arena, completion) => raw.mln_map_set_image_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeStringView(url, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void setProjectionMode(ProjectionMode mode) => _discard(
    (arena, completion) => raw.mln_map_set_projection_mode(
      _handle.raw,
      _writeProjectionMode(mode, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setRenderingStatsViewEnabled(bool enabled) => _discard(
    (arena, completion) => raw.mln_map_set_rendering_stats_view_enabled(
      _handle.raw,
      enabled,
      completion,
      nativeDiagnostic,
    ),
  );
  void setStyleImage(
    String imageId,
    PremultipliedRgba8Image image, {
    StyleImageOptions? options,
  }) => _discard(
    (arena, completion) => raw.mln_map_set_style_image(
      _handle.raw,
      nativeStringView(imageId, arena).value,
      _writePremultipliedRgba8Image(image, arena),
      options == null ? nullptr : _writeStyleImageOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setStyleJson(Object? json) => _discard(
    (arena, completion) => raw.mln_map_set_style_json(
      _handle.raw,
      _nativeJsonView(json, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setStyleLayerProperty(
    String layerId,
    String propertyName,
    Object? value,
  ) => _discard(
    (arena, completion) => raw.mln_map_set_style_layer_property(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView(propertyName, arena).value,
      _nativeJsonView(value, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setStyleLightJson(Object? lightJson) => _discard(
    (arena, completion) => raw.mln_map_set_style_light_json(
      _handle.raw,
      _nativeJsonView(lightJson, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setStyleLightProperty(String propertyName, Object? value) => _discard(
    (arena, completion) => raw.mln_map_set_style_light_property(
      _handle.raw,
      nativeStringView(propertyName, arena).value,
      _nativeJsonView(value, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setStyleSourceVolatile(String sourceId, bool isVolatile) => _discard(
    (arena, completion) => raw.mln_map_set_style_source_volatile(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      isVolatile,
      completion,
      nativeDiagnostic,
    ),
  );
  void setStyleTransitionOptions(StyleTransitionOptions options) => _discard(
    (arena, completion) => raw.mln_map_set_style_transition_options(
      _handle.raw,
      _writeStyleTransitionOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setStyleUrl(String url) => _discard(
    (arena, completion) => raw.mln_map_set_style_url(
      _handle.raw,
      nativeStringView(url, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void setTileOptions(MapTileOptions options) => _discard(
    (arena, completion) => raw.mln_map_set_tile_options(
      _handle.raw,
      _writeMapTileOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setViewportOptions(MapViewportOptions options) => _discard(
    (arena, completion) => raw.mln_map_set_viewport_options(
      _handle.raw,
      _writeMapViewportOptions(options, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void updateCamera(CameraUpdate update) => _discard((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_update_camera(
        _handle.raw,
        _writeCameraUpdate(update, arena, registrations),
        completion,
        nativeDiagnostic,
      ),
    );
  });
}

/// Issued `mln_map` handle id.
extension type const _NativeMap(int raw) implements NativeHandle {}

/// Owner of one native `mln_map` handle.
///
/// A map, which holds map state independent of any render target.
///
/// See `mln_map` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class MapHandle implements Finalizable, _CallbackRootOwner {
  MapHandle._(this._parent, _NativeMap handle)
    : _state = NativeHandleState(handle, 'MapHandle');
  // Keeps the parent owner reachable while this owner lives.
  // ignore: unused_field
  final RuntimeHandle _parent;
  // Roots this owner's callback registrations for as long as it lives.
  @override
  final _callbackRoots = _CallbackRoots();
  final NativeHandleState<_NativeMap> _state;
  _NativeMap get _handle => _state.handle;
  MapHandleDiscarding get discarding => MapHandleDiscarding._(this);

  /// The native handle id, which runtime events report as their source.
  ///
  /// It stays readable after close, so events drained later still match
  /// this handle.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'MapHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Adds a custom geometry source.
  ///
  /// See `mln_map_add_custom_geometry_source` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addCustomGeometrySource(
    String sourceId,
    CustomGeometrySourceOptions options,
  ) => _command((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_add_custom_geometry_source(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        registrations.add(
          _prepareCustomGeometrySourceOptions(options, registrations.roots),
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
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_add_custom_mvt_vector_source(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        registrations.add(
          _prepareCustomMvtVectorSourceOptions(options, registrations.roots),
        ),
        completion,
        nativeDiagnostic,
      ),
    );
  });

  /// Adds a GeoJSON source with prepared inline data.
  ///
  /// See `mln_map_add_geo_json_source_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addGeoJsonSourceData(
    String sourceId,
    GeoJsonSourceDataHandle data,
  ) => _command(
    (arena, completion) => raw.mln_map_add_geo_json_source_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      data._handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Adds a GeoJSON source with URL data.
  ///
  /// See `mln_map_add_geo_json_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addGeoJsonSourceUrl(
    String sourceId,
    String url, {
    GeoJsonSourceOptions? options,
  }) => _command(
    (arena, completion) => raw.mln_map_add_geo_json_source_url(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      nativeStringView(url, arena).value,
      options == null ? nullptr : _writeGeoJsonSourceOptions(options, arena),
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
      _writeLatLng(coordinates[index], arena, into: nativecoordinates + index);
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
      _writeLatLng(coordinates[index], arena, into: nativecoordinates + index);
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

  /// Adds one style layer from a full style-spec layer JSON object.
  ///
  /// See `mln_map_add_style_layer_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> addStyleLayerJson(
    Object? layerJson,
    String beforeLayerId,
  ) => _command(
    (arena, completion) => raw.mln_map_add_style_layer_json(
      _handle.raw,
      _nativeJsonView(layerJson, arena),
      nativeStringView(beforeLayerId, arena).value,
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
    Object? sourceJson,
  ) => _command(
    (arena, completion) => raw.mln_map_add_style_source_json(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _nativeJsonView(sourceJson, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Submits one atomic relative camera update.
  ///
  /// See `mln_map_apply_camera_delta` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> applyCameraDelta(CameraDelta delta) =>
      _command((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_map_apply_camera_delta(
            _handle.raw,
            _writeCameraDelta(delta, arena, registrations),
            completion,
            nativeDiagnostic,
          ),
        );
      });

  /// Starts attachment of a ring of caller-owned Metal textures.
  ///
  /// See `mln_map_attach_metal_borrowed_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  NewRenderSession attachMetalBorrowedTexture(
    MetalBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_metal_borrowed_texture(
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
            (RenderSessionHandle._(this, _NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(
            raw.mln_render_session_dispose(handle, nullptr, nativeDiagnostic),
          );
        },
      ),
      NewRenderSession.new,
    );
  }

  /// Starts attachment of a session-owned Metal texture ring.
  ///
  /// See `mln_map_attach_metal_owned_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  NewRenderSession attachMetalOwnedTexture(
    MetalOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_metal_owned_texture(
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
            (RenderSessionHandle._(this, _NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(
            raw.mln_render_session_dispose(handle, nullptr, nativeDiagnostic),
          );
        },
      ),
      NewRenderSession.new,
    );
  }

  /// Starts attachment of a Metal surface target.
  ///
  /// See `mln_map_attach_metal_surface` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  NewRenderSession attachMetalSurface(
    MetalSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_metal_surface(
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
            (RenderSessionHandle._(this, _NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(
            raw.mln_render_session_dispose(handle, nullptr, nativeDiagnostic),
          );
        },
      ),
      NewRenderSession.new,
    );
  }

  /// Starts attachment of a ring of caller-owned OpenGL textures.
  ///
  /// See `mln_map_attach_open_gl_borrowed_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  NewRenderSession attachOpenGlBorrowedTexture(
    OpenGlBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_open_gl_borrowed_texture(
          _handle.raw,
          _writeOpenGlBorrowedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, _NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(
            raw.mln_render_session_dispose(handle, nullptr, nativeDiagnostic),
          );
        },
      ),
      NewRenderSession.new,
    );
  }

  /// Starts attachment of a session-owned OpenGL texture ring.
  ///
  /// See `mln_map_attach_open_gl_owned_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  NewRenderSession attachOpenGlOwnedTexture(
    OpenGlOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_open_gl_owned_texture(
          _handle.raw,
          _writeOpenGlOwnedTextureDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, _NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(
            raw.mln_render_session_dispose(handle, nullptr, nativeDiagnostic),
          );
        },
      ),
      NewRenderSession.new,
    );
  }

  /// Starts attachment of an OpenGL surface target.
  ///
  /// See `mln_map_attach_open_gl_surface` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  NewRenderSession attachOpenGlSurface(
    OpenGlSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_open_gl_surface(
          _handle.raw,
          _writeOpenGlSurfaceDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          outSession,
          completion,
          nativeDiagnostic,
        ),
      ),
      (handle) => _adoptOwned(
        handle,
        () =>
            (RenderSessionHandle._(this, _NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(
            raw.mln_render_session_dispose(handle, nullptr, nativeDiagnostic),
          );
        },
      ),
      NewRenderSession.new,
    );
  }

  /// Starts attachment of a ring of caller-owned Vulkan images.
  ///
  /// See `mln_map_attach_vulkan_borrowed_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  NewRenderSession attachVulkanBorrowedTexture(
    VulkanBorrowedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_vulkan_borrowed_texture(
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
            (RenderSessionHandle._(this, _NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(
            raw.mln_render_session_dispose(handle, nullptr, nativeDiagnostic),
          );
        },
      ),
      NewRenderSession.new,
    );
  }

  /// Starts attachment of a session-owned Vulkan texture ring.
  ///
  /// See `mln_map_attach_vulkan_owned_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  NewRenderSession attachVulkanOwnedTexture(
    VulkanOwnedTextureDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_vulkan_owned_texture(
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
            (RenderSessionHandle._(this, _NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(
            raw.mln_render_session_dispose(handle, nullptr, nativeDiagnostic),
          );
        },
      ),
      NewRenderSession.new,
    );
  }

  /// Starts attachment of a Vulkan surface target.
  ///
  /// See `mln_map_attach_vulkan_surface` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  NewRenderSession attachVulkanSurface(
    VulkanSurfaceDescriptor descriptor,
    RenderSessionAttachOptions options,
  ) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_vulkan_surface(
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
            (RenderSessionHandle._(this, _NativeRenderSession(handle))
              .._state.retain(registrations)),
        (handle) {
          _check(
            raw.mln_render_session_dispose(handle, nullptr, nativeDiagnostic),
          );
        },
      ),
      NewRenderSession.new,
    );
  }

  /// Begins a command group, which holds this map's render updates until the
  /// group ends.
  ///
  /// See `mln_map_begin_command_group` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> beginCommandGroup() => _command(
    (arena, completion) => raw.mln_map_begin_command_group(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered query for a camera that fits a GeoJSON geometry.
  ///
  /// See `mln_map_camera_for_geometry` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CameraOptions> cameraForGeometry(
    Object? geometry, {
    CameraFitOptions? fitOptions,
  }) => _query(
    _resultCameraOptions,
    (arena, completion) => raw.mln_map_camera_for_geometry(
      _handle.raw,
      _nativeJsonView(geometry, arena),
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
      _writeLatLng(coordinates[index], arena, into: nativecoordinates + index);
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

  /// Cancels the camera transitions of the commands whose animation carried
  /// transition_id, and leaves every other transition running.
  ///
  /// See `mln_map_cancel_camera_transition` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> cancelCameraTransition(int transitionId) =>
      _command(
        (arena, completion) => raw.mln_map_cancel_camera_transition(
          _handle.raw,
          transitionId,
          completion,
          nativeDiagnostic,
        ),
      );

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

  /// Ends the innermost command group that `mln_map_begin_command_group()`
  /// began.
  ///
  /// See `mln_map_end_command_group` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> endCommandGroup() => _command(
    (arena, completion) => raw.mln_map_end_command_group(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered camera read.
  ///
  /// See `mln_map_get_camera` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CameraQueryResult> getCamera() => _query(
    _resultCameraQueryResult,
    (arena, completion) =>
        raw.mln_map_get_camera(_handle.raw, completion, nativeDiagnostic),
  );

  /// Starts an ordered read of per-feature state from this map.
  ///
  /// See `mln_map_get_feature_state` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<Object?> getFeatureState(FeatureStateSelector selector) => _query(
    _resultObjectOrNull,
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
  Future<Object?> getGlobalState() => _query(
    _resultObjectOrNull,
    (arena, completion) =>
        raw.mln_map_get_global_state(_handle.raw, completion, nativeDiagnostic),
  );

  /// Copies image source coordinates.
  ///
  /// See `mln_map_get_image_source_coordinates` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<List<LatLng>> getImageSourceCoordinates(String sourceId) => _queryList(
    _resultLatLng,
    (arena, completion) => raw.mln_map_get_image_source_coordinates(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered copy of the last successfully parsed style document.
  ///
  /// See `mln_map_get_loaded_style_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<Object?> getLoadedStyleJson() => _query(
    _resultObjectOrNull,
    (arena, completion) => raw.mln_map_get_loaded_style_json(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Copies the map's latest published snapshot.
  ///
  /// See `mln_map_get_snapshot` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  MapSnapshot getSnapshot() => withNativeArena((arena) {
    final outSnapshot = arena<raw.mln_map_snapshot>();
    outSnapshot.ref.size = sizeOf<raw.mln_map_snapshot>();
    _check(
      raw.mln_map_get_snapshot(_handle.raw, outSnapshot, nativeDiagnostic),
    );
    return _readMapSnapshot(outSnapshot.ref);
  });

  /// Copies one complete runtime style image.
  ///
  /// See `mln_map_get_style_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<StyleImageInfo?> getStyleImage(String imageId) => _queryOptional(
    _resultStyleImageInfo,
    (arena, completion) => raw.mln_map_get_style_image(
      _handle.raw,
      nativeStringView(imageId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Copies the complete metadata of one style layer.
  ///
  /// See `mln_map_get_style_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<StyleLayerInfo?> getStyleLayer(String layerId) => _queryOptional(
    _resultStyleLayerInfo,
    (arena, completion) => raw.mln_map_get_style_layer(
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
  Future<Object?> getStyleLayerJson(String layerId) => _queryOptional(
    _resultObjectOrNull,
    (arena, completion) => raw.mln_map_get_style_layer_json(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Serializes one layer property as a style-spec JSON value.
  ///
  /// See `mln_map_get_style_layer_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<Object?> getStyleLayerProperty(String layerId, String propertyName) =>
      _queryOptional(
        _resultObjectOrNull,
        (arena, completion) => raw.mln_map_get_style_layer_property(
          _handle.raw,
          nativeStringView(layerId, arena).value,
          nativeStringView(propertyName, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Serializes one style light property as a style-spec JSON value.
  ///
  /// See `mln_map_get_style_light_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<Object?> getStyleLightProperty(String propertyName) => _queryOptional(
    _resultObjectOrNull,
    (arena, completion) => raw.mln_map_get_style_light_property(
      _handle.raw,
      nativeStringView(propertyName, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Copies the complete metadata of one style source.
  ///
  /// See `mln_map_get_style_source` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<StyleSourceInfo?> getStyleSource(String sourceId) => _queryOptional(
    _resultStyleSourceInfo,
    (arena, completion) => raw.mln_map_get_style_source(
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

  /// Starts an ordered copy of the last requested style URL.
  ///
  /// See `mln_map_get_style_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<String> getStyleUrl() => _query(
    _resultString,
    (arena, completion) =>
        raw.mln_map_get_style_url(_handle.raw, completion, nativeDiagnostic),
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

  /// Starts an ordered query of every style layer in style order.
  ///
  /// See `mln_map_list_style_layers` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<List<StyleLayerInfo>> listStyleLayers() => _queryList(
    _resultStyleLayerInfo,
    (arena, completion) => raw.mln_map_list_style_layers(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Lists every style source in style order.
  ///
  /// See `mln_map_list_style_sources` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<List<StyleSourceInfo>> listStyleSources() => _queryList(
    _resultStyleSourceInfo,
    (arena, completion) => raw.mln_map_list_style_sources(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Moves one style layer before another layer or to the top.
  ///
  /// See `mln_map_move_style_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> moveStyleLayer(
    String layerId,
    String beforeLayerId,
  ) => _command(
    (arena, completion) => raw.mln_map_move_style_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView(beforeLayerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Releases a map asynchronously.
  ///
  /// See `mln_map_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> close() => _state.closeAsync(
    (handle) => _submit(
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

  /// Requests one still image for a static or tile map, which renders itself.
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
    Object? data,
  ) => _command(
    (arena, completion) => raw.mln_map_set_custom_geometry_source_tile_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      _nativeJsonView(data, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Reports a custom geometry source error for one canonical tile.
  ///
  /// See `mln_map_set_custom_geometry_source_tile_error` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setCustomGeometrySourceTileError(
    String sourceId,
    CanonicalTileId tileId,
    String message,
  ) => _command(
    (arena, completion) => raw.mln_map_set_custom_geometry_source_tile_error(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      nativeStringView(message, arena).value,
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
  Future<CommandCompletion> setDebugOptions(MapDebugFlag options) => _command(
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
  Future<CommandCompletion> setEventMask(RuntimeEventFlag mask) => _command(
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
    Object? state,
  ) => _command(
    (arena, completion) => raw.mln_map_set_feature_state(
      _handle.raw,
      _writeFeatureStateSelector(selector, arena),
      _nativeJsonView(state, arena),
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
  /// See `mln_map_set_geo_json_source_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setGeoJsonSourceData(
    String sourceId,
    GeoJsonSourceDataHandle data,
  ) => _command(
    (arena, completion) => raw.mln_map_set_geo_json_source_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      data._handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Overrides one GeoJSON source's synchronous tiling at runtime.
  ///
  /// See `mln_map_set_geo_json_source_synchronous_tiling` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setGeoJsonSourceSynchronousTiling(
    String sourceId,
    bool enabled,
  ) => _command(
    (arena, completion) => raw.mln_map_set_geo_json_source_synchronous_tiling(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      enabled,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Updates one GeoJSON source to load data from a URL.
  ///
  /// See `mln_map_set_geo_json_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setGeoJsonSourceUrl(String sourceId, String url) =>
      _command(
        (arena, completion) => raw.mln_map_set_geo_json_source_url(
          _handle.raw,
          nativeStringView(sourceId, arena).value,
          nativeStringView(url, arena).value,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Submits a global-state JSON value. JSON null restores the style default.
  ///
  /// See `mln_map_set_global_state_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setGlobalStateProperty(
    String propertyName,
    Object? value,
  ) => _command(
    (arena, completion) => raw.mln_map_set_global_state_property(
      _handle.raw,
      nativeStringView(propertyName, arena).value,
      _nativeJsonView(value, arena),
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
      _writeLatLng(coordinates[index], arena, into: nativecoordinates + index);
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

  /// Queues an inline style JSON command. A committed command means the map
  /// applied the JSON, not that the style loaded: style loading events report
  /// the resource and parse results that follow.
  ///
  /// See `mln_map_set_style_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> setStyleJson(Object? json) => _command(
    (arena, completion) => raw.mln_map_set_style_json(
      _handle.raw,
      _nativeJsonView(json, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets one layer property using its MapLibre style-spec property name.
  ///
  /// See `mln_map_set_style_layer_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setStyleLayerProperty(
    String layerId,
    String propertyName,
    Object? value,
  ) => _command(
    (arena, completion) => raw.mln_map_set_style_layer_property(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      nativeStringView(propertyName, arena).value,
      _nativeJsonView(value, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets the style light from a style-spec light JSON object.
  ///
  /// See `mln_map_set_style_light_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<CommandCompletion> setStyleLightJson(Object? lightJson) => _command(
    (arena, completion) => raw.mln_map_set_style_light_json(
      _handle.raw,
      _nativeJsonView(lightJson, arena),
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
    Object? value,
  ) => _command(
    (arena, completion) => raw.mln_map_set_style_light_property(
      _handle.raw,
      nativeStringView(propertyName, arena).value,
      _nativeJsonView(value, arena),
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

  /// Queues a style URL command. A committed command means the map accepted the
  /// URL, not that the style loaded: style loading events report the network,
  /// decode, and parse results that follow.
  ///
  /// See `mln_map_set_style_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> setStyleUrl(String url) => _command(
    (arena, completion) => raw.mln_map_set_style_url(
      _handle.raw,
      nativeStringView(url, arena).value,
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

  /// Submits one atomic camera update.
  ///
  /// See `mln_map_update_camera` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> updateCamera(CameraUpdate update) =>
      _command((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_map_update_camera(
            _handle.raw,
            _writeCameraUpdate(update, arena, registrations),
            completion,
            nativeDiagnostic,
          ),
        );
      });
}

/// Issued `mln_map_projection` handle id.
extension type const _NativeMapProjection(int raw) implements NativeHandle {}

/// Owner of one native `mln_map_projection` handle.
///
/// A standalone projection of one map view.
///
/// See `mln_map_projection` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class MapProjectionHandle implements Finalizable {
  MapProjectionHandle._(_NativeMapProjection handle)
    : _state = NativeHandleState(handle, 'MapProjectionHandle');
  final NativeHandleState<_NativeMapProjection> _state;
  _NativeMapProjection get _handle => _state.handle;

  /// The native handle id.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'MapProjectionHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Creates a standalone projection of a view, such as the view of a rendered
  /// frame or of a map snapshot.
  ///
  /// See `mln_map_projection_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  static MapProjectionHandle create(MapView view) => withNativeArena((arena) {
    ensureAbiVersion();
    final outProjection = arena<Uint64>();
    _check(
      raw.mln_map_projection_create(
        _writeMapView(view, arena),
        outProjection,
        nativeDiagnostic,
      ),
    );
    return _adoptOwned(
      outProjection.value,
      () => MapProjectionHandle._(_NativeMapProjection(outProjection.value)),
      (handle) {
        raw.mln_map_projection_release(handle);
      },
    );
  });

  /// Copies the projection camera into out_camera.
  ///
  /// See `mln_map_projection_get_camera` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  CameraOptions getCamera() => withNativeArena((arena) {
    final outCamera = arena<raw.mln_camera_options>();
    outCamera.ref.size = sizeOf<raw.mln_camera_options>();
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

  /// Converts screen points to geographic coordinates.
  ///
  /// See `mln_map_projection_lat_lngs_for_pixels` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  List<LatLng> latLngsForPixels(List<ScreenPoint> points) =>
      withNativeArena((arena) {
        final nativepoints = arena<raw.mln_screen_point>(
          points.isEmpty ? 1 : points.length,
        );
        for (var index = 0; index < points.length; index++) {
          _writeScreenPoint(points[index], arena, into: nativepoints + index);
        }
        final outCoordinates = arena<raw.mln_lat_lng>(
          points.isEmpty ? 1 : points.length,
        );
        _check(
          raw.mln_map_projection_lat_lngs_for_pixels(
            _handle.raw,
            nativepoints,
            points.length,
            outCoordinates,
            nativeDiagnostic,
          ),
        );
        return List<LatLng>.unmodifiable(
          List.generate(
            points.length,
            (index) => _readLatLng(outCoordinates[index]),
          ),
        );
      });

  /// Converts screen points to unwrapped geographic coordinates.
  ///
  /// See `mln_map_projection_lat_lngs_for_pixels_unwrapped` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  List<LatLng> latLngsForPixelsUnwrapped(List<ScreenPoint> points) =>
      withNativeArena((arena) {
        final nativepoints = arena<raw.mln_screen_point>(
          points.isEmpty ? 1 : points.length,
        );
        for (var index = 0; index < points.length; index++) {
          _writeScreenPoint(points[index], arena, into: nativepoints + index);
        }
        final outCoordinates = arena<raw.mln_lat_lng>(
          points.isEmpty ? 1 : points.length,
        );
        _check(
          raw.mln_map_projection_lat_lngs_for_pixels_unwrapped(
            _handle.raw,
            nativepoints,
            points.length,
            outCoordinates,
            nativeDiagnostic,
          ),
        );
        return List<LatLng>.unmodifiable(
          List.generate(
            points.length,
            (index) => _readLatLng(outCoordinates[index]),
          ),
        );
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

  /// Converts geographic coordinates to screen points.
  ///
  /// See `mln_map_projection_pixels_for_lat_lngs` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  List<ScreenPoint> pixelsForLatLngs(List<LatLng> coordinates) =>
      withNativeArena((arena) {
        final nativecoordinates = arena<raw.mln_lat_lng>(
          coordinates.isEmpty ? 1 : coordinates.length,
        );
        for (var index = 0; index < coordinates.length; index++) {
          _writeLatLng(
            coordinates[index],
            arena,
            into: nativecoordinates + index,
          );
        }
        final outPoints = arena<raw.mln_screen_point>(
          coordinates.isEmpty ? 1 : coordinates.length,
        );
        _check(
          raw.mln_map_projection_pixels_for_lat_lngs(
            _handle.raw,
            nativecoordinates,
            coordinates.length,
            outPoints,
            nativeDiagnostic,
          ),
        );
        return List<ScreenPoint>.unmodifiable(
          List.generate(
            coordinates.length,
            (index) => _readScreenPoint(outPoints[index]),
          ),
        );
      });

  /// Releases a standalone projection.
  ///
  /// See `mln_map_projection_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  void close() => _state.close((handle) {
    raw.mln_map_projection_release(handle.raw);
    return nativeStatusOk;
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
}

extension type RenderSessionHandleDiscarding._(RenderSessionHandle _owner) {
  _NativeRenderSession get _handle => _owner._handle;
  void resize(LogicalExtent extent) => _discard(
    (arena, completion) => raw.mln_render_session_resize(
      _handle.raw,
      _writeLogicalExtent(extent, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );
}

/// Issued `mln_render_session` handle id.
extension type const _NativeRenderSession(int raw) implements NativeHandle {}

/// Owner of one native `mln_render_session` handle.
///
/// A render session, which renders one map to one render target.
///
/// See `mln_render_session` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class RenderSessionHandle implements Finalizable {
  RenderSessionHandle._(this._parent, _NativeRenderSession handle)
    : _state = NativeHandleState(handle, 'RenderSessionHandle');
  // Keeps the parent owner reachable while this owner lives.
  // ignore: unused_field
  final MapHandle _parent;
  final NativeHandleState<_NativeRenderSession> _state;
  _NativeRenderSession get _handle => _state.handle;
  RenderSessionHandleDiscarding get discarding =>
      RenderSessionHandleDiscarding._(this);

  /// The native handle id.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'RenderSessionHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Irreversibly ends session work and destroys the session's graphics objects
  /// where that is safe.
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
      () => AcquiredFrameHandle._(this, _NativeAcquiredFrame(outFrame.value)),
      (handle) {
        _check(raw.mln_acquired_frame_discard(handle, nativeDiagnostic));
      },
    );
  });

  /// Ends every frame demand that has not started with
  /// `MLN_RENDER_RESULT_NO_UPDATE` and completes after earlier driver work has
  /// finished. A running demand finishes its attempt and ends any wait for a
  /// map update or a free slot. After completion, a new frame demand is
  /// required to render again, except that a pending still image submits its
  /// next demand itself; see `mln_map_request_still_image()`.
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

  /// Consumes a session in any state and retires it, then runs completion, if
  /// given, once the host may destroy the target's graphics objects.
  ///
  /// See `mln_render_session_dispose` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  Future<void> close() => _state.closeAsync(
    (handle) => _submit(
      (arena, completion) => raw.mln_render_session_dispose(
        handle.raw,
        completion,
        nativeDiagnostic,
      ),
    ),
  );

  /// Consumes a session in any state and retires it, then runs completion, if
  /// given, once the host may destroy the target's graphics objects.
  ///
  /// See `mln_render_session_dispose` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void dispose() => _state.close(
    (handle) =>
        raw.mln_render_session_dispose(handle.raw, nullptr, nativeDiagnostic),
  );

  /// Drains every currently queued terminal frame result into an independently
  /// owned batch and borrows its view. The records remain stable until the
  /// batch is released.
  ///
  /// See `mln_render_session_drain_frame_results` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  List<RenderFrameResult> drainFrameResults() => withNativeArena((arena) {
    final outBatch = arena<Uint64>();
    final outView = arena<raw.mln_render_frame_batch_view>();
    outView.ref.size = sizeOf<raw.mln_render_frame_batch_view>();
    if (!_present(
      raw.mln_render_session_drain_frame_results(
        _handle.raw,
        outBatch,
        outView,
        nativeDiagnostic,
      ),
      raw.MLN_STATUS_NOT_READY,
    )) {
      return const [];
    }
    return _drain(outBatch.value, (batch) {
      raw.mln_render_frame_batch_release(batch);
    }, () => _readRenderFrameBatchView(outView.ref));
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

  /// Starts a feature-extension query against the latest driver state. The
  /// completion borrows one `mln_buffer_view` holding UTF-8 JSON (value_count
  /// 1), valid only for the callback.
  ///
  /// See `mln_render_session_query_feature_extensions` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
  Future<Object?> queryFeatureExtensions(
    String sourceId,
    Object? feature,
    String extensionValue,
    String extensionField,
    Object? arguments,
  ) => _query(
    _resultObjectOrNull,
    (arena, completion) => raw.mln_render_session_query_feature_extensions(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _nativeJsonView(feature, arena),
      nativeStringView(extensionValue, arena).value,
      nativeStringView(extensionField, arena).value,
      _nativeJsonView(arguments, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts a rendered-feature query against the last frame that the session
  /// rendered, so render after a camera or style change for a result that
  /// matches it.
  ///
  /// See `mln_render_session_query_rendered_features` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
  Future<List<RenderedFeature>> queryRenderedFeatures(
    RenderedQueryGeometry geometry, {
    RenderedFeatureQueryOptions? options,
  }) => _queryList(
    _resultRenderedFeature,
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

  /// Starts a source-feature query against the tiles that the source loaded for
  /// rendering, which it loads only while a visible layer uses it.
  ///
  /// See `mln_render_session_query_source_features` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
  Future<List<SourceFeature>> querySourceFeatures(
    String sourceId, {
    SourceFeatureQueryOptions? options,
  }) => _queryList(
    _resultSourceFeature,
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

  /// Reads back the latest frame of the session's owned texture as
  /// premultiplied RGBA8.
  ///
  /// See `mln_render_session_read_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<TextureReadbackResult> readTexture() => _query(
    _resultTextureReadbackResult,
    (arena, completion) => raw.mln_render_session_read_texture(
      _handle.raw,
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
  /// terminal result record. Caller-driver sessions require service on the
  /// graphics thread. Core-worker sessions need no host service.
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

  /// Starts an ordered logical resize of the driver and the map. The driver
  /// applies the extent in order with its other work, and later driver work
  /// does not wait for the map. The completion runs after both have applied it
  /// and the map has published its snapshot. Frames render once a map update
  /// matches the applied extent; static and tile maps publish that update for
  /// their next still image.
  ///
  /// See `mln_render_session_resize` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  Future<CommandCompletion> resize(LogicalExtent extent) => _command(
    (arena, completion) => raw.mln_render_session_resize(
      _handle.raw,
      _writeLogicalExtent(extent, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Services every queued item for a caller-graphics-thread driver. The first
  /// successful service call fixes the session's graphics-thread identity;
  /// later calls from another native thread return `MLN_STATUS_WRONG_THREAD`.
  /// The target context must be current. Core-worker sessions return
  /// `MLN_STATUS_INVALID_STATE`.
  ///
  /// An isolate cannot service a caller-graphics-thread driver, because Dart
  /// can resume it on another native thread after any `await`. Host native code
  /// services such a session by passing its `id` to
  /// `mln_render_session_service_driver_work` on the graphics thread.
  ///
  /// See `mln_render_session_service_driver_work` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void serviceDriverWork() => _check(
    raw.mln_render_session_service_driver_work(_handle.raw, nativeDiagnostic),
  );

  /// Starts an ordered replacement of every texture of a caller-owned Metal
  /// ring.
  ///
  /// See `mln_render_session_set_metal_borrowed_texture_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<void> setMetalBorrowedTextureTarget(
    MetalBorrowedTextureDescriptor descriptor,
  ) => _run(
    (arena, completion) =>
        raw.mln_render_session_set_metal_borrowed_texture_target(
          _handle.raw,
          _writeMetalBorrowedTextureDescriptor(descriptor, arena),
          completion,
          nativeDiagnostic,
        ),
  );

  /// Starts an ordered Metal surface replacement.
  ///
  /// See `mln_render_session_set_metal_surface_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  Future<void> setMetalSurfaceTarget(MetalSurfaceDescriptor descriptor) => _run(
    (arena, completion) => raw.mln_render_session_set_metal_surface_target(
      _handle.raw,
      _writeMetalSurfaceDescriptor(descriptor, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts an ordered replacement of every texture of a caller-owned OpenGL
  /// ring.
  ///
  /// See `mln_render_session_set_open_gl_borrowed_texture_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<void> setOpenGlBorrowedTextureTarget(
    OpenGlBorrowedTextureDescriptor descriptor,
  ) => _run(
    (arena, completion) =>
        raw.mln_render_session_set_open_gl_borrowed_texture_target(
          _handle.raw,
          _writeOpenGlBorrowedTextureDescriptor(descriptor, arena),
          completion,
          nativeDiagnostic,
        ),
  );

  /// Starts an ordered OpenGL surface replacement.
  ///
  /// See `mln_render_session_set_open_gl_surface_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  Future<void> setOpenGlSurfaceTarget(OpenGlSurfaceDescriptor descriptor) =>
      _run(
        (arena, completion) =>
            raw.mln_render_session_set_open_gl_surface_target(
              _handle.raw,
              _writeOpenGlSurfaceDescriptor(descriptor, arena),
              completion,
              nativeDiagnostic,
            ),
      );

  /// Starts an ordered replacement of every image of a caller-owned Vulkan
  /// ring.
  ///
  /// See `mln_render_session_set_vulkan_borrowed_texture_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<void> setVulkanBorrowedTextureTarget(
    VulkanBorrowedTextureDescriptor descriptor,
  ) => _run(
    (arena, completion) =>
        raw.mln_render_session_set_vulkan_borrowed_texture_target(
          _handle.raw,
          _writeVulkanBorrowedTextureDescriptor(descriptor, arena),
          completion,
          nativeDiagnostic,
        ),
  );

  /// Starts an ordered Vulkan surface replacement.
  ///
  /// See `mln_render_session_set_vulkan_surface_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  Future<void> setVulkanSurfaceTarget(VulkanSurfaceDescriptor descriptor) =>
      _run(
        (arena, completion) => raw.mln_render_session_set_vulkan_surface_target(
          _handle.raw,
          _writeVulkanSurfaceDescriptor(descriptor, arena),
          completion,
          nativeDiagnostic,
        ),
      );
}

/// Issued `mln_resource_request` handle id.
extension type const _NativeResourceRequest(int raw) implements NativeHandle {}

/// Owner of one native `mln_resource_request` handle.
///
/// A resource request that a resource provider handles.
///
/// See `mln_resource_request` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class ResourceRequestHandle implements Finalizable, _CallbackRootOwner {
  ResourceRequestHandle._(_NativeResourceRequest handle)
    : _state = NativeHandleState(handle, 'ResourceRequestHandle');
  // Roots this owner's callback registrations for as long as it lives.
  @override
  final _callbackRoots = _CallbackRoots();
  final NativeHandleState<_NativeResourceRequest> _state;
  _NativeResourceRequest get _handle => _state.handle;

  /// The native handle id.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'ResourceRequestHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Completes a C API resource provider request and consumes its handle.
  ///
  /// See `mln_resource_request_complete` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void complete(ResourceResponse response) => _state.answer(
    (handle) => withNativeArena(
      (arena) => raw.mln_resource_request_complete(
        handle.raw,
        _writeResourceResponse(response, arena),
        nativeDiagnostic,
      ),
    ),
  );

  /// Reports whether MapLibre has cancelled a C API resource provider request.
  ///
  /// See `mln_resource_request_is_cancelled` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  bool isCancelled() => withNativeArena((arena) {
    final outCancelled = arena<Bool>();
    _check(
      raw.mln_resource_request_is_cancelled(
        _handle.raw,
        outCancelled,
        nativeDiagnostic,
      ),
    );
    return outCancelled.value;
  });

  /// Hands an outstanding provider request to the built-in network loader and
  /// consumes its handle.
  ///
  /// See `mln_resource_request_pass_through` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void passThrough() => _state.answer(
    (handle) =>
        raw.mln_resource_request_pass_through(handle.raw, nativeDiagnostic),
  );

  /// Releases the provider's resource request handle without answering it.
  ///
  /// See `mln_resource_request_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void close() => _state.close((handle) {
    raw.mln_resource_request_release(handle.raw);
    return nativeStatusOk;
  });

  /// Registers a callback that runs when MapLibre cancels a C API resource
  /// provider request. A request that is already cancelled registers nothing
  /// and reports true through out_cancelled, and the caller handles that
  /// cancellation itself.
  ///
  /// See `mln_resource_request_set_cancel_callback` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  bool setCancelCallback(ResourceRequestCancelCallback handler) =>
      withNativeArena((arena) {
        final outCancelled = arena<Bool>();
        final registrations = _NativeRegistrations(_callbackRoots);
        _check(
          registrations.run(
            () => raw.mln_resource_request_set_cancel_callback(
              _handle.raw,
              registrations.add(
                _prepareResourceRequestCancelHandler(
                  handler,
                  registrations.roots,
                ),
              ),
              outCancelled,
              nativeDiagnostic,
            ),
            declined: () => outCancelled.value,
          ),
        );
        return outCancelled.value;
      });
}

/// Issued `mln_resource_transform_request` handle id.
extension type const _NativeResourceTransformRequest(int raw)
    implements NativeHandle {}

/// Owner of one native `mln_resource_transform_request` handle.
///
/// A resource URL transform that waits for its host's answer.
///
/// See `mln_resource_transform_request` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class ResourceTransformRequestHandle implements Finalizable {
  ResourceTransformRequestHandle._(_NativeResourceTransformRequest handle)
    : _state = NativeHandleState(handle, 'ResourceTransformRequestHandle');
  final NativeHandleState<_NativeResourceTransformRequest> _state;
  // ignore: unused_element
  _NativeResourceTransformRequest get _handle => _state.handle;

  /// The native handle id.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'ResourceTransformRequestHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Answers a resource transform request with a replacement URL and consumes
  /// its handle.
  ///
  /// See `mln_resource_transform_request_complete` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void complete(String url) => _state.answer(
    (handle) => withNativeArena(
      (arena) => raw.mln_resource_transform_request_complete(
        handle.raw,
        nativeStringView(url, arena).value,
        nativeDiagnostic,
      ),
    ),
  );

  /// Releases a resource transform request without answering it.
  ///
  /// See `mln_resource_transform_request_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void close() => _state.close((handle) {
    raw.mln_resource_transform_request_release(handle.raw);
    return nativeStatusOk;
  });
}

extension type RuntimeHandleDiscarding._(RuntimeHandle _owner) {
  _NativeRuntime get _handle => _owner._handle;
  _CallbackRoots get _callbackRoots => _owner._callbackRoots;
  void clearEventHandler() => _discard(
    (arena, completion) => raw.mln_runtime_clear_event_handler(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );
  void clearHttpHeaderTransform() => _discard(
    (arena, completion) => raw.mln_runtime_clear_http_header_transform(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );
  void clearResourceProvider() => _discard(
    (arena, completion) => raw.mln_runtime_clear_resource_provider(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );
  void clearResourceTransform() => _discard(
    (arena, completion) => raw.mln_runtime_clear_resource_transform(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );
  void setEventHandler(RuntimeEventCallback handler) =>
      _discard((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_runtime_set_event_handler(
            _handle.raw,
            registrations.add(
              _prepareRuntimeEventHandler(handler, registrations.roots),
            ),
            completion,
            nativeDiagnostic,
          ),
        );
      });
  void setHttpHeaderTransform(HttpHeaderTransform transform) =>
      _discard((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_runtime_set_http_header_transform(
            _handle.raw,
            registrations.add(_prepareHttpHeaderTransform(transform)),
            completion,
            nativeDiagnostic,
          ),
        );
      });
  void setResourceProvider(ResourceProvider provider) =>
      _discard((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_runtime_set_resource_provider(
            _handle.raw,
            registrations.add(
              _prepareResourceProvider(provider, registrations.roots),
            ),
            completion,
            nativeDiagnostic,
          ),
        );
      });
  void setResourceTransform(ResourceTransformCallback transform) =>
      _discard((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_runtime_set_resource_transform(
            _handle.raw,
            registrations.add(
              _prepareResourceTransform(transform, registrations.roots),
            ),
            completion,
            nativeDiagnostic,
          ),
        );
      });
}

/// Issued `mln_runtime` handle id.
extension type const _NativeRuntime(int raw) implements NativeHandle {}

/// Owner of one native `mln_runtime` handle.
///
/// A runtime: the native scheduler thread and event store for its maps.
///
/// See `mln_runtime` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class RuntimeHandle implements Finalizable, _CallbackRootOwner {
  RuntimeHandle._(_NativeRuntime handle)
    : _state = NativeHandleState(handle, 'RuntimeHandle');
  // Roots this owner's callback registrations for as long as it lives.
  @override
  final _callbackRoots = _CallbackRoots();
  final NativeHandleState<_NativeRuntime> _state;
  _NativeRuntime get _handle => _state.handle;
  RuntimeHandleDiscarding get discarding => RuntimeHandleDiscarding._(this);

  /// The native handle id, which runtime events report as their source.
  ///
  /// It stays readable after close, so events drained later still match
  /// this handle.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'RuntimeHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Stops event delivery and retires the previous handler.
  ///
  /// See `mln_runtime_clear_event_handler` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<CommandCompletion> clearEventHandler() => _command(
    (arena, completion) => raw.mln_runtime_clear_event_handler(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Clears the runtime-scoped outgoing HTTP header transform.
  ///
  /// See `mln_runtime_clear_http_header_transform` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<CommandCompletion> clearHttpHeaderTransform() => _command(
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
  Future<CommandCompletion> clearResourceProvider() => _command(
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
  Future<CommandCompletion> clearResourceTransform() => _command(
    (arena, completion) => raw.mln_runtime_clear_resource_transform(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Creates a runtime.
  ///
  /// See `mln_runtime_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  static RuntimeHandle create(RuntimeOptions options) =>
      withNativeArena((arena) {
        ensureAbiVersion();
        final outRuntime = arena<Uint64>();
        _check(
          raw.mln_runtime_create(
            _writeRuntimeOptions(options, arena),
            outRuntime,
            nativeDiagnostic,
          ),
        );
        return _adoptOwned(
          outRuntime.value,
          () => RuntimeHandle._(_NativeRuntime(outRuntime.value)),
          (handle) {
            _check(raw.mln_runtime_dispose(handle, nativeDiagnostic));
          },
        );
      });

  /// Starts creation of a map on the runtime.
  ///
  /// See `mln_runtime_create_map` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  NewMap createMap(MapOptions options) => _attach(
    (arena, completion, outMap) => raw.mln_runtime_create_map(
      _handle.raw,
      _writeMapOptions(options, arena),
      outMap,
      completion,
      nativeDiagnostic,
    ),
    (handle) => _adoptOwned(
      handle,
      () => MapHandle._(this, _NativeMap(handle)),
      (handle) {
        _check(raw.mln_map_dispose(handle, nativeDiagnostic));
      },
    ),
    NewMap.new,
  );

  /// Starts creating an offline region.
  ///
  /// See `mln_runtime_create_offline_region` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<OfflineRegionInfo> createOfflineRegion(
    OfflineRegionDefinition definition,
    Uint8List metadata,
  ) => _query(_resultOfflineRegionInfo, (arena, completion) {
    final bytesmetadata = nativeBufferView(metadata, arena);
    return raw.mln_runtime_create_offline_region(
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
  /// See `mln_runtime_delete_offline_region` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> deleteOfflineRegion(int regionId) => _run(
    (arena, completion) => raw.mln_runtime_delete_offline_region(
      _handle.raw,
      regionId,
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

  /// Reports which runtime-scoped event types this runtime queues.
  ///
  /// See `mln_runtime_get_event_mask` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  RuntimeEventFlag getEventMask() => withNativeArena((arena) {
    final outMask = arena<Uint64>();
    _check(
      raw.mln_runtime_get_event_mask(_handle.raw, outMask, nativeDiagnostic),
    );
    return RuntimeEventFlag.fromRawValue(outMask.value);
  });

  /// Starts getting one offline region by ID.
  ///
  /// See `mln_runtime_get_offline_region` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<OfflineRegionInfo?> getOfflineRegion(int regionId) => _queryOptional(
    _resultOfflineRegionInfo,
    (arena, completion) => raw.mln_runtime_get_offline_region(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts getting the current download status for an offline region.
  ///
  /// See `mln_runtime_get_offline_region_status` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<OfflineRegionStatus> getOfflineRegionStatus(int regionId) => _query(
    _resultOfflineRegionStatus,
    (arena, completion) => raw.mln_runtime_get_offline_region_status(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Invalidates cached resources for an offline region.
  ///
  /// See `mln_runtime_invalidate_offline_region` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> invalidateOfflineRegion(int regionId) => _run(
    (arena, completion) => raw.mln_runtime_invalidate_offline_region(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts listing the offline regions in the runtime database.
  ///
  /// See `mln_runtime_list_offline_regions` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<List<OfflineRegionInfo>> listOfflineRegions() => _queryList(
    _resultOfflineRegionInfo,
    (arena, completion) => raw.mln_runtime_list_offline_regions(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts merging offline regions from another MapLibre offline database.
  ///
  /// See `mln_runtime_merge_offline_regions` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<List<OfflineRegionInfo>> mergeOfflineRegions(
    String sideDatabasePath,
  ) => _queryList(
    _resultOfflineRegionInfo,
    (arena, completion) => raw.mln_runtime_merge_offline_regions(
      _handle.raw,
      nativeStringView(sideDatabasePath, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Starts release of a runtime after checking its children.
  ///
  /// See `mln_runtime_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> close() => _state.closeAsync(
    (handle) => _submit(
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

  /// Selects how runtime and map events reach the host.
  ///
  /// See `mln_runtime_set_event_handler` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<CommandCompletion> setEventHandler(RuntimeEventCallback handler) =>
      _command((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_runtime_set_event_handler(
            _handle.raw,
            registrations.add(
              _prepareRuntimeEventHandler(handler, registrations.roots),
            ),
            completion,
            nativeDiagnostic,
          ),
        );
      });

  /// Selects which runtime-scoped event types this runtime queues.
  ///
  /// See `mln_runtime_set_event_mask` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void setEventMask(RuntimeEventFlag mask) => _check(
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
  Future<CommandCompletion> setHttpHeaderTransform(
    HttpHeaderTransform transform,
  ) => _command((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_runtime_set_http_header_transform(
        _handle.raw,
        registrations.add(_prepareHttpHeaderTransform(transform)),
        completion,
        nativeDiagnostic,
      ),
    );
  });

  /// Starts a change to this runtime's maximum ambient cache size.
  ///
  /// See `mln_runtime_set_maximum_ambient_cache_size` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> setMaximumAmbientCacheSize(int size) => _run(
    (arena, completion) => raw.mln_runtime_set_maximum_ambient_cache_size(
      _handle.raw,
      size,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets an offline region's native download state.
  ///
  /// See `mln_runtime_set_offline_region_download_state` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> setOfflineRegionDownloadState(
    int regionId,
    OfflineRegionDownloadState state,
  ) => _run(
    (arena, completion) => raw.mln_runtime_set_offline_region_download_state(
      _handle.raw,
      regionId,
      state.rawValue,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Enables or disables runtime events for an offline region.
  ///
  /// See `mln_runtime_set_offline_region_observed` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> setOfflineRegionObserved(int regionId, bool observed) => _run(
    (arena, completion) => raw.mln_runtime_set_offline_region_observed(
      _handle.raw,
      regionId,
      observed,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Registers or replaces a runtime-scoped network resource provider.
  ///
  /// See `mln_runtime_set_resource_provider` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<CommandCompletion> setResourceProvider(ResourceProvider provider) =>
      _command((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_runtime_set_resource_provider(
            _handle.raw,
            registrations.add(
              _prepareResourceProvider(provider, registrations.roots),
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
  Future<CommandCompletion> setResourceTransform(
    ResourceTransformCallback transform,
  ) => _command((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_runtime_set_resource_transform(
        _handle.raw,
        registrations.add(
          _prepareResourceTransform(transform, registrations.roots),
        ),
        completion,
        nativeDiagnostic,
      ),
    );
  });

  /// Starts updating opaque binary metadata for an offline region.
  ///
  /// See `mln_runtime_update_offline_region_metadata` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<OfflineRegionInfo> updateOfflineRegionMetadata(
    int regionId,
    Uint8List metadata,
  ) => _query(_resultOfflineRegionInfo, (arena, completion) {
    final bytesmetadata = nativeBufferView(metadata, arena);
    return raw.mln_runtime_update_offline_region_metadata(
      _handle.raw,
      regionId,
      bytesmetadata.data.cast(),
      bytesmetadata.size,
      completion,
      nativeDiagnostic,
    );
  });
}

/// Issued `mln_style_expression` handle id.
extension type const _NativeStyleExpression(int raw) implements NativeHandle {}

/// Owner of one native `mln_style_expression` handle.
///
/// A parsed style expression, immutable and independent of any map.
///
/// See `mln_style_expression` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class StyleExpressionHandle implements Finalizable {
  StyleExpressionHandle._(_NativeStyleExpression handle)
    : _state = NativeHandleState(handle, 'StyleExpressionHandle');
  final NativeHandleState<_NativeStyleExpression> _state;
  _NativeStyleExpression get _handle => _state.handle;

  /// The native handle id.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'StyleExpressionHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Parses a style expression.
  ///
  /// See `mln_style_expression_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
  static StyleExpressionHandle create(
    Object? expression, {
    StyleExpressionOptions? options,
  }) => withNativeArena((arena) {
    ensureAbiVersion();
    final outExpression = arena<Uint64>();
    _check(
      raw.mln_style_expression_create(
        _nativeJsonView(expression, arena),
        options == null
            ? nullptr
            : _writeStyleExpressionOptions(options, arena),
        outExpression,
        nativeDiagnostic,
      ),
    );
    return _adoptOwned(
      outExpression.value,
      () =>
          StyleExpressionHandle._(_NativeStyleExpression(outExpression.value)),
      (handle) {
        raw.mln_style_expression_release(handle);
      },
    );
  });

  /// Evaluates a parsed expression and writes its result as UTF-8 JSON.
  ///
  /// See `mln_style_expression_evaluate` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
  Object? evaluate({StyleExpressionInputs? inputs}) => withNativeArena((arena) {
    final text = _CallerSizedText();
    final nativeInputs = inputs == null
        ? nullptr
        : _writeStyleExpressionInputs(inputs, arena);
    _check(
      text.call(
        arena,
        (textStorage, textCapacity, textLength) =>
            raw.mln_style_expression_evaluate(
              _handle.raw,
              nativeInputs,
              textStorage,
              textCapacity,
              textLength,
              nativeDiagnostic,
            ),
      ),
    );
    return _decodeJson(text.text);
  });

  /// Copies what a parsed expression depends on into out_info.
  ///
  /// See `mln_style_expression_get_info` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
  StyleExpressionInfo getInfo() => withNativeArena((arena) {
    final outInfo = arena<raw.mln_style_expression_info>();
    outInfo.ref.size = sizeOf<raw.mln_style_expression_info>();
    _check(
      raw.mln_style_expression_get_info(_handle.raw, outInfo, nativeDiagnostic),
    );
    return _readStyleExpressionInfo(outInfo.ref);
  });

  /// Releases a style expression.
  ///
  /// See `mln_style_expression_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
  void close() => _state.close((handle) {
    raw.mln_style_expression_release(handle.raw);
    return nativeStatusOk;
  });
}

/// A new map and the completion that reports it ready.
final class NewMap {
  const NewMap(this.map, this.ready);

  /// The map, which the caller owns before it is ready.
  final MapHandle map;

  /// Completes once the map is ready, or fails with its status.
  /// Await it or call `ignore()`, because the zone reports a failure
  /// that nothing handles.
  final Future<void> ready;
}

/// A new session and the completion that reports it ready.
final class NewRenderSession {
  const NewRenderSession(this.session, this.ready);

  /// The session, which the caller owns before it is ready.
  final RenderSessionHandle session;

  /// Completes once the session is ready, or fails with its status.
  /// Await it or call `ignore()`, because the zone reports a failure
  /// that nothing handles.
  final Future<void> ready;
}

/// A `MetalTextureFrame` whose native objects are readable only while the
/// callback of `AcquiredFrameHandle.withMetalTexture` runs. Its
/// other members stay readable after the callback returns.
final class ScopedMetalTextureFrame {
  ScopedMetalTextureFrame._(this._scope, this._value);
  final MetalTextureFrame _value;
  final _NativeViewScope _scope;
  int get generation => _value.generation;
  int get width => _value.width;
  int get height => _value.height;
  double get scaleFactor => _value.scaleFactor;
  int get frameId => _value.frameId;
  int get slot => _value.slot;
  ScopedNativePointer get texture => ScopedNativePointer(
    _value.texture.address,
    checkValid: _scope.checkActive,
    debugName: 'MetalTextureFrame.texture',
  );
  ScopedNativePointer get device => ScopedNativePointer(
    _value.device.address,
    checkValid: _scope.checkActive,
    debugName: 'MetalTextureFrame.device',
  );
  int get pixelFormat => _value.pixelFormat;
}

/// A `OpenGlTextureFrame` whose native objects are readable only while the
/// callback of `AcquiredFrameHandle.withOpenGlTexture` runs. Its
/// other members stay readable after the callback returns.
final class ScopedOpenGlTextureFrame {
  ScopedOpenGlTextureFrame._(this._scope, this._value);
  final OpenGlTextureFrame _value;
  final _NativeViewScope _scope;
  int get generation => _value.generation;
  int get width => _value.width;
  int get height => _value.height;
  double get scaleFactor => _value.scaleFactor;
  int get frameId => _value.frameId;
  int get slot => _value.slot;
  int get texture => _scope.active(_value).texture;
  int get target => _value.target;
  int get internalFormat => _value.internalFormat;
  int get format => _value.format;
  int get type => _value.type;
}

/// A `GpuSync` whose native objects are readable only while the
/// callback of `AcquiredFrameHandle.withProducerSync` runs. Its
/// other members stay readable after the callback returns.
final class ScopedGpuSync {
  ScopedGpuSync._(this._scope, this._value);
  final GpuSync _value;
  final _NativeViewScope _scope;
  GpuSyncKind get kind => _value.kind;
  int get handle => _scope.active(_value).handle;
}

/// A `VulkanTextureFrame` whose native objects are readable only while the
/// callback of `AcquiredFrameHandle.withVulkanTexture` runs. Its
/// other members stay readable after the callback returns.
final class ScopedVulkanTextureFrame {
  ScopedVulkanTextureFrame._(this._scope, this._value);
  final VulkanTextureFrame _value;
  final _NativeViewScope _scope;
  int get generation => _value.generation;
  int get width => _value.width;
  int get height => _value.height;
  double get scaleFactor => _value.scaleFactor;
  int get frameId => _value.frameId;
  int get slot => _value.slot;
  int get image => _scope.active(_value).image;
  int get imageView => _scope.active(_value).imageView;
  ScopedNativePointer get device => ScopedNativePointer(
    _value.device.address,
    checkValid: _scope.checkActive,
    debugName: 'VulkanTextureFrame.device',
  );
  int get format => _value.format;
  int get layout => _value.layout;
}
