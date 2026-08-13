part of 'runtime.dart';

final class OperationHandle implements Finalizable {
  OperationHandle._(this._runtime, this._id, this._kind, this._resultKind) {
    _runtime._registerOperation(this);
    _leakReporter = NativeLeakReporter(
      this,
      'OperationHandle',
      NativeHandle(_id == 0 ? 1 : _id),
    );
  }

  final RuntimeHandle _runtime;
  final _OfflineOperationKind _kind;
  final _OfflineOperationResultKind _resultKind;

  final int _id;
  late final NativeLeakReporter _leakReporter;

  var _resultConsumed = false;
  var _released = false;

  /// Whether the operation observer has been released.
  bool get isReleased => _released;

  /// Whether a typed take or discard consumed this operation's result.
  bool get isResultConsumed => _resultConsumed;

  /// Reports whether this operation has reached a terminal disposition.
  bool poll() {
    _requireLive();
    return withNativeArena((arena) {
      final outCompleted = arena<Bool>();
      _check(raw.mln_operation_poll(_id, outCompleted));
      return outCompleted.value;
    });
  }

  /// Waits for this operation to reach a terminal disposition.
  ///
  /// A null or negative [timeout] waits without a deadline. A zero timeout
  /// performs a nonblocking check. A positive timeout waits for at most that
  /// duration and returns false when the deadline expires.
  bool wait({Duration? timeout}) {
    _requireLive();
    final timeoutMilliseconds = switch (timeout) {
      null => -1,
      final value when value.isNegative => -1,
      final value => value.inMilliseconds,
    };
    return withNativeArena((arena) {
      final outCompleted = arena<Bool>();
      _check(raw.mln_operation_wait(_id, timeoutMilliseconds, outCompleted));
      return outCompleted.value;
    });
  }

  /// Requests cancellation of this operation.
  void cancel() {
    _requireLive();
    _check(raw.mln_operation_cancel(_id));
  }

  /// Reports the completed operation's terminal status.
  MaplibreStatus get terminalStatus {
    _requireLive();
    return withNativeArena((arena) {
      final outStatus = arena<Int32>();
      _check(raw.mln_operation_get_status(_id, outStatus));
      return MaplibreStatus.fromNativeStatusCode(outStatus.value);
    });
  }

  /// Copies the completed operation's diagnostic.
  String get diagnostic {
    _requireLive();
    return withNativeArena((arena) {
      final outSize = arena<Size>();
      _check(raw.mln_operation_copy_diagnostic(_id, nullptr, 0, outSize));
      final size = outSize.value;
      if (size == 0) {
        return '';
      }
      final bytes = arena<Uint8>(size);
      _check(
        raw.mln_operation_copy_diagnostic(
          _id,
          bytes.cast<Char>(),
          size,
          outSize,
        ),
      );
      return utf8.decode(bytes.asTypedList(outSize.value));
    });
  }

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
      _check(raw.mln_runtime_offline_region_create_take_result(_id, outRegion));
      _markResultConsumed();
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
          _id,
          outRegion,
          outFound,
        ),
      );
      _markResultConsumed();
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
      _check(raw.mln_runtime_offline_regions_list_take_result(_id, outRegions));
      _markResultConsumed();
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
          _id,
          outRegions,
        ),
      );
      _markResultConsumed();
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
          _id,
          outRegion,
        ),
      );
      _markResultConsumed();
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
        raw.mln_runtime_offline_region_get_status_take_result(_id, outStatus),
      );
      _markResultConsumed();
      return _offlineRegionStatusFromNative(outStatus.ref);
    });
  }

  /// Discards the completed operation's untaken result.
  void discardResult() {
    _requireLive();
    if (_resultConsumed) {
      throwInvalidState('offline operation result has already been consumed');
    }
    _check(raw.mln_operation_discard_result(_id));
    _markResultConsumed();
  }

  /// Releases this operation observer and any untaken result.
  void release() {
    if (_released) {
      return;
    }
    _released = true;
    _runtime._unregisterOperationId(_id);
    _leakReporter.close();
    raw.mln_operation_release(_id);
  }

  void _markResultConsumed() {
    _resultConsumed = true;
  }

  void _requireResult(
    _OfflineOperationKind expectedKind,
    _OfflineOperationResultKind expected,
    String accessorName,
  ) {
    _requireLive();
    if (_resultConsumed) {
      throwInvalidState('offline operation result has already been consumed');
    }
    if (_kind != expectedKind || _resultKind != expected) {
      throwInvalidState(
        '$accessorName cannot take ${_resultKind.name} result from '
        '${_kind.name} operation; expected ${expected.name} result from '
        '${expectedKind.name}',
      );
    }
  }

  void _requireLive() {
    if (_released) {
      throwInvalidState('offline operation has been released');
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
