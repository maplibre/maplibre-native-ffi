part of 'runtime.dart';

Uint8List _copyBufferView(raw.mln_buffer_view view) {
  if (view.size == 0) return Uint8List(0);
  if (view.data == nullptr) {
    throwInvalidState('native completion returned an invalid buffer');
  }
  return Uint8List.fromList(view.data.cast<Uint8>().asTypedList(view.size));
}

raw.mln_runtime_options _runtimeOptionsToNative(
  RuntimeOptions options,
  Allocator allocator,
) {
  final result = raw.mln_runtime_options_default();
  final assetPath = options.assetPath;
  if (assetPath != null) {
    result.asset_path = nativeUtf8CString(
      assetPath,
      allocator,
    ).pointer.cast<Char>();
  }
  final cachePath = options.cachePath;
  if (cachePath != null) {
    result.cache_path = nativeUtf8CString(
      cachePath,
      allocator,
    ).pointer.cast<Char>();
  }
  result.event_mask = options.eventMask.value;
  return result;
}

raw.mln_offline_region_definition _offlineRegionDefinitionToNative(
  OfflineRegionDefinition definition,
  Allocator allocator,
) {
  final result = Struct.create<raw.mln_offline_region_definition>();
  result.size = sizeOf<raw.mln_offline_region_definition>();
  switch (definition) {
    case OfflineTilePyramidRegionDefinition():
      result.type = raw
          .mln_offline_region_definition_type
          .MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
          .value;
      result.data.tile_pyramid = _offlineTilePyramidDefinitionToNative(
        definition,
        allocator,
      );
    case OfflineGeometryRegionDefinition():
      result.type = raw
          .mln_offline_region_definition_type
          .MLN_OFFLINE_REGION_DEFINITION_GEOMETRY
          .value;
      result.data.geometry = _offlineGeometryDefinitionToNative(
        definition,
        allocator,
      );
    case UnknownOfflineRegionDefinition():
      throwInvalidArgument(
        'unknown offline region definition type ${definition.rawType} cannot '
        'be used as native input',
      );
  }
  return result;
}

raw.mln_offline_tile_pyramid_region_definition
_offlineTilePyramidDefinitionToNative(
  OfflineTilePyramidRegionDefinition definition,
  Allocator allocator,
) {
  final result =
      Struct.create<raw.mln_offline_tile_pyramid_region_definition>();
  result.size = sizeOf<raw.mln_offline_tile_pyramid_region_definition>();
  result.style_url = nativeUtf8CString(
    definition.styleUrl,
    allocator,
  ).pointer.cast<Char>();
  result.bounds = native_struct.latLngBoundsToNative(definition.bounds);
  result.min_zoom = definition.minZoom;
  result.max_zoom = definition.maxZoom;
  result.pixel_ratio = definition.pixelRatio;
  result.include_ideographs = definition.includeIdeographs;
  return result;
}

raw.mln_offline_geometry_region_definition _offlineGeometryDefinitionToNative(
  OfflineGeometryRegionDefinition definition,
  Allocator allocator,
) {
  final result = Struct.create<raw.mln_offline_geometry_region_definition>();
  result.size = sizeOf<raw.mln_offline_geometry_region_definition>();
  result.style_url = nativeUtf8CString(
    definition.styleUrl,
    allocator,
  ).pointer.cast<Char>();
  result.geometry = nativeBufferView(definition.geometry, allocator);
  result.min_zoom = definition.minZoom;
  result.max_zoom = definition.maxZoom;
  result.pixel_ratio = definition.pixelRatio;
  result.include_ideographs = definition.includeIdeographs;
  return result;
}

Pointer<Char> _nativeOwnedCString(String value) =>
    nativeUtf8CString(value, calloc).pointer.cast<Char>();

void _checkNativeCString(String value) {
  if (value.contains('\u0000')) {
    throwInvalidArgument(
      'null-terminated strings must not contain embedded NUL',
    );
  }
}

void _checkOptionalNativeCString(String? value) {
  if (value != null) {
    _checkNativeCString(value);
  }
}

void _checkResourceResponseNativeStrings(ResourceResponse response) {
  _checkOptionalNativeCString(response.errorMessage);
  _checkOptionalNativeCString(response.etag);
}

int _uint32(int value, String name) {
  if (value < 0 || value > 0xffffffff) {
    throwInvalidArgument('$name must fit uint32');
  }
  return value;
}

int _positiveUint32(int value, String name) {
  if (value <= 0 || value > 0xffffffff) {
    throwInvalidArgument('$name must be between 1 and 4294967295');
  }
  return value;
}

int _uint16(int value, String name) {
  if (value < 0 || value > 0xffff) {
    throwInvalidArgument('$name must be between 0 and 65535');
  }
  return value;
}

int _uint16Positive(int value, String name) {
  if (value <= 0 || value > 0xffff) {
    throwInvalidArgument('$name must be between 1 and 65535');
  }
  return value;
}

Pointer<Uint8> _nativeBytes(Uint8List? bytes, Allocator allocator) {
  if (bytes == null || bytes.isEmpty) {
    return nullptr.cast<Uint8>();
  }
  final nativeBytes = allocator<Uint8>(bytes.length);
  nativeBytes.asTypedList(bytes.length).setAll(0, bytes);
  return nativeBytes;
}

OfflineRegionInfo _offlineRegionInfoFromNative(
  raw.mln_offline_region_info info,
) {
  return OfflineRegionInfo(
    id: info.id,
    definition: _offlineRegionDefinitionFromNative(info.definition),
    metadata: info.metadata == nullptr || info.metadata_size == 0
        ? Uint8List(0)
        : Uint8List.fromList(info.metadata.asTypedList(info.metadata_size)),
  );
}

OfflineRegionDefinition _offlineRegionDefinitionFromNative(
  raw.mln_offline_region_definition definition,
) {
  final tilePyramidTag = raw
      .mln_offline_region_definition_type
      .MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
      .value;
  final geometryTag = raw
      .mln_offline_region_definition_type
      .MLN_OFFLINE_REGION_DEFINITION_GEOMETRY
      .value;
  if (definition.type == tilePyramidTag) {
    final tilePyramid = definition.data.tile_pyramid;
    return OfflineTilePyramidRegionDefinition(
      styleUrl: tilePyramid.style_url.cast<Utf8>().toDartString(),
      bounds: native_struct.latLngBoundsFromNative(tilePyramid.bounds),
      minZoom: tilePyramid.min_zoom,
      maxZoom: tilePyramid.max_zoom,
      pixelRatio: tilePyramid.pixel_ratio,
      includeIdeographs: tilePyramid.include_ideographs,
    );
  }
  if (definition.type != geometryTag) {
    return UnknownOfflineRegionDefinition(definition.type);
  }
  final geometry = definition.data.geometry;
  return OfflineGeometryRegionDefinition(
    styleUrl: geometry.style_url.cast<Utf8>().toDartString(),
    geometry: Uint8List.fromList(
      geometry.geometry.data.cast<Uint8>().asTypedList(geometry.geometry.size),
    ),
    minZoom: geometry.min_zoom,
    maxZoom: geometry.max_zoom,
    pixelRatio: geometry.pixel_ratio,
    includeIdeographs: geometry.include_ideographs,
  );
}

OfflineRegionStatus _offlineRegionStatusFromNative(
  raw.mln_offline_region_status status,
) {
  return OfflineRegionStatus(
    downloadState: OfflineRegionDownloadState.fromRawValue(
      status.download_state,
    ),
    completedResourceCount: uint64FromNative(status.completed_resource_count),
    completedResourceSize: uint64FromNative(status.completed_resource_size),
    completedTileCount: uint64FromNative(status.completed_tile_count),
    requiredTileCount: uint64FromNative(status.required_tile_count),
    completedTileSize: uint64FromNative(status.completed_tile_size),
    requiredResourceCount: uint64FromNative(status.required_resource_count),
    requiredResourceCountIsPrecise: status.required_resource_count_is_precise,
    complete: status.complete,
  );
}
