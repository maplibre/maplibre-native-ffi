part of 'runtime.dart';

raw.mln_premultiplied_rgba8_image _premultipliedRgba8ImageToNative(
  PremultipliedRgba8Image image,
  Allocator allocator,
) {
  final result = raw.mln_premultiplied_rgba8_image_default();
  result.width = image.width;
  result.height = image.height;
  result.stride = image.stride;
  final bytes = image.bytes;
  result.byte_length = bytes.length;
  if (bytes.isNotEmpty) {
    final nativeBytes = allocator<Uint8>(bytes.length);
    for (var index = 0; index < bytes.length; index += 1) {
      nativeBytes[index] = bytes[index];
    }
    result.pixels = nativeBytes;
  }
  return result;
}

/// Materializes native image options. The stretch arrays live in `arena`, which
/// native borrows for the duration of the call.
raw.mln_style_image_options _styleImageOptionsToNative(
  StyleImageOptions options,
  Allocator arena,
) {
  final result = raw.mln_style_image_options_default();
  final pixelRatio = options.pixelRatio;
  if (pixelRatio != null) {
    result.fields |= raw
        .mln_style_image_option_field
        .MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
        .value;
    result.pixel_ratio = pixelRatio;
  }
  final sdf = options.sdf;
  if (sdf != null) {
    result.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_SDF.value;
    result.sdf = sdf;
  }
  final stretchX = options.stretchX;
  if (stretchX != null) {
    result.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_STRETCH_X.value;
    result.stretch_x = _nativeStretches(stretchX, arena);
    result.stretch_x_count = stretchX.length;
  }
  final stretchY = options.stretchY;
  if (stretchY != null) {
    result.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_STRETCH_Y.value;
    result.stretch_y = _nativeStretches(stretchY, arena);
    result.stretch_y_count = stretchY.length;
  }
  final content = options.content;
  if (content != null) {
    result.fields |=
        raw.mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_CONTENT.value;
    result.content.left = content.left;
    result.content.top = content.top;
    result.content.right = content.right;
    result.content.bottom = content.bottom;
  }
  final textFitWidth = options.textFitWidth;
  if (textFitWidth != null) {
    result.fields |= raw
        .mln_style_image_option_field
        .MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
        .value;
    result.text_fit_width = textFitWidth.rawValue;
  }
  final textFitHeight = options.textFitHeight;
  if (textFitHeight != null) {
    result.fields |= raw
        .mln_style_image_option_field
        .MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
        .value;
    result.text_fit_height = textFitHeight.rawValue;
  }
  return result;
}

Pointer<raw.mln_image_stretch> _nativeStretches(
  List<ImageStretch> stretches,
  Allocator arena,
) {
  if (stretches.isEmpty) {
    return nullptr;
  }
  final array = arena<raw.mln_image_stretch>(stretches.length);
  for (var index = 0; index < stretches.length; index += 1) {
    array[index]
      ..from = stretches[index].from
      ..to = stretches[index].to;
  }
  return array;
}

raw.mln_style_transition_options _styleTransitionOptionsToNative(
  StyleTransitionOptions options,
) {
  final result = raw.mln_style_transition_options_default();
  final enablePlacementTransitions = options.enablePlacementTransitions;
  if (enablePlacementTransitions != null) {
    result.fields |= raw
        .mln_style_transition_option_field
        .MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
        .value;
    result.enable_placement_transitions = enablePlacementTransitions;
  }
  final durationMs = options.durationMs;
  if (durationMs != null) {
    result.fields |= raw
        .mln_style_transition_option_field
        .MLN_STYLE_TRANSITION_OPTION_DURATION
        .value;
    result.duration_ms = durationMs;
  }
  final delayMs = options.delayMs;
  if (delayMs != null) {
    result.fields |= raw
        .mln_style_transition_option_field
        .MLN_STYLE_TRANSITION_OPTION_DELAY
        .value;
    result.delay_ms = delayMs;
  }
  return result;
}

StyleTransitionOptions _styleTransitionOptionsFromNative(
  raw.mln_style_transition_options options,
) {
  final hasDuration =
      options.fields &
          raw
              .mln_style_transition_option_field
              .MLN_STYLE_TRANSITION_OPTION_DURATION
              .value !=
      0;
  final hasDelay =
      options.fields &
          raw
              .mln_style_transition_option_field
              .MLN_STYLE_TRANSITION_OPTION_DELAY
              .value !=
      0;
  final hasPlacement =
      options.fields &
          raw
              .mln_style_transition_option_field
              .MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
              .value !=
      0;
  return StyleTransitionOptions(
    durationMs: hasDuration ? options.duration_ms : null,
    delayMs: hasDelay ? options.delay_ms : null,
    enablePlacementTransitions: hasPlacement
        ? options.enable_placement_transitions
        : null,
  );
}

StyleImageInfo _styleImageInfoFromNative(raw.mln_style_image_info info) {
  return StyleImageInfo(
    width: info.width,
    height: info.height,
    stride: info.stride,
    byteLength: info.byte_length,
    pixelRatio: info.pixel_ratio,
    sdf: info.sdf,
    stretchXCount: info.stretch_x_count,
    stretchYCount: info.stretch_y_count,
    content: info.has_content
        ? ImageContent(
            left: info.content.left,
            top: info.content.top,
            right: info.content.right,
            bottom: info.content.bottom,
          )
        : null,
    textFitWidth: info.has_text_fit_width
        ? StyleImageTextFit.fromRawValue(info.text_fit_width)
        : null,
    textFitHeight: info.has_text_fit_height
        ? StyleImageTextFit.fromRawValue(info.text_fit_height)
        : null,
  );
}

final class _CustomGeometryCallbackState extends RetainedCallbackState {
  _CustomGeometryCallbackState(
    CustomGeometrySourceOptions options,
    void Function() onReleased,
  ) {
    // The C API invokes this once when a removal, style load, or map close
    // stops referencing the source.
    releaseUserData =
        NativeCallable<
          raw.mln_custom_geometry_source_release_callbackFunction
        >.listener((Pointer<Void> _) {
          onReleased();
          _retire();
        });
    fetchTile =
        NativeCallable<
          raw.mln_custom_geometry_source_tile_callbackFunction
        >.listener((Pointer<Void> _, raw.mln_canonical_tile_id tileId) {
          if (_isRetirementTile(tileId)) {
            _receiveRetirementSignal();
            return;
          }
          runUpcall(() => _invokeTileCallback(options.fetchTile, tileId));
        });
    cancelTile = options.cancelTile == null
        ? null
        : NativeCallable<
            raw.mln_custom_geometry_source_tile_callbackFunction
          >.listener((Pointer<Void> _, raw.mln_canonical_tile_id tileId) {
            if (_isRetirementTile(tileId)) {
              _receiveRetirementSignal();
              return;
            }
            runUpcall(() => _invokeTileCallback(options.cancelTile!, tileId));
          });
  }

  late final NativeCallable<
    raw.mln_custom_geometry_source_tile_callbackFunction
  >
  fetchTile;
  late final NativeCallable<
    raw.mln_custom_geometry_source_tile_callbackFunction
  >?
  cancelTile;
  late final NativeCallable<
    raw.mln_custom_geometry_source_release_callbackFunction
  >
  releaseUserData;
  var _retirementSignals = 0;
  var _retirementQueued = false;

  bool get retirementQueuedForTesting => _retirementQueued;

  /// Starts retirement, which the tile callbacks finish once the native
  /// retirement records reach this isolate behind any queued real ones.
  void _retire() {
    if (_retirementQueued) {
      return;
    }
    _retirementQueued = true;
    _retirementSignals = cancelTile == null ? 1 : 2;
    raw.mln_adapter_custom_geometry_callbacks_retire(
      fetchTile.nativeFunction,
      cancelTile?.nativeFunction ??
          nullptr
              .cast<
                NativeFunction<
                  raw.mln_custom_geometry_source_tile_callbackFunction
                >
              >(),
      nullptr,
    );
  }

  void _receiveRetirementSignal() {
    _retirementSignals -= 1;
    if (_retirementSignals == 0) {
      close();
    }
  }

  bool _isRetirementTile(raw.mln_canonical_tile_id tileId) =>
      tileId.z == 255 && tileId.x == 0 && tileId.y == 0;

  @override
  void closeResources() {
    fetchTile.close();
    cancelTile?.close();
    releaseUserData.close();
  }
}

void _invokeTileCallback(
  CustomGeometryTileCallback callback,
  raw.mln_canonical_tile_id tileId,
) {
  try {
    callback(CanonicalTileId(z: tileId.z, x: tileId.x, y: tileId.y));
  } catch (_) {
    // An exception must not escape into native callback machinery.
  }
}

raw.mln_custom_geometry_source_options _customGeometrySourceOptionsToNative(
  CustomGeometrySourceOptions options,
  _CustomGeometryCallbackState callbackState,
) {
  final result = raw.mln_custom_geometry_source_options_default();
  result.fetch_tile = callbackState.fetchTile.nativeFunction;
  result.release_user_data = callbackState.releaseUserData.nativeFunction;
  result.cancel_tile =
      callbackState.cancelTile?.nativeFunction ??
      nullptr
          .cast<
            NativeFunction<raw.mln_custom_geometry_source_tile_callbackFunction>
          >();

  final minZoom = options.minZoom;
  if (minZoom != null) {
    result.fields |= raw
        .mln_custom_geometry_source_option_field
        .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
        .value;
    result.min_zoom = minZoom;
  }
  final maxZoom = options.maxZoom;
  if (maxZoom != null) {
    result.fields |= raw
        .mln_custom_geometry_source_option_field
        .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
        .value;
    result.max_zoom = maxZoom;
  }
  final tolerance = options.tolerance;
  if (tolerance != null) {
    result.fields |= raw
        .mln_custom_geometry_source_option_field
        .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
        .value;
    result.tolerance = tolerance;
  }
  final tileSize = options.tileSize;
  if (tileSize != null) {
    result.fields |= raw
        .mln_custom_geometry_source_option_field
        .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
        .value;
    result.tile_size = _uint16Positive(tileSize, 'custom geometry tile size');
  }
  final buffer = options.buffer;
  if (buffer != null) {
    result.fields |= raw
        .mln_custom_geometry_source_option_field
        .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
        .value;
    result.buffer = _uint16(buffer, 'custom geometry buffer');
  }
  final clip = options.clip;
  if (clip != null) {
    result.fields |= raw
        .mln_custom_geometry_source_option_field
        .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
        .value;
    result.clip = clip;
  }
  final wrap = options.wrap;
  if (wrap != null) {
    result.fields |= raw
        .mln_custom_geometry_source_option_field
        .MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
        .value;
    result.wrap = wrap;
  }
  return result;
}

raw.mln_canonical_tile_id _canonicalTileIdToNative(CanonicalTileId tileId) {
  final result = Struct.create<raw.mln_canonical_tile_id>();
  result.z = _uint32(tileId.z, 'tile z');
  result.x = _uint32(tileId.x, 'tile x');
  result.y = _uint32(tileId.y, 'tile y');
  return result;
}

raw.mln_resource_response _resourceResponseToNative(
  ResourceResponse response,
  Allocator allocator,
) {
  final result = Struct.create<raw.mln_resource_response>();
  result.size = sizeOf<raw.mln_resource_response>();
  result.status = response.status.rawValue;
  result.error_reason = response.errorReason.rawValue;
  final bytes = response.bytes;
  if (bytes != null && bytes.isNotEmpty) {
    final nativeBytes = allocator<Uint8>(bytes.length);
    for (var index = 0; index < bytes.length; index += 1) {
      nativeBytes[index] = bytes[index];
    }
    result.bytes = nativeBytes;
    result.byte_count = bytes.length;
  }
  final errorMessage = response.errorMessage;
  if (errorMessage != null) {
    result.error_message = nativeUtf8CString(
      errorMessage,
      allocator,
    ).pointer.cast<Char>();
  }
  result.must_revalidate = response.mustRevalidate;
  final modifiedUnixMs = response.modifiedUnixMs;
  if (modifiedUnixMs != null) {
    result.has_modified = true;
    result.modified_unix_ms = modifiedUnixMs;
  }
  final expiresUnixMs = response.expiresUnixMs;
  if (expiresUnixMs != null) {
    result.has_expires = true;
    result.expires_unix_ms = expiresUnixMs;
  }
  final etag = response.etag;
  if (etag != null) {
    result.etag = nativeUtf8CString(etag, allocator).pointer.cast<Char>();
  }
  final retryAfterUnixMs = response.retryAfterUnixMs;
  if (retryAfterUnixMs != null) {
    result.has_retry_after = true;
    result.retry_after_unix_ms = retryAfterUnixMs;
  }
  return result;
}

void _freeNativeResourceResponse(
  raw.mln_resource_response response,
  Allocator allocator,
) {
  if (response.bytes != nullptr) {
    allocator.free(response.bytes);
  }
  if (response.error_message != nullptr) {
    allocator.free(response.error_message);
  }
  if (response.etag != nullptr) {
    allocator.free(response.etag);
  }
}

raw.mln_rendered_query_geometry _renderedQueryGeometryToNative(
  RenderedQueryGeometry geometry,
  Allocator allocator,
) {
  switch (geometry) {
    case RenderedQueryPoint(:final point):
      return raw.mln_rendered_query_geometry_point(
        native_struct.screenPointToNative(point),
      );
    case RenderedQueryBox(:final box):
      final nativeBox = Struct.create<raw.mln_screen_box>();
      nativeBox.min = native_struct.screenPointToNative(box.min);
      nativeBox.max = native_struct.screenPointToNative(box.max);
      return raw.mln_rendered_query_geometry_box(nativeBox);
    case RenderedQueryLineString(:final points):
      final nativePoints = points.isEmpty
          ? nullptr.cast<raw.mln_screen_point>()
          : allocator<raw.mln_screen_point>(points.length);
      for (var index = 0; index < points.length; index += 1) {
        nativePoints[index] = native_struct.screenPointToNative(points[index]);
      }
      return raw.mln_rendered_query_geometry_line_string(
        nativePoints,
        points.length,
      );
  }
}

Pointer<raw.mln_rendered_feature_query_options>
_renderedFeatureQueryOptionsToNative(
  RenderedFeatureQueryOptions options,
  Allocator allocator,
) {
  final nativeOptions = allocator<raw.mln_rendered_feature_query_options>();
  nativeOptions.ref = raw.mln_rendered_feature_query_options_default();
  final layerIds = options.layerIds;
  if (layerIds != null) {
    nativeOptions.ref.fields |= raw
        .mln_rendered_feature_query_option_field
        .MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
        .value;
    nativeOptions.ref.layer_ids = _stringViewArray(layerIds, allocator);
    nativeOptions.ref.layer_id_count = layerIds.length;
  }
  final filter = options.filter;
  if (filter != null) {
    final nativeFilter = allocator<raw.mln_buffer_view>();
    nativeFilter.ref = nativeBufferView(filter, allocator);
    nativeOptions.ref.filter = nativeFilter;
  }
  return nativeOptions;
}

Pointer<raw.mln_source_feature_query_options>
_sourceFeatureQueryOptionsToNative(
  SourceFeatureQueryOptions options,
  Allocator allocator,
) {
  final nativeOptions = allocator<raw.mln_source_feature_query_options>();
  nativeOptions.ref = raw.mln_source_feature_query_options_default();
  final sourceLayerIds = options.sourceLayerIds;
  if (sourceLayerIds != null) {
    nativeOptions.ref.fields |= raw
        .mln_source_feature_query_option_field
        .MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
        .value;
    nativeOptions.ref.source_layer_ids = _stringViewArray(
      sourceLayerIds,
      allocator,
    );
    nativeOptions.ref.source_layer_id_count = sourceLayerIds.length;
  }
  final filter = options.filter;
  if (filter != null) {
    final nativeFilter = allocator<raw.mln_buffer_view>();
    nativeFilter.ref = nativeBufferView(filter, allocator);
    nativeOptions.ref.filter = nativeFilter;
  }
  return nativeOptions;
}

Pointer<raw.mln_style_tile_source_options> _nativeTileSourceOptions(
  TileSourceOptions options,
  Allocator allocator,
) {
  final nativeOptions = allocator<raw.mln_style_tile_source_options>();
  nativeOptions.ref = raw.mln_style_tile_source_options_default();
  final minZoom = options.minZoom;
  if (minZoom != null) {
    nativeOptions.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
        .value;
    nativeOptions.ref.min_zoom = minZoom;
  }
  final maxZoom = options.maxZoom;
  if (maxZoom != null) {
    nativeOptions.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
        .value;
    nativeOptions.ref.max_zoom = maxZoom;
  }
  final attribution = options.attribution;
  if (attribution != null) {
    nativeOptions.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
        .value;
    nativeOptions.ref.attribution = nativeStringView(
      attribution,
      allocator,
    ).value;
  }
  final scheme = options.scheme;
  if (scheme != null) {
    nativeOptions.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
        .value;
    nativeOptions.ref.scheme = scheme.rawValue;
  }
  final bounds = options.bounds;
  if (bounds != null) {
    nativeOptions.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
        .value;
    nativeOptions.ref.bounds = native_struct.latLngBoundsToNative(bounds);
  }
  final tileSize = options.tileSize;
  if (tileSize != null) {
    nativeOptions.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
        .value;
    nativeOptions.ref.tile_size = _uint16Positive(
      tileSize,
      'tile source tile size',
    );
  }
  final vectorEncoding = options.vectorEncoding;
  if (vectorEncoding != null) {
    nativeOptions.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
        .value;
    nativeOptions.ref.vector_encoding = vectorEncoding.rawValue;
  }
  final rasterDemEncoding = options.rasterDemEncoding;
  if (rasterDemEncoding != null) {
    nativeOptions.ref.fields |= raw
        .mln_style_tile_source_option_field
        .MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
        .value;
    nativeOptions.ref.raster_encoding = rasterDemEncoding.rawValue;
  }
  return nativeOptions;
}

Pointer<raw.mln_geojson_source_options> _nativeGeoJsonSourceOptions(
  GeoJsonSourceOptions options,
  Allocator allocator,
) {
  final nativeOptions = allocator<raw.mln_geojson_source_options>();
  nativeOptions.ref = raw.mln_geojson_source_options_default();
  final minZoom = options.minZoom;
  if (minZoom != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
        .value;
    nativeOptions.ref.min_zoom = minZoom;
  }
  final maxZoom = options.maxZoom;
  if (maxZoom != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
        .value;
    nativeOptions.ref.max_zoom = maxZoom;
  }
  final tolerance = options.tolerance;
  if (tolerance != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
        .value;
    nativeOptions.ref.tolerance = tolerance;
  }
  final clusterMaxZoom = options.clusterMaxZoom;
  if (clusterMaxZoom != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
        .value;
    nativeOptions.ref.cluster_max_zoom = clusterMaxZoom;
  }
  final clusterProperties = options.clusterProperties;
  if (clusterProperties != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
        .value;
    nativeOptions.ref.cluster_properties = nativeBufferView(
      clusterProperties,
      allocator,
    );
  }
  final tileSize = options.tileSize;
  if (tileSize != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
        .value;
    nativeOptions.ref.tile_size = _uint32(tileSize, 'GeoJSON tile size');
  }
  final buffer = options.buffer;
  if (buffer != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_BUFFER
        .value;
    nativeOptions.ref.buffer = _uint32(buffer, 'GeoJSON buffer');
  }
  final clusterRadius = options.clusterRadius;
  if (clusterRadius != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
        .value;
    nativeOptions.ref.cluster_radius = _uint32(
      clusterRadius,
      'GeoJSON cluster radius',
    );
  }
  final clusterMinPoints = options.clusterMinPoints;
  if (clusterMinPoints != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
        .value;
    nativeOptions.ref.cluster_min_points = _uint32(
      clusterMinPoints,
      'GeoJSON cluster minimum points',
    );
  }
  final lineMetrics = options.lineMetrics;
  if (lineMetrics != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS
        .value;
    nativeOptions.ref.line_metrics = lineMetrics;
  }
  final cluster = options.cluster;
  if (cluster != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_CLUSTER
        .value;
    nativeOptions.ref.cluster = cluster;
  }
  final synchronousUpdate = options.synchronousUpdate;
  if (synchronousUpdate != null) {
    nativeOptions.ref.fields |= raw
        .mln_geojson_source_option_field
        .MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE
        .value;
    nativeOptions.ref.synchronous_update = synchronousUpdate;
  }
  return nativeOptions;
}

Pointer<raw.mln_lat_lng> _latLngArray(
  List<LatLng> coordinates,
  Allocator allocator,
) {
  if (coordinates.isEmpty) {
    return nullptr.cast<raw.mln_lat_lng>();
  }
  final nativeCoordinates = allocator<raw.mln_lat_lng>(coordinates.length);
  for (var index = 0; index < coordinates.length; index += 1) {
    nativeCoordinates[index] = native_struct.latLngToNative(coordinates[index]);
  }
  return nativeCoordinates;
}

Pointer<raw.mln_buffer_view> _stringViewArray(
  List<String> values,
  Allocator allocator,
) {
  if (values.isEmpty) {
    return nullptr.cast<raw.mln_buffer_view>();
  }
  final views = allocator<raw.mln_buffer_view>(values.length);
  for (var index = 0; index < values.length; index += 1) {
    views[index] = nativeStringView(values[index], allocator).value;
  }
  return views;
}

raw.mln_render_target_extent _renderTargetExtentToNative(
  RenderTargetExtent value,
) {
  final result = Struct.create<raw.mln_render_target_extent>();
  result.size = sizeOf<raw.mln_render_target_extent>();
  result.width = _positiveUint32(value.width, 'render target width');
  result.height = _positiveUint32(value.height, 'render target height');
  result.scale_factor = value.scaleFactor;
  return result;
}

raw.mln_metal_context_descriptor _metalContextDescriptorToNative(
  MetalContextDescriptor value,
) {
  final result = Struct.create<raw.mln_metal_context_descriptor>();
  result.size = sizeOf<raw.mln_metal_context_descriptor>();
  result.device = Pointer<Void>.fromAddress(value.device.address);
  return result;
}

raw.mln_vulkan_context_descriptor _vulkanContextDescriptorToNative(
  VulkanContextDescriptor value,
) {
  final result = Struct.create<raw.mln_vulkan_context_descriptor>();
  result.size = sizeOf<raw.mln_vulkan_context_descriptor>();
  result.instance = Pointer<Void>.fromAddress(value.instance.address);
  result.physical_device = Pointer<Void>.fromAddress(
    value.physicalDevice.address,
  );
  result.device = Pointer<Void>.fromAddress(value.device.address);
  result.graphics_queue = Pointer<Void>.fromAddress(
    value.graphicsQueue.address,
  );
  result.graphics_queue_family_index = value.graphicsQueueFamilyIndex;
  result.get_instance_proc_addr = Pointer<Void>.fromAddress(
    value.getInstanceProcAddr.address,
  );
  result.get_device_proc_addr = Pointer<Void>.fromAddress(
    value.getDeviceProcAddr.address,
  );
  return result;
}

raw.mln_opengl_context_descriptor _openglContextDescriptorToNative(
  OpenGLContextDescriptor value,
  Allocator arena,
) {
  final result = Struct.create<raw.mln_opengl_context_descriptor>();
  result.size = sizeOf<raw.mln_opengl_context_descriptor>();
  result.ownershipAsInt = value.ownership.rawValue;
  switch (value) {
    case WglContextDescriptor():
      result.platformAsInt =
          raw.mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_WGL.value;
      result.data.wgl.size = sizeOf<raw.mln_wgl_context_descriptor>();
      result.data.wgl.device_context = Pointer<Void>.fromAddress(
        value.deviceContext.address,
      );
      result.data.wgl.share_context = Pointer<Void>.fromAddress(
        value.shareContext.address,
      );
      result.data.wgl.get_proc_address = Pointer<Void>.fromAddress(
        value.getProcAddress.address,
      );
    case EglContextDescriptor():
      result.platformAsInt =
          raw.mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_EGL.value;
      result.data.egl.size = sizeOf<raw.mln_egl_context_descriptor>();
      result.data.egl.display = Pointer<Void>.fromAddress(
        value.display.address,
      );
      result.data.egl.config = Pointer<Void>.fromAddress(value.config.address);
      result.data.egl.share_context = Pointer<Void>.fromAddress(
        value.shareContext.address,
      );
      result.data.egl.client_apiAsInt = value.clientApi.rawValue;
      result.data.egl.get_proc_address = Pointer<Void>.fromAddress(
        value.getProcAddress.address,
      );
    case WebGLContextDescriptor():
      result.platformAsInt = raw
          .mln_opengl_context_platform
          .MLN_OPENGL_CONTEXT_PLATFORM_WEBGL
          .value;
      result.data.webgl.size = sizeOf<raw.mln_webgl_context_descriptor>();
      result.data.webgl.kind = value.isTransferredCanvas ? 1 : 0;
      result.data.webgl.context = value.context;
      result.data.webgl.canvas_selector = value.canvasSelector == null
          ? Struct.create<raw.mln_buffer_view>()
          : nativeStringView(value.canvasSelector!, arena).value;
  }
  return result;
}

raw.mln_metal_surface_descriptor _metalSurfaceDescriptorToNative(
  MetalSurfaceDescriptor value,
) {
  final result = raw.mln_metal_surface_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _metalContextDescriptorToNative(value.context);
  result.layer = Pointer<Void>.fromAddress(value.layer.address);
  return result;
}

raw.mln_vulkan_surface_descriptor _vulkanSurfaceDescriptorToNative(
  VulkanSurfaceDescriptor value,
) {
  final result = raw.mln_vulkan_surface_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _vulkanContextDescriptorToNative(value.context);
  result.surface = Pointer<Void>.fromAddress(value.surface.address);
  return result;
}

raw.mln_opengl_surface_descriptor _openglSurfaceDescriptorToNative(
  OpenGLSurfaceDescriptor value,
  Allocator arena,
) {
  final result = raw.mln_opengl_surface_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _openglContextDescriptorToNative(value.context, arena);
  result.surface = Pointer<Void>.fromAddress(value.surface.address);
  return result;
}

raw.mln_metal_owned_texture_descriptor _metalOwnedTextureDescriptorToNative(
  MetalOwnedTextureDescriptor value,
) {
  final result = raw.mln_metal_owned_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _metalContextDescriptorToNative(value.context);
  return result;
}

raw.mln_metal_borrowed_texture_descriptor
_metalBorrowedTextureDescriptorToNative(MetalBorrowedTextureDescriptor value) {
  final result = raw.mln_metal_borrowed_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.physical_width = _positiveUint32(
    value.physicalWidth,
    'physical texture width',
  );
  result.physical_height = _positiveUint32(
    value.physicalHeight,
    'physical texture height',
  );
  result.texture = Pointer<Void>.fromAddress(value.texture.address);
  return result;
}

raw.mln_vulkan_owned_texture_descriptor _vulkanOwnedTextureDescriptorToNative(
  VulkanOwnedTextureDescriptor value,
) {
  final result = raw.mln_vulkan_owned_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _vulkanContextDescriptorToNative(value.context);
  return result;
}

raw.mln_vulkan_borrowed_texture_descriptor
_vulkanBorrowedTextureDescriptorToNative(
  VulkanBorrowedTextureDescriptor value,
) {
  final result = raw.mln_vulkan_borrowed_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.physical_width = _positiveUint32(
    value.physicalWidth,
    'physical image width',
  );
  result.physical_height = _positiveUint32(
    value.physicalHeight,
    'physical image height',
  );
  result.context = _vulkanContextDescriptorToNative(value.context);
  result.image = Pointer<Void>.fromAddress(value.image.address);
  result.image_view = Pointer<Void>.fromAddress(value.imageView.address);
  result.format = value.format;
  result.initial_layout = value.initialLayout;
  result.final_layout = value.finalLayout;
  return result;
}

raw.mln_opengl_owned_texture_descriptor _openglOwnedTextureDescriptorToNative(
  OpenGLOwnedTextureDescriptor value,
  Allocator arena,
) {
  final result = raw.mln_opengl_owned_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _openglContextDescriptorToNative(value.context, arena);
  return result;
}

raw.mln_opengl_borrowed_texture_descriptor
_openglBorrowedTextureDescriptorToNative(
  OpenGLBorrowedTextureDescriptor value,
  Allocator arena,
) {
  final result = raw.mln_opengl_borrowed_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.physical_width = _positiveUint32(
    value.physicalWidth,
    'physical texture width',
  );
  result.physical_height = _positiveUint32(
    value.physicalHeight,
    'physical texture height',
  );
  result.context = _openglContextDescriptorToNative(value.context, arena);
  result.texture = value.texture;
  result.target = value.target;
  return result;
}

raw.mln_webgpu_context_descriptor _webGPUContextDescriptorToNative(
  WebGPUContextDescriptor value,
) {
  final result = Struct.create<raw.mln_webgpu_context_descriptor>();
  result.size = sizeOf<raw.mln_webgpu_context_descriptor>();
  result.instance = Pointer<Void>.fromAddress(value.instance.address);
  result.device = Pointer<Void>.fromAddress(value.device.address);
  result.queue = Pointer<Void>.fromAddress(value.queue.address);
  return result;
}

raw.mln_webgpu_surface_descriptor _webGPUSurfaceDescriptorToNative(
  WebGPUSurfaceDescriptor value,
) {
  final result = raw.mln_webgpu_surface_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _webGPUContextDescriptorToNative(value.context);
  result.surface = Pointer<Void>.fromAddress(value.surface.address);
  result.format = value.format;
  return result;
}

raw.mln_webgpu_owned_texture_descriptor _webGPUOwnedTextureDescriptorToNative(
  WebGPUOwnedTextureDescriptor value,
) {
  final result = raw.mln_webgpu_owned_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _webGPUContextDescriptorToNative(value.context);
  return result;
}

raw.mln_webgpu_borrowed_texture_descriptor
_webGPUBorrowedTextureDescriptorToNative(
  WebGPUBorrowedTextureDescriptor value,
) {
  final result = raw.mln_webgpu_borrowed_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.physical_width = _positiveUint32(
    value.physicalWidth,
    'physical texture width',
  );
  result.physical_height = _positiveUint32(
    value.physicalHeight,
    'physical texture height',
  );
  result.context = _webGPUContextDescriptorToNative(value.context);
  result.texture = Pointer<Void>.fromAddress(value.texture.address);
  result.texture_view = Pointer<Void>.fromAddress(value.textureView.address);
  result.format = value.format;
  return result;
}

Pointer<raw.mln_camera_options> _nativeCamera(
  CameraOptions camera,
  Allocator allocator,
) {
  final nativeCamera = allocator<raw.mln_camera_options>();
  nativeCamera.ref = native_struct.cameraOptionsToNative(
    camera,
    raw.mln_camera_options_default(),
  );
  return nativeCamera;
}

Pointer<raw.mln_animation_options> _nativeAnimation(
  AnimationOptions? animation,
  Allocator allocator,
) {
  if (animation == null) {
    return nullptr.cast<raw.mln_animation_options>();
  }
  final nativeAnimation = allocator<raw.mln_animation_options>();
  nativeAnimation.ref = native_struct.animationOptionsToNative(
    animation,
    raw.mln_animation_options_default(),
  );
  return nativeAnimation;
}

List<String> _copyStyleIdList(NativeStyleIdList list) {
  try {
    return withNativeArena((arena) {
      final outCount = arena<Size>();
      _check(raw.mln_style_id_list_count(list.raw, outCount));
      final ids = <String>[];
      for (var index = 0; index < outCount.value; index += 1) {
        final outId = arena<raw.mln_buffer_view>();
        _check(raw.mln_style_id_list_get(list.raw, index, outId));
        ids.add(_copyStringView(outId.ref) ?? '');
      }
      return ids;
    });
  } finally {
    raw.mln_style_id_list_destroy(list.raw);
  }
}

List<String> _copyStyleStringList(NativeStyleStringList list) {
  try {
    return withNativeArena((arena) {
      final outCount = arena<Size>();
      _check(raw.mln_style_string_list_count(list.raw, outCount));
      final values = <String>[];
      for (var index = 0; index < outCount.value; index += 1) {
        final outValue = arena<raw.mln_buffer_view>();
        _check(raw.mln_style_string_list_get(list.raw, index, outValue));
        values.add(_copyStringView(outValue.ref) ?? '');
      }
      return values;
    });
  } finally {
    raw.mln_style_string_list_destroy(list.raw);
  }
}

String? _copyStringView(raw.mln_buffer_view view) =>
    _copyNativeString(view.data, view.size);

String? _copyNativeString(Pointer<Void> pointer, int byteLength) {
  if (pointer == nullptr || byteLength == 0) {
    return null;
  }
  return pointer.cast<Utf8>().toDartString(length: byteLength);
}

void _check(int status) {
  ensureAbiVersion();
  checkNativeStatus(status, _c.threadLastErrorMessage);
}
