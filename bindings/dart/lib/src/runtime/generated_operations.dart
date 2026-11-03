// Generated from the C headers by tools/bindgen. Do not edit.
part of 'runtime.dart';

MetalTextureFrame _readMetalTextureFrame(raw.mln_metal_texture_frame source) =>
    MetalTextureFrame(
      width: source.width,
      height: source.height,
      scaleFactor: source.scale_factor,
      slot: source.slot,
      texture: NativePointer(source.texture.address),
      device: NativePointer(source.device.address),
      pixelFormat: source.pixel_format,
    );

OpenGlTextureFrame _readOpenGlTextureFrame(
  raw.mln_open_gl_texture_frame source,
) => OpenGlTextureFrame(
  width: source.width,
  height: source.height,
  scaleFactor: source.scale_factor,
  slot: source.slot,
  texture: source.texture,
  target: source.target,
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
  result.ref.value = value.value;
  return result;
}

GpuSync _readGpuSync(raw.mln_gpu_sync source) => GpuSync(
  kind: GpuSyncKind.fromRawValue(source.kind),
  handle: source.handle,
  value: source.value,
);

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
  result.ref.axonometric = value.axonometric;
  result.ref.x_skew = value.xSkew;
  result.ref.y_skew = value.ySkew;
  return result;
}

ProjectionMode _readProjectionMode(raw.mln_projection_mode source) =>
    ProjectionMode(
      axonometric: source.axonometric,
      xSkew: source.x_skew,
      ySkew: source.y_skew,
    );

Pointer<raw.mln_map_viewport> _writeMapViewport(
  MapViewport value,
  Allocator arena, {
  Pointer<raw.mln_map_viewport>? into,
}) {
  final result = into ?? arena<raw.mln_map_viewport>();
  result.ref.north_orientation = value.northOrientation.rawValue;
  result.ref.constrain_mode = value.constrainMode.rawValue;
  result.ref.viewport_mode = value.viewportMode.rawValue;
  result.ref.frustum_offset = _writeEdgeInsets(value.frustumOffset, arena).ref;
  return result;
}

MapViewport _readMapViewport(raw.mln_map_viewport source) => MapViewport(
  northOrientation: NorthOrientation.fromRawValue(source.north_orientation),
  constrainMode: ConstrainMode.fromRawValue(source.constrain_mode),
  viewportMode: ViewportMode.fromRawValue(source.viewport_mode),
  frustumOffset: _readEdgeInsets(source.frustum_offset),
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
  result.ref.viewport = _writeMapViewport(value.viewport, arena).ref;
  return result;
}

MapView _readMapView(raw.mln_map_view source) => MapView(
  camera: _readCamera(source.camera),
  extent: _readLogicalExtent(source.extent),
  projectionMode: _readProjectionMode(source.projection_mode),
  viewport: _readMapViewport(source.viewport),
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
      mode: RenderMode.fromRawValue(source.mode),
    );

VulkanTextureFrame _readVulkanTextureFrame(
  raw.mln_vulkan_texture_frame source,
) => VulkanTextureFrame(
  width: source.width,
  height: source.height,
  scaleFactor: source.scale_factor,
  slot: source.slot,
  image: source.image,
  imageView: source.image_view,
  device: NativePointer(source.device.address),
  format: source.format,
  layout: source.layout,
);

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

Pointer<raw.mln_logical_size> _writeLogicalSize(
  LogicalSize value,
  Allocator arena, {
  Pointer<raw.mln_logical_size>? into,
}) {
  final result = into ?? arena<raw.mln_logical_size>();
  result.ref.width = _nativeInteger(value.width, 0, 4294967295);
  result.ref.height = _nativeInteger(value.height, 0, 4294967295);
  return result;
}

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
    result.ref.cluster_min_points = value.clusterMinPoints!;
  }
  if (value.lineMetrics != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS;
    result.ref.line_metrics = value.lineMetrics!;
  }
  if (value.cluster != null) {
    result.ref.fields |= raw.MLN_GEO_JSON_SOURCE_OPTION_CLUSTER;
    result.ref.cluster = value.cluster!;
  }
  return result;
}

GeoJsonSourceOptions _readGeoJsonSourceOptions(
  raw.mln_geo_json_source_options source,
) => GeoJsonSourceOptions(
  minZoom: (source.fields & raw.MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM) != 0
      ? source.min_zoom
      : null,
  maxZoom: (source.fields & raw.MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM) != 0
      ? source.max_zoom
      : null,
  tolerance: (source.fields & raw.MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE) != 0
      ? source.tolerance
      : null,
  clusterMaxZoom:
      (source.fields & raw.MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM) != 0
      ? source.cluster_max_zoom
      : null,
  clusterProperties: _decodeJsonView(source.cluster_properties),
  buffer: (source.fields & raw.MLN_GEO_JSON_SOURCE_OPTION_BUFFER) != 0
      ? source.buffer
      : null,
  clusterRadius:
      (source.fields & raw.MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS) != 0
      ? source.cluster_radius
      : null,
  clusterMinPoints:
      (source.fields & raw.MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS) != 0
      ? source.cluster_min_points
      : null,
  lineMetrics:
      (source.fields & raw.MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS) != 0
      ? source.line_metrics
      : null,
  cluster: (source.fields & raw.MLN_GEO_JSON_SOURCE_OPTION_CLUSTER) != 0
      ? source.cluster
      : null,
);

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

final raw.mln_log_callback _deliverLogCallback = isolateCallable(
  NativeCallable<raw.mln_log_callbackFunction>.isolateLocal((
    Pointer<Void> userData,
    int severity,
    int event,
    int code,
    raw.mln_buffer_view message,
  ) {
    runDispatched<LogCallback>(userData, #mln_log_callback, (callback) {
      if (callback != null) {
        callback(
          LogSeverity.fromRawValue(severity),
          LogEvent.fromRawValue(event),
          code,
          utf8.decode(_copyBufferView(message)),
        );
      }
    });
  }),
);

_NativeRegistration<raw.mln_log_handler> _prepareLogHandler(
  LogHandler value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_log_handler>();
    result.ref.size = sizeOf<raw.mln_log_handler>();
    result.ref.platform_severities = value.platformSeverities.rawValue;
    final registered = registration = roots.registerQueued({
      #mln_log_callback: value.callback,
    });
    result.ref.callback = _deliverLogCallback;
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = registered.userData;
    result.ref.release_user_data = dispatchedRelease;
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

final raw.mln_custom_source_fetch_tile_callback
_deliverCustomSourceFetchTileCallback = isolateCallable(
  NativeCallable<
    raw.mln_custom_source_fetch_tile_callbackFunction
  >.isolateLocal((
    Pointer<Void> userData,
    int map,
    raw.mln_buffer_view sourceId,
    raw.mln_canonical_tile_id tileId,
  ) {
    runDispatched<CustomSourceFetchTileCallback>(
      userData,
      #mln_custom_source_fetch_tile_callback,
      (callback) {
        if (callback != null) {
          callback(
            MapHandle._borrowed(_NativeMap(map)),
            utf8.decode(_copyBufferView(sourceId)),
            _readCanonicalTileId(tileId),
          );
        }
      },
    );
  }),
);

final raw.mln_custom_source_cancel_tile_callback
_deliverCustomSourceCancelTileCallback = isolateCallable(
  NativeCallable<
    raw.mln_custom_source_cancel_tile_callbackFunction
  >.isolateLocal((
    Pointer<Void> userData,
    int map,
    raw.mln_buffer_view sourceId,
    raw.mln_canonical_tile_id tileId,
  ) {
    runDispatched<CustomSourceCancelTileCallback>(
      userData,
      #mln_custom_source_cancel_tile_callback,
      (callback) {
        if (callback != null) {
          callback(
            MapHandle._borrowed(_NativeMap(map)),
            utf8.decode(_copyBufferView(sourceId)),
            _readCanonicalTileId(tileId),
          );
        }
      },
    );
  }),
);

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
      #mln_custom_source_fetch_tile_callback: value.fetchTile,
      if (value.cancelTile != null)
        #mln_custom_source_cancel_tile_callback: value.cancelTile!,
    });
    result.ref.fetch_tile = _deliverCustomSourceFetchTileCallback;
    if (value.cancelTile != null) {
      result.ref.cancel_tile = _deliverCustomSourceCancelTileCallback;
    }
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = registered.userData;
    result.ref.release_user_data = dispatchedRelease;
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

_NativeRegistration<raw.mln_custom_vector_source_options>
_prepareCustomVectorSourceOptions(
  CustomVectorSourceOptions value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_custom_vector_source_options>();
    result.ref.size = sizeOf<raw.mln_custom_vector_source_options>();
    if (value.minZoom != null) {
      result.ref.fields |= raw.MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM;
      result.ref.min_zoom = value.minZoom!;
    }
    if (value.maxZoom != null) {
      result.ref.fields |= raw.MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM;
      result.ref.max_zoom = value.maxZoom!;
    }
    final registered = registration = roots.registerQueued({
      #mln_custom_source_fetch_tile_callback: value.fetchTile,
      if (value.cancelTile != null)
        #mln_custom_source_cancel_tile_callback: value.cancelTile!,
    });
    result.ref.fetch_tile = _deliverCustomSourceFetchTileCallback;
    if (value.cancelTile != null) {
      result.ref.cancel_tile = _deliverCustomSourceCancelTileCallback;
    }
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = registered.userData;
    result.ref.release_user_data = dispatchedRelease;
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

final raw.mln_camera_transition_end_callback
_deliverCameraTransitionEndCallback = isolateCallable(
  NativeCallable<raw.mln_camera_transition_end_callbackFunction>.isolateLocal((
    Pointer<Void> userData,
    Pointer<raw.mln_camera_transition_end> end,
  ) {
    runDispatched<CameraTransitionEndCallback>(
      userData,
      #mln_camera_transition_end_callback,
      (callback) {
        if (callback != null) {
          callback(_readCameraTransitionEnd(end.ref));
        }
      },
    );
  }),
);

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
      #mln_camera_transition_end_callback: value,
    });
    result.ref.callback = _deliverCameraTransitionEndCallback;
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = registered.userData;
    result.ref.release_user_data = dispatchedRelease;
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
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
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
  result.ref.command_queue = Pointer<Void>.fromAddress(
    value.commandQueue.address,
  ).cast();
  return result;
}

final raw.mln_wake_callback _deliverWakeCallback = isolateCallable(
  NativeCallable<raw.mln_wake_callbackFunction>.isolateLocal((
    Pointer<Void> userData,
  ) {
    runDispatched<WakeCallback>(userData, #mln_wake_callback, (callback) {
      if (callback != null) {
        callback();
      }
    });
  }),
);

_NativeRegistration<raw.mln_wake> _prepareWake(
  Wake value,
  _CallbackRoots roots,
) {
  final native = _takeNativeRegistration(value);
  if (native == null) return _prepareQueuedWake(value, roots);
  final arena = Arena();
  final result = arena<raw.mln_wake>();
  result.ref.callback = Pointer.fromAddress(native.callbacks[0].address);
  result.ref.user_data = Pointer.fromAddress(native.userData.address);
  result.ref.release_user_data = Pointer.fromAddress(
    native.releaseUserData.address,
  );
  return _NativeRegistration(result, native.release, arena.releaseAll);
}

_NativeRegistration<raw.mln_wake> _prepareQueuedWake(
  Wake value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_wake>();
    if (value.callback == null) {
      return _NativeRegistration(result, () {}, arena.releaseAll);
    }
    final registered = registration = roots.registerQueued({
      #mln_wake_callback: value.callback!,
    });
    result.ref.callback = _deliverWakeCallback;
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = registered.userData;
    result.ref.release_user_data = dispatchedRelease;
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
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
  result.ref.producer_sync_kinds = value.producerSyncKinds.rawValue;
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

Pointer<raw.mln_metal_surface_descriptor> _writeMetalSurfaceDescriptor(
  MetalSurfaceDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_metal_surface_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_metal_surface_descriptor>();
  result.ref.size = sizeOf<raw.mln_metal_surface_descriptor>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
  result.ref.context = _writeMetalContextDescriptor(value.context, arena).ref;
  result.ref.layer = Pointer<Void>.fromAddress(value.layer.address).cast();
  return result;
}

Pointer<raw.mln_offscreen_descriptor> _writeOffscreenDescriptor(
  OffscreenDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_offscreen_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_offscreen_descriptor>();
  result.ref.size = sizeOf<raw.mln_offscreen_descriptor>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
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
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
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

Pointer<raw.mln_open_gl_surface_descriptor> _writeOpenGlSurfaceDescriptor(
  OpenGlSurfaceDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_open_gl_surface_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_open_gl_surface_descriptor>();
  result.ref.size = sizeOf<raw.mln_open_gl_surface_descriptor>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
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
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
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
  result.ref.final_layout = _nativeInteger(value.finalLayout, 0, 4294967295);
  result.ref.owner = value.owner.rawValue;
  return result;
}

Pointer<raw.mln_vulkan_surface_descriptor> _writeVulkanSurfaceDescriptor(
  VulkanSurfaceDescriptor value,
  Allocator arena, {
  Pointer<raw.mln_vulkan_surface_descriptor>? into,
}) {
  final result = into ?? arena<raw.mln_vulkan_surface_descriptor>();
  result.ref.size = sizeOf<raw.mln_vulkan_surface_descriptor>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
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

Pointer<raw.mln_camera_fit_update> _writeCameraFitUpdate(
  CameraFitUpdate value,
  Allocator arena,
  _NativeRegistrations registrations, {
  Pointer<raw.mln_camera_fit_update>? into,
}) {
  final result = into ?? arena<raw.mln_camera_fit_update>();
  result.ref.size = sizeOf<raw.mln_camera_fit_update>();
  result.ref.mode = value.mode.rawValue;
  result.ref.fit = _writeCameraFitOptions(value.fit, arena).ref;
  result.ref.animation = _writeAnimationOptions(
    value.animation,
    arena,
    registrations,
  ).ref;
  return result;
}

CameraQueryResult _readCameraQueryResult(raw.mln_camera_query_result source) =>
    CameraQueryResult(
      generation: source.generation,
      camera: _readCamera(source.camera),
    );

Pointer<raw.mln_feature_id> _writeFeatureId(
  FeatureId value,
  Allocator arena, {
  Pointer<raw.mln_feature_id>? into,
}) {
  final result = into ?? arena<raw.mln_feature_id>();

  switch (value) {
    case FeatureIdStringValue():
      result.ref.type = 1;
      result.ref.data.string_value = nativeStringView(value.value, arena).value;
    case FeatureIdUintValue():
      result.ref.type = 2;
      result.ref.data.uint_value = value.value;
    case FeatureIdIntValue():
      result.ref.type = 3;
      result.ref.data.int_value = value.value;
    case FeatureIdDoubleValue():
      result.ref.type = 4;
      result.ref.data.double_value = value.value;
  }
  return result;
}

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
    result.ref.feature_id = _writeFeatureId(value.featureId!, arena).ref;
  }
  result.ref.state_key = nativeStringView(value.stateKey, arena).value;
  return result;
}

MapTileSettings _readMapTileSettings(raw.mln_map_tile_settings source) =>
    MapTileSettings(
      prefetchZoomDelta: source.prefetch_zoom_delta,
      lodMinRadius: source.lod_min_radius,
      lodScale: source.lod_scale,
      lodPitchThreshold: source.lod_pitch_threshold,
      lodZoomShift: source.lod_zoom_shift,
      lodMode: TileLodMode.fromRawValue(source.lod_mode),
    );

CameraBounds _readCameraBounds(raw.mln_camera_bounds source) => CameraBounds(
  bounds: _readLatLngBounds(source.bounds),
  unbounded: source.unbounded,
  minZoom: source.min_zoom,
  maxZoom: source.max_zoom,
  minPitch: source.min_pitch,
  maxPitch: source.max_pitch,
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

FreeCamera _readFreeCamera(raw.mln_free_camera source) => FreeCamera(
  position: _readVec3(source.position),
  orientation: _readQuaternion(source.orientation),
);

MapSnapshot _readMapSnapshot(raw.mln_map_snapshot source) => MapSnapshot(
  debugFlags: MapDebugFlag.fromRawValue(source.debug_flags),
  generation: source.generation,
  fullyLoaded: source.fully_loaded,
  renderingStatsViewEnabled: source.rendering_stats_view_enabled,
  gestureInProgress: source.gesture_in_progress,
  transitionInProgress: source.transition_in_progress,
  eventMask: RuntimeEventFlag.fromRawValue(source.event_mask),
  latestRenderUpdateGeneration: source.latest_render_update_generation,
  tile: _readMapTileSettings(source.tile),
  bounds: _readCameraBounds(source.bounds),
  freeCamera: _readFreeCamera(source.free_camera),
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
  keepUnused: source.keep_unused,
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
      geoJson: (source.fields & raw.MLN_STYLE_SOURCE_INFO_GEO_JSON) != 0
          ? _readGeoJsonSourceOptions(source.geo_json)
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

MapProjectionCameraResult _readMapProjectionCameraResult(
  raw.mln_map_projection_camera_result source,
) => MapProjectionCameraResult(camera: _readCamera(source.camera));

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

Pointer<raw.mln_projection_mode_options> _writeProjectionModeOptions(
  ProjectionModeOptions value,
  Allocator arena, {
  Pointer<raw.mln_projection_mode_options>? into,
}) {
  final result = into ?? arena<raw.mln_projection_mode_options>();
  result.ref.size = sizeOf<raw.mln_projection_mode_options>();
  if (value.axonometric != null) {
    result.ref.fields |= raw.MLN_PROJECTION_MODE_OPTION_AXONOMETRIC;
    result.ref.axonometric = value.axonometric!;
  }
  if (value.xSkew != null) {
    result.ref.fields |= raw.MLN_PROJECTION_MODE_OPTION_X_SKEW;
    result.ref.x_skew = value.xSkew!;
  }
  if (value.ySkew != null) {
    result.ref.fields |= raw.MLN_PROJECTION_MODE_OPTION_Y_SKEW;
    result.ref.y_skew = value.ySkew!;
  }
  return result;
}

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
  if (value.keepUnused != null) {
    result.ref.fields |= raw.MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED;
    result.ref.keep_unused = value.keepUnused!;
  }
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

Pointer<raw.mln_offscreen_device_options> _writeOffscreenDeviceOptions(
  OffscreenDeviceOptions value,
  Allocator arena, {
  Pointer<raw.mln_offscreen_device_options>? into,
}) {
  final result = into ?? arena<raw.mln_offscreen_device_options>();
  result.ref.size = sizeOf<raw.mln_offscreen_device_options>();
  if (value.adapterIndex != null) {
    result.ref.fields |= raw.MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX;
    result.ref.adapter_index = _nativeInteger(
      value.adapterIndex!,
      0,
      4294967295,
    );
  }
  if (value.singleContext != null) {
    result.ref.fields |= raw.MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT;
    result.ref.single_context = value.singleContext!;
  }
  if (value.clientApi != null) {
    result.ref.fields |= raw.MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API;
    result.ref.client_api = value.clientApi!.rawValue;
  }
  return result;
}

RenderAbandonResult _readRenderAbandonResult(
  raw.mln_render_abandon_result source,
) => RenderAbandonResult(
  disposition: RenderAbandonDisposition.fromRawValue(source.disposition),
  quarantinedResourceCount: source.quarantined_resource_count,
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
  producerSyncKind: GpuSyncKind.fromRawValue(source.producer_sync_kind),
  consumerSyncKinds: GpuSyncKindFlag.fromRawValue(source.consumer_sync_kinds),
);

RenderSessionSnapshot _readRenderSessionSnapshot(
  raw.mln_render_session_snapshot source,
) => RenderSessionSnapshot(
  state: RenderSessionState.fromRawValue(source.state),
  extent: _readLogicalExtent(source.extent),
  generation: source.generation,
  frameGeneration: source.frame_generation,
  acquiredFrameCount: source.acquired_frame_count,
  failureStatus: Status.fromRawValue(source.failure_status),
  hasTarget: source.has_target,
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

Pointer<raw.mln_metal_borrowed_texture_replacement>
_writeMetalBorrowedTextureReplacement(
  MetalBorrowedTextureReplacement value,
  Allocator arena, {
  Pointer<raw.mln_metal_borrowed_texture_replacement>? into,
}) {
  final result = into ?? arena<raw.mln_metal_borrowed_texture_replacement>();
  result.ref.size = sizeOf<raw.mln_metal_borrowed_texture_replacement>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
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

Pointer<raw.mln_metal_surface_replacement> _writeMetalSurfaceReplacement(
  MetalSurfaceReplacement value,
  Allocator arena, {
  Pointer<raw.mln_metal_surface_replacement>? into,
}) {
  final result = into ?? arena<raw.mln_metal_surface_replacement>();
  result.ref.size = sizeOf<raw.mln_metal_surface_replacement>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
  result.ref.layer = Pointer<Void>.fromAddress(value.layer.address).cast();
  return result;
}

Pointer<raw.mln_open_gl_borrowed_texture_replacement>
_writeOpenGlBorrowedTextureReplacement(
  OpenGlBorrowedTextureReplacement value,
  Allocator arena, {
  Pointer<raw.mln_open_gl_borrowed_texture_replacement>? into,
}) {
  final result = into ?? arena<raw.mln_open_gl_borrowed_texture_replacement>();
  result.ref.size = sizeOf<raw.mln_open_gl_borrowed_texture_replacement>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
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
  return result;
}

Pointer<raw.mln_open_gl_surface_replacement> _writeOpenGlSurfaceReplacement(
  OpenGlSurfaceReplacement value,
  Allocator arena, {
  Pointer<raw.mln_open_gl_surface_replacement>? into,
}) {
  final result = into ?? arena<raw.mln_open_gl_surface_replacement>();
  result.ref.size = sizeOf<raw.mln_open_gl_surface_replacement>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
  result.ref.surface = Pointer<Void>.fromAddress(value.surface.address).cast();
  return result;
}

Pointer<raw.mln_vulkan_borrowed_texture_replacement>
_writeVulkanBorrowedTextureReplacement(
  VulkanBorrowedTextureReplacement value,
  Allocator arena, {
  Pointer<raw.mln_vulkan_borrowed_texture_replacement>? into,
}) {
  final result = into ?? arena<raw.mln_vulkan_borrowed_texture_replacement>();
  result.ref.size = sizeOf<raw.mln_vulkan_borrowed_texture_replacement>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
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
  return result;
}

Pointer<raw.mln_vulkan_surface_replacement> _writeVulkanSurfaceReplacement(
  VulkanSurfaceReplacement value,
  Allocator arena, {
  Pointer<raw.mln_vulkan_surface_replacement>? into,
}) {
  final result = into ?? arena<raw.mln_vulkan_surface_replacement>();
  result.ref.size = sizeOf<raw.mln_vulkan_surface_replacement>();
  result.ref.logical_size = _writeLogicalSize(value.logicalSize, arena).ref;
  result.ref.surface = value.surface;
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

final raw.mln_resource_request_cancel_callback
_deliverResourceRequestCancelCallback = isolateCallable(
  NativeCallable<raw.mln_resource_request_cancel_callbackFunction>.isolateLocal(
    (Pointer<Void> userData, int handle) {
      runDispatched<ResourceRequestCancelCallback>(
        userData,
        #mln_resource_request_cancel_callback,
        (callback) {
          if (callback != null) {
            callback();
          }
        },
      );
    },
  ),
);

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
      #mln_resource_request_cancel_callback: value,
    });
    result.ref.callback = _deliverResourceRequestCancelCallback;
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = registered.userData;
    result.ref.release_user_data = dispatchedRelease;
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

Pointer<raw.mln_resource_transform_error> _writeResourceTransformError(
  ResourceTransformError value,
  Allocator arena, {
  Pointer<raw.mln_resource_transform_error>? into,
}) {
  final result = into ?? arena<raw.mln_resource_transform_error>();
  result.ref.size = sizeOf<raw.mln_resource_transform_error>();
  result.ref.reason = value.reason.rawValue;
  result.ref.message = nativeStringView(value.message, arena).value;
  if (value.retryAfterUnixMs != null) {
    result.ref.fields |= raw.MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER;
    result.ref.retry_after_unix_ms = value.retryAfterUnixMs!;
  }
  return result;
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
  if (value.tileCacheEnabled != null) {
    result.ref.fields |= raw.MLN_MAP_OPTION_TILE_CACHE_ENABLED;
    result.ref.tile_cache_enabled = value.tileCacheEnabled!;
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

RuntimeEventCameraChange _readRuntimeEventCameraChange(
  raw.mln_runtime_event_camera_change source,
) => RuntimeEventCameraChange(mode: CameraChangeMode.fromRawValue(source.mode));

RuntimeEventLoadingFailed _readRuntimeEventLoadingFailed(
  raw.mln_runtime_event_loading_failed source,
) => RuntimeEventLoadingFailed(error: MapLoadError.fromRawValue(source.error));

RenderingStats _readRenderingStats(raw.mln_rendering_stats source) =>
    RenderingStats(
      encodingTimeMs: source.encoding_time_ms,
      renderingTimeMs: source.rendering_time_ms,
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

RuntimeEventStyleImage _readRuntimeEventStyleImage(
  raw.mln_runtime_event_style_image source,
) => RuntimeEventStyleImage(
  imageId: utf8.decode(_copyBufferView(source.image_id)),
);

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
  sourceId: utf8.decode(_copyBufferView(source.source_id)),
  operation: TileOperation.fromRawValue(source.operation),
  tileId: _readTileId(source.tile_id),
);

RuntimeEventSourceChanged _readRuntimeEventSourceChanged(
  raw.mln_runtime_event_source_changed source,
) => RuntimeEventSourceChanged(
  sourceId: utf8.decode(_copyBufferView(source.source_id)),
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

RuntimeEventGlyphs _readRuntimeEventGlyphs(
  raw.mln_runtime_event_glyphs source,
) => RuntimeEventGlyphs(
  fontStack: List<String>.unmodifiable(
    List.generate(
      source.font_count,
      (index) => utf8.decode(_copyBufferView(source.font_stack[index])),
    ),
  ),
  first: source.first,
  last: source.last,
  fontFace: source.font_face,
);

RuntimeEventSprite _readRuntimeEventSprite(
  raw.mln_runtime_event_sprite source,
) => RuntimeEventSprite(
  id: utf8.decode(_copyBufferView(source.id)),
  url: utf8.decode(_copyBufferView(source.url)),
);

RuntimeEvent _readRuntimeEvent(
  raw.mln_runtime_event source, {
  Uint8List Function()? rawRecord,
}) => RuntimeEvent(
  type: RuntimeEventType.fromRawValue(source.type),
  sourceType: RuntimeEventSourceType.fromRawValue(source.source_type),
  source: source.source,
  generation: source.generation,
  message: utf8.decode(_copyBufferView(source.message)),
  payload: switch (source.payload_type) {
    1 => RuntimeEventPayloadCameraChange(
      _readRuntimeEventCameraChange(source.payload.camera_change),
    ),
    2 => RuntimeEventPayloadLoadingFailed(
      _readRuntimeEventLoadingFailed(source.payload.loading_failed),
    ),
    3 => RuntimeEventPayloadRenderFrame(
      _readRuntimeEventRenderFrame(source.payload.render_frame),
    ),
    4 => RuntimeEventPayloadStyleImage(
      _readRuntimeEventStyleImage(source.payload.style_image),
    ),
    5 => RuntimeEventPayloadTileAction(
      _readRuntimeEventTileAction(source.payload.tile_action),
    ),
    6 => RuntimeEventPayloadSourceChanged(
      _readRuntimeEventSourceChanged(source.payload.source_changed),
    ),
    7 => RuntimeEventPayloadOfflineRegionStatus(
      _readRuntimeEventOfflineRegionStatus(
        source.payload.offline_region_status,
      ),
    ),
    8 => RuntimeEventPayloadOfflineRegionResponseError(
      _readRuntimeEventOfflineRegionResponseError(
        source.payload.offline_region_response_error,
      ),
    ),
    9 => RuntimeEventPayloadOfflineRegionTileCountLimit(
      _readRuntimeEventOfflineRegionTileCountLimit(
        source.payload.offline_region_tile_count_limit,
      ),
    ),
    10 => RuntimeEventPayloadGlyphs(
      _readRuntimeEventGlyphs(source.payload.glyphs),
    ),
    11 => RuntimeEventPayloadSprite(
      _readRuntimeEventSprite(source.payload.sprite),
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

final raw.mln_runtime_event_callback _deliverRuntimeEventCallback =
    isolateCallable(
      NativeCallable<raw.mln_runtime_event_callbackFunction>.isolateLocal((
        Pointer<Void> userData,
        Pointer<raw.mln_event_batch_view> events,
      ) {
        runDispatched<RuntimeEventCallback>(
          userData,
          #mln_runtime_event_callback,
          (callback) {
            if (callback != null) {
              callback(_readEventBatchView(events.ref));
            }
          },
        );
      }),
    );

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
      #mln_runtime_event_callback: value,
    });
    result.ref.callback = _deliverRuntimeEventCallback;
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = registered.userData;
    result.ref.release_user_data = dispatchedRelease;
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

final raw.mln_resource_provider_callback _deliverResourceProviderCallback =
    isolateCallable(
      NativeCallable<raw.mln_resource_provider_callbackFunction>.isolateLocal((
        Pointer<Void> userData,
        Pointer<raw.mln_resource_request_info> request,
        int handle,
      ) {
        runDispatched<ResourceProviderCallback>(
          userData,
          #mln_resource_provider_callback,
          (callback) {
            final owner = ResourceRequestHandle._(
              _NativeResourceRequest(handle),
            );
            if (callback == null) {
              owner.close();
              return;
            }
            try {
              callback(_readResourceRequestInfo(request.ref), owner);
            } catch (_) {
              owner.close();
              rethrow;
            }
          },
        );
      }),
    );

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
      #mln_resource_provider_callback: value.callback,
    });
    result.ref.callback = _deliverResourceProviderCallback;
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = registered.userData;
    result.ref.release_user_data = dispatchedRelease;
    return _NativeRegistration(result, registered.reject, arena.releaseAll);
  } catch (_) {
    registration?.reject();
    arena.releaseAll();
    rethrow;
  }
}

final raw.mln_resource_transform_callback _deliverResourceTransformCallback =
    isolateCallable(
      NativeCallable<raw.mln_resource_transform_callbackFunction>.isolateLocal((
        Pointer<Void> userData,
        int kind,
        raw.mln_buffer_view url,
        int request,
      ) {
        runDispatched<ResourceTransformCallback>(
          userData,
          #mln_resource_transform_callback,
          (callback) {
            final owner = ResourceTransformRequestHandle._(
              _NativeResourceTransformRequest(request),
            );
            if (callback == null) {
              owner.close();
              return;
            }
            try {
              callback(
                ResourceKind.fromRawValue(kind),
                utf8.decode(_copyBufferView(url)),
                owner,
              );
            } catch (_) {
              owner.close();
              rethrow;
            }
          },
        );
      }),
    );

Pointer<raw.mln_url_query_parameter> _writeUrlQueryParameter(
  UrlQueryParameter value,
  Allocator arena, {
  Pointer<raw.mln_url_query_parameter>? into,
}) {
  final result = into ?? arena<raw.mln_url_query_parameter>();
  result.ref.name = nativeStringView(value.name, arena).value;
  result.ref.value = nativeStringView(value.value, arena).value;
  return result;
}

Pointer<raw.mln_resource_transform_rule> _writeResourceTransformRule(
  ResourceTransformRule value,
  Allocator arena, {
  Pointer<raw.mln_resource_transform_rule>? into,
}) {
  final result = into ?? arena<raw.mln_resource_transform_rule>();
  result.ref.size = sizeOf<raw.mln_resource_transform_rule>();
  result.ref.kinds = value.kinds.rawValue;
  result.ref.flags = value.flags.rawValue;
  result.ref.url = nativeStringView(value.url, arena).value;
  result.ref.parameters = arena<raw.mln_url_query_parameter>(
    value.parameters.isEmpty ? 1 : value.parameters.length,
  );
  result.ref.parameter_count = value.parameters.length;
  for (var index = 0; index < value.parameters.length; index++) {
    _writeUrlQueryParameter(
      value.parameters[index],
      arena,
      into: result.ref.parameters + index,
    );
  }
  return result;
}

_NativeRegistration<raw.mln_resource_transform> _prepareResourceTransform(
  ResourceTransform value,
  _CallbackRoots roots,
) {
  final arena = Arena();
  DeliveryRegistration? registration;
  try {
    final result = arena<raw.mln_resource_transform>();
    result.ref.size = sizeOf<raw.mln_resource_transform>();
    result.ref.rules = arena<raw.mln_resource_transform_rule>(
      value.rules.isEmpty ? 1 : value.rules.length,
    );
    result.ref.rule_count = value.rules.length;
    for (var index = 0; index < value.rules.length; index++) {
      _writeResourceTransformRule(
        value.rules[index],
        arena,
        into: result.ref.rules + index,
      );
    }
    if (value.callback == null) {
      return _NativeRegistration(result, () {}, arena.releaseAll);
    }
    final registered = registration = roots.registerQueued({
      #mln_resource_transform_callback: value.callback!,
    });
    result.ref.callback = _deliverResourceTransformCallback;
    result.ref.queue = isolateDeliveryQueue.handle;
    result.ref.user_data = registered.userData;
    result.ref.release_user_data = dispatchedRelease;
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

/// Gives the library the Java VM and the app context that it needs on
/// Android.
///
/// Call it before creating a runtime. `jniEnv` is the calling thread's
/// `JNIEnv*`, and `context` is an Android `Context`, such as the application.
///
/// See `mln_android_init` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
void initAndroid(NativePointer jniEnv, NativePointer context) {
  ensureAbiVersion();
  return _check(
    raw.mln_android_init(
      Pointer<Void>.fromAddress(jniEnv.address).cast(),
      Pointer<Void>.fromAddress(context.address).cast(),
      nativeDiagnostic,
    ),
  );
}

/// Returns the API epoch of the library.
///
/// Headers and a library with different epochs aren't compatible.
///
/// See `mln_c_version` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
int cVersion() {
  final nativeResult = raw.mln_c_version();
  return nativeResult;
}

/// Computes the camera that shows exactly one tile on a square map.
///
/// Jump a map in `MapMode.tile` to the camera to render `tile`, where
/// `logicalSize` is the map's size. The camera centers the tile and zooms
/// until the tile fills the map: to zoom z for a 512-pixel map, or z - 1 for
/// a 256-pixel one, plus 0.0001 to keep the neighboring tiles out. It looks
/// straight down with north up and no padding, so the map's earlier camera
/// doesn't change the view. The map keeps the camera within its zoom limits,
/// so for a deep tile, raise the maximum zoom past the camera's with
/// `MapHandle.setBounds()`.
///
/// See `mln_camera_for_tile` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
///
/// Errors:
///
/// - `Status.invalidArgument` when `logicalSize` isn't square, or is wider
///   than 512 pixels, the width of a vector tile at its zoom. For denser
///   tiles, raise the map's scale factor instead.
/// - `Status.invalidArgument` when `tile.x` or `tile.y` isn't less than 2 to
///   the power `tile.z`, or when the camera's zoom would fall outside 0 to
///   25.5, as for tile 0/0/0 on a 256-pixel map.
CameraOptions cameraForTile(CanonicalTileId tile, LogicalSize logicalSize) =>
    withNativeArena((arena) {
      ensureAbiVersion();
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      _check(
        raw.mln_camera_for_tile(
          _writeCanonicalTileId(tile, arena).ref,
          _writeLogicalSize(logicalSize, arena).ref,
          outCamera,
          nativeDiagnostic,
        ),
      );
      return _readCameraOptions(outCamera.ref);
    });

/// Converts meters in the Web Mercator projection to a coordinate.
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

/// Sends log messages to the platform's log again, instead of a callback.
///
/// See `mln_log_clear_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
void clearLogCallback() {
  ensureAbiVersion();
  return _check(raw.mln_log_clear_callback(nativeDiagnostic));
}

/// Selects the severities that the library logs on a background thread.
///
/// The library logs other severities on the thread that logged them. `mask`
/// applies to every runtime in the process.
///
/// See `mln_log_set_async_severity_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
void setLogAsyncSeverityMask(LogSeverityFlag mask) {
  ensureAbiVersion();
  return _check(
    raw.mln_log_set_async_severity_mask(mask.rawValue, nativeDiagnostic),
  );
}

/// Sends the log messages of every runtime in the process to a callback.
///
/// The callback replaces the one set before.
///
/// The callback runs asynchronously in the isolate and zone that registered
/// it, after native queues the call.
///
/// See `mln_log_set_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
void setLogCallback(LogHandler handler) {
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

/// Computes the size in pixels of a target with a logical size and scale
/// factor.
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

/// Reads whether the library may use the network.
///
/// See `mln_network_get_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
NetworkStatus getNetworkStatus() => withNativeArena((arena) {
  ensureAbiVersion();
  final outStatus = arena<Uint32>();
  _check(raw.mln_network_get_status(outStatus, nativeDiagnostic));
  return NetworkStatus.fromRawValue(outStatus.value);
});

/// Tells the library whether it may use the network.
///
/// While the status is `NetworkStatus.offline`, the library starts no network
/// requests. The requests that wait start when the status returns to
/// `NetworkStatus.online`. The status starts as online and applies to every
/// runtime in the process.
///
/// See `mln_network_set_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
void setNetworkStatus(NetworkStatus status) {
  ensureAbiVersion();
  return _check(raw.mln_network_set_status(status.rawValue, nativeDiagnostic));
}

/// Reports the OpenGL context platforms that this build of the library has.
///
/// See `mln_open_gl_supported_context_provider_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
OpenGlContextProviderFlag openGlSupportedContextProviderMask() {
  ensureAbiVersion();
  final nativeResult = raw.mln_open_gl_supported_context_provider_mask();
  return OpenGlContextProviderFlag.fromRawValue(nativeResult);
}

/// Returns the function that registers a plugin's style layer types.
///
/// Pass the function to the plugin's registration entry point before loading
/// a style that uses the plugin's layer types. Plugin callbacks run on
/// library threads and can keep running while the process exits, so they must
/// not use state that the process destroys at exit.
///
/// See `mln_plugin_get_register_function_v1` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
NativePointer getPluginRegisterFunctionV1() {
  ensureAbiVersion();
  final nativeResult = raw.mln_plugin_get_register_function_v1();
  return NativePointer(nativeResult.address);
}

/// Converts a coordinate to meters in the Web Mercator projection.
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

/// Returns the render backends that this build of the library draws with.
///
/// See `mln_supported_render_backend_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
RenderBackendFlag supportedRenderBackendMask() {
  ensureAbiVersion();
  final nativeResult = raw.mln_supported_render_backend_mask();
  return RenderBackendFlag.fromRawValue(nativeResult);
}

/// Issued native handle id.
extension type const _NativeAcquiredFrame(int raw) implements NativeHandle {}

/// Owner of one native handle.
///
/// A rendered frame that the caller holds to read or show.
///
/// See `mln_acquired_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class AcquiredFrameHandle implements Finalizable {
  AcquiredFrameHandle._(RenderSessionHandle parent, _NativeAcquiredFrame handle)
    : _state = NativeHandleState(handle, 'AcquiredFrameHandle')..retain(parent);
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

  /// Gives up an acquired frame without saying when your GPU stops reading it.
  ///
  /// Because your GPU may still read the frame's texture, the session never
  /// renders into that texture again.
  ///
  /// See `mln_acquired_frame_discard` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void discard() => _state.close(
    (handle) => raw.mln_acquired_frame_discard(handle.raw, nativeDiagnostic),
  );

  /// Gets the Metal texture of an acquired frame.
  ///
  /// See `mln_acquired_frame_get_metal_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.targetLost` when the session failed, or you detached, abandoned,
  ///   or disposed it.
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

  /// Gets the OpenGL texture of an acquired frame.
  ///
  /// See `mln_acquired_frame_get_open_gl_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.targetLost` when the session failed, or you detached, abandoned,
  ///   or disposed it.
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

  /// Reads what to wait for before you read an acquired frame's texture.
  ///
  /// See `mln_acquired_frame_get_producer_sync` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.targetLost` when the session failed, or you detached, abandoned,
  ///   or disposed it.
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

  /// Reads the frame result of an acquired frame.
  ///
  /// It succeeds until you release or discard the frame, whatever the session's
  /// state.
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

  /// Gets the Vulkan image of an acquired frame.
  ///
  /// See `mln_acquired_frame_get_vulkan_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.targetLost` when the session failed, or you detached, abandoned,
  ///   or disposed it.
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

  /// Gives an acquired frame back to the session once you're done reading it.
  ///
  /// `consumerCompletion` says when your GPU work that reads the texture
  /// finishes. Pass `GpuSyncKind.cpuComplete` when that work has already
  /// finished. The session waits for that point before it renders into the
  /// texture again. If that wait fails, such as for an OpenGL fence outside the
  /// session's share group, the session fails and never renders into the
  /// texture again.
  ///
  /// See `mln_acquired_frame_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when `RenderSessionCapabilities.consumerSyncKinds`
  ///   lacks the kind of `consumerCompletion`.
  void release(GpuSync consumerCompletion) => _state.close(
    (handle) => withNativeArena(
      (arena) => raw.mln_acquired_frame_release(
        handle.raw,
        _writeGpuSync(consumerCompletion, arena),
        nativeDiagnostic,
      ),
    ),
  );
}

/// Issued native handle id.
extension type const _NativeGeoJsonSourceData(int raw)
    implements NativeHandle {}

/// Owner of one native handle.
///
/// GeoJSON data, parsed and ready to add to a GeoJSON source.
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

  /// Prepares GeoJSON data for GeoJSON sources.
  ///
  /// `data` is a GeoJSON feature collection, feature, or geometry. Preparing it
  /// can be slow, but adding the result to a source is quick, and several
  /// sources can share it.
  ///
  /// See `mln_geo_json_source_data_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when `data` isn't valid GeoJSON, when an option
  ///   in `options` is out of range, or when `options` turns on clustering and
  ///   `data` isn't a feature collection of points.
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

  /// Releases prepared GeoJSON data.
  ///
  /// Sources that use the data keep it.
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
  void addCustomVectorSource(
    String sourceId,
    CustomVectorSourceOptions options,
  ) => _discard((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_add_custom_vector_source(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        registrations.add(
          _prepareCustomVectorSourceOptions(options, registrations.roots),
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
  void cancelCameraTransitions() => _discard(
    (arena, completion) => raw.mln_map_cancel_camera_transitions(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );
  void cancelCameraTransitionsWithId(int transitionId) => _discard(
    (arena, completion) => raw.mln_map_cancel_camera_transitions_with_id(
      _handle.raw,
      transitionId,
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
  void fitCameraToGeometry(Object? geometry, CameraFitUpdate update) =>
      _discard((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_map_fit_camera_to_geometry(
            _handle.raw,
            _nativeJsonView(geometry, arena),
            _writeCameraFitUpdate(update, arena, registrations),
            completion,
            nativeDiagnostic,
          ),
        );
      });
  void fitCameraToLatLngBounds(LatLngBounds bounds, CameraFitUpdate update) =>
      _discard((arena, completion) {
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_map_fit_camera_to_lat_lng_bounds(
            _handle.raw,
            _writeLatLngBounds(bounds, arena).ref,
            _writeCameraFitUpdate(update, arena, registrations),
            completion,
            nativeDiagnostic,
          ),
        );
      });
  void fitCameraToLatLngs(List<LatLng> coordinates, CameraFitUpdate update) =>
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
        final registrations = _NativeRegistrations(_callbackRoots);
        return registrations.run(
          () => raw.mln_map_fit_camera_to_lat_lngs(
            _handle.raw,
            nativecoordinates,
            coordinates.length,
            _writeCameraFitUpdate(update, arena, registrations),
            completion,
            nativeDiagnostic,
          ),
        );
      });
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
  void invalidateCustomVectorSourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) => _discard(
    (arena, completion) => raw.mln_map_invalidate_custom_vector_source_tile(
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
  void resize(LogicalSize logicalSize) => _discard(
    (arena, completion) => raw.mln_map_resize(
      _handle.raw,
      _writeLogicalSize(logicalSize, arena).ref,
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
  void setCustomVectorSourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    StyleVectorTileEncoding encoding,
    Uint8List data,
  ) => _discard(
    (arena, completion) => raw.mln_map_set_custom_vector_source_tile_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      encoding.rawValue,
      nativeBufferView(data, arena),
      completion,
      nativeDiagnostic,
    ),
  );
  void setCustomVectorSourceTileError(
    String sourceId,
    CanonicalTileId tileId,
    String message,
  ) => _discard(
    (arena, completion) => raw.mln_map_set_custom_vector_source_tile_error(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      nativeStringView(message, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );
  void setDebugFlags(MapDebugFlag flags) => _discard(
    (arena, completion) => raw.mln_map_set_debug_flags(
      _handle.raw,
      flags.rawValue,
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
  void setProjectionMode(ProjectionModeOptions options) => _discard(
    (arena, completion) => raw.mln_map_set_projection_mode(
      _handle.raw,
      _writeProjectionModeOptions(options, arena),
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

/// Issued native handle id.
extension type const _NativeMap(int raw) implements NativeHandle {}

/// Owner of one native handle.
///
/// A map, with its style and camera.
///
/// See `mln_map` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class MapHandle implements Finalizable, _CallbackRootOwner {
  MapHandle._(RuntimeHandle parent, _NativeMap handle)
    : _state = NativeHandleState(handle, 'MapHandle')..retain(parent),
      _callbackRoots = _CallbackRoots();
  MapHandle._borrowed(_NativeMap handle)
    : _state = NativeHandleState(handle, 'MapHandle', borrowed: true),
      _callbackRoots = _globalCallbackRoots;
  // Roots this owner's callback registrations for as long as it lives.
  @override
  final _CallbackRoots _callbackRoots;
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

  /// Adds a source whose GeoJSON tiles come from callbacks.
  ///
  /// See `mln_map_add_custom_geometry_source` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.alreadyExists` when the style already has a source named
  ///   `sourceId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Adds a source whose vector tiles come from callbacks.
  ///
  /// See `mln_map_add_custom_vector_source` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.alreadyExists` when the style already has a source named
  ///   `sourceId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
  Future<CommandCompletion> addCustomVectorSource(
    String sourceId,
    CustomVectorSourceOptions options,
  ) => _command((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_add_custom_vector_source(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        registrations.add(
          _prepareCustomVectorSourceOptions(options, registrations.roots),
        ),
        completion,
        nativeDiagnostic,
      ),
    );
  });

  /// Adds a GeoJSON source with prepared data.
  ///
  /// The source uses the options that `data` was prepared with. `data` can be
  /// released once this function returns.
  ///
  /// See `mln_map_add_geo_json_source_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.alreadyExists` when the style already has a source named
  ///   `sourceId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Adds an image source that shows `image`.
  ///
  /// `coordinates` are the image's four corners, clockwise from the top left.
  ///
  /// See `mln_map_add_image_source_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.alreadyExists` when the style already has a source named
  ///   `sourceId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Adds a layer to the map's style.
  ///
  /// `layerJson` is a
  /// \[layer\](https://maplibre.org/maplibre-style-spec/layers/) object. The
  /// layer goes below the layer named `beforeLayerId`, or above every layer
  /// when `beforeLayerId` is empty.
  ///
  /// See `mln_map_add_style_layer_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no layer named `beforeLayerId`, or
  ///   no source with the ID that the layer uses.
  /// - `Status.alreadyExists` when the style already has a layer with the new
  ///   layer's ID.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Adds a source to the map's style.
  ///
  /// `sourceJson` is a
  /// \[source\](https://maplibre.org/maplibre-style-spec/sources/) object.
  ///
  /// See `mln_map_add_style_source_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.alreadyExists` when the style already has a source named
  ///   `sourceId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Moves the camera relative to where it is, as a gesture does.
  ///
  /// `delta` can pan the map's content by an offset on the screen, scale the
  /// zoom, and add to the bearing and pitch. An anchor keeps one screen point
  /// in place while the scale, bearing, or pitch changes. An anchor needs one
  /// of those changes, and combines with an offset only when the delta has no
  /// animation. The function finishes when the map applies the delta, before an
  /// animation ends.
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

  /// Creates a session that renders the map into a ring of your Metal textures.
  ///
  /// See `mln_map_attach_metal_borrowed_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when `supportedRenderBackendMask()` lacks
  ///   `RenderBackendFlag.metal`, or when the target supports none of the
  ///   `producer_sync_kinds` in `options`.
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

  /// Creates a session that renders the map to a Metal layer.
  ///
  /// See `mln_map_attach_metal_surface` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when `supportedRenderBackendMask()` lacks
  ///   `RenderBackendFlag.metal`.
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

  /// Attaches a render session that renders the map offscreen.
  ///
  /// Read each image's pixels with `RenderSessionHandle.readTexture()`.
  ///
  /// The session renders on `device`. When `device` is null, the session
  /// creates a device with the default options, and releases it when the
  /// session ends.
  ///
  /// See `mln_map_attach_offscreen` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidState` when the map already has a render session.
  /// - `Status.nativeError` when `device` is null and the platform can't create
  ///   a device.
  NewRenderSession attachOffscreen(
    OffscreenDescriptor descriptor,
    RenderSessionAttachOptions options, {
    OffscreenDeviceHandle? device,
  }) {
    final registrations = _NativeRegistrations(_CallbackRoots());
    return _attach(
      (arena, completion, outSession) => registrations.run(
        () => raw.mln_map_attach_offscreen(
          _handle.raw,
          _writeOffscreenDescriptor(descriptor, arena),
          _writeRenderSessionAttachOptions(options, arena, registrations),
          (device?._handle.raw ?? 0),
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

  /// Creates a session that renders the map into a ring of your OpenGL
  /// textures.
  ///
  /// See `mln_map_attach_open_gl_borrowed_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when `openGlSupportedContextProviderMask()` lacks
  ///   the context's platform, or when the target supports none of the
  ///   `producer_sync_kinds` in `options`.
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

  /// Creates a session that renders the map to an OpenGL surface.
  ///
  /// See `mln_map_attach_open_gl_surface` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when `openGlSupportedContextProviderMask()` lacks
  ///   the context's platform.
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

  /// Creates a session that renders the map into a ring of your Vulkan images.
  ///
  /// See `mln_map_attach_vulkan_borrowed_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when `supportedRenderBackendMask()` lacks
  ///   `RenderBackendFlag.vulkan`, or when the target supports none of the
  ///   `producer_sync_kinds` in `options`.
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

  /// Creates a session that renders the map to a Vulkan surface.
  ///
  /// See `mln_map_attach_vulkan_surface` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when `supportedRenderBackendMask()` lacks
  ///   `RenderBackendFlag.vulkan`.
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

  /// Holds back the map's changes from rendering until
  /// `MapHandle.endCommandGroup()`, so they show in the same frame.
  ///
  /// Groups nest, and the changes render when the outermost group ends.
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

  /// Computes a camera that fits a GeoJSON geometry in the map's view.
  ///
  /// `geometry` is a GeoJSON geometry object with at least one coordinate, not
  /// a feature. The camera keeps the map's bearing and pitch unless
  /// `fitOptions` sets them, and its zoom stays within the map's zoom limits.
  ///
  /// See `mln_map_camera_for_geometry` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when the padding in `fitOptions` leaves no room
  ///   for the area: its left and right add up to at least the map's width, or
  ///   its top and bottom to at least the map's height.
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

  /// Computes a camera that fits `bounds` in the map's view.
  ///
  /// The camera keeps the map's bearing and pitch unless `fitOptions` sets
  /// them, and its zoom stays within the map's zoom limits.
  ///
  /// See `mln_map_camera_for_lat_lng_bounds` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when the padding in `fitOptions` leaves no room
  ///   for the area: its left and right add up to at least the map's width, or
  ///   its top and bottom to at least the map's height.
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

  /// Computes a camera that fits `coordinates` in the map's view.
  ///
  /// `coordinates` must not be empty. The camera keeps the map's bearing and
  /// pitch unless `fitOptions` sets them, and its zoom stays within the map's
  /// zoom limits.
  ///
  /// See `mln_map_camera_for_lat_lngs` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when the padding in `fitOptions` leaves no room
  ///   for the area: its left and right add up to at least the map's width, or
  ///   its top and bottom to at least the map's height.
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

  /// Stops every running camera animation where it is.
  ///
  /// The end handler of each stopped animation reports
  /// `CameraTransitionOutcome.cancelled`.
  ///
  /// See `mln_map_cancel_camera_transitions` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> cancelCameraTransitions() => _command(
    (arena, completion) => raw.mln_map_cancel_camera_transitions(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Stops the running camera animations whose animation options set
  /// `transitionId`.
  ///
  /// Other animations keep running. The end handler of each stopped animation
  /// reports `CameraTransitionOutcome.cancelled`.
  ///
  /// See `mln_map_cancel_camera_transitions_with_id` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> cancelCameraTransitionsWithId(int transitionId) =>
      _command(
        (arena, completion) => raw.mln_map_cancel_camera_transitions_with_id(
          _handle.raw,
          transitionId,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Releases the map in any state, and finishes in the background.
  ///
  /// Use it from destructors and finalizers. Otherwise, use
  /// `MapHandle.close()`.
  ///
  /// See `mln_map_dispose` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  void dispose() => _state.close(
    (handle) => raw.mln_map_dispose(handle.raw, nativeDiagnostic),
  );

  /// Logs debugging information about the map's style and sources.
  ///
  /// See `mln_map_dump_debug_logs` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> dumpDebugLogs() => _command(
    (arena, completion) =>
        raw.mln_map_dump_debug_logs(_handle.raw, completion, nativeDiagnostic),
  );

  /// Ends the group that `MapHandle.beginCommandGroup()` began.
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

  /// Moves the camera to fit a GeoJSON geometry in the map's view, at once or
  /// with an animation.
  ///
  /// `geometry` is a GeoJSON geometry object with at least one coordinate, not
  /// a feature. The map computes the camera when it runs the command, from the
  /// camera and size that earlier commands leave, as
  /// `MapHandle.cameraForGeometry()` would then. The function finishes when the
  /// map applies the change, before an animation ends.
  ///
  /// See `mln_map_fit_camera_to_geometry` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when the padding in `update` leaves no room for
  ///   the area: its left and right add up to at least the map's width, or its
  ///   top and bottom to at least the map's height. The camera doesn't move.
  Future<CommandCompletion> fitCameraToGeometry(
    Object? geometry,
    CameraFitUpdate update,
  ) => _command((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_fit_camera_to_geometry(
        _handle.raw,
        _nativeJsonView(geometry, arena),
        _writeCameraFitUpdate(update, arena, registrations),
        completion,
        nativeDiagnostic,
      ),
    );
  });

  /// Moves the camera to fit `bounds` in the map's view, at once or with an
  /// animation.
  ///
  /// The map computes the camera when it runs the command, from the camera and
  /// size that earlier commands leave, as `MapHandle.cameraForLatLngBounds()`
  /// would then. The function finishes when the map applies the change, before
  /// an animation ends.
  ///
  /// See `mln_map_fit_camera_to_lat_lng_bounds` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when the padding in `update` leaves no room for
  ///   the area: its left and right add up to at least the map's width, or its
  ///   top and bottom to at least the map's height. The camera doesn't move.
  Future<CommandCompletion> fitCameraToLatLngBounds(
    LatLngBounds bounds,
    CameraFitUpdate update,
  ) => _command((arena, completion) {
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_fit_camera_to_lat_lng_bounds(
        _handle.raw,
        _writeLatLngBounds(bounds, arena).ref,
        _writeCameraFitUpdate(update, arena, registrations),
        completion,
        nativeDiagnostic,
      ),
    );
  });

  /// Moves the camera to fit `coordinates` in the map's view, at once or with
  /// an animation.
  ///
  /// `coordinates` must not be empty. The map computes the camera when it runs
  /// the command, from the camera and size that earlier commands leave, as
  /// `MapHandle.cameraForLatLngs()` would then. The function finishes when the
  /// map applies the change, before an animation ends.
  ///
  /// See `mln_map_fit_camera_to_lat_lngs` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when the padding in `update` leaves no room for
  ///   the area: its left and right add up to at least the map's width, or its
  ///   top and bottom to at least the map's height. The camera doesn't move.
  Future<CommandCompletion> fitCameraToLatLngs(
    List<LatLng> coordinates,
    CameraFitUpdate update,
  ) => _command((arena, completion) {
    final nativecoordinates = arena<raw.mln_lat_lng>(
      coordinates.isEmpty ? 1 : coordinates.length,
    );
    for (var index = 0; index < coordinates.length; index++) {
      _writeLatLng(coordinates[index], arena, into: nativecoordinates + index);
    }
    final registrations = _NativeRegistrations(_callbackRoots);
    return registrations.run(
      () => raw.mln_map_fit_camera_to_lat_lngs(
        _handle.raw,
        nativecoordinates,
        coordinates.length,
        _writeCameraFitUpdate(update, arena, registrations),
        completion,
        nativeDiagnostic,
      ),
    );
  });

  /// Reads the map's camera.
  ///
  /// The query reads the camera after the map applies the commands sent before
  /// it.
  ///
  /// See `mln_map_get_camera` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CameraQueryResult> getCamera() => _query(
    _resultCameraQueryResult,
    (arena, completion) =>
        raw.mln_map_get_camera(_handle.raw, completion, nativeDiagnostic),
  );

  /// Reads the state of a feature, as a JSON object.
  ///
  /// The object is empty when the feature has no state.
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

  /// Reads the global state of the map's style as a JSON object.
  ///
  /// See `mln_map_get_global_state` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  Future<Object?> getGlobalState() => _query(
    _resultObjectOrNull,
    (arena, completion) =>
        raw.mln_map_get_global_state(_handle.raw, completion, nativeDiagnostic),
  );

  /// Reads the corners of an image source's image, clockwise from the top left.
  ///
  /// See `mln_map_get_image_source_coordinates` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't an image
  ///   source.
  Future<List<LatLng>> getImageSourceCoordinates(String sourceId) => _queryList(
    _resultLatLng,
    (arena, completion) => raw.mln_map_get_image_source_coordinates(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Reads the JSON of the style that the map loaded.
  ///
  /// The JSON doesn't include the changes made to the style since it loaded.
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

  /// Copies the map's state.
  ///
  /// The copy doesn't wait for pending commands. To read the camera after they
  /// apply, use `MapHandle.getCamera()`.
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

  /// Reads an image of the map's style.
  ///
  /// Completes with no value when the style has no image named `imageId`.
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

  /// Reads a layer of the map's style.
  ///
  /// Completes with no value when the style has no layer named `layerId`.
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

  /// Reads a layer of the map's style as a
  /// \[layer\](https://maplibre.org/maplibre-style-spec/layers/) object.
  ///
  /// Completes with no value when the style has no layer named `layerId`.
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

  /// Reads a property of a layer in the map's style.
  ///
  /// Completes with no value when the layer doesn't set the property.
  ///
  /// See `mln_map_get_style_layer_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no layer named `layerId`.
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

  /// Reads a property of the light of the map's style.
  ///
  /// Completes with no value when the light doesn't set the property.
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

  /// Reads a source of the map's style.
  ///
  /// Completes with no value when the style has no source named `sourceId`.
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

  /// Reads the default transition of the map's style.
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

  /// Reads the URL of the map's style.
  ///
  /// The URL is empty when the style came from `MapHandle.setStyleJson()`.
  ///
  /// See `mln_map_get_style_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<String> getStyleUrl() => _query(
    _resultString,
    (arena, completion) =>
        raw.mln_map_get_style_url(_handle.raw, completion, nativeDiagnostic),
  );

  /// Makes a custom geometry source ask again for the tiles in `bounds` that
  /// the map still needs.
  ///
  /// See `mln_map_invalidate_custom_geometry_source_region` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a custom
  ///   geometry source.
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

  /// Makes a custom geometry source ask for a tile again when the map needs it.
  ///
  /// See `mln_map_invalidate_custom_geometry_source_tile` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a custom
  ///   geometry source.
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

  /// Makes a custom vector source ask for a tile again when the map needs it.
  ///
  /// See `mln_map_invalidate_custom_vector_source_tile` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a custom
  ///   vector source.
  Future<CommandCompletion> invalidateCustomVectorSourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) => _command(
    (arena, completion) => raw.mln_map_invalidate_custom_vector_source_tile(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Computes the area that the map would show with `camera`.
  ///
  /// Camera fields that `camera` doesn't set come from the map's camera. The
  /// longitudes can fall outside -180 to 180, so a view across the antimeridian
  /// gives one continuous range.
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

  /// Lists the layers of the map's style, from bottom to top.
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

  /// Lists the sources of the map's style.
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

  /// Moves a layer of the map's style below the layer named `beforeLayerId`, or
  /// above every layer when `beforeLayerId` is empty.
  ///
  /// See `mln_map_move_style_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no layer named `layerId` or
  ///   `beforeLayerId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Releases the map.
  ///
  /// Detach the map's render session first.
  ///
  /// See `mln_map_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<void> close() => _state.closeAsync(
    (handle) => _submit(
      (arena, completion) =>
          raw.mln_map_release(handle.raw, completion, nativeDiagnostic),
    ),
  );

  /// Removes the feature state that `selector` selects.
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

  /// Removes an image from the map's style.
  ///
  /// See `mln_map_remove_style_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no image named `imageId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
  Future<CommandCompletion> removeStyleImage(String imageId) => _command(
    (arena, completion) => raw.mln_map_remove_style_image(
      _handle.raw,
      nativeStringView(imageId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Removes a layer from the map's style.
  ///
  /// See `mln_map_remove_style_layer` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no layer named `layerId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
  Future<CommandCompletion> removeStyleLayer(String layerId) => _command(
    (arena, completion) => raw.mln_map_remove_style_layer(
      _handle.raw,
      nativeStringView(layerId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Removes a source from the map's style.
  ///
  /// See `mln_map_remove_style_source` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidState` when a layer still uses the source, or while the
  ///   style isn't loaded, as `MapHandle.setStyleUrl()` describes.
  Future<CommandCompletion> removeStyleSource(String sourceId) => _command(
    (arena, completion) => raw.mln_map_remove_style_source(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Renders a new frame of a continuous map, even when nothing changed.
  ///
  /// See `mln_map_request_repaint` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  Future<CommandCompletion> requestRepaint() => _command(
    (arena, completion) =>
        raw.mln_map_request_repaint(_handle.raw, completion, nativeDiagnostic),
  );

  /// Renders a still image of a static or tile map.
  ///
  /// The map's render session draws the image once every resource that the
  /// image needs has loaded.
  ///
  /// See `mln_map_request_still_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.cancelled` when the map is released before the image is done.
  /// - `Status.nativeError` when the style or a resource that the image needs
  ///   fails to load.
  Future<void> requestStillImage() => _run(
    (arena, completion) => raw.mln_map_request_still_image(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets the map's size.
  ///
  /// A map with a render session takes its size from the session, so resize the
  /// session with `RenderSessionHandle.resize()` instead.
  ///
  /// See `mln_map_resize` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidState` when the map has a render session.
  /// - `Status.invalidState` when a render session attached to the map before
  ///   the resize applied.
  Future<CommandCompletion> resize(LogicalSize logicalSize) => _command(
    (arena, completion) => raw.mln_map_resize(
      _handle.raw,
      _writeLogicalSize(logicalSize, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Limits the camera's center to an area, and its zoom and pitch to ranges.
  ///
  /// Limits that `options` doesn't set keep their current values. When the
  /// camera is outside the new limits, it moves inside them.
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

  /// Sets the data of a tile in a custom geometry source.
  ///
  /// `data` is a GeoJSON feature collection. The source ignores data for a tile
  /// that it hasn't asked for, or has since cancelled.
  ///
  /// See `mln_map_set_custom_geometry_source_tile_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a custom
  ///   geometry source.
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

  /// Reports that a tile of a custom geometry source failed to load.
  ///
  /// See `mln_map_set_custom_geometry_source_tile_error` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a custom
  ///   geometry source.
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

  /// Sets the data of a tile in a custom vector source.
  ///
  /// `data` is the tile, encoded as `encoding`. The source ignores data for a
  /// tile that it hasn't asked for, or has since cancelled.
  ///
  /// See `mln_map_set_custom_vector_source_tile_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a custom
  ///   vector source.
  Future<CommandCompletion> setCustomVectorSourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    StyleVectorTileEncoding encoding,
    Uint8List data,
  ) => _command(
    (arena, completion) => raw.mln_map_set_custom_vector_source_tile_data(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      encoding.rawValue,
      nativeBufferView(data, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Reports that a tile of a custom vector source failed to load.
  ///
  /// See `mln_map_set_custom_vector_source_tile_error` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a custom
  ///   vector source.
  Future<CommandCompletion> setCustomVectorSourceTileError(
    String sourceId,
    CanonicalTileId tileId,
    String message,
  ) => _command(
    (arena, completion) => raw.mln_map_set_custom_vector_source_tile_error(
      _handle.raw,
      nativeStringView(sourceId, arena).value,
      _writeCanonicalTileId(tileId, arena).ref,
      nativeStringView(message, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Draws debugging overlays, such as tile borders, on the map.
  ///
  /// `flags` replaces the overlays set before.
  ///
  /// See `mln_map_set_debug_flags` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> setDebugFlags(MapDebugFlag flags) => _command(
    (arena, completion) => raw.mln_map_set_debug_flags(
      _handle.raw,
      flags.rawValue,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Selects the events that the map reports.
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

  /// Merges `state` into the state of a feature.
  ///
  /// `state` is a JSON object. Its keys replace the same keys in the feature's
  /// state, and the other keys stay.
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

  /// Places the camera at a position and orientation in space.
  ///
  /// The map adjusts them to a camera that it can show. It stops running camera
  /// animations first, and their end handlers report
  /// `CameraTransitionOutcome.cancelled`.
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

  /// Replaces the data of a GeoJSON source.
  ///
  /// `data` must have been prepared with the source's options, which
  /// `MapHandle.getStyleSource()` reports, though `min_zoom` may differ. `data`
  /// can be released once this function returns.
  ///
  /// See `mln_map_set_geo_json_source_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a
  ///   GeoJSON source, or `data` was prepared with other options.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Sets whether a GeoJSON source builds its tiles while the map renders.
  ///
  /// When `enabled` is true, the source builds its tiles while the map renders
  /// instead of in the background, so new data shows in the next frame. Until
  /// this is called, a source follows the `synchronousUpdate` member of its
  /// source JSON, which defaults to false. This setting replaces that value.
  ///
  /// See `mln_map_set_geo_json_source_synchronous_tiling` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a
  ///   GeoJSON source.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Makes a GeoJSON source load its data from `url`.
  ///
  /// See `mln_map_set_geo_json_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't a
  ///   GeoJSON source.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Sets a value that the style's `global-state`
  /// \[expressions\](https://maplibre.org/maplibre-style-spec/expressions/)
  /// read.
  ///
  /// See `mln_map_set_global_state_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Moves the image of an image source.
  ///
  /// `coordinates` are the image's four corners, clockwise from the top left.
  ///
  /// See `mln_map_set_image_source_coordinates` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't an image
  ///   source.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Replaces the image of an image source.
  ///
  /// See `mln_map_set_image_source_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't an image
  ///   source.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Makes an image source load its image from `url`.
  ///
  /// See `mln_map_set_image_source_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidArgument` when the source named `sourceId` isn't an image
  ///   source.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Sets whether the map draws in perspective or axonometric projection, and
  /// how much it skews the view.
  ///
  /// Settings that `options` doesn't set keep their current values.
  ///
  /// See `mln_map_set_projection_mode` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
  Future<CommandCompletion> setProjectionMode(ProjectionModeOptions options) =>
      _command(
        (arena, completion) => raw.mln_map_set_projection_mode(
          _handle.raw,
          _writeProjectionModeOptions(options, arena),
          completion,
          nativeDiagnostic,
        ),
      );

  /// Shows or hides an overlay of rendering statistics on the map.
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

  /// Adds an image to the map's style, or replaces the image named `imageId`.
  ///
  /// See `mln_map_set_style_image` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Loads the map's style from `json`.
  ///
  /// The command completes once the style has parsed, and the map's style is
  /// then loaded, as `MapHandle.setStyleUrl()` describes. The style's sources,
  /// sprites, and glyphs load in the background.
  ///
  /// See `mln_map_set_style_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.nativeError` when `json` isn't a valid style. The map keeps its
  ///   current style.
  Future<CommandCompletion> setStyleJson(Object? json) => _command(
    (arena, completion) => raw.mln_map_set_style_json(
      _handle.raw,
      _nativeJsonView(json, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets a property of a layer in the map's style.
  ///
  /// `propertyName` is one of the layer's layout or paint
  /// \[properties\](https://maplibre.org/maplibre-style-spec/layers/), or
  /// `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null
  /// `value` resets any of these but `source` and `source-layer` to its
  /// default.
  ///
  /// See `mln_map_set_style_layer_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no layer named `layerId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Replaces the \[light\](https://maplibre.org/maplibre-style-spec/light/) of
  /// the map's style.
  ///
  /// See `mln_map_set_style_light_json` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
  Future<CommandCompletion> setStyleLightJson(Object? lightJson) => _command(
    (arena, completion) => raw.mln_map_set_style_light_json(
      _handle.raw,
      _nativeJsonView(lightJson, arena),
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets a property of the light of the map's style.
  ///
  /// See `mln_map_set_style_light_property` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Sets whether a source is volatile.
  ///
  /// The map doesn't store the tiles of a volatile source in the persistent
  /// cache.
  ///
  /// See `mln_map_set_style_source_volatile` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the style has no source named `sourceId`.
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Sets the default transition of the map's style.
  ///
  /// See `mln_map_set_style_transition_options` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidState` while the style isn't loaded, as
  ///   `MapHandle.setStyleUrl()` describes.
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

  /// Loads the map's style from `url`.
  ///
  /// The command completes once the style document has loaded and parsed. The
  /// style's sources, sprites, and glyphs then load in the background.
  ///
  /// The map's style is loaded from the time a style document parses until the
  /// next call to this function. It isn't loaded before the map's first style
  /// parses, while a style URL loads, or after a style URL fails to load. While
  /// the style isn't loaded, the commands that change it complete with
  /// `Status.invalidState`, because the next style to parse would replace their
  /// changes. To change a new map's style without a style document, first load
  /// an empty style, such as `{"version":8,"sources":{},"layers":[]}`.
  ///
  /// See `mln_map_set_style_url` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.nativeError` when the style document fails to load or isn't a
  ///   valid style.
  /// - `Status.cancelled` when the map is released, or this function or
  ///   `MapHandle.setStyleJson()` loads another style, before the style
  ///   document parses.
  Future<CommandCompletion> setStyleUrl(String url) => _command(
    (arena, completion) => raw.mln_map_set_style_url(
      _handle.raw,
      nativeStringView(url, arena).value,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Sets how the map chooses which tiles to load, and at what detail.
  ///
  /// Settings that `options` doesn't set keep their current values.
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

  /// Sets how the map orients and constrains its view.
  ///
  /// Settings that `options` doesn't set keep their current values.
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

  /// Moves the camera, at once or with an animation.
  ///
  /// Camera fields that `update` doesn't set keep their values, and the update
  /// stops any running animation of the fields it sets. The function finishes
  /// when the map applies the update, before an animation ends. To learn when
  /// an animation ends, set an `CameraTransitionEndCallback` in its animation
  /// options.
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

/// Issued native handle id.
extension type const _NativeMapProjection(int raw) implements NativeHandle {}

/// Owner of one native handle.
///
/// A camera view for converting between screen points and coordinates.
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

  /// Creates a projection that converts coordinates for one view of a map.
  ///
  /// Create it from the `view` of an `MapSnapshot` or an `RenderFrameResult`.
  /// The projection doesn't follow the map, so create a new one after the map's
  /// camera or size changes.
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

  /// Reads the projection's camera.
  ///
  /// See `mln_map_projection_get_camera` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  MapProjectionCameraResult getCamera() => withNativeArena((arena) {
    final outResult = arena<raw.mln_map_projection_camera_result>();
    outResult.ref.size = sizeOf<raw.mln_map_projection_camera_result>();
    _check(
      raw.mln_map_projection_get_camera(
        _handle.raw,
        outResult,
        nativeDiagnostic,
      ),
    );
    return _readMapProjectionCameraResult(outResult.ref);
  });

  /// Finds the coordinate at a screen point.
  ///
  /// The longitude stays between -180 and 180.
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

  /// Finds the coordinate at a screen point, without wrapping its longitude.
  ///
  /// The longitude can fall outside -180 to 180, to show which copy of the
  /// world the point is in.
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

  /// Finds the coordinates at several screen points, as
  /// `MapProjectionHandle.latLngForPixel()` does.
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

  /// Finds the coordinates at several screen points, as
  /// `MapProjectionHandle.latLngForPixelUnwrapped()` does.
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

  /// Computes how many meters one screen pixel covers at `latitude`, at the
  /// projection's zoom.
  ///
  /// The result ignores the camera's pitch.
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

  /// Finds the screen point of a coordinate.
  ///
  /// The point is in the copy of the world nearest the camera's center.
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

  /// Finds the screen points of several coordinates, as
  /// `MapProjectionHandle.pixelForLatLng()` does.
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

  /// Releases a projection.
  ///
  /// See `mln_map_projection_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
  void close() => _state.close((handle) {
    raw.mln_map_projection_release(handle.raw);
    return nativeStatusOk;
  });

  /// Moves the projection's camera, without changing the map.
  ///
  /// Camera fields that `camera` doesn't set keep their values.
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

/// Issued native handle id.
extension type const _NativeOffscreenDevice(int raw) implements NativeHandle {}

/// Owner of one native handle.
///
/// A GPU device that the library creates to render maps offscreen.
///
/// See `mln_offscreen_device` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class OffscreenDeviceHandle implements Finalizable {
  OffscreenDeviceHandle._(_NativeOffscreenDevice handle)
    : _state = NativeHandleState(handle, 'OffscreenDeviceHandle');
  final NativeHandleState<_NativeOffscreenDevice> _state;
  // ignore: unused_element
  _NativeOffscreenDevice get _handle => _state.handle;

  /// The native handle id.
  int get id => _state.handleId;

  /// Whether the binding has accepted a close, dispose, discard, or release
  /// of this handle.
  bool get isClosed => _state.isClosed;

  @override
  String toString() =>
      'OffscreenDeviceHandle(id=0x${id.toRadixString(16)}, closed=$isClosed)';

  /// Creates a GPU device for rendering maps offscreen.
  ///
  /// `MapHandle.attachOffscreen()` creates a device of its own when it gets
  /// none. Create one to share it among the sessions on many maps, or to pick a
  /// GPU.
  ///
  /// A browser WebGPU build can't create a device here, because the browser
  /// answers a request for one asynchronously. Attach without a device instead.
  ///
  /// See `mln_offscreen_device_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` in a browser WebGPU build, or when the device can't
  ///   render with `client_api`.
  /// - `Status.nativeError` when `adapter_index` names no GPU, or the platform
  ///   can't create the device.
  static OffscreenDeviceHandle create({OffscreenDeviceOptions? options}) =>
      withNativeArena((arena) {
        ensureAbiVersion();
        final outDevice = arena<Uint64>();
        _check(
          raw.mln_offscreen_device_create(
            options == null
                ? nullptr
                : _writeOffscreenDeviceOptions(options, arena),
            outDevice,
            nativeDiagnostic,
          ),
        );
        return _adoptOwned(
          outDevice.value,
          () =>
              OffscreenDeviceHandle._(_NativeOffscreenDevice(outDevice.value)),
          (handle) {
            raw.mln_offscreen_device_release(handle);
          },
        );
      });

  /// Releases an offscreen device.
  ///
  /// Render sessions on the device keep working until they detach.
  ///
  /// See `mln_offscreen_device_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
  void close() => _state.close((handle) {
    raw.mln_offscreen_device_release(handle.raw);
    return nativeStatusOk;
  });
}

extension type RenderSessionHandleDiscarding._(RenderSessionHandle _owner) {
  _NativeRenderSession get _handle => _owner._handle;
  void resize(LogicalSize logicalSize) => _discard(
    (arena, completion) => raw.mln_render_session_resize(
      _handle.raw,
      _writeLogicalSize(logicalSize, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );
}

/// Issued native handle id.
extension type const _NativeRenderSession(int raw) implements NativeHandle {}

/// Owner of one native handle.
///
/// A render session draws a map into a surface or a texture.
///
/// See `mln_render_session` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class RenderSessionHandle implements Finalizable {
  RenderSessionHandle._(MapHandle parent, _NativeRenderSession handle)
    : _state = NativeHandleState(handle, 'RenderSessionHandle')..retain(parent);
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

  /// Stops the session right away, without running its pending graphics work.
  ///
  /// Use it when the target's graphics objects are going away and a detach
  /// can't finish first. Pending work completes with `Status.targetLost`, and
  /// pending frame demands finish with `RenderResult.targetNotReady`. Once the
  /// call returns, the library no longer uses your graphics objects, and you
  /// can attach another session to the map. Dispose the session afterward.
  ///
  /// Only the driver thread can destroy OpenGL and WebGPU objects, so abandon
  /// keeps them until the process exits, reports how many it kept, and logs a
  /// warning. Detach the session instead to free them. A disposal that another
  /// thread starts first retires the session itself, and the abandon then fails
  /// as it does for any released session.
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

  /// Takes the newest rendered frame from the session's texture ring, if one is
  /// ready.
  ///
  /// Older frames that you haven't acquired are dropped, and the session can
  /// render over a frame before you acquire it. While you hold a frame in every
  /// texture of the ring, new frames wait until you release one. When no frame
  /// is ready, it succeeds and writes the null handle.
  ///
  /// See `mln_render_session_acquire_frame` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when the session lacks
  ///   `RenderSessionCapabilityFlag.frameAcquisition`.
  /// - `RenderSessionSnapshot.failureStatus`, `Status.targetLost` or
  ///   `Status.nativeError`, when the session has failed. The diagnostic
  ///   carries the failure's message.
  /// - `Status.invalidState` when you detached, abandoned, or disposed the
  ///   session.
  AcquiredFrameHandle? acquireFrame() => withNativeArena((arena) {
    final outFrame = arena<Uint64>();
    _check(
      raw.mln_render_session_acquire_frame(
        _handle.raw,
        outFrame,
        nativeDiagnostic,
      ),
    );
    if (outFrame.value == 0) {
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

  /// Ends the session's pending frame demands and waits for its earlier work.
  ///
  /// Frame demands that haven't started, including those that wait for a map
  /// update, finish with `RenderResult.noUpdate`.
  ///
  /// See `mln_render_session_barrier` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.targetLost` when the session loses its target before the barrier
  ///   runs.
  Future<void> barrier() => _run(
    (arena, completion) => raw.mln_render_session_barrier(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Clears the data that the session's renderer holds, such as loaded tiles,
  /// and draws the map again.
  ///
  /// See `mln_render_session_clear_data` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.targetLost` when the session loses its target before the work
  ///   runs.
  Future<void> clearData() => _run(
    (arena, completion) => raw.mln_render_session_clear_data(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Asks the session to render a frame.
  ///
  /// You can call it once the attach call returns. While the attach runs, the
  /// demand waits for it to complete. Every demand finishes with one frame
  /// result that carries its token, and
  /// `RenderSessionHandle.drainFrameResults()` takes it. A demand without
  /// `FrameDemandFlag.force` replaces each pending demand with the same flags.
  ///
  /// See `mln_render_session_demand_frame` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `RenderSessionSnapshot.failureStatus`, `Status.targetLost` or
  ///   `Status.nativeError`, when the session has failed. The diagnostic
  ///   carries the failure's message.
  /// - `Status.invalidState` when you detached, abandoned, or disposed the
  ///   session, or discarded a frame from every texture of its ring.
  void demandFrame(FrameDemand demand) => withNativeArena((arena) {
    _check(
      raw.mln_render_session_demand_frame(
        _handle.raw,
        _writeFrameDemand(demand, arena),
        nativeDiagnostic,
      ),
    );
  });

  /// Detaches the session from its map and frees its graphics objects.
  ///
  /// Release every acquired frame first. Pending frame demands finish with
  /// `RenderResult.targetNotReady`. Once the detach completes, the library no
  /// longer uses your graphics objects, and you can attach another session to
  /// the map. Dispose the session afterward.
  ///
  /// See `mln_render_session_detach` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.targetLost` when the session loses its target before the detach
  ///   runs.
  Future<void> detach() => _run(
    (arena, completion) => raw.mln_render_session_detach(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Releases the session in any state.
  ///
  /// An attached session detaches first, and frees its graphics objects, unless
  /// you hold one of its frames. A session driven by
  /// `RenderDriverKind.callerGraphicsThread` detaches at your next
  /// `RenderSessionHandle.serviceDriverWork()`, so keep servicing it until the
  /// disposal completes. If you release or dispose its map before that service,
  /// the session is abandoned instead. Any other session is abandoned as
  /// `RenderSessionHandle.abandon()` does. Once the disposal completes, the
  /// library no longer uses your graphics objects.
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

  /// Releases the session in any state.
  ///
  /// An attached session detaches first, and frees its graphics objects, unless
  /// you hold one of its frames. A session driven by
  /// `RenderDriverKind.callerGraphicsThread` detaches at your next
  /// `RenderSessionHandle.serviceDriverWork()`, so keep servicing it until the
  /// disposal completes. If you release or dispose its map before that service,
  /// the session is abandoned instead. Any other session is abandoned as
  /// `RenderSessionHandle.abandon()` does. Once the disposal completes, the
  /// library no longer uses your graphics objects.
  ///
  /// See `mln_render_session_dispose` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void dispose() => _state.close(
    (handle) =>
        raw.mln_render_session_dispose(handle.raw, nullptr, nativeDiagnostic),
  );

  /// Takes the frame results that the session has queued.
  ///
  /// When no result is queued, it succeeds and writes the null handle and an
  /// empty view.
  ///
  /// See `mln_render_session_drain_frame_results` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  List<RenderFrameResult> drainFrameResults() => withNativeArena((arena) {
    final outBatch = arena<Uint64>();
    final outView = arena<raw.mln_render_frame_batch_view>();
    outView.ref.size = sizeOf<raw.mln_render_frame_batch_view>();
    _check(
      raw.mln_render_session_drain_frame_results(
        _handle.raw,
        outBatch,
        outView,
        nativeDiagnostic,
      ),
    );
    if (outBatch.value == 0) {
      return const [];
    }
    return _drain(outBatch.value, (batch) {
      raw.mln_render_frame_batch_release(batch);
    }, () => _readRenderFrameBatchView(outView.ref));
  });

  /// Logs debugging information about the session's renderer.
  ///
  /// See `mln_render_session_dump_debug_logs` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.targetLost` when the session loses its target before the work
  ///   runs.
  Future<void> dumpDebugLogs() => _run(
    (arena, completion) => raw.mln_render_session_dump_debug_logs(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Reads what the session supports.
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

  /// Reads the message of the error that made the session fail.
  ///
  /// `RenderSessionSnapshot.failureStatus` reports the error's status. The
  /// message is empty while the session hasn't failed, and doesn't change once
  /// it fails.
  ///
  /// See `mln_render_session_get_failure` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  String getFailure() => withNativeArena((arena) {
    final text = _CallerSizedText();
    _check(
      text.call(
        arena,
        (textStorage, textCapacity, textLength) =>
            raw.mln_render_session_get_failure(
              _handle.raw,
              textStorage,
              textCapacity,
              textLength,
              nativeDiagnostic,
            ),
      ),
    );
    return utf8.decode(text.text);
  });

  /// Reads the session's current state.
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

  /// Reads a value that a source computes for one of its features.
  ///
  /// A clustered GeoJSON source answers the `supercluster` extension for a
  /// cluster feature. Its `extensionField` is `children` for the cluster's
  /// child features, `leaves` for the points in the cluster, or
  /// `expansion-zoom` for the zoom level where the cluster splits. For
  /// `leaves`, `arguments` can set `limit` and `offset`. Without `limit`, the
  /// result has up to 10 points and ignores `offset`. The result is JSON: a
  /// GeoJSON feature collection, a number, or null when the source has no
  /// answer.
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

  /// Finds the features that the session drew in an area of the screen.
  ///
  /// The query reads the last frame that the session rendered. A feature that
  /// several layers draw appears once for each layer.
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

  /// Finds the features of the source named `sourceId`.
  ///
  /// The query covers only the tiles that the source has loaded for the current
  /// view. A source loads tiles only while a visible layer uses it.
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

  /// Reads the pixels of the last frame that the session rendered.
  ///
  /// See `mln_render_session_read_texture` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when the session's capabilities lack
  ///   `RenderSessionCapabilityFlag.readback`.
  /// - `Status.invalidState` when the session hasn't rendered a frame since it
  ///   attached, resized, or changed its target.
  Future<TextureReadbackResult> readTexture() => _query(
    _resultTextureReadbackResult,
    (arena, completion) => raw.mln_render_session_read_texture(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Frees memory that the session's renderer can rebuild, such as caches.
  ///
  /// See `mln_render_session_reduce_memory_use` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.targetLost` when the session loses its target before the work
  ///   runs.
  Future<void> reduceMemoryUse() => _run(
    (arena, completion) => raw.mln_render_session_reduce_memory_use(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Lets go of the session's surface or textures, and keeps the rest of the
  /// session.
  ///
  /// Use it when the platform takes a surface away, such as when your app goes
  /// to the background. Release every acquired frame first. Until you give the
  /// session a new target, frame demands finish with
  /// `RenderResult.targetNotReady`. Once the release completes, you can destroy
  /// the old target.
  ///
  /// See `mln_render_session_release_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when the session renders offscreen, since the
  ///   session owns that target.
  /// - `Status.unsupported` when the target is a WebGL canvas.
  /// - `Status.targetLost` when the session loses its target before the release
  ///   runs.
  Future<void> releaseTarget() => _run(
    (arena, completion) => raw.mln_render_session_release_target(
      _handle.raw,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Resizes the session's target and its map.
  ///
  /// The session renders at the map's scale factor. Release every acquired
  /// frame first.
  ///
  /// See `mln_render_session_resize` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when the session renders into a borrowed ring. Give
  ///   it new textures of the new size instead.
  /// - `Status.targetLost` when the session loses its target before the resize
  ///   applies.
  Future<CommandCompletion> resize(LogicalSize logicalSize) => _command(
    (arena, completion) => raw.mln_render_session_resize(
      _handle.raw,
      _writeLogicalSize(logicalSize, arena).ref,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Runs the session's ready graphics work on your graphics thread.
  ///
  /// Use it with `RenderDriverKind.callerGraphicsThread`, and call it from the
  /// same thread every time. Call it each time
  /// `RenderSessionAttachOptions.driverWorkWake` is called, even while you
  /// demand no frames, and until a detach or disposal completes. It returns
  /// once no work is ready.
  ///
  /// It accepts a disposed session until the session retires. The service after
  /// the disposal detaches the session and frees its graphics objects.
  ///
  /// An isolate cannot service a caller-graphics-thread driver, because Dart
  /// can resume it on another native thread after any `await`. Host native code
  /// services such a session on the graphics thread by passing its `id` to the
  /// C function that `RenderSessionHandle.serviceDriverWork()` calls.
  ///
  /// See `mln_render_session_service_driver_work` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
  void serviceDriverWork() => _check(
    raw.mln_render_session_service_driver_work(_handle.raw, nativeDiagnostic),
  );

  /// Replaces the Metal textures that a borrowed ring renders into.
  ///
  /// Use it to resize the ring, or to give the session textures again after
  /// `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
  /// Once the replacement completes, the session no longer uses the old
  /// textures. A replacement with the textures and sizes that the ring renders
  /// into already changes nothing.
  ///
  /// See `mln_render_session_set_metal_borrowed_texture_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when the new textures have a different pixel format
  ///   than the old ones. Attach a new session to change it.
  Future<void> setMetalBorrowedTextureTarget(
    MetalBorrowedTextureReplacement replacement,
  ) => _run(
    (arena, completion) =>
        raw.mln_render_session_set_metal_borrowed_texture_target(
          _handle.raw,
          _writeMetalBorrowedTextureReplacement(replacement, arena),
          completion,
          nativeDiagnostic,
        ),
  );

  /// Replaces the Metal layer that the session renders to.
  ///
  /// Use it to give the session a layer again after
  /// `RenderSessionHandle.releaseTarget()`, to move it to another layer, or to
  /// resize it. Once the replacement completes, the session no longer uses the
  /// old layer. A replacement with the layer and logical size that the session
  /// renders to already changes nothing.
  ///
  /// See `mln_render_session_set_metal_surface_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  Future<void> setMetalSurfaceTarget(MetalSurfaceReplacement replacement) =>
      _run(
        (arena, completion) => raw.mln_render_session_set_metal_surface_target(
          _handle.raw,
          _writeMetalSurfaceReplacement(replacement, arena),
          completion,
          nativeDiagnostic,
        ),
      );

  /// Replaces the OpenGL textures that a borrowed ring renders into.
  ///
  /// Use it to resize the ring, or to give the session textures again after
  /// `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
  /// Once the replacement completes, the session no longer uses the old
  /// textures. A replacement with the textures and sizes that the ring renders
  /// into already changes nothing.
  ///
  /// See `mln_render_session_set_open_gl_borrowed_texture_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<void> setOpenGlBorrowedTextureTarget(
    OpenGlBorrowedTextureReplacement replacement,
  ) => _run(
    (arena, completion) =>
        raw.mln_render_session_set_open_gl_borrowed_texture_target(
          _handle.raw,
          _writeOpenGlBorrowedTextureReplacement(replacement, arena),
          completion,
          nativeDiagnostic,
        ),
  );

  /// Replaces the OpenGL surface that the session presents to.
  ///
  /// Use it to give the session a surface again after
  /// `RenderSessionHandle.releaseTarget()`, to move it to another surface, or
  /// to resize it. Once the replacement completes, the session no longer uses
  /// the old surface. A replacement with the surface and logical size that the
  /// session presents to already changes nothing.
  ///
  /// See `mln_render_session_set_open_gl_surface_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  Future<void> setOpenGlSurfaceTarget(OpenGlSurfaceReplacement replacement) =>
      _run(
        (arena, completion) =>
            raw.mln_render_session_set_open_gl_surface_target(
              _handle.raw,
              _writeOpenGlSurfaceReplacement(replacement, arena),
              completion,
              nativeDiagnostic,
            ),
      );

  /// Replaces the Vulkan images that a borrowed ring renders into.
  ///
  /// Use it to resize the ring, or to give the session images again after
  /// `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
  /// Once the replacement completes, the session no longer uses the old images.
  /// A replacement with the images and sizes that the ring renders into already
  /// changes nothing.
  ///
  /// See `mln_render_session_set_vulkan_borrowed_texture_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
  Future<void> setVulkanBorrowedTextureTarget(
    VulkanBorrowedTextureReplacement replacement,
  ) => _run(
    (arena, completion) =>
        raw.mln_render_session_set_vulkan_borrowed_texture_target(
          _handle.raw,
          _writeVulkanBorrowedTextureReplacement(replacement, arena),
          completion,
          nativeDiagnostic,
        ),
  );

  /// Replaces the Vulkan surface that the session presents to.
  ///
  /// Use it to give the session a surface again after
  /// `RenderSessionHandle.releaseTarget()`, to move it to another surface, or
  /// to resize it. Once the replacement completes, the session no longer uses
  /// the old surface. A replacement with the surface and logical size that the
  /// session presents to already changes nothing.
  ///
  /// See `mln_render_session_set_vulkan_surface_target` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when the new surface needs a different color format
  ///   or pre-rotation support than the old one. Attach a new session instead.
  Future<void> setVulkanSurfaceTarget(VulkanSurfaceReplacement replacement) =>
      _run(
        (arena, completion) => raw.mln_render_session_set_vulkan_surface_target(
          _handle.raw,
          _writeVulkanSurfaceReplacement(replacement, arena),
          completion,
          nativeDiagnostic,
        ),
      );
}

/// Issued native handle id.
extension type const _NativeResourceRequest(int raw) implements NativeHandle {}

/// Owner of one native handle.
///
/// A resource load that a resource provider answers.
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

  /// Answers a resource request.
  ///
  /// A failed answer fails the request with `ResourceErrorReason.other`, as
  /// `ResourceRequestHandle.close()` does.
  ///
  /// See `mln_resource_request_complete` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidState` when the library cancelled the request.
  void complete(ResourceResponse response) => _state.answer(
    (handle) => withNativeArena(
      (arena) => raw.mln_resource_request_complete(
        handle.raw,
        _writeResourceResponse(response, arena),
        nativeDiagnostic,
      ),
    ),
  );

  /// Reads whether the library cancelled a resource request.
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

  /// Hands a resource request back to the library to load as usual.
  ///
  /// A failed call fails the request with `ResourceErrorReason.other`, as
  /// `ResourceRequestHandle.close()` does.
  ///
  /// See `mln_resource_request_pass_through` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidState` when the library cancelled the request.
  void passThrough() => _state.answer(
    (handle) =>
        raw.mln_resource_request_pass_through(handle.raw, nativeDiagnostic),
  );

  /// Releases a resource request.
  ///
  /// See `mln_resource_request_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void close() => _state.close((handle) {
    raw.mln_resource_request_release(handle.raw);
    return nativeStatusOk;
  });

  /// Calls a callback when the library cancels a resource request.
  ///
  /// The callback never runs after the request is answered. A request takes one
  /// cancel callback. When the library has already cancelled the request, the
  /// call reports that instead and the callback never runs.
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

/// Issued native handle id.
extension type const _NativeResourceTransformRequest(int raw)
    implements NativeHandle {}

/// Owner of one native handle.
///
/// A URL rewrite that a resource transform answers with the URL to load.
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

  /// Answers a resource transform request with the URL to load instead.
  ///
  /// `url` must not be empty. A failed answer fails the request with
  /// `ResourceErrorReason.other`, as `ResourceTransformRequestHandle.close()`
  /// does.
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

  /// Fails a resource transform request without sending it.
  ///
  /// The library retries a request that failed for a reason it retries, such as
  /// `ResourceErrorReason.connection`, and the retry calls the transform again.
  /// When this call fails, the request fails with `ResourceErrorReason.other`,
  /// as `ResourceTransformRequestHandle.close()` does.
  ///
  /// See `mln_resource_transform_request_fail` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void fail(ResourceTransformError error) => _state.answer(
    (handle) => withNativeArena(
      (arena) => raw.mln_resource_transform_request_fail(
        handle.raw,
        _writeResourceTransformError(error, arena),
        nativeDiagnostic,
      ),
    ),
  );

  /// Releases a resource transform request.
  ///
  /// Releasing a request without an answer fails it with
  /// `ResourceErrorReason.other`.
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
  void setResourceTransform(ResourceTransform transform) =>
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

/// Issued native handle id.
extension type const _NativeRuntime(int raw) implements NativeHandle {}

/// Owner of one native handle.
///
/// A runtime owns maps and the network and cache settings that they share.
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

  /// Stops sending the events of the runtime and its maps to a callback.
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

  /// Stops adding headers to the runtime's HTTP requests.
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

  /// Removes the runtime's resource provider, so the library loads every
  /// request itself.
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

  /// Stops rewriting the URLs of the runtime's network requests.
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
  /// A runtime holds maps and the resource loading, cache, and offline regions
  /// that they share. In a browser, create runtimes on a Web Worker, not on the
  /// main thread.
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

  /// Creates a map on `runtime`.
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

  /// Creates an offline region in the runtime's cache file.
  ///
  /// `metadata` holds any bytes to store with the region, such as its name. The
  /// region doesn't download until
  /// `RuntimeHandle.setOfflineRegionDownloadState()` makes its download active.
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

  /// Deletes the offline region `regionId` from the runtime's cache file.
  ///
  /// See `mln_runtime_delete_offline_region` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the cache file has no region `regionId`.
  Future<void> deleteOfflineRegion(int regionId) => _run(
    (arena, completion) => raw.mln_runtime_delete_offline_region(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Releases a runtime in any state, without waiting.
  ///
  /// The runtime finishes its teardown after its maps are released.
  ///
  /// See `mln_runtime_dispose` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  void dispose() => _state.close(
    (handle) => raw.mln_runtime_dispose(handle.raw, nativeDiagnostic),
  );

  /// Reads the offline region `regionId`.
  ///
  /// The result is null when the cache file has no region with that ID.
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

  /// Reads the download progress of the offline region `regionId`.
  ///
  /// See `mln_runtime_get_offline_region_status` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the cache file has no region `regionId`.
  Future<OfflineRegionStatus> getOfflineRegionStatus(int regionId) => _query(
    _resultOfflineRegionStatus,
    (arena, completion) => raw.mln_runtime_get_offline_region_status(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Makes maps check each resource of the offline region `regionId` with the
  /// server before they use it.
  ///
  /// The region keeps its resources.
  ///
  /// See `mln_runtime_invalidate_offline_region` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the cache file has no region `regionId`.
  Future<void> invalidateOfflineRegion(int regionId) => _run(
    (arena, completion) => raw.mln_runtime_invalidate_offline_region(
      _handle.raw,
      regionId,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Lists the offline regions in the runtime's cache file.
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

  /// Copies the offline regions of another cache file into the runtime's cache
  /// file.
  ///
  /// A region whose definition and metadata match one that the runtime's file
  /// already has isn't copied again. The result lists the runtime's copy of
  /// each region in the other file.
  ///
  /// See `mln_runtime_merge_offline_regions` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when `sideDatabasePath` isn't a readable
  ///   database file.
  /// - `Status.nativeError` when the merge would exceed the limit on tiles from
  ///   MapLibre's demo tile server that
  ///   `RuntimeEventType.offlineRegionTileCountLimitExceeded` describes.
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

  /// Releases a runtime.
  ///
  /// Release the runtime's maps first.
  ///
  /// See `mln_runtime_release` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> close() => _state.closeAsync(
    (handle) => _submit(
      (arena, completion) =>
          raw.mln_runtime_release(handle.raw, completion, nativeDiagnostic),
    ),
  );

  /// Runs a maintenance action on the runtime's cache database.
  ///
  /// See `mln_runtime_run_cache_database_operation` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<void> runCacheDatabaseOperation(CacheDatabaseOperation operation) =>
      _run(
        (arena, completion) => raw.mln_runtime_run_cache_database_operation(
          _handle.raw,
          operation.rawValue,
          completion,
          nativeDiagnostic,
        ),
      );

  /// Sends the events of the runtime and its maps to a callback.
  ///
  /// The callback replaces the one set before.
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

  /// Adds headers to the runtime's HTTP requests.
  ///
  /// The transform replaces the one set before. In a browser or on OpenHarmony,
  /// serve the requests with `RuntimeHandle.setResourceProvider()` instead.
  ///
  /// See `mln_runtime_set_http_header_transform` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.unsupported` when the library runs in a browser or on
  ///   OpenHarmony.
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

  /// Limits the size of the runtime's ambient cache.
  ///
  /// `size` is in bytes. Offline regions don't count toward it.
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

  /// Starts or pauses the download of the offline region `regionId`.
  ///
  /// A download stops when the runtime closes.
  ///
  /// See `mln_runtime_set_offline_region_download_state` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the cache file has no region `regionId`.
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

  /// Selects the events that the runtime reports about the download of the
  /// offline region `regionId`.
  ///
  /// `events` holds bits of `RuntimeEventFlag.allRuntimeEvents` and replaces
  /// the region's previous selection. `RuntimeEventFlag.none` stops the
  /// region's events.
  ///
  /// See `mln_runtime_set_offline_region_observed` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the cache file has no region `regionId`.
  Future<void> setOfflineRegionObserved(
    int regionId,
    RuntimeEventFlag events,
  ) => _run(
    (arena, completion) => raw.mln_runtime_set_offline_region_observed(
      _handle.raw,
      regionId,
      events.rawValue,
      completion,
      nativeDiagnostic,
    ),
  );

  /// Serves the runtime's network requests through a callback.
  ///
  /// The provider receives the network requests that its routes match, and the
  /// library loads the rest as usual. A provider can serve URL schemes that the
  /// library can't load itself. The provider replaces the one set before.
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

  /// Rewrites the URLs of the runtime's network requests.
  ///
  /// The transform replaces the one set before.
  ///
  /// See `mln_runtime_set_resource_transform` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
  Future<CommandCompletion> setResourceTransform(ResourceTransform transform) =>
      _command((arena, completion) {
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

  /// Replaces the metadata of the offline region `regionId`.
  ///
  /// See `mln_runtime_update_offline_region_metadata` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.notFound` when the cache file has no region `regionId`.
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

/// Issued native handle id.
extension type const _NativeStyleExpression(int raw) implements NativeHandle {}

/// Owner of one native handle.
///
/// A parsed style expression.
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

  /// Parses a style
  /// \[expression\](https://maplibre.org/maplibre-style-spec/expressions/) to
  /// evaluate without a map.
  ///
  /// See `mln_style_expression_create` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when `expression` isn't a valid expression, or
  ///   its result can't have the expected type.
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

  /// Evaluates a style expression and writes its result as JSON.
  ///
  /// A number in the result that isn't finite becomes null.
  ///
  /// See `mln_style_expression_evaluate` in the
  /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
  ///
  /// Errors:
  ///
  /// - `Status.invalidArgument` when the expression fails to evaluate, such as
  ///   when it reads an input that `inputs` doesn't set.
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

  /// Reads what a style expression's result depends on.
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

  /// Releases a parsed style expression.
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
  int get width => _value.width;
  int get height => _value.height;
  double get scaleFactor => _value.scaleFactor;
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
  int get width => _value.width;
  int get height => _value.height;
  double get scaleFactor => _value.scaleFactor;
  int get slot => _value.slot;
  int get texture => _scope.active(_value).texture;
  int get target => _value.target;
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
  int get value => _value.value;
}

/// A `VulkanTextureFrame` whose native objects are readable only while the
/// callback of `AcquiredFrameHandle.withVulkanTexture` runs. Its
/// other members stay readable after the callback returns.
final class ScopedVulkanTextureFrame {
  ScopedVulkanTextureFrame._(this._scope, this._value);
  final VulkanTextureFrame _value;
  final _NativeViewScope _scope;
  int get width => _value.width;
  int get height => _value.height;
  double get scaleFactor => _value.scaleFactor;
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
