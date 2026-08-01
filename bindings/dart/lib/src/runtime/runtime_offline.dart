part of 'runtime.dart';

final class OfflineOperationHandle implements Finalizable {
  OfflineOperationHandle._(
    this._runtime,
    this._id,
    this._kind,
    this._resultKind,
  ) {
    _runtime._registerOfflineOperation(this);
    _leakReporter = NativeLeakReporter(
      this,
      'OfflineOperationHandle',
      NativeHandle(_id == 0 ? 1 : _id),
    );
  }

  final RuntimeHandle _runtime;
  final _OfflineOperationKind _kind;
  final _OfflineOperationResultKind _resultKind;

  final int _id;
  late final NativeLeakReporter _leakReporter;

  var _discarded = false;

  /// Whether this operation has been discarded by Dart.
  bool get isDiscarded => _discarded;

  /// Takes a completed offline region create result.
  OfflineRegionInfo takeCreatedRegion() {
    _requireResult(
      _OfflineOperationKind.regionCreate,
      _OfflineOperationResultKind.region,
      'takeCreatedRegion',
    );
    return withNativeArena((arena) {
      final outRegion = arena<Uint64>();
      outRegion.value = 0;
      _check(
        raw.mln_runtime_offline_region_create_take_result(
          _runtime._handle.raw,
          _id,
          outRegion,
        ),
      );
      _markDiscarded();
      return _copyOfflineRegionSnapshot(
        NativeOfflineRegionSnapshot(outRegion.value),
      );
    });
  }

  /// Takes a completed optional offline region get result.
  OfflineRegionInfo? takeOptionalRegion() {
    _requireResult(
      _OfflineOperationKind.regionGet,
      _OfflineOperationResultKind.optionalRegion,
      'takeOptionalRegion',
    );
    return withNativeArena((arena) {
      final outRegion = arena<Uint64>();
      outRegion.value = 0;
      final outFound = arena<Bool>();
      _check(
        raw.mln_runtime_offline_region_get_take_result(
          _runtime._handle.raw,
          _id,
          outRegion,
          outFound,
        ),
      );
      _markDiscarded();
      return outFound.value
          ? _copyOfflineRegionSnapshot(
              NativeOfflineRegionSnapshot(outRegion.value),
            )
          : null;
    });
  }

  /// Takes a completed offline regions list result.
  List<OfflineRegionInfo> takeRegionList() {
    _requireResult(
      _OfflineOperationKind.regionsList,
      _OfflineOperationResultKind.regionList,
      'takeRegionList',
    );
    return withNativeArena((arena) {
      final outRegions = arena<Uint64>();
      outRegions.value = 0;
      _check(
        raw.mln_runtime_offline_regions_list_take_result(
          _runtime._handle.raw,
          _id,
          outRegions,
        ),
      );
      _markDiscarded();
      return _copyOfflineRegionList(NativeOfflineRegionList(outRegions.value));
    });
  }

  /// Takes a completed offline regions merge result.
  List<OfflineRegionInfo> takeMergedRegionList() {
    _requireResult(
      _OfflineOperationKind.regionsMergeDatabase,
      _OfflineOperationResultKind.regionList,
      'takeMergedRegionList',
    );
    return withNativeArena((arena) {
      final outRegions = arena<Uint64>();
      outRegions.value = 0;
      _check(
        raw.mln_runtime_offline_regions_merge_database_take_result(
          _runtime._handle.raw,
          _id,
          outRegions,
        ),
      );
      _markDiscarded();
      return _copyOfflineRegionList(NativeOfflineRegionList(outRegions.value));
    });
  }

  /// Takes a completed offline region metadata update result.
  OfflineRegionInfo takeUpdatedRegionMetadata() {
    _requireResult(
      _OfflineOperationKind.regionUpdateMetadata,
      _OfflineOperationResultKind.region,
      'takeUpdatedRegionMetadata',
    );
    return withNativeArena((arena) {
      final outRegion = arena<Uint64>();
      outRegion.value = 0;
      _check(
        raw.mln_runtime_offline_region_update_metadata_take_result(
          _runtime._handle.raw,
          _id,
          outRegion,
        ),
      );
      _markDiscarded();
      return _copyOfflineRegionSnapshot(
        NativeOfflineRegionSnapshot(outRegion.value),
      );
    });
  }

  /// Takes a completed offline region status result.
  OfflineRegionStatus takeRegionStatus() {
    _requireResult(
      _OfflineOperationKind.regionGetStatus,
      _OfflineOperationResultKind.regionStatus,
      'takeRegionStatus',
    );
    return withNativeArena((arena) {
      final outStatus = arena<raw.mln_offline_region_status>();
      outStatus.ref.size = sizeOf<raw.mln_offline_region_status>();
      _check(
        raw.mln_runtime_offline_region_get_status_take_result(
          _runtime._handle.raw,
          _id,
          outStatus,
        ),
      );
      _markDiscarded();
      return _offlineRegionStatusFromNative(outStatus.ref);
    });
  }

  /// Discards runtime-owned state for this operation.
  void discard() {
    if (_discarded) {
      return;
    }
    _check(
      raw.mln_runtime_offline_operation_discard(_runtime._handle.raw, _id),
    );
    _markDiscarded();
  }

  void _markDiscarded() {
    _discarded = true;
    _runtime._unregisterOfflineOperation(_id);
    _leakReporter.close();
  }

  void _requireResult(
    _OfflineOperationKind expectedKind,
    _OfflineOperationResultKind expected,
    String accessorName,
  ) {
    if (_discarded) {
      throwInvalidState('offline operation has been discarded');
    }
    if (_kind != expectedKind || _resultKind != expected) {
      throwInvalidState(
        '$accessorName cannot take ${_resultKind.name} result from '
        '${_kind.name} operation; expected ${expected.name} result from '
        '${expectedKind.name}',
      );
    }
  }
}

enum _OfflineOperationKind {
  ambientCache('ambient cache'),
  regionCreate('region create'),
  regionGet('region get'),
  regionsList('regions list'),
  regionsMergeDatabase('regions merge database'),
  regionUpdateMetadata('region update metadata'),
  regionGetStatus('region get status'),
  regionSetObserved('region set observed'),
  regionSetDownloadState('region set download state'),
  regionInvalidate('region invalidate'),
  regionDelete('region delete'),
  setMaximumAmbientCacheSize('set maximum ambient cache size');

  const _OfflineOperationKind(this.name);

  final String name;
}

enum _OfflineOperationResultKind {
  none('none'),
  region('region'),
  optionalRegion('optional region'),
  regionList('region list'),
  regionStatus('region status');

  const _OfflineOperationResultKind(this.name);

  final String name;
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
  result.geometry = native_geometry
      .nativeGeometry(definition.geometry, allocator)
      .pointer;
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
  for (var index = 0; index < bytes.length; index += 1) {
    nativeBytes[index] = bytes[index];
  }
  return nativeBytes;
}

OfflineRegionInfo _copyOfflineRegionSnapshot(
  NativeOfflineRegionSnapshot snapshot,
) {
  try {
    return withNativeArena((arena) {
      final outInfo = arena<raw.mln_offline_region_info>();
      outInfo.ref.size = sizeOf<raw.mln_offline_region_info>();
      _check(raw.mln_offline_region_snapshot_get(snapshot.raw, outInfo));
      return _offlineRegionInfoFromNative(outInfo.ref);
    });
  } finally {
    raw.mln_offline_region_snapshot_destroy(snapshot.raw);
  }
}

List<OfflineRegionInfo> _copyOfflineRegionList(NativeOfflineRegionList list) {
  try {
    return withNativeArena((arena) {
      final outCount = arena<Size>();
      _check(raw.mln_offline_region_list_count(list.raw, outCount));
      return [
        for (var index = 0; index < outCount.value; index += 1)
          _copyOfflineRegionListEntry(list, index, arena),
      ];
    });
  } finally {
    raw.mln_offline_region_list_destroy(list.raw);
  }
}

OfflineRegionInfo _copyOfflineRegionListEntry(
  NativeOfflineRegionList list,
  int index,
  Allocator allocator,
) {
  final outInfo = allocator<raw.mln_offline_region_info>();
  outInfo.ref.size = sizeOf<raw.mln_offline_region_info>();
  _check(raw.mln_offline_region_list_get(list.raw, index, outInfo));
  return _offlineRegionInfoFromNative(outInfo.ref);
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
    geometry: native_geometry.geometryFromNative(geometry.geometry.ref),
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

/// Copied runtime event returned by [RuntimeHandle.pollEvent].
