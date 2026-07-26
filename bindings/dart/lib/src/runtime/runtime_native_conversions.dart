part of 'runtime.dart';

raw.mln_premultiplied_rgba8_image _premultipliedRgba8ImageToNative(
  PremultipliedRgba8Image image,
  Allocator allocator,
) {
  final result = _c.raw.mln_premultiplied_rgba8_image_default();
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

raw.mln_style_image_options _styleImageOptionsToNative(
  StyleImageOptions options,
) {
  final result = _c.raw.mln_style_image_options_default();
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
  return result;
}

StyleImageInfo _styleImageInfoFromNative(raw.mln_style_image_info info) {
  return StyleImageInfo(
    width: info.width,
    height: info.height,
    stride: info.stride,
    byteLength: info.byte_length,
    pixelRatio: info.pixel_ratio,
    sdf: info.sdf,
  );
}

final class _CustomGeometryCallbackState extends RetainedCallbackState {
  _CustomGeometryCallbackState(CustomGeometrySourceOptions options) {
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
  var _retirementSignals = 0;
  var _retirementQueued = false;

  bool get retirementQueuedForTesting => _retirementQueued;

  void retire() {
    if (_retirementQueued) {
      return;
    }
    _retirementQueued = true;
    _retirementSignals = cancelTile == null ? 1 : 2;
    _c.raw.mln_dart_custom_geometry_callbacks_retire(
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
  }
}

void _invokeTileCallback(
  CustomGeometryTileCallback callback,
  raw.mln_canonical_tile_id tileId,
) {
  try {
    callback(CanonicalTileId(z: tileId.z, x: tileId.x, y: tileId.y));
  } catch (_) {
    // Listener callbacks are asynchronous notifications; exceptions are
    // contained so they never escape through native callback machinery.
  }
}

raw.mln_custom_geometry_source_options _customGeometrySourceOptionsToNative(
  CustomGeometrySourceOptions options,
  _CustomGeometryCallbackState callbackState,
) {
  final result = _c.raw.mln_custom_geometry_source_options_default();
  result.fetch_tile = callbackState.fetchTile.nativeFunction;
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
      return _c.raw.mln_rendered_query_geometry_point(
        native_struct.screenPointToNative(point),
      );
    case RenderedQueryBox(:final box):
      final nativeBox = Struct.create<raw.mln_screen_box>();
      nativeBox.min = native_struct.screenPointToNative(box.min);
      nativeBox.max = native_struct.screenPointToNative(box.max);
      return _c.raw.mln_rendered_query_geometry_box(nativeBox);
    case RenderedQueryLineString(:final points):
      final nativePoints = points.isEmpty
          ? nullptr.cast<raw.mln_screen_point>()
          : allocator<raw.mln_screen_point>(points.length);
      for (var index = 0; index < points.length; index += 1) {
        nativePoints[index] = native_struct.screenPointToNative(points[index]);
      }
      return _c.raw.mln_rendered_query_geometry_line_string(
        nativePoints,
        points.length,
      );
  }
}

Pointer<raw.mln_feature_state_selector> _featureStateSelectorToNative(
  FeatureStateSelector selector,
  Allocator allocator,
) {
  final nativeSelector = allocator<raw.mln_feature_state_selector>();
  nativeSelector.ref.size = sizeOf<raw.mln_feature_state_selector>();
  nativeSelector.ref.source_id = nativeStringView(
    selector.sourceId,
    allocator,
  ).value;
  final sourceLayerId = selector.sourceLayerId;
  if (sourceLayerId != null) {
    nativeSelector.ref.fields |= raw
        .mln_feature_state_selector_field
        .MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
        .value;
    nativeSelector.ref.source_layer_id = nativeStringView(
      sourceLayerId,
      allocator,
    ).value;
  }
  final featureId = selector.featureId;
  if (featureId != null) {
    nativeSelector.ref.fields |= raw
        .mln_feature_state_selector_field
        .MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
        .value;
    nativeSelector.ref.feature_id = nativeStringView(
      featureId,
      allocator,
    ).value;
  }
  final stateKey = selector.stateKey;
  if (stateKey != null) {
    nativeSelector.ref.fields |= raw
        .mln_feature_state_selector_field
        .MLN_FEATURE_STATE_SELECTOR_STATE_KEY
        .value;
    nativeSelector.ref.state_key = nativeStringView(stateKey, allocator).value;
  }
  return nativeSelector;
}

Pointer<raw.mln_rendered_feature_query_options>
_renderedFeatureQueryOptionsToNative(
  RenderedFeatureQueryOptions options,
  Allocator allocator,
) {
  final nativeOptions = allocator<raw.mln_rendered_feature_query_options>();
  nativeOptions.ref = _c.raw.mln_rendered_feature_query_options_default();
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
    nativeOptions.ref.filter = native_json
        .nativeJsonValue(filter, allocator)
        .pointer;
  }
  return nativeOptions;
}

Pointer<raw.mln_source_feature_query_options>
_sourceFeatureQueryOptionsToNative(
  SourceFeatureQueryOptions options,
  Allocator allocator,
) {
  final nativeOptions = allocator<raw.mln_source_feature_query_options>();
  nativeOptions.ref = _c.raw.mln_source_feature_query_options_default();
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
    nativeOptions.ref.filter = native_json
        .nativeJsonValue(filter, allocator)
        .pointer;
  }
  return nativeOptions;
}

Pointer<raw.mln_style_tile_source_options> _nativeTileSourceOptions(
  TileSourceOptions options,
  Allocator allocator,
) {
  final nativeOptions = allocator<raw.mln_style_tile_source_options>();
  nativeOptions.ref = _c.raw.mln_style_tile_source_options_default();
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

Pointer<raw.mln_string_view> _stringViewArray(
  List<String> values,
  Allocator allocator,
) {
  if (values.isEmpty) {
    return nullptr.cast<raw.mln_string_view>();
  }
  final views = allocator<raw.mln_string_view>(values.length);
  for (var index = 0; index < values.length; index += 1) {
    views[index] = nativeStringView(values[index], allocator).value;
  }
  return views;
}

List<QueriedFeature> _copyFeatureQueryResult(
  Pointer<raw.mln_feature_query_result> result,
) {
  try {
    return withNativeArena((arena) {
      final outCount = arena<Size>();
      _check(_c.raw.mln_feature_query_result_count(result, outCount));
      return [
        for (var index = 0; index < outCount.value; index += 1)
          _copyQueriedFeature(result, index, arena),
      ];
    });
  } finally {
    _c.raw.mln_feature_query_result_destroy(result);
  }
}

QueriedFeature _copyQueriedFeature(
  Pointer<raw.mln_feature_query_result> result,
  int index,
  Allocator allocator,
) {
  final outFeature = allocator<raw.mln_queried_feature>();
  outFeature.ref.size = sizeOf<raw.mln_queried_feature>();
  _check(_c.raw.mln_feature_query_result_get(result, index, outFeature));
  final feature = outFeature.ref;
  final state =
      (feature.fields &
                  raw
                      .mln_queried_feature_field
                      .MLN_QUERIED_FEATURE_STATE
                      .value) ==
              0 ||
          feature.state == nullptr
      ? null
      : native_json.jsonValueFromNative(feature.state.ref);
  return QueriedFeature(
    feature: native_geometry.featureGeoJsonFromNative(feature.feature),
    sourceId:
        (feature.fields &
                raw
                    .mln_queried_feature_field
                    .MLN_QUERIED_FEATURE_SOURCE_ID
                    .value) ==
            0
        ? null
        : _copyStringView(feature.source_id),
    sourceLayerId:
        (feature.fields &
                raw
                    .mln_queried_feature_field
                    .MLN_QUERIED_FEATURE_SOURCE_LAYER_ID
                    .value) ==
            0
        ? null
        : _copyStringView(feature.source_layer_id),
    state: state,
  );
}

FeatureExtensionResult _copyFeatureExtensionResult(
  Pointer<raw.mln_feature_extension_result> result,
) {
  try {
    return withNativeArena((arena) {
      final outInfo = arena<raw.mln_feature_extension_result_info>();
      outInfo.ref.size = sizeOf<raw.mln_feature_extension_result_info>();
      _check(_c.raw.mln_feature_extension_result_get(result, outInfo));
      final info = outInfo.ref;
      return switch (info.type) {
        1 => FeatureExtensionValue(
          native_json.jsonValueFromNative(info.data.value.ref),
        ),
        2 => FeatureExtensionFeatureCollection(
          native_geometry.featureCollectionFromNative(
            info.data.feature_collection,
          ),
        ),
        _ => FeatureExtensionUnknown(info.type),
      };
    });
  } finally {
    _c.raw.mln_feature_extension_result_destroy(result);
  }
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
) {
  final result = Struct.create<raw.mln_opengl_context_descriptor>();
  result.size = sizeOf<raw.mln_opengl_context_descriptor>();
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
      result.data.egl.get_proc_address = Pointer<Void>.fromAddress(
        value.getProcAddress.address,
      );
  }
  return result;
}

raw.mln_metal_surface_descriptor _metalSurfaceDescriptorToNative(
  MetalSurfaceDescriptor value,
) {
  final result = _c.raw.mln_metal_surface_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _metalContextDescriptorToNative(value.context);
  result.layer = Pointer<Void>.fromAddress(value.layer.address);
  return result;
}

raw.mln_vulkan_surface_descriptor _vulkanSurfaceDescriptorToNative(
  VulkanSurfaceDescriptor value,
) {
  final result = _c.raw.mln_vulkan_surface_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _vulkanContextDescriptorToNative(value.context);
  result.surface = Pointer<Void>.fromAddress(value.surface.address);
  return result;
}

raw.mln_opengl_surface_descriptor _openglSurfaceDescriptorToNative(
  OpenGLSurfaceDescriptor value,
) {
  final result = _c.raw.mln_opengl_surface_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _openglContextDescriptorToNative(value.context);
  result.surface = Pointer<Void>.fromAddress(value.surface.address);
  return result;
}

raw.mln_metal_owned_texture_descriptor _metalOwnedTextureDescriptorToNative(
  MetalOwnedTextureDescriptor value,
) {
  final result = _c.raw.mln_metal_owned_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _metalContextDescriptorToNative(value.context);
  return result;
}

raw.mln_metal_borrowed_texture_descriptor
_metalBorrowedTextureDescriptorToNative(MetalBorrowedTextureDescriptor value) {
  final result = _c.raw.mln_metal_borrowed_texture_descriptor_default();
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
  final result = _c.raw.mln_vulkan_owned_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _vulkanContextDescriptorToNative(value.context);
  return result;
}

raw.mln_vulkan_borrowed_texture_descriptor
_vulkanBorrowedTextureDescriptorToNative(
  VulkanBorrowedTextureDescriptor value,
) {
  final result = _c.raw.mln_vulkan_borrowed_texture_descriptor_default();
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
) {
  final result = _c.raw.mln_opengl_owned_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.context = _openglContextDescriptorToNative(value.context);
  return result;
}

raw.mln_opengl_borrowed_texture_descriptor
_openglBorrowedTextureDescriptorToNative(
  OpenGLBorrowedTextureDescriptor value,
) {
  final result = _c.raw.mln_opengl_borrowed_texture_descriptor_default();
  result.extent = _renderTargetExtentToNative(value.extent);
  result.physical_width = _positiveUint32(
    value.physicalWidth,
    'physical texture width',
  );
  result.physical_height = _positiveUint32(
    value.physicalHeight,
    'physical texture height',
  );
  result.context = _openglContextDescriptorToNative(value.context);
  result.texture = value.texture;
  result.target = value.target;
  return result;
}

Pointer<raw.mln_camera_options> _nativeCamera(
  CameraOptions camera,
  Allocator allocator,
) {
  final nativeCamera = allocator<raw.mln_camera_options>();
  nativeCamera.ref = native_struct.cameraOptionsToNative(
    camera,
    _c.raw.mln_camera_options_default(),
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
    _c.raw.mln_animation_options_default(),
  );
  return nativeAnimation;
}

Pointer<raw.mln_screen_point> _nativeScreenPoint(
  ScreenPoint? point,
  Allocator allocator,
) {
  if (point == null) {
    return nullptr.cast<raw.mln_screen_point>();
  }
  final nativePoint = allocator<raw.mln_screen_point>();
  nativePoint.ref = native_struct.screenPointToNative(point);
  return nativePoint;
}

String? _copyStyleSourceAttribution(
  Pointer<raw.mln_map> map,
  raw.mln_string_view sourceId,
  bool hasAttribution,
  int attributionSize,
  Allocator allocator,
) {
  if (!hasAttribution) {
    return null;
  }
  final buffer = attributionSize == 0
      ? nullptr.cast<Char>()
      : allocator<Char>(attributionSize);
  final outSize = allocator<Size>();
  final outFound = allocator<Bool>();
  _check(
    _c.raw.mln_map_copy_style_source_attribution(
      map,
      sourceId,
      buffer,
      attributionSize,
      outSize,
      outFound,
    ),
  );
  if (!outFound.value) {
    return null;
  }
  if (outSize.value == 0) {
    return '';
  }
  return buffer.cast<Utf8>().toDartString(length: outSize.value);
}

JsonValue? _copyJsonSnapshot(Pointer<raw.mln_json_snapshot> snapshot) {
  if (snapshot == nullptr) {
    return null;
  }
  try {
    return withNativeArena((arena) {
      final outValue = arena<Pointer<raw.mln_json_value>>();
      outValue.value = nullptr;
      _check(_c.raw.mln_json_snapshot_get(snapshot, outValue));
      if (outValue.value == nullptr) {
        return null;
      }
      return native_json.jsonValueFromNative(outValue.value.ref);
    });
  } finally {
    _c.raw.mln_json_snapshot_destroy(snapshot);
  }
}

List<String> _copyStyleIdList(Pointer<raw.mln_style_id_list> list) {
  try {
    return withNativeArena((arena) {
      final outCount = arena<Size>();
      _check(_c.raw.mln_style_id_list_count(list, outCount));
      final ids = <String>[];
      for (var index = 0; index < outCount.value; index += 1) {
        final outId = arena<raw.mln_string_view>();
        _check(_c.raw.mln_style_id_list_get(list, index, outId));
        ids.add(_copyStringView(outId.ref) ?? '');
      }
      return ids;
    });
  } finally {
    _c.raw.mln_style_id_list_destroy(list);
  }
}

String? _copyStringView(raw.mln_string_view view) =>
    _copyNativeString(view.data, view.size);

String? _copyNativeString(Pointer<Char> pointer, int byteLength) {
  if (pointer == nullptr || byteLength == 0) {
    return null;
  }
  return pointer.cast<Utf8>().toDartString(length: byteLength);
}

int _statusCode(raw.mln_status status) => status.value;

void _check(raw.mln_status status) {
  checkNativeStatus(_statusCode(status), _c.threadLastErrorMessage);
}
