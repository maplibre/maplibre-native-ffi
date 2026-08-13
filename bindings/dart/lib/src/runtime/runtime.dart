import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../camera/camera.dart';
import '../error/maplibre_exception.dart';
import '../geo/geo.dart';
import '../internal/callback/callback_state.dart';
import '../internal/c/maplibre_native_c.dart';
import '../internal/c/maplibre_native_c.g.dart' as raw;
import '../internal/lifecycle/lifecycle.dart';
import '../internal/lifecycle/native_handles.dart';
import '../internal/lifecycle/frame_construction.dart';
import '../internal/memory/memory.dart';
import '../internal/status/status.dart';
import '../internal/struct/struct.dart' as native_struct;
import '../internal/value/uint64.dart';
import '../offline/offline.dart';
import '../query/query.dart';
import '../render/native_pointer.dart';
import '../render/targets.dart';
import '../resource/resource.dart';
import '../style/style.dart';

part 'runtime_resource_callbacks.dart';
part 'runtime_offline.dart';
part 'runtime_render_handles.dart';
part 'runtime_native_conversions.dart';

final MaplibreNativeCApi _c = MaplibreNativeCApi.open();

const int _resourceKindWildcard = 0xffffffff;

/// Read-only view of one custom-geometry callback root for lifecycle tests.
final class CustomGeometryCallbackLifecycleProbe {
  CustomGeometryCallbackLifecycleProbe._(this._state);

  final _CustomGeometryCallbackState _state;

  bool get retirementQueued => _state.retirementQueuedForTesting;
  bool get closed => _state.closedForTesting;
}

/// Returns the callback root currently owned by [map] for lifecycle tests.
CustomGeometryCallbackLifecycleProbe? customGeometryCallbackProbeForTesting(
  MapHandle map,
  String sourceId,
) {
  final state =
      map._customGeometryCallbacks[sourceId] ??
      map._pendingStyleCallbacks.values
          .where((pending) => pending.sourceId == sourceId)
          .firstOrNull
          ?.state;
  return state == null ? null : CustomGeometryCallbackLifecycleProbe._(state);
}

/// Dart resource provider callback run asynchronously on its receiver isolate.
typedef ResourceProviderCallback =
    void Function(ResourceRequest request, ResourceRequestHandle handle);

/// Receiver-isolate resource provider definition.
final class ResourceProvider {
  /// Creates a resource provider with native-owned routing rules.
  ResourceProvider({
    required List<ResourceProviderRoute> routes,
    required this.callback,
  }) : routes = List.unmodifiable(routes);

  /// Exact routes handled by this provider.
  final List<ResourceProviderRoute> routes;

  /// Callback invoked on the receiver isolate for matching requests.
  final ResourceProviderCallback callback;
}

/// Runtime creation options.
final class RuntimeOptions {
  /// Creates runtime options.
  const RuntimeOptions({
    this.assetPath,
    this.cachePath,
    RuntimeEventMask? eventMask,
  }) : _eventMask = eventMask;

  /// Filesystem root for `asset://` URLs.
  final String? assetPath;

  /// Cache database path.
  final String? cachePath;

  final RuntimeEventMask? _eventMask;

  /// Runtime-originated event types this runtime queues.
  ///
  /// Defaults to the C options default's selection, which is every event type
  /// the loaded library reports. A bit a newer library selects and this binding
  /// does not name is kept, and its events reach a host as unknown event and
  /// payload domains. See [RuntimeHandle.setEventMask].
  RuntimeEventMask get eventMask =>
      _eventMask ??
      RuntimeEventMask(raw.mln_runtime_options_default().event_mask);
}

/// Runtime handle with autonomous native execution.
final class RuntimeHandle {
  RuntimeHandle._(NativeRuntime handle, this._notificationSource)
    : _state = NativeHandleState(handle, 'RuntimeHandle') {
    _notificationListener =
        NativeCallable<raw.mln_notification_callbackFunction>.listener((
          Pointer<Void> _,
        ) {
          scheduleMicrotask(_drainNotificationSource);
        });
    try {
      _check(
        raw.mln_notification_source_set_callback(
          _notificationSource,
          _notificationListener.nativeFunction,
          nullptr,
        ),
      );
    } catch (_) {
      _notificationListener.close();
      rethrow;
    }
  }
  final NativeHandleState<NativeRuntime> _state;
  int _notificationSource;
  late final NativeCallable<raw.mln_notification_callbackFunction>
  _notificationListener;
  final _maps = <int, WeakReference<MapHandle>>{};
  final _operations = <int, WeakReference<OperationHandle>>{};
  final _operationWaiters = <int, Completer<void>>{};
  final _queuedRuntimeEvents = <RuntimeEvent>[];
  _ResourceTransformState? _resourceTransformState;
  _HttpHeaderTransformState? _httpHeaderTransformState;
  _ResourceProviderRulesState? _resourceProviderRulesState;
  _ResourceProviderCallbackState? _resourceProviderCallbackState;
  final _pendingResourceCommands = <int, void Function(bool committed)>{};
  final _resourceProviderQueues = <int, _ResourceProviderCallbackState>{};

  /// Creates a runtime without blocking the calling isolate.
  static Future<RuntimeHandle> create({
    RuntimeOptions options = const RuntimeOptions(),
  }) async {
    ensureAbiVersion();
    final source = withNativeArena((arena) {
      final outSource = arena<Uint64>()..value = 0;
      _check(raw.mln_notification_source_create(outSource));
      return outSource.value;
    });
    var operation = 0;
    var runtimeHandle = 0;
    try {
      operation = withNativeArena((arena) {
        final nativeOptions = arena<raw.mln_runtime_options>();
        nativeOptions.ref = _runtimeOptionsToNative(options, arena);
        nativeOptions.ref.notification_source = source;
        final outOperation = arena<Uint64>()..value = 0;
        _check(raw.mln_runtime_create_start(nativeOptions, outOperation));
        return outOperation.value;
      });
      await _waitForStandaloneOperation(source, operation);
      _throwIfOperationFailed(operation);
      runtimeHandle = withNativeArena((arena) {
        final outRuntime = arena<Uint64>()..value = 0;
        _check(raw.mln_runtime_create_take_result(operation, outRuntime));
        return outRuntime.value;
      });
      raw.mln_operation_release(operation);
      operation = 0;
      final runtime = RuntimeHandle._(NativeRuntime(runtimeHandle), source);
      runtimeHandle = 0;
      return runtime;
    } catch (_) {
      if (operation != 0) {
        raw.mln_operation_release(operation);
      }
      if (runtimeHandle != 0) {
        final closeOperation = withNativeArena((arena) {
          final outOperation = arena<Uint64>()..value = 0;
          _check(raw.mln_runtime_close_start(runtimeHandle, outOperation));
          return outOperation.value;
        });
        await _waitForStandaloneOperation(source, closeOperation);
        raw.mln_operation_release(closeOperation);
      }
      raw.mln_notification_source_close(source);
      rethrow;
    }
  }

  NativeRuntime get _handle => _state.handle;

  /// Whether this runtime has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  /// Drains queued runtime events into one batch of Dart-owned copies.
  ///
  /// Events arrive in queue order. Every field, message, and payload is copied
  /// before this returns, so a batch stays readable for as long as the host
  /// keeps it.
  ///
  /// The default zero [maxEvents] drains every queued event. A positive value
  /// drains at most that many and reports the rest in
  /// [RuntimeEventBatch.remainingCount].
  RuntimeEventBatch drainEvents({int maxEvents = 0}) {
    final _ = _handle;
    if (maxEvents < 0) {
      throwInvalidArgument('maxEvents must not be negative');
    }
    _queuedRuntimeEvents.addAll(_drainNativeEvents().events);
    final count = maxEvents == 0 || maxEvents > _queuedRuntimeEvents.length
        ? _queuedRuntimeEvents.length
        : maxEvents;
    final events = _queuedRuntimeEvents.sublist(0, count);
    _queuedRuntimeEvents.removeRange(0, count);
    return RuntimeEventBatch._(
      events: events,
      remainingCount: _queuedRuntimeEvents.length,
    );
  }

  RuntimeEventBatch _drainNativeEvents() {
    return withNativeArena((arena) {
      final outBatch = arena<Uint64>()..value = 0;
      _check(raw.mln_runtime_drain_events(_handle.raw, 0, outBatch));
      final view = arena<raw.mln_runtime_event_batch_view>();
      view.ref.size = sizeOf<raw.mln_runtime_event_batch_view>();
      try {
        _check(raw.mln_event_batch_get(outBatch.value, view));
        return RuntimeEventBatch._fromNative(view.ref, this);
      } finally {
        raw.mln_event_batch_release(outBatch.value);
      }
    });
  }

  /// Selects which runtime-originated event types this runtime queues.
  ///
  /// The runtime reads the bits in [RuntimeEventMask.allRuntimeEvents] and
  /// ignores the rest, so [RuntimeEventMask.all] selects every
  /// runtime-originated type. Narrowing gates later events and keeps queued
  /// ones. A bit outside [RuntimeEventMask.all] reports invalid argument.
  void setEventMask(RuntimeEventMask mask) {
    _check(raw.mln_runtime_set_event_mask(_handle.raw, mask.value));
  }

  /// Reports which runtime-originated event types this runtime queues.
  ///
  /// This reports the mask the runtime was created with or last narrowed to.
  RuntimeEventMask get eventMask {
    return withNativeArena((arena) {
      final outMask = arena<Uint64>();
      _check(raw.mln_runtime_get_event_mask(_handle.raw, outMask));
      return RuntimeEventMask(outMask.value);
    });
  }

  /// Registers exact native-owned URL rewrite rules for network resources.
  ///
  /// Returns the accepted runtime-wide command ID.
  BigInt setResourceUrlRewriteRules(List<ResourceUrlRewriteRule> rules) {
    final state = _ResourceTransformState(rules);
    try {
      final commandId = withNativeArena((arena) {
        final transform = arena<raw.mln_resource_transform>();
        transform.ref.size = sizeOf<raw.mln_resource_transform>();
        transform.ref.callback = _c.adapterResourceTransformRewriteCallback();
        transform.ref.user_data = state.pointer.cast<Void>();
        final outCommandId = arena<Uint64>()..value = 0;
        _check(
          raw.mln_runtime_set_resource_transform(
            _handle.raw,
            transform,
            outCommandId,
          ),
        );
        return outCommandId.value;
      });
      _recordResourceCommand(commandId, (committed) {
        if (committed) {
          _resourceTransformState?.close();
          _resourceTransformState = state;
        } else {
          state.close();
        }
      });
      return uint64FromNative(commandId);
    } catch (_) {
      state.close();
      rethrow;
    }
  }

  /// Clears runtime-scoped URL rewrite rules.
  ///
  /// Returns the accepted runtime-wide command ID.
  BigInt clearResourceTransform() {
    final commandId = withNativeArena((arena) {
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_runtime_clear_resource_transform(_handle.raw, outCommandId),
      );
      return outCommandId.value;
    });
    _recordResourceCommand(commandId, (committed) {
      if (committed) {
        _resourceTransformState?.close();
        _resourceTransformState = null;
      }
    });
    return uint64FromNative(commandId);
  }

  /// Registers native-owned HTTP header routes evaluated on network threads.
  ///
  /// Returns the accepted runtime-wide command ID.
  BigInt setHttpHeaderTransformRules(List<HttpHeaderTransformRule> rules) {
    final state = _HttpHeaderTransformState(rules);
    try {
      final commandId = withNativeArena((arena) {
        final transform = arena<raw.mln_http_header_transform>();
        transform.ref.size = sizeOf<raw.mln_http_header_transform>();
        transform.ref.callback = _c.adapterHttpHeaderTransformCallback();
        transform.ref.user_data = state.pointer.cast<Void>();
        final outCommandId = arena<Uint64>()..value = 0;
        _check(
          raw.mln_runtime_set_http_header_transform(
            _handle.raw,
            transform,
            outCommandId,
          ),
        );
        return outCommandId.value;
      });
      _recordResourceCommand(commandId, (committed) {
        if (committed) {
          _httpHeaderTransformState?.close();
          _httpHeaderTransformState = state;
        } else {
          state.close();
        }
      });
      return uint64FromNative(commandId);
    } catch (_) {
      state.close();
      rethrow;
    }
  }

  /// Clears native-owned HTTP header transform routes.
  ///
  /// Returns the accepted runtime-wide command ID.
  BigInt clearHttpHeaderTransform() {
    final commandId = withNativeArena((arena) {
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_runtime_clear_http_header_transform(_handle.raw, outCommandId),
      );
      return outCommandId.value;
    });
    _recordResourceCommand(commandId, (committed) {
      if (committed) {
        _httpHeaderTransformState?.close();
        _httpHeaderTransformState = null;
      }
    });
    return uint64FromNative(commandId);
  }

  /// Registers or replaces exact native-owned response rules.
  ///
  /// Returns the accepted runtime-wide command ID.
  BigInt setResourceProviderRules(List<ResourceProviderRule> rules) {
    final state = _ResourceProviderRulesState(rules);
    try {
      final commandId = withNativeArena((arena) {
        final provider = arena<raw.mln_resource_provider>();
        provider.ref.size = sizeOf<raw.mln_resource_provider>();
        provider.ref.callback = _c.adapterResourceProviderRulesCallback();
        provider.ref.user_data = state.pointer.cast<Void>();
        final outCommandId = arena<Uint64>()..value = 0;
        _check(
          raw.mln_runtime_set_resource_provider(
            _handle.raw,
            provider,
            outCommandId,
          ),
        );
        return outCommandId.value;
      });
      _recordResourceCommand(commandId, (committed) {
        if (committed) {
          _closeActiveResourceProvider();
          _resourceProviderRulesState = state;
        } else {
          state.close();
        }
      });
      return uint64FromNative(commandId);
    } catch (_) {
      state.close();
      rethrow;
    }
  }

  /// Registers or replaces a queued Dart resource provider callback.
  ///
  /// Returns the accepted runtime-wide command ID.
  BigInt setResourceProvider(ResourceProvider provider) {
    final state = _ResourceProviderCallbackState(provider, _notificationSource);
    _resourceProviderQueues[state.queue] = state;
    try {
      final commandId = withNativeArena((arena) {
        final nativeProvider = arena<raw.mln_resource_provider>();
        nativeProvider.ref.size = sizeOf<raw.mln_resource_provider>();
        nativeProvider.ref.callback = _c
            .adapterQueuedResourceProviderCallback();
        nativeProvider.ref.user_data = state.pointer.cast<Void>();
        final outCommandId = arena<Uint64>()..value = 0;
        _check(
          raw.mln_runtime_set_resource_provider(
            _handle.raw,
            nativeProvider,
            outCommandId,
          ),
        );
        return outCommandId.value;
      });
      _recordResourceCommand(commandId, (committed) {
        if (committed) {
          _closeActiveResourceProvider();
          _resourceProviderCallbackState = state;
        } else {
          _closeResourceProviderCallback(state);
        }
      });
      return uint64FromNative(commandId);
    } catch (_) {
      _closeResourceProviderCallback(state);
      rethrow;
    }
  }

  /// Clears the runtime-scoped network resource provider.
  ///
  /// Returns the accepted runtime-wide command ID.
  BigInt clearResourceProvider() {
    final commandId = withNativeArena((arena) {
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_runtime_clear_resource_provider(_handle.raw, outCommandId),
      );
      return outCommandId.value;
    });
    _recordResourceCommand(commandId, (committed) {
      if (committed) {
        _closeActiveResourceProvider();
      }
    });
    return uint64FromNative(commandId);
  }

  void _recordResourceCommand(
    int commandId,
    void Function(bool committed) finish,
  ) {
    _pendingResourceCommands[commandId] = finish;
  }

  void _finishResourceCommand(int commandId, CommandDisposition disposition) {
    _pendingResourceCommands
        .remove(commandId)
        ?.call(disposition == CommandDisposition.committed);
  }

  void _closeResourceProviderCallback(_ResourceProviderCallbackState state) {
    _resourceProviderQueues.remove(state.queue);
    state.retire();
  }

  void _closeActiveResourceProvider() {
    _resourceProviderRulesState?.close();
    _resourceProviderRulesState = null;
    final callbackState = _resourceProviderCallbackState;
    if (callbackState != null) {
      _closeResourceProviderCallback(callbackState);
      _resourceProviderCallbackState = null;
    }
  }

  void _drainNotificationSource() {
    if (_notificationSource == 0) {
      return;
    }
    for (final endpoint in _drainReadyEndpoints(_notificationSource)) {
      if (endpoint.kind ==
          raw
              .mln_notification_endpoint_kind
              .MLN_NOTIFICATION_ENDPOINT_OPERATION
              .value) {
        _operationWaiters.remove(endpoint.id)?.complete();
      } else if (endpoint.kind ==
          raw
              .mln_notification_endpoint_kind
              .MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS
              .value) {
        _queuedRuntimeEvents.addAll(_drainNativeEvents().events);
      } else if (endpoint.kind ==
          raw
              .mln_notification_endpoint_kind
              .MLN_NOTIFICATION_ENDPOINT_ADAPTER_RESOURCE_REQUESTS
              .value) {
        _resourceProviderQueues[endpoint.id]?.drain();
      }
    }
  }

  Future<void> _waitForOperation(int operation) async {
    await Future<void>.delayed(Duration.zero);
    if (_operationCompleted(operation)) {
      return;
    }
    final completer = Completer<void>();
    _operationWaiters[operation] = completer;
    if (_operationCompleted(operation)) {
      _operationWaiters.remove(operation);
      return;
    }
    await completer.future;
  }

  Future<T> _takeOperation<T>(int operation, T Function() takeResult) async {
    try {
      await _waitForOperation(operation);
      _throwIfOperationFailed(operation);
      return takeResult();
    } finally {
      _operationWaiters.remove(operation);
      raw.mln_operation_release(operation);
    }
  }

  Future<void> _finishOperation(int operation) =>
      _takeOperation<void>(operation, () {});

  /// Starts an ambient cache maintenance operation.
  OperationHandle runAmbientCacheOperation(AmbientCacheOperation operation) {
    return withNativeArena((arena) {
      final outOperationId = arena<Uint64>();
      _check(
        raw.mln_runtime_run_ambient_cache_operation_start(
          _handle.raw,
          operation.rawValue,
          outOperationId,
        ),
      );
      return OperationHandle._(
        this,
        outOperationId.value,
        _OfflineOperationKind.ambientCache,
        _OfflineOperationResultKind.none,
      );
    });
  }

  /// Starts a change to this runtime's maximum ambient cache size.
  ///
  /// MapLibre evicts ambient resources to fit the new budget, so lowering it
  /// discards cached resources. Offline regions are unaffected.
  OperationHandle setMaximumAmbientCacheSize(BigInt size) {
    return withNativeArena((arena) {
      final outOperationId = arena<Uint64>();
      _check(
        raw.mln_runtime_set_maximum_ambient_cache_size_start(
          _handle.raw,
          uint64ToNative(size, 'maximum ambient cache size'),
          outOperationId,
        ),
      );
      return OperationHandle._(
        this,
        outOperationId.value,
        _OfflineOperationKind.setMaximumAmbientCacheSize,
        _OfflineOperationResultKind.none,
      );
    });
  }

  /// Starts creating an offline region.
  OperationHandle createOfflineRegion(
    OfflineRegionDefinition definition, {
    Uint8List? metadata,
  }) {
    return withNativeArena((arena) {
      final nativeDefinition = arena<raw.mln_offline_region_definition>();
      nativeDefinition.ref = _offlineRegionDefinitionToNative(
        definition,
        arena,
      );
      final nativeMetadata = _nativeBytes(metadata, arena);
      final outOperationId = arena<Uint64>();
      _check(
        raw.mln_runtime_offline_region_create_start(
          _handle.raw,
          nativeDefinition,
          nativeMetadata,
          metadata?.length ?? 0,
          outOperationId,
        ),
      );
      return OperationHandle._(
        this,
        outOperationId.value,
        _OfflineOperationKind.regionCreate,
        _OfflineOperationResultKind.region,
      );
    });
  }

  /// Starts getting an offline region snapshot by ID.
  OperationHandle getOfflineRegion(int regionId) => _startOfflineOperation(
    _OfflineOperationKind.regionGet,
    _OfflineOperationResultKind.optionalRegion,
    (outOperationId) {
      _check(
        raw.mln_runtime_offline_region_get_start(
          _handle.raw,
          regionId,
          outOperationId,
        ),
      );
    },
  );

  /// Starts listing offline region snapshots.
  OperationHandle listOfflineRegions() => _startOfflineOperation(
    _OfflineOperationKind.regionsList,
    _OfflineOperationResultKind.regionList,
    (outOperationId) {
      _check(
        raw.mln_runtime_offline_regions_list_start(_handle.raw, outOperationId),
      );
    },
  );

  /// Starts merging offline regions from another database path.
  OperationHandle mergeOfflineRegionDatabase(String sideDatabasePath) {
    return withNativeArena((arena) {
      final nativePath = nativeUtf8CString(sideDatabasePath, arena);
      final outOperationId = arena<Uint64>();
      _check(
        raw.mln_runtime_offline_regions_merge_database_start(
          _handle.raw,
          nativePath.pointer.cast<Char>(),
          outOperationId,
        ),
      );
      return OperationHandle._(
        this,
        outOperationId.value,
        _OfflineOperationKind.regionsMergeDatabase,
        _OfflineOperationResultKind.regionList,
      );
    });
  }

  /// Starts updating opaque offline region metadata.
  OperationHandle updateOfflineRegionMetadata(
    int regionId,
    Uint8List metadata,
  ) {
    return withNativeArena((arena) {
      final nativeMetadata = _nativeBytes(metadata, arena);
      final outOperationId = arena<Uint64>();
      _check(
        raw.mln_runtime_offline_region_update_metadata_start(
          _handle.raw,
          regionId,
          nativeMetadata,
          metadata.length,
          outOperationId,
        ),
      );
      return OperationHandle._(
        this,
        outOperationId.value,
        _OfflineOperationKind.regionUpdateMetadata,
        _OfflineOperationResultKind.region,
      );
    });
  }

  /// Starts getting the current offline region status.
  OperationHandle getOfflineRegionStatus(int regionId) =>
      _startOfflineOperation(
        _OfflineOperationKind.regionGetStatus,
        _OfflineOperationResultKind.regionStatus,
        (outOperationId) {
          _check(
            raw.mln_runtime_offline_region_get_status_start(
              _handle.raw,
              regionId,
              outOperationId,
            ),
          );
        },
      );

  /// Starts enabling or disabling offline region observation.
  OperationHandle setOfflineRegionObserved(int regionId, bool observed) =>
      _startOfflineOperation(
        _OfflineOperationKind.regionSetObserved,
        _OfflineOperationResultKind.none,
        (outOperationId) {
          _check(
            raw.mln_runtime_offline_region_set_observed_start(
              _handle.raw,
              regionId,
              observed,
              outOperationId,
            ),
          );
        },
      );

  /// Starts changing an offline region's download state.
  OperationHandle setOfflineRegionDownloadState(
    int regionId,
    OfflineRegionDownloadState state,
  ) => _startOfflineOperation(
    _OfflineOperationKind.regionSetDownloadState,
    _OfflineOperationResultKind.none,
    (outOperationId) {
      _check(
        raw.mln_runtime_offline_region_set_download_state_start(
          _handle.raw,
          regionId,
          state.rawValue,
          outOperationId,
        ),
      );
    },
  );

  /// Starts invalidating cached resources for an offline region.
  OperationHandle invalidateOfflineRegion(int regionId) =>
      _startOfflineOperation(
        _OfflineOperationKind.regionInvalidate,
        _OfflineOperationResultKind.none,
        (outOperationId) {
          _check(
            raw.mln_runtime_offline_region_invalidate_start(
              _handle.raw,
              regionId,
              outOperationId,
            ),
          );
        },
      );

  /// Starts deleting an offline region.
  OperationHandle deleteOfflineRegion(int regionId) => _startOfflineOperation(
    _OfflineOperationKind.regionDelete,
    _OfflineOperationResultKind.none,
    (outOperationId) {
      _check(
        raw.mln_runtime_offline_region_delete_start(
          _handle.raw,
          regionId,
          outOperationId,
        ),
      );
    },
  );

  OperationHandle _startOfflineOperation(
    _OfflineOperationKind kind,
    _OfflineOperationResultKind resultKind,
    void Function(Pointer<Uint64> outOperationId) start,
  ) {
    return withNativeArena((arena) {
      final outOperationId = arena<Uint64>();
      start(outOperationId);
      return OperationHandle._(this, outOperationId.value, kind, resultKind);
    });
  }

  /// Creates a map without blocking the calling isolate.
  Future<MapHandle> createMap({MapOptions options = const MapOptions()}) =>
      MapHandle.create(this, options: options);

  void _registerMap(MapHandle map) {
    _maps[map._handle.raw] = WeakReference(map);
  }

  void _unregisterMapId(int id) {
    _maps.remove(id);
  }

  void _registerOperation(OperationHandle operation) {
    _operations[operation._id] = WeakReference(operation);
  }

  void _unregisterOperationId(int id) {
    _operations.remove(id);
  }

  /// Completes after every previously accepted runtime command.
  Future<void> barrier() {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      _check(raw.mln_runtime_barrier_start(_handle.raw, outOperation));
      return outOperation.value;
    });
    return _finishOperation(operation);
  }

  /// Closes this runtime after its native executor has stopped.
  Future<void> close() async {
    final collectedOperationIds = _operations.entries
        .where((entry) => entry.value.target == null)
        .map((entry) => entry.key)
        .toList(growable: false);
    for (final operationId in collectedOperationIds) {
      raw.mln_operation_release(operationId);
      _operations.remove(operationId);
    }
    if (_operations.isNotEmpty) {
      throwInvalidState(
        'RuntimeHandle has ${_operations.length} live operation(s); '
        'release every operation before closing',
      );
    }
    await _state.closeAsync((handle) async {
      final operation = withNativeArena((arena) {
        final outOperation = arena<Uint64>()..value = 0;
        _check(raw.mln_runtime_close_start(handle.raw, outOperation));
        return outOperation.value;
      });
      await _finishOperation(operation);
    });
    if (_notificationSource == 0) {
      return;
    }
    _check(raw.mln_notification_source_clear_callback(_notificationSource));
    for (final finish in _pendingResourceCommands.values.toList()) {
      finish(false);
    }
    _pendingResourceCommands.clear();
    _closeActiveResourceProvider();
    final source = _notificationSource;
    if (source != 0) {
      _check(raw.mln_notification_source_close(source));
      _notificationSource = 0;
      _notificationListener.close();
    }
    _resourceTransformState?.close();
    _resourceTransformState = null;
    _httpHeaderTransformState?.close();
    _httpHeaderTransformState = null;
  }
}

List<({int kind, int id})> _drainReadyEndpoints(int source) {
  return withNativeArena((arena) {
    final outBatch = arena<Uint64>()..value = 0;
    _check(raw.mln_notification_source_drain_ready(source, outBatch));
    final view = arena<raw.mln_ready_batch_view>();
    view.ref.size = sizeOf<raw.mln_ready_batch_view>();
    try {
      _check(raw.mln_ready_batch_get(outBatch.value, view));
      final endpoints = <({int kind, int id})>[];
      final first = view.ref.endpoints.cast<Uint8>();
      for (var index = 0; index < view.ref.endpoint_count; index += 1) {
        final endpoint = (first + index * view.ref.endpoint_size)
            .cast<raw.mln_ready_endpoint>()
            .ref;
        endpoints.add((kind: endpoint.kind, id: endpoint.id));
      }
      return endpoints;
    } finally {
      raw.mln_ready_batch_release(outBatch.value);
    }
  });
}

bool _operationCompleted(int operation) {
  return withNativeArena((arena) {
    final completed = arena<Bool>();
    _check(raw.mln_operation_poll(operation, completed));
    return completed.value;
  });
}

String _operationDiagnostic(int operation) {
  return withNativeArena((arena) {
    final outSize = arena<Size>();
    _check(raw.mln_operation_copy_diagnostic(operation, nullptr, 0, outSize));
    if (outSize.value == 0) {
      return '';
    }
    final bytes = arena<Uint8>(outSize.value);
    _check(
      raw.mln_operation_copy_diagnostic(
        operation,
        bytes.cast<Char>(),
        outSize.value,
        outSize,
      ),
    );
    return utf8.decode(bytes.asTypedList(outSize.value));
  });
}

void _throwIfOperationFailed(int operation) {
  withNativeArena((arena) {
    final outStatus = arena<Int32>();
    _check(raw.mln_operation_get_status(operation, outStatus));
    if (outStatus.value != raw.mln_status.MLN_STATUS_OK) {
      throw MaplibreException.forNativeStatusCode(
        outStatus.value,
        _operationDiagnostic(operation),
      );
    }
  });
}

Future<void> _waitForStandaloneOperation(int source, int operation) async {
  final completer = Completer<void>();
  late final NativeCallable<raw.mln_notification_callbackFunction> listener;
  listener = NativeCallable<raw.mln_notification_callbackFunction>.listener((
    Pointer<Void> _,
  ) {
    for (final endpoint in _drainReadyEndpoints(source)) {
      if (endpoint.kind ==
              raw
                  .mln_notification_endpoint_kind
                  .MLN_NOTIFICATION_ENDPOINT_OPERATION
                  .value &&
          endpoint.id == operation &&
          !completer.isCompleted) {
        completer.complete();
      }
    }
  });
  try {
    _check(
      raw.mln_notification_source_set_callback(
        source,
        listener.nativeFunction,
        nullptr,
      ),
    );
    if (!_operationCompleted(operation)) {
      await completer.future;
    }
  } finally {
    _check(raw.mln_notification_source_clear_callback(source));
    listener.close();
  }
}

/// One batch of runtime events copied out of the native event arena.
final class RuntimeEventBatch {
  RuntimeEventBatch._({
    required List<RuntimeEvent> events,
    required this.remainingCount,
  }) : events = List.unmodifiable(events);

  factory RuntimeEventBatch._fromNative(
    raw.mln_runtime_event_batch_view batch,
    RuntimeHandle runtime,
  ) {
    final eventSize = batch.event_size;
    final events = <RuntimeEvent>[];
    final first = batch.events.cast<Uint8>();
    for (var index = 0; index < batch.event_count; index += 1) {
      // Events are reached by the batch's own stride, because a later C API
      // version widens the record beyond the size this binding compiled
      // against.
      final nativeEvent = (first + index * eventSize)
          .cast<raw.mln_runtime_event>();
      final event = RuntimeEvent._fromNative(
        nativeEvent,
        eventSize,
        batch.messages,
        runtime,
      );
      events.add(event);
      final payload = event.payload;
      if (payload is RuntimeEventCommandFinished) {
        runtime._finishResourceCommand(
          payload.commandIdNative,
          payload.disposition,
        );
        final source = event.source;
        if (source is MapRuntimeEventSource) {
          source.map?._finishStyleCommand(
            payload.commandIdNative,
            payload.disposition,
          );
        }
      }
    }
    return RuntimeEventBatch._(
      events: events,
      remainingCount: batch.remaining_count,
    );
  }

  /// Drained events in queue order.
  final List<RuntimeEvent> events;

  /// Events still queued after this batch.
  ///
  /// A nonzero count means another drain reports more events, so a host that
  /// bounds a drain learns to come back.
  final int remainingCount;
}

/// Event types a map or a runtime queues, as one bit per [RuntimeEventType].
///
/// Each bit is `1 << type.rawValue`, so a host that decoded an event type
/// computes its bit the same way these constants name it.
final class RuntimeEventMask {
  /// Creates a mask from raw bits.
  const RuntimeEventMask(this.value);

  /// Selects no event type.
  static const none = RuntimeEventMask(0);

  static const mapCameraWillChange = RuntimeEventMask(1 << 1);
  static const mapCameraIsChanging = RuntimeEventMask(1 << 2);
  static const mapCameraDidChange = RuntimeEventMask(1 << 3);
  static const mapStyleLoaded = RuntimeEventMask(1 << 4);
  static const mapLoadingStarted = RuntimeEventMask(1 << 5);
  static const mapLoadingFinished = RuntimeEventMask(1 << 6);
  static const mapLoadingFailed = RuntimeEventMask(1 << 7);
  static const mapIdle = RuntimeEventMask(1 << 8);
  static const mapRenderUpdateAvailable = RuntimeEventMask(1 << 9);
  static const mapRenderError = RuntimeEventMask(1 << 10);
  static const mapStillImageFinished = RuntimeEventMask(1 << 11);
  static const mapStillImageFailed = RuntimeEventMask(1 << 12);
  static const mapRenderFrameStarted = RuntimeEventMask(1 << 13);
  static const mapRenderFrameFinished = RuntimeEventMask(1 << 14);
  static const mapRenderMapStarted = RuntimeEventMask(1 << 15);
  static const mapRenderMapFinished = RuntimeEventMask(1 << 16);
  static const mapStyleImageMissing = RuntimeEventMask(1 << 17);
  static const mapTileAction = RuntimeEventMask(1 << 18);
  static const offlineRegionStatusChanged = RuntimeEventMask(1 << 19);
  static const offlineRegionResponseError = RuntimeEventMask(1 << 20);
  static const offlineRegionTileCountLimitExceeded = RuntimeEventMask(1 << 21);

  static const mapCameraTransitionFinished = RuntimeEventMask(1 << 23);
  static const commandFinished = RuntimeEventMask(1 << 24);

  static const _mapEventBits =
      (1 << 1) |
      (1 << 2) |
      (1 << 3) |
      (1 << 4) |
      (1 << 5) |
      (1 << 6) |
      (1 << 7) |
      (1 << 8) |
      (1 << 9) |
      (1 << 10) |
      (1 << 11) |
      (1 << 12) |
      (1 << 13) |
      (1 << 14) |
      (1 << 15) |
      (1 << 16) |
      (1 << 17) |
      (1 << 18) |
      (1 << 23);
  static const _runtimeEventBits =
      (1 << 19) | (1 << 20) | (1 << 21) | (1 << 24);

  /// Selects every map-originated event type this binding names.
  static const allMapEvents = RuntimeEventMask(_mapEventBits);

  /// Selects every runtime-originated event type this binding names.
  static const allRuntimeEvents = RuntimeEventMask(_runtimeEventBits);

  /// Selects every event type this binding names.
  ///
  /// Both setters accept this value: a map reads the map-originated bits and a
  /// runtime reads the runtime-originated ones.
  static const all = RuntimeEventMask(_mapEventBits | _runtimeEventBits);

  /// Raw mask bits.
  final int value;

  /// Whether this mask selects no event type.
  bool get isEmpty => value == 0;

  /// Returns a mask holding the bits of this mask and of [other].
  RuntimeEventMask operator |(RuntimeEventMask other) =>
      RuntimeEventMask(value | other.value);

  /// Whether this mask selects [type].
  bool contains(RuntimeEventType type) => (value & (1 << type.rawValue)) != 0;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventMask && other.value == value;

  @override
  int get hashCode => value.hashCode;
}

final class RuntimeEvent {
  RuntimeEvent._({
    required this.type,
    required this.eventType,
    required this.sourceType,
    required this.source,
    required this.code,
    required this.payloadType,
    required this.payload,
    required this.message,
  });

  factory RuntimeEvent._fromNative(
    Pointer<raw.mln_runtime_event> event,
    int eventSize,
    Pointer<Char> messages,
    RuntimeHandle runtime,
  ) {
    final value = event.ref;
    return RuntimeEvent._(
      type: value.type,
      eventType: RuntimeEventType.fromRawValue(value.type),
      sourceType: value.source_type,
      source: RuntimeEventSource._fromNative(value, runtime),
      code: value.code,
      payloadType: value.payload_type,
      payload: RuntimeEventPayload._fromNative(event, eventSize),
      message: _copyNativeString(
        (messages + value.message_offset).cast<Void>(),
        value.message_size,
      ),
    );
  }

  /// Raw native event type.
  final int type;

  /// Typed event type, preserving unknown raw values.
  final RuntimeEventType eventType;

  /// Raw native event source type.
  final int sourceType;

  /// Typed event source, preserving unknown raw values.
  final RuntimeEventSource source;

  /// Native event code.
  final int code;

  /// Raw native payload type.
  final int payloadType;

  /// Typed event payload copied into Dart-owned values.
  final RuntimeEventPayload payload;

  /// Copied event message, when one was provided.
  final String? message;
}

/// Decodes a synthesized batch through the production decoder for tests.
RuntimeEventBatch decodeRuntimeEventBatchForTesting(
  raw.mln_runtime_event_batch_view batch,
  RuntimeHandle runtime,
) => RuntimeEventBatch._fromNative(batch, runtime);

/// Runtime event type with forward-compatible unknown values.
final class RuntimeEventType {
  const RuntimeEventType._(this.rawValue, this.name);

  static const mapCameraWillChange = RuntimeEventType._(
    1,
    'mapCameraWillChange',
  );
  static const mapCameraIsChanging = RuntimeEventType._(
    2,
    'mapCameraIsChanging',
  );
  static const mapCameraDidChange = RuntimeEventType._(3, 'mapCameraDidChange');
  static const mapStyleLoaded = RuntimeEventType._(4, 'mapStyleLoaded');
  static const mapLoadingStarted = RuntimeEventType._(5, 'mapLoadingStarted');
  static const mapLoadingFinished = RuntimeEventType._(6, 'mapLoadingFinished');
  static const mapLoadingFailed = RuntimeEventType._(7, 'mapLoadingFailed');
  static const mapIdle = RuntimeEventType._(8, 'mapIdle');
  static const mapRenderUpdateAvailable = RuntimeEventType._(
    9,
    'mapRenderUpdateAvailable',
  );
  static const mapRenderError = RuntimeEventType._(10, 'mapRenderError');
  static const mapStillImageFinished = RuntimeEventType._(
    11,
    'mapStillImageFinished',
  );
  static const mapStillImageFailed = RuntimeEventType._(
    12,
    'mapStillImageFailed',
  );
  static const mapRenderFrameStarted = RuntimeEventType._(
    13,
    'mapRenderFrameStarted',
  );
  static const mapRenderFrameFinished = RuntimeEventType._(
    14,
    'mapRenderFrameFinished',
  );
  static const mapRenderMapStarted = RuntimeEventType._(
    15,
    'mapRenderMapStarted',
  );
  static const mapRenderMapFinished = RuntimeEventType._(
    16,
    'mapRenderMapFinished',
  );
  static const mapStyleImageMissing = RuntimeEventType._(
    17,
    'mapStyleImageMissing',
  );
  static const mapTileAction = RuntimeEventType._(18, 'mapTileAction');
  static const offlineRegionStatusChanged = RuntimeEventType._(
    19,
    'offlineRegionStatusChanged',
  );
  static const offlineRegionResponseError = RuntimeEventType._(
    20,
    'offlineRegionResponseError',
  );
  static const offlineRegionTileCountLimitExceeded = RuntimeEventType._(
    21,
    'offlineRegionTileCountLimitExceeded',
  );

  static const mapCameraTransitionFinished = RuntimeEventType._(
    23,
    'mapCameraTransitionFinished',
  );
  static const commandFinished = RuntimeEventType._(24, 'commandFinished');

  factory RuntimeEventType.fromRawValue(int rawValue) => switch (rawValue) {
    1 => mapCameraWillChange,
    2 => mapCameraIsChanging,
    3 => mapCameraDidChange,
    4 => mapStyleLoaded,
    5 => mapLoadingStarted,
    6 => mapLoadingFinished,
    7 => mapLoadingFailed,
    8 => mapIdle,
    9 => mapRenderUpdateAvailable,
    10 => mapRenderError,
    11 => mapStillImageFinished,
    12 => mapStillImageFailed,
    13 => mapRenderFrameStarted,
    14 => mapRenderFrameFinished,
    15 => mapRenderMapStarted,
    16 => mapRenderMapFinished,
    17 => mapStyleImageMissing,
    18 => mapTileAction,
    19 => offlineRegionStatusChanged,
    20 => offlineRegionResponseError,
    21 => offlineRegionTileCountLimitExceeded,

    23 => mapCameraTransitionFinished,
    24 => commandFinished,
    _ => RuntimeEventType._(rawValue, 'unknown($rawValue)'),
  };

  final int rawValue;
  final String name;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventType && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Camera change kind carried by camera will-change and did-change events.
final class CameraChangeMode {
  const CameraChangeMode._(this.rawValue, this.name);

  /// The camera reached its new value without an animated transition.
  static const immediate = CameraChangeMode._(0, 'immediate');

  /// The camera moved as part of an animated transition.
  static const animated = CameraChangeMode._(1, 'animated');

  /// Creates a camera change mode while preserving unknown native values.
  factory CameraChangeMode.fromRawValue(int rawValue) => switch (rawValue) {
    0 => immediate,
    1 => animated,
    _ => CameraChangeMode._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is CameraChangeMode && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Runtime event source type with forward-compatible unknown values.
final class RuntimeEventSourceType {
  const RuntimeEventSourceType._(this.rawValue, this.name);

  static const runtime = RuntimeEventSourceType._(0, 'runtime');
  static const map = RuntimeEventSourceType._(1, 'map');

  factory RuntimeEventSourceType.fromRawValue(int rawValue) =>
      switch (rawValue) {
        0 => runtime,
        1 => map,
        _ => RuntimeEventSourceType._(rawValue, 'unknown($rawValue)'),
      };

  final int rawValue;
  final String name;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventSourceType && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Typed runtime event source copied from the native event.
///
/// [sourceId] is the native source identity, which names one object for the
/// life of the process. It is an identity value, not a handle: it grants no
/// access to the object it names, and it stays comparable against the id of a
/// handle a host holds even after that handle is released. Every event carries
/// it, including an event whose source type this build does not name and a
/// map-sourced event whose id matches no live map.
sealed class RuntimeEventSource {
  const RuntimeEventSource(this.sourceType, this.sourceId);

  factory RuntimeEventSource._fromNative(
    raw.mln_runtime_event event,
    RuntimeHandle runtime,
  ) {
    final sourceType = RuntimeEventSourceType.fromRawValue(event.source_type);
    final sourceId = event.source;
    if (sourceType == RuntimeEventSourceType.runtime) {
      return RuntimeRuntimeEventSource(runtime, sourceId);
    }
    if (sourceType == RuntimeEventSourceType.map) {
      final map = runtime._maps[sourceId]?.target;
      return MapRuntimeEventSource(map, sourceId);
    }
    return UnknownRuntimeEventSource(sourceType, sourceId);
  }

  final RuntimeEventSourceType sourceType;

  /// Raw native source identity.
  final int sourceId;
}

/// Runtime-scoped event source.
final class RuntimeRuntimeEventSource extends RuntimeEventSource {
  const RuntimeRuntimeEventSource(this.runtime, int sourceId)
    : super(RuntimeEventSourceType.runtime, sourceId);

  final RuntimeHandle runtime;
}

/// Map-scoped event source.
final class MapRuntimeEventSource extends RuntimeEventSource {
  const MapRuntimeEventSource(this.map, int sourceId)
    : super(RuntimeEventSourceType.map, sourceId);

  /// Map handle when still alive in this runtime.
  ///
  /// A map that closed, or that this runtime never wrapped, leaves this null
  /// while [sourceId] still names the source map.
  final MapHandle? map;
}

/// Unknown event source type.
final class UnknownRuntimeEventSource extends RuntimeEventSource {
  const UnknownRuntimeEventSource(super.sourceType, super.sourceId);
}

/// Render mode reported by render event payloads.
final class RenderMode {
  const RenderMode._(this.rawValue, this.name);

  static const partial = RenderMode._(0, 'partial');
  static const full = RenderMode._(1, 'full');

  factory RenderMode.fromRawValue(int rawValue) => switch (rawValue) {
    0 => partial,
    1 => full,
    _ => RenderMode._(rawValue, 'unknown($rawValue)'),
  };

  final int rawValue;
  final String name;
}

/// Renderer timing and count statistics copied from render events.
final class RenderingStats {
  const RenderingStats({
    required this.encodingTime,
    required this.renderingTime,
    required this.frameCount,
    required this.drawCallCount,
    required this.totalDrawCallCount,
  });

  factory RenderingStats._fromNative(raw.mln_rendering_stats stats) =>
      RenderingStats(
        encodingTime: stats.encoding_time,
        renderingTime: stats.rendering_time,
        frameCount: stats.frame_count,
        drawCallCount: stats.draw_call_count,
        totalDrawCallCount: stats.total_draw_call_count,
      );

  final double encodingTime;
  final double renderingTime;
  final int frameCount;
  final int drawCallCount;
  final int totalDrawCallCount;
}

/// Tile operation reported by tile-action runtime events.
final class TileOperation {
  const TileOperation._(this.rawValue, this.name);

  static const requestedFromCache = TileOperation._(0, 'requestedFromCache');
  static const requestedFromNetwork = TileOperation._(
    1,
    'requestedFromNetwork',
  );
  static const loadFromNetwork = TileOperation._(2, 'loadFromNetwork');
  static const loadFromCache = TileOperation._(3, 'loadFromCache');
  static const startParse = TileOperation._(4, 'startParse');
  static const endParse = TileOperation._(5, 'endParse');
  static const error = TileOperation._(6, 'error');
  static const cancelled = TileOperation._(7, 'cancelled');
  static const nullOperation = TileOperation._(8, 'null');

  factory TileOperation.fromRawValue(int rawValue) => switch (rawValue) {
    0 => requestedFromCache,
    1 => requestedFromNetwork,
    2 => loadFromNetwork,
    3 => loadFromCache,
    4 => startParse,
    5 => endParse,
    6 => error,
    7 => cancelled,
    8 => nullOperation,
    _ => TileOperation._(rawValue, 'unknown($rawValue)'),
  };

  final int rawValue;
  final String name;
}

/// Terminal disposition of an accepted runtime command.
final class CommandDisposition {
  const CommandDisposition._(this.rawValue, this.name);

  static const committed = CommandDisposition._(0, 'committed');
  static const superseded = CommandDisposition._(1, 'superseded');
  static const failed = CommandDisposition._(2, 'failed');
  static const cancelled = CommandDisposition._(3, 'cancelled');

  factory CommandDisposition.fromRawValue(int rawValue) => switch (rawValue) {
    0 => committed,
    1 => superseded,
    2 => failed,
    3 => cancelled,
    _ => CommandDisposition._(rawValue, 'unknown($rawValue)'),
  };

  final int rawValue;
  final String name;

  @override
  bool operator ==(Object other) =>
      other is CommandDisposition && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Typed runtime event payload copied into Dart-owned values.
sealed class RuntimeEventPayload {
  const RuntimeEventPayload(this.rawPayloadType);

  factory RuntimeEventPayload._fromNative(
    Pointer<raw.mln_runtime_event> event,
    int eventSize,
  ) {
    final rawPayloadType = event.ref.payload_type;
    final payload = event.ref.payload;
    return switch (rawPayloadType) {
      0 => const RuntimeEventPayloadNone(),
      1 => _renderFramePayload(payload.render_frame),
      2 => _renderMapPayload(payload.render_map),
      4 => _tileActionPayload(payload.tile_action),
      5 => _offlineRegionStatusPayload(payload.offline_region_status),
      6 => _offlineRegionResponseErrorPayload(
        payload.offline_region_response_error,
      ),
      7 => _offlineRegionTileCountLimitPayload(
        payload.offline_region_tile_count_limit,
      ),
      // Offline operation completion is observed through the operation handle,
      // not through the runtime event queue.
      9 => _cameraTransitionFinishedPayload(payload.camera_transition_finished),
      10 => _commandFinishedPayload(payload.command_finished),
      _ => RuntimeEventPayloadUnknown(
        rawPayloadType,
        _copyRuntimePayloadWindow(event, eventSize),
      ),
    };
  }

  final int rawPayloadType;
}

/// Runtime event with no payload.
final class RuntimeEventPayloadNone extends RuntimeEventPayload {
  const RuntimeEventPayloadNone() : super(0);
}

/// Render-frame event payload.
final class RuntimeEventRenderFrame extends RuntimeEventPayload {
  const RuntimeEventRenderFrame({
    required this.mode,
    required this.rawMode,
    required this.needsRepaint,
    required this.placementChanged,
    required this.stats,
  }) : super(1);

  final RenderMode mode;
  final int rawMode;
  final bool needsRepaint;
  final bool placementChanged;
  final RenderingStats stats;
}

/// Render-map event payload.
final class RuntimeEventRenderMap extends RuntimeEventPayload {
  const RuntimeEventRenderMap({required this.mode, required this.rawMode})
    : super(2);

  final RenderMode mode;
  final int rawMode;
}

/// Tile-action event payload.
///
/// The event message carries the source ID.
final class RuntimeEventTileAction extends RuntimeEventPayload {
  const RuntimeEventTileAction({
    required this.operation,
    required this.rawOperation,
    required this.tileId,
  }) : super(4);

  final TileOperation operation;
  final int rawOperation;
  final TileId tileId;
}

/// Offline-region status event payload.
final class RuntimeEventOfflineRegionStatus extends RuntimeEventPayload {
  const RuntimeEventOfflineRegionStatus({
    required this.regionId,
    required this.status,
  }) : super(5);

  final int regionId;
  final OfflineRegionStatus status;
}

/// Offline-region response error event payload.
final class RuntimeEventOfflineRegionResponseError extends RuntimeEventPayload {
  const RuntimeEventOfflineRegionResponseError({
    required this.regionId,
    required this.reason,
    required this.rawReason,
  }) : super(6);

  final int regionId;
  final ResourceErrorReason reason;
  final int rawReason;
}

/// Offline-region tile-count limit event payload.
final class RuntimeEventOfflineRegionTileCountLimit
    extends RuntimeEventPayload {
  const RuntimeEventOfflineRegionTileCountLimit({
    required this.regionId,
    required this.limit,
  }) : super(7);

  final int regionId;
  final BigInt limit;
}

/// Camera-transition-finished event payload.
final class RuntimeEventCameraTransitionFinished extends RuntimeEventPayload {
  const RuntimeEventCameraTransitionFinished({required this.transitionId})
    : super(9);

  /// Caller-chosen transition identity across the full `uint64_t` domain.
  final BigInt transitionId;
}

/// Terminal event for one accepted runtime command.
final class RuntimeEventCommandFinished extends RuntimeEventPayload {
  const RuntimeEventCommandFinished({
    required this.commandId,
    required this.disposition,
    required this.rawDisposition,
    required this.generation,
    required this.commandIdNative,
  }) : super(10);

  /// Runtime-wide command identity across the full `uint64_t` domain.
  final BigInt commandId;

  /// Typed terminal disposition.
  final CommandDisposition disposition;

  /// Raw terminal disposition.
  final int rawDisposition;

  /// Committed generation, or zero when no generation was committed.
  final BigInt generation;

  final int commandIdNative;
}

/// Payload of a type this binding does not name, copied as raw bytes.
final class RuntimeEventPayloadUnknown extends RuntimeEventPayload {
  RuntimeEventPayloadUnknown(super.rawPayloadType, Uint8List bytes)
    : bytes = Uint8List.fromList(bytes).asUnmodifiableView();

  /// The event's fixed payload window, copied byte for byte.
  final Uint8List bytes;
}

RuntimeEventPayload _renderFramePayload(
  raw.mln_runtime_event_render_frame value,
) => RuntimeEventRenderFrame(
  mode: RenderMode.fromRawValue(value.mode),
  rawMode: value.mode,
  needsRepaint: value.needs_repaint,
  placementChanged: value.placement_changed,
  stats: RenderingStats._fromNative(value.stats),
);

RuntimeEventPayload _commandFinishedPayload(
  raw.mln_runtime_event_command_finished value,
) => RuntimeEventCommandFinished(
  commandId: uint64FromNative(value.command_id),
  disposition: CommandDisposition.fromRawValue(value.disposition),
  rawDisposition: value.disposition,
  generation: uint64FromNative(value.generation),
  commandIdNative: value.command_id,
);

RuntimeEventPayload _renderMapPayload(raw.mln_runtime_event_render_map value) =>
    RuntimeEventRenderMap(
      mode: RenderMode.fromRawValue(value.mode),
      rawMode: value.mode,
    );

RuntimeEventPayload _tileActionPayload(
  raw.mln_runtime_event_tile_action value,
) => RuntimeEventTileAction(
  operation: TileOperation.fromRawValue(value.operation),
  rawOperation: value.operation,
  tileId: TileId(
    overscaledZ: value.tile_id.overscaled_z,
    wrap: value.tile_id.wrap,
    canonicalZ: value.tile_id.canonical_z,
    canonicalX: value.tile_id.canonical_x,
    canonicalY: value.tile_id.canonical_y,
  ),
);

RuntimeEventPayload _offlineRegionStatusPayload(
  raw.mln_runtime_event_offline_region_status value,
) => RuntimeEventOfflineRegionStatus(
  regionId: value.region_id,
  status: _offlineRegionStatusFromNative(value.status),
);

RuntimeEventPayload _offlineRegionResponseErrorPayload(
  raw.mln_runtime_event_offline_region_response_error value,
) => RuntimeEventOfflineRegionResponseError(
  regionId: value.region_id,
  reason: ResourceErrorReason.fromRawValue(value.reason),
  rawReason: value.reason,
);

RuntimeEventPayload _offlineRegionTileCountLimitPayload(
  raw.mln_runtime_event_offline_region_tile_count_limit value,
) => RuntimeEventOfflineRegionTileCountLimit(
  regionId: value.region_id,
  limit: uint64FromNative(value.limit),
);

RuntimeEventPayload _cameraTransitionFinishedPayload(
  raw.mln_runtime_event_camera_transition_finished value,
) => RuntimeEventCameraTransitionFinished(
  transitionId: uint64FromNative(value.transition_id),
);

/// Byte offset of the payload union inside one native event record.
///
/// `dart:ffi` has no `offsetof`. The union is the record's last member and
/// carries the record's own alignment, so the difference of the two sizes is
/// that offset. The C API keeps the offset stable across versions.
final int _runtimeEventPayloadOffset =
    sizeOf<raw.mln_runtime_event>() - sizeOf<raw.mln_runtime_event_payload>();

/// Copies the payload window of an event whose payload type this binding does
/// not name.
///
/// The window is the batch's event stride minus the payload's offset, so it
/// covers every payload byte a later C API version adds.
Uint8List _copyRuntimePayloadWindow(
  Pointer<raw.mln_runtime_event> event,
  int eventSize,
) {
  final windowSize = eventSize - _runtimeEventPayloadOffset;
  if (windowSize <= 0) {
    return Uint8List(0);
  }
  return Uint8List.fromList(
    (event.cast<Uint8>() + _runtimeEventPayloadOffset).asTypedList(windowSize),
  );
}

/// Map rendering mode used when creating a map.
final class MapMode {
  const MapMode._(this.rawValue, this.name);

  /// Continuously updates as data arrives and map state changes.
  static const continuous = MapMode._(0, 'continuous');

  /// Produces one-off still images of an arbitrary viewport.
  static const staticMap = MapMode._(1, 'static');

  /// Produces one-off still images for a single tile.
  static const tile = MapMode._(2, 'tile');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Map debug overlay option mask.
final class MapDebugOptions {
  /// Creates a debug option mask from raw bits.
  const MapDebugOptions(this.bits);

  /// No debug overlays.
  static const none = MapDebugOptions(0);

  /// Tile border overlay.
  static const tileBorders = MapDebugOptions(1 << 1);

  /// Parse status overlay.
  static const parseStatus = MapDebugOptions(1 << 2);

  /// Timestamp overlay.
  static const timestamps = MapDebugOptions(1 << 3);

  /// Collision overlay.
  static const collision = MapDebugOptions(1 << 4);

  /// Overdraw overlay.
  static const overdraw = MapDebugOptions(1 << 5);

  /// Stencil clip overlay.
  static const stencilClip = MapDebugOptions(1 << 6);

  /// Depth buffer overlay.
  static const depthBuffer = MapDebugOptions(1 << 7);

  /// Raw debug overlay bits.
  final int bits;

  /// Returns a mask containing bits from this mask and [other].
  MapDebugOptions union(MapDebugOptions other) =>
      MapDebugOptions(bits | other.bits);

  /// Returns true when all [option] bits are present.
  bool contains(MapDebugOptions option) => (bits & option.bits) == option.bits;
}

/// Map creation options.
final class MapOptions {
  /// Creates map options.
  const MapOptions({
    this.width = 256,
    this.height = 256,
    this.scaleFactor = 1,
    this.mapMode = MapMode.continuous,
    this.fastPforEnabled = false,
    RuntimeEventMask? eventMask,
  }) : _eventMask = eventMask;

  /// Initial map width in logical pixels.
  final int width;

  /// Initial map height in logical pixels.
  final int height;

  /// Initial map scale factor.
  final double scaleFactor;

  /// Map rendering mode.
  final MapMode mapMode;

  /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
  /// encodings, fixed for the lifetime of the map.
  ///
  /// A map created with this false decodes every other MLT encoding and logs a
  /// tile parse warning for the FastPFOR ones.
  final bool fastPforEnabled;

  final RuntimeEventMask? _eventMask;

  /// Map-originated event types this map queues during and after construction.
  ///
  /// Defaults to the C options default's selection, which is every event type
  /// the loaded library reports. A bit a newer library selects and this binding
  /// does not name is kept, and its events reach a host as unknown event and
  /// payload domains. See [MapHandle.setEventMask].
  RuntimeEventMask get eventMask =>
      _eventMask ?? RuntimeEventMask(raw.mln_map_options_default().event_mask);

  @override
  bool operator ==(Object other) =>
      other is MapOptions &&
      other.width == width &&
      other.height == height &&
      other.scaleFactor == scaleFactor &&
      other.mapMode == mapMode &&
      other.fastPforEnabled == fastPforEnabled &&
      other.eventMask == eventMask;

  @override
  int get hashCode => Object.hash(
    width,
    height,
    scaleFactor,
    mapMode,
    fastPforEnabled,
    eventMask,
  );
}

/// A map's logical viewport size and pixel ratio.
final class MapSize {
  /// Creates a map size snapshot.
  const MapSize({
    required this.width,
    required this.height,
    required this.scaleFactor,
  });

  /// Logical viewport width.
  final int width;

  /// Logical viewport height.
  final int height;

  /// Map pixel ratio.
  final double scaleFactor;

  @override
  bool operator ==(Object other) =>
      other is MapSize &&
      other.width == width &&
      other.height == height &&
      other.scaleFactor == scaleFactor;

  @override
  int get hashCode => Object.hash(width, height, scaleFactor);
}

/// Immutable camera snapshot and its map generation.
final class CameraSnapshot {
  /// Creates a copied camera snapshot.
  const CameraSnapshot({required this.camera, required this.generation});

  /// Camera options copied from native memory.
  final CameraOptions camera;

  /// Generation of the complete map snapshot that supplied [camera].
  final BigInt generation;
}

/// Immutable map state copied from the native snapshot.
final class MapSnapshot {
  /// Creates a copied map snapshot.
  const MapSnapshot({
    required this.generation,
    required this.camera,
    required this.size,
    required this.projectionMode,
    required this.viewportOptions,
    required this.loading,
    required this.fullyRendered,
    required this.repaintDemand,
    required this.eventMask,
    required this.latestRenderUpdateGeneration,
  });

  /// Generation of every field in this snapshot.
  final BigInt generation;

  /// Camera copied from the snapshot.
  final CameraOptions camera;

  /// Logical extent copied from the snapshot.
  final MapSize size;

  /// Projection mode copied from the snapshot.
  final ProjectionModeOptions projectionMode;

  /// Viewport options copied from the snapshot.
  final MapViewportOptions viewportOptions;

  /// Whether a style or resource load is active.
  final bool loading;

  /// Whether the current map state has rendered completely.
  final bool fullyRendered;

  /// Whether the map currently requests a repaint.
  final bool repaintDemand;

  /// Map event types selected in this snapshot.
  final RuntimeEventMask eventMask;

  /// Generation of the latest render update.
  final BigInt latestRenderUpdateGeneration;
}

/// Map handle bound to a retained runtime.
final class MapHandle {
  MapHandle._(this._runtime, NativeMap handle, this._acceptedEventMask)
    : _state = NativeHandleState(handle, 'MapHandle');

  /// Creates a map without blocking the calling isolate.
  static Future<MapHandle> create(
    RuntimeHandle runtime, {
    MapOptions options = const MapOptions(),
  }) async {
    final operation = withNativeArena((arena) {
      final nativeOptions = arena<raw.mln_map_options>();
      nativeOptions.ref = raw.mln_map_options_default();
      nativeOptions.ref.initial_extent.width = _positiveUint32(
        options.width,
        'map width',
      );
      nativeOptions.ref.initial_extent.height = _positiveUint32(
        options.height,
        'map height',
      );
      nativeOptions.ref.initial_extent.scale_factor = options.scaleFactor;
      nativeOptions.ref.map_mode = options.mapMode.rawValue;
      nativeOptions.ref.fast_pfor_enabled = options.fastPforEnabled;
      nativeOptions.ref.event_mask = options.eventMask.value;
      final outOperation = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_create_start(
          runtime._handle.raw,
          nativeOptions,
          outOperation,
        ),
      );
      return outOperation.value;
    });
    final map = await runtime._takeOperation(operation, () {
      return withNativeArena((arena) {
        final outMap = arena<Uint64>()..value = 0;
        _check(raw.mln_map_create_take_result(operation, outMap));
        return MapHandle._(runtime, NativeMap(outMap.value), options.eventMask);
      });
    });
    runtime._registerMap(map);
    return map;
  }

  final RuntimeHandle _runtime;
  final NativeHandleState<NativeMap> _state;
  RuntimeEventMask _acceptedEventMask;

  /// Callback roots of the custom-geometry sources this map still holds, each
  /// released by the C API's own release callback.
  final _customGeometryCallbacks = <String, _CustomGeometryCallbackState>{};
  final _pendingStyleCallbacks =
      <int, ({String sourceId, _CustomGeometryCallbackState state})>{};

  /// Whether this map has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  NativeMap get _handle {
    final _ = _runtime._handle;
    return _state.handle;
  }

  /// Loads a style URL and returns the accepted command ID.
  BigInt setStyleUrl(String url) {
    return withNativeArena<BigInt>((arena) {
      final nativeUrl = nativeUtf8CString(url, arena);
      final outCommandId = arena<Uint64>();
      _check(
        raw.mln_map_set_style_url(
          _handle.raw,
          nativeUrl.pointer.cast<Char>(),
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Loads inline style JSON and returns the accepted command ID.
  BigInt setStyleJson(Uint8List json) {
    return withNativeArena<BigInt>((arena) {
      final nativeJson = nativeBufferView(json, arena);
      final outCommandId = arena<Uint64>();
      _check(raw.mln_map_set_style_json(_handle.raw, nativeJson, outCommandId));
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Returns the style document this map's style was last parsed from.
  ///
  /// This is the loaded document, not a serialization of the live style:
  /// runtime mutations do not change it, and a failed parse leaves the
  /// previously parsed document in place. The result is empty when no document
  /// has been parsed.
  Future<Uint8List> getLoadedStyleJson() {
    return _styleOperation(
      (arena, outOperation) {
        _check(raw.mln_map_loaded_style_json_start(_handle.raw, outOperation));
      },
      (operation, arena) {
        final outJson = arena<Uint64>()..value = 0;
        _check(raw.mln_map_loaded_style_json_take_result(operation, outJson));
        return copyOwnedBuffer(NativeOwnedBufferHandle(outJson.value));
      },
    );
  }

  /// Returns the URL this map's style was last requested from.
  ///
  /// [setStyleUrl] records the URL when the request is made, before the
  /// response arrives, and [setStyleJson] clears it, so this can disagree with
  /// [getLoadedStyleJson] while a load is in flight or after one fails. The
  /// result is empty when no URL bytes are available.
  Future<String> getStyleUrl() async {
    final bytes = await _styleOperation(
      (arena, outOperation) {
        _check(raw.mln_map_style_url_start(_handle.raw, outOperation));
      },
      (operation, arena) {
        final outUrl = arena<Uint64>()..value = 0;
        _check(raw.mln_map_style_url_take_result(operation, outUrl));
        return copyOwnedBuffer(NativeOwnedBufferHandle(outUrl.value));
      },
    );
    return utf8.decode(bytes);
  }

  /// Selects map-originated events and returns the accepted command ID.
  BigInt setEventMask(RuntimeEventMask mask) {
    final commandId = withNativeArena<BigInt>((arena) {
      final outCommandId = arena<Uint64>();
      _check(raw.mln_map_set_event_mask(_handle.raw, mask.value, outCommandId));
      return uint64FromNative(outCommandId.value);
    });
    _acceptedEventMask = mask;
    return commandId;
  }

  /// Reports the most recently accepted map event mask.
  RuntimeEventMask get eventMask => _acceptedEventMask;

  /// Copies the latest immutable map snapshot.
  MapSnapshot snapshot() {
    return withNativeArena((arena) {
      final outSnapshot = arena<raw.mln_map_snapshot>();
      outSnapshot.ref.size = sizeOf<raw.mln_map_snapshot>();
      _check(raw.mln_map_snapshot_get(_handle.raw, outSnapshot));
      final value = outSnapshot.ref;
      return MapSnapshot(
        generation: uint64FromNative(value.generation),
        camera: native_struct.cameraOptionsFromNative(value.camera),
        size: MapSize(
          width: value.logical_extent.width,
          height: value.logical_extent.height,
          scaleFactor: value.logical_extent.scale_factor,
        ),
        projectionMode: native_struct.projectionModeOptionsFromNative(
          value.projection_mode,
        ),
        viewportOptions: native_struct.mapViewportOptionsFromNative(
          value.viewport,
        ),
        loading: value.loading,
        fullyRendered: value.fully_rendered,
        repaintDemand: value.repaint_demand,
        eventMask: RuntimeEventMask(value.event_mask),
        latestRenderUpdateGeneration: uint64FromNative(
          value.latest_render_update_generation,
        ),
      );
    });
  }

  /// Resizes the map and returns the accepted command ID.
  BigInt resize(MapSize size) {
    return withNativeArena<BigInt>((arena) {
      final extent = arena<raw.mln_logical_extent>();
      extent.ref.width = _positiveUint32(size.width, 'map width');
      extent.ref.height = _positiveUint32(size.height, 'map height');
      extent.ref.scale_factor = size.scaleFactor;
      final outCommandId = arena<Uint64>();
      _check(raw.mln_map_resize(_handle.raw, extent.ref, outCommandId));
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Requests a repaint and returns its runtime-wide command ID.
  BigInt requestRepaint() {
    return withNativeArena<BigInt>((arena) {
      final outCommandId = arena<Uint64>();
      _check(raw.mln_map_request_repaint(_handle.raw, outCommandId));
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Requests one still image without blocking the calling isolate.
  Future<void> requestStillImage() {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      _check(raw.mln_map_request_still_image_start(_handle.raw, outOperation));
      return outOperation.value;
    });
    return _runtime._finishOperation(operation);
  }

  /// Applies MapLibre debug overlay options.
  BigInt setDebugOptions(MapDebugOptions options) {
    return withNativeArena<BigInt>((arena) {
      final outCommandId = arena<Uint64>();
      _check(
        raw.mln_map_set_debug_options(_handle.raw, options.bits, outCommandId),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Reads current debug overlay options after prior commands.
  Future<MapDebugOptions> debugOptions() => _mapQuery(
    raw.mln_map_get_debug_options_start,
    (operation, arena) {
      final outOptions = arena<Uint32>();
      _check(raw.mln_map_get_debug_options_take_result(operation, outOptions));
      return MapDebugOptions(outOptions.value);
    },
  );

  /// Dumps map debug logs through MapLibre Native logging.
  BigInt dumpDebugLogs() {
    return withNativeArena<BigInt>((arena) {
      final outCommandId = arena<Uint64>();
      _check(raw.mln_map_dump_debug_logs(_handle.raw, outCommandId));
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies the latest camera snapshot.
  CameraSnapshot cameraSnapshot() {
    return withNativeArena((arena) {
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      final outGeneration = arena<Uint64>();
      _check(
        raw.mln_map_camera_snapshot_get(_handle.raw, outCamera, outGeneration),
      );
      return CameraSnapshot(
        camera: native_struct.cameraOptionsFromNative(outCamera.ref),
        generation: uint64FromNative(outGeneration.value),
      );
    });
  }

  /// Reads the camera after every previously accepted command.
  Future<CameraSnapshot> queryCamera() {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      _check(raw.mln_map_camera_query_start(_handle.raw, outOperation));
      return outOperation.value;
    });
    return _runtime._takeOperation(operation, () {
      return withNativeArena((arena) {
        final outResult = arena<raw.mln_camera_query_result>();
        outResult.ref.size = sizeOf<raw.mln_camera_query_result>();
        _check(raw.mln_map_camera_query_take_result(operation, outResult));
        return CameraSnapshot(
          camera: native_struct.cameraOptionsFromNative(outResult.ref.camera),
          generation: uint64FromNative(outResult.ref.generation),
        );
      });
    });
  }

  /// Submits an atomic camera update and returns its command ID.
  BigInt updateCamera(
    CameraOptions camera, {
    CameraUpdateMode mode = CameraUpdateMode.jump,
    AnimationOptions? animation,
    int gesturePhase = 0,
    BigInt? gestureId,
    BigInt? animationId,
  }) {
    return withNativeArena((arena) {
      final update = arena<raw.mln_camera_update>();
      update.ref = raw.mln_camera_update_default();
      update.ref.mode = mode.rawValue;
      update.ref.camera = _nativeCamera(camera, arena).ref;
      if (animation != null) {
        update.ref.animation = _nativeAnimation(animation, arena).ref;
      }
      update.ref.gesture_phase = gesturePhase;
      update.ref.gesture_id = uint64ToNative(
        gestureId ?? BigInt.zero,
        'gestureId',
      );
      update.ref.animation_id = uint64ToNative(
        animationId ?? BigInt.zero,
        'animationId',
      );
      final outCommandId = arena<Uint64>();
      _check(raw.mln_map_update_camera(_handle.raw, update, outCommandId));
      return uint64FromNative(outCommandId.value);
    });
  }

  Future<T> _mapQuery<T>(
    int Function(int, Pointer<Uint64>) start,
    T Function(int, Arena) take,
  ) {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      _check(start(_handle.raw, outOperation));
      return outOperation.value;
    });
    return _runtime._takeOperation(
      operation,
      () => withNativeArena((arena) => take(operation, arena)),
    );
  }

  /// Enables or disables the rendering stats overlay.
  BigInt setRenderingStatsViewEnabled(bool enabled) {
    return withNativeArena<BigInt>((arena) {
      final outCommandId = arena<Uint64>();
      _check(
        raw.mln_map_set_rendering_stats_view_enabled(
          _handle.raw,
          enabled,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Reads whether the rendering stats overlay is enabled.
  Future<bool> renderingStatsViewEnabled() => _mapQuery(
    raw.mln_map_get_rendering_stats_view_enabled_start,
    (operation, arena) {
      final outEnabled = arena<Bool>();
      _check(
        raw.mln_map_get_rendering_stats_view_enabled_take_result(
          operation,
          outEnabled,
        ),
      );
      return outEnabled.value;
    },
  );

  /// Reads whether MapLibre considers the map fully loaded.
  Future<bool> isFullyLoaded() =>
      _mapQuery(raw.mln_map_is_fully_loaded_start, (operation, arena) {
        final outLoaded = arena<Bool>();
        _check(raw.mln_map_is_fully_loaded_take_result(operation, outLoaded));
        return outLoaded.value;
      });

  /// Reads live viewport controls after prior commands.
  Future<MapViewportOptions> viewportOptions() =>
      _mapQuery(raw.mln_map_get_viewport_options_start, (operation, arena) {
        final outOptions = arena<raw.mln_map_viewport_options>();
        outOptions.ref.size = sizeOf<raw.mln_map_viewport_options>();
        _check(
          raw.mln_map_get_viewport_options_take_result(operation, outOptions),
        );
        return native_struct.mapViewportOptionsFromNative(outOptions.ref);
      });

  /// Applies selected live map viewport and render-transform controls.
  BigInt setViewportOptions(MapViewportOptions options) {
    return withNativeArena<BigInt>((arena) {
      final nativeOptions = arena<raw.mln_map_viewport_options>();
      nativeOptions.ref = native_struct.mapViewportOptionsToNative(
        options,
        raw.mln_map_viewport_options_default(),
      );
      final outCommandId = arena<Uint64>();
      _check(
        raw.mln_map_set_viewport_options(
          _handle.raw,
          nativeOptions,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Reads tile tuning controls after prior commands.
  Future<MapTileOptions> tileOptions() =>
      _mapQuery(raw.mln_map_get_tile_options_start, (operation, arena) {
        final outOptions = arena<raw.mln_map_tile_options>();
        outOptions.ref.size = sizeOf<raw.mln_map_tile_options>();
        _check(raw.mln_map_get_tile_options_take_result(operation, outOptions));
        return native_struct.mapTileOptionsFromNative(outOptions.ref);
      });

  /// Applies selected tile prefetch and LOD tuning controls.
  BigInt setTileOptions(MapTileOptions options) {
    return withNativeArena<BigInt>((arena) {
      final nativeOptions = arena<raw.mln_map_tile_options>();
      nativeOptions.ref = native_struct.mapTileOptionsToNative(
        options,
        raw.mln_map_tile_options_default(),
      );
      final outCommandId = arena<Uint64>();
      _check(
        raw.mln_map_set_tile_options(_handle.raw, nativeOptions, outCommandId),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Reads map camera constraints after prior commands.
  Future<BoundOptions> bounds() =>
      _mapQuery(raw.mln_map_get_bounds_start, (operation, arena) {
        final outOptions = arena<raw.mln_bound_options>();
        outOptions.ref.size = sizeOf<raw.mln_bound_options>();
        _check(raw.mln_map_get_bounds_take_result(operation, outOptions));
        return native_struct.boundOptionsFromNative(outOptions.ref);
      });

  /// Applies selected map camera constraint options.
  BigInt setBounds(BoundOptions options) {
    return withNativeArena<BigInt>((arena) {
      final nativeOptions = arena<raw.mln_bound_options>();
      nativeOptions.ref = native_struct.boundOptionsToNative(
        options,
        raw.mln_bound_options_default(),
      );
      final outCommandId = arena<Uint64>();
      _check(raw.mln_map_set_bounds(_handle.raw, nativeOptions, outCommandId));
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Reads the free camera after prior commands.
  Future<FreeCameraOptions> freeCameraOptions() => _mapQuery(
    raw.mln_map_get_free_camera_options_start,
    (operation, arena) {
      final outOptions = arena<raw.mln_free_camera_options>();
      outOptions.ref.size = sizeOf<raw.mln_free_camera_options>();
      _check(
        raw.mln_map_get_free_camera_options_take_result(operation, outOptions),
      );
      return native_struct.freeCameraOptionsFromNative(outOptions.ref);
    },
  );

  /// Applies selected free camera position and orientation fields.
  BigInt setFreeCameraOptions(FreeCameraOptions options) {
    return withNativeArena<BigInt>((arena) {
      final nativeOptions = arena<raw.mln_free_camera_options>();
      nativeOptions.ref = native_struct.freeCameraOptionsToNative(
        options,
        raw.mln_free_camera_options_default(),
      );
      final outCommandId = arena<Uint64>();
      _check(
        raw.mln_map_set_free_camera_options(
          _handle.raw,
          nativeOptions,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies axonometric rendering options from the latest map snapshot.
  ProjectionModeOptions projectionMode() => snapshot().projectionMode;

  /// Applies selected axonometric rendering option fields.
  BigInt setProjectionMode(ProjectionModeOptions mode) {
    return withNativeArena<BigInt>((arena) {
      final nativeMode = arena<raw.mln_projection_mode>();
      nativeMode.ref = native_struct.projectionModeOptionsToNative(
        mode,
        raw.mln_projection_mode_default(),
      );
      final outCommandId = arena<Uint64>();
      _check(
        raw.mln_map_set_projection_mode(_handle.raw, nativeMode, outCommandId),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Computes a camera that fits geographic bounds after prior commands.
  Future<CameraOptions> cameraForLatLngBounds(
    LatLngBounds bounds, {
    CameraFitOptions fitOptions = const CameraFitOptions(),
  }) => _cameraFitQuery((arena, outOperation) {
    final nativeFitOptions = arena<raw.mln_camera_fit_options>();
    nativeFitOptions.ref = native_struct.cameraFitOptionsToNative(
      fitOptions,
      raw.mln_camera_fit_options_default(),
    );
    _check(
      raw.mln_map_camera_for_lat_lng_bounds_start(
        _handle.raw,
        native_struct.latLngBoundsToNative(bounds),
        nativeFitOptions,
        outOperation,
      ),
    );
  }, raw.mln_map_camera_for_lat_lng_bounds_take_result);

  /// Computes a camera that fits geographic coordinates after prior commands.
  Future<CameraOptions> cameraForLatLngs(
    List<LatLng> coordinates, {
    CameraFitOptions fitOptions = const CameraFitOptions(),
  }) => _cameraFitQuery((arena, outOperation) {
    final nativeFitOptions = arena<raw.mln_camera_fit_options>();
    nativeFitOptions.ref = native_struct.cameraFitOptionsToNative(
      fitOptions,
      raw.mln_camera_fit_options_default(),
    );
    _check(
      raw.mln_map_camera_for_lat_lngs_start(
        _handle.raw,
        _latLngArray(coordinates, arena),
        coordinates.length,
        nativeFitOptions,
        outOperation,
      ),
    );
  }, raw.mln_map_camera_for_lat_lngs_take_result);

  /// Computes a camera that fits geometry after prior commands.
  Future<CameraOptions> cameraForGeometry(
    Uint8List geometry, {
    CameraFitOptions fitOptions = const CameraFitOptions(),
  }) => _cameraFitQuery((arena, outOperation) {
    final nativeFitOptions = arena<raw.mln_camera_fit_options>();
    nativeFitOptions.ref = native_struct.cameraFitOptionsToNative(
      fitOptions,
      raw.mln_camera_fit_options_default(),
    );
    _check(
      raw.mln_map_camera_for_geometry_start(
        _handle.raw,
        nativeBufferView(geometry, arena),
        nativeFitOptions,
        outOperation,
      ),
    );
  }, raw.mln_map_camera_for_geometry_take_result);

  Future<CameraOptions> _cameraFitQuery(
    void Function(Arena, Pointer<Uint64>) start,
    int Function(int, Pointer<raw.mln_camera_options>) take,
  ) {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      start(arena, outOperation);
      return outOperation.value;
    });
    return _runtime._takeOperation(operation, () {
      return withNativeArena((arena) {
        final outCamera = arena<raw.mln_camera_options>();
        outCamera.ref.size = sizeOf<raw.mln_camera_options>();
        _check(take(operation, outCamera));
        return native_struct.cameraOptionsFromNative(outCamera.ref);
      });
    });
  }

  /// Computes wrapped bounds after prior commands.
  Future<LatLngBounds> latLngBoundsForCamera(CameraOptions camera) =>
      _latLngBoundsForCamera(camera, unwrapped: false);

  /// Computes unwrapped bounds after prior commands.
  Future<LatLngBounds> latLngBoundsForCameraUnwrapped(CameraOptions camera) =>
      _latLngBoundsForCamera(camera, unwrapped: true);

  Future<LatLngBounds> _latLngBoundsForCamera(
    CameraOptions camera, {
    required bool unwrapped,
  }) {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      final nativeCamera = _nativeCamera(camera, arena);
      _check(
        unwrapped
            ? raw.mln_map_lat_lng_bounds_for_camera_unwrapped_start(
                _handle.raw,
                nativeCamera,
                outOperation,
              )
            : raw.mln_map_lat_lng_bounds_for_camera_start(
                _handle.raw,
                nativeCamera,
                outOperation,
              ),
      );
      return outOperation.value;
    });
    return _runtime._takeOperation(operation, () {
      return withNativeArena((arena) {
        final outBounds = arena<raw.mln_lat_lng_bounds>();
        _check(
          unwrapped
              ? raw.mln_map_lat_lng_bounds_for_camera_unwrapped_take_result(
                  operation,
                  outBounds,
                )
              : raw.mln_map_lat_lng_bounds_for_camera_take_result(
                  operation,
                  outBounds,
                ),
        );
        return native_struct.latLngBoundsFromNative(outBounds.ref);
      });
    });
  }

  /// Converts a geographic coordinate after prior commands.
  Future<ScreenPoint> pixelForLatLng(LatLng coordinate) {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_pixel_for_lat_lng_start(
          _handle.raw,
          native_struct.latLngToNative(coordinate),
          outOperation,
        ),
      );
      return outOperation.value;
    });
    return _runtime._takeOperation(operation, () {
      return withNativeArena((arena) {
        final outPoint = arena<raw.mln_screen_point>();
        _check(raw.mln_map_pixel_for_lat_lng_take_result(operation, outPoint));
        return native_struct.screenPointFromNative(outPoint.ref);
      });
    });
  }

  /// Converts a screen point after prior commands.
  Future<LatLng> latLngForPixel(ScreenPoint point) {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_lat_lng_for_pixel_start(
          _handle.raw,
          native_struct.screenPointToNative(point),
          outOperation,
        ),
      );
      return outOperation.value;
    });
    return _runtime._takeOperation(operation, () {
      return withNativeArena((arena) {
        final outCoordinate = arena<raw.mln_lat_lng>();
        _check(
          raw.mln_map_lat_lng_for_pixel_take_result(operation, outCoordinate),
        );
        return native_struct.latLngFromNative(outCoordinate.ref);
      });
    });
  }

  /// Converts geographic coordinates after prior commands.
  Future<List<ScreenPoint>> pixelsForLatLngs(List<LatLng> coordinates) =>
      _coordinateListQuery<ScreenPoint>(
        coordinates.length,
        (arena, outOperation) => _check(
          raw.mln_map_pixels_for_lat_lngs_start(
            _handle.raw,
            _latLngArray(coordinates, arena),
            coordinates.length,
            outOperation,
          ),
        ),
        (operation, arena, count) {
          final values = count == 0
              ? nullptr.cast<raw.mln_screen_point>()
              : arena<raw.mln_screen_point>(count);
          final outCount = arena<Size>();
          _check(
            raw.mln_map_pixels_for_lat_lngs_take_result(
              operation,
              values,
              count,
              outCount,
            ),
          );
          return [
            for (var index = 0; index < outCount.value; index += 1)
              native_struct.screenPointFromNative(values[index]),
          ];
        },
      );

  /// Converts screen points after prior commands.
  Future<List<LatLng>> latLngsForPixels(List<ScreenPoint> points) =>
      _coordinateListQuery<LatLng>(
        points.length,
        (arena, outOperation) {
          final nativePoints = points.isEmpty
              ? nullptr.cast<raw.mln_screen_point>()
              : arena<raw.mln_screen_point>(points.length);
          for (var index = 0; index < points.length; index += 1) {
            nativePoints[index] = native_struct.screenPointToNative(
              points[index],
            );
          }
          _check(
            raw.mln_map_lat_lngs_for_pixels_start(
              _handle.raw,
              nativePoints,
              points.length,
              outOperation,
            ),
          );
        },
        (operation, arena, count) {
          final values = count == 0
              ? nullptr.cast<raw.mln_lat_lng>()
              : arena<raw.mln_lat_lng>(count);
          final outCount = arena<Size>();
          _check(
            raw.mln_map_lat_lngs_for_pixels_take_result(
              operation,
              values,
              count,
              outCount,
            ),
          );
          return [
            for (var index = 0; index < outCount.value; index += 1)
              native_struct.latLngFromNative(values[index]),
          ];
        },
      );

  Future<List<T>> _coordinateListQuery<T>(
    int count,
    void Function(Arena, Pointer<Uint64>) start,
    List<T> Function(int, Arena, int) take,
  ) {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      start(arena, outOperation);
      return outOperation.value;
    });
    return _runtime._takeOperation(
      operation,
      () => withNativeArena((arena) => take(operation, arena, count)),
    );
  }

  /// Creates a projection helper without blocking the calling isolate.
  Future<MapProjectionHandle> createProjection() {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      _check(raw.mln_map_projection_create_start(_handle.raw, outOperation));
      return outOperation.value;
    });
    return _runtime._takeOperation(operation, () {
      return withNativeArena((arena) {
        final outProjection = arena<Uint64>()..value = 0;
        _check(
          raw.mln_map_projection_create_take_result(operation, outProjection),
        );
        return MapProjectionHandle._(
          _runtime,
          NativeMapProjection(outProjection.value),
        );
      });
    });
  }

  /// Sets or replaces one runtime style image.
  BigInt setStyleImage(
    String imageId,
    PremultipliedRgba8Image image, {
    StyleImageOptions? options,
  }) {
    final resolvedOptions = options ?? StyleImageOptions();
    return withNativeArena((arena) {
      final nativeId = nativeStringView(imageId, arena);
      final nativeImage = arena<raw.mln_premultiplied_rgba8_image>();
      nativeImage.ref = _premultipliedRgba8ImageToNative(image, arena);
      final nativeOptions = arena<raw.mln_style_image_options>();
      nativeOptions.ref = _styleImageOptionsToNative(resolvedOptions, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_style_image(
          _handle.raw,
          nativeId.value,
          nativeImage,
          nativeOptions,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies one runtime style image's stretchable intervals, or null when no
  /// image carries [imageId]. The record holds horizontal intervals first.
  Future<({List<ImageStretch> stretchX, List<ImageStretch> stretchY})?>
  getStyleImageStretches(String imageId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(imageId, arena);
        _check(
          raw.mln_map_copy_style_image_stretches_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outXCount = arena<Size>();
        final outYCount = arena<Size>();
        final outFound = arena<Bool>();
        _check(
          raw.mln_map_copy_style_image_stretches_take_result(
            operation,
            nullptr,
            0,
            outXCount,
            nullptr,
            0,
            outYCount,
            outFound,
          ),
        );
        if (!outFound.value) {
          return null;
        }

        final xCount = outXCount.value;
        final yCount = outYCount.value;
        final rawX = xCount == 0
            ? nullptr.cast<raw.mln_image_stretch>()
            : arena<raw.mln_image_stretch>(xCount);
        final rawY = yCount == 0
            ? nullptr.cast<raw.mln_image_stretch>()
            : arena<raw.mln_image_stretch>(yCount);
        _check(
          raw.mln_map_copy_style_image_stretches_take_result(
            operation,
            rawX,
            xCount,
            outXCount,
            rawY,
            yCount,
            outYCount,
            outFound,
          ),
        );
        List<ImageStretch> read(
          Pointer<raw.mln_image_stretch> array,
          int count,
        ) => List<ImageStretch>.generate(
          count,
          (index) => ImageStretch(array[index].from, array[index].to),
        );
        return (
          stretchX: read(rawX, outXCount.value),
          stretchY: read(rawY, outYCount.value),
        );
      },
    );
  }

  /// Removes one runtime style image and returns whether one was removed.
  Future<bool> removeStyleImage(String imageId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(imageId, arena);
        _check(
          raw.mln_map_remove_style_image_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outRemoved = arena<Bool>();
        _check(
          raw.mln_map_remove_style_image_take_result(operation, outRemoved),
        );
        return outRemoved.value;
      },
    );
  }

  /// Reports whether one runtime style image exists.
  Future<bool> styleImageExists(String imageId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(imageId, arena);
        _check(
          raw.mln_map_style_image_exists_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outExists = arena<Bool>();
        _check(
          raw.mln_map_style_image_exists_take_result(operation, outExists),
        );
        return outExists.value;
      },
    );
  }

  /// Copies fixed metadata for one runtime style image.
  Future<StyleImageInfo?> getStyleImageInfo(String imageId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(imageId, arena);
        _check(
          raw.mln_map_get_style_image_info_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outInfo = arena<raw.mln_style_image_info>();
        outInfo.ref = raw.mln_style_image_info_default();
        final outFound = arena<Bool>();
        _check(
          raw.mln_map_get_style_image_info_take_result(
            operation,
            outInfo,
            outFound,
          ),
        );
        return outFound.value ? _styleImageInfoFromNative(outInfo.ref) : null;
      },
    );
  }

  /// Copies one runtime style image as premultiplied RGBA8 pixels.
  Future<StyleImage?> copyStyleImagePremultipliedRgba8(String imageId) async {
    final info = await getStyleImageInfo(imageId);
    if (info == null) {
      return null;
    }
    final bytes = await _styleOperation<Uint8List?>(
      (arena, outOperation) {
        final nativeId = nativeStringView(imageId, arena);
        _check(
          raw.mln_map_copy_style_image_premultiplied_rgba8_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outPixels = arena<Uint64>()..value = 0;
        final outFound = arena<Bool>();
        _check(
          raw.mln_map_copy_style_image_premultiplied_rgba8_take_result(
            operation,
            outPixels,
            outFound,
          ),
        );
        return outFound.value
            ? copyOwnedBuffer(NativeOwnedBufferHandle(outPixels.value))
            : null;
      },
    );
    return bytes == null ? null : StyleImage(info: info, bytes: bytes);
  }

  Future<T> _styleOperation<T>(
    void Function(Arena arena, Pointer<Uint64> outOperation) start,
    T Function(int operation, Arena arena) take,
  ) {
    final operation = withNativeArena((arena) {
      final outOperation = arena<Uint64>()..value = 0;
      start(arena, outOperation);
      return outOperation.value;
    });
    return _runtime._takeOperation(
      operation,
      () => withNativeArena((arena) => take(operation, arena)),
    );
  }

  Future<String?> _copyStyleSourceText(
    String sourceId,
    int Function(int, raw.mln_buffer_view, Pointer<Uint64>) start,
    int Function(int, Pointer<Uint64>, Pointer<Bool>) take,
  ) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(sourceId, arena);
        _check(start(_handle.raw, nativeId.value, outOperation));
      },
      (operation, arena) {
        final outBuffer = arena<Uint64>()..value = 0;
        final outFound = arena<Bool>();
        _check(take(operation, outBuffer, outFound));
        if (!outFound.value) {
          return null;
        }
        return utf8.decode(
          copyOwnedBuffer(NativeOwnedBufferHandle(outBuffer.value)),
        );
      },
    );
  }

  Future<String> _copyLayerText(
    String layerId,
    int Function(int, raw.mln_buffer_view, Pointer<Uint64>) start,
    int Function(int, Pointer<Uint64>) take,
  ) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(layerId, arena);
        _check(start(_handle.raw, nativeId.value, outOperation));
      },
      (operation, arena) {
        final outBuffer = arena<Uint64>()..value = 0;
        _check(take(operation, outBuffer));
        return utf8.decode(
          copyOwnedBuffer(NativeOwnedBufferHandle(outBuffer.value)),
        );
      },
    );
  }

  /// Adds one style source from a style-spec source JSON object.
  ///
  /// Returns the accepted runtime-wide command ID.
  BigInt addStyleSourceJson(String sourceId, Uint8List sourceJson) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeSourceJson = nativeBufferView(sourceJson, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_style_source_json(
          _handle.raw,
          nativeId.value,
          nativeSourceJson,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a GeoJSON source that loads from [url].
  BigInt addGeoJsonSourceUrl(
    String sourceId,
    String url, {
    GeoJsonSourceOptions? options,
  }) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      final nativeOptions = _nativeGeoJsonSourceOptions(
        options ?? GeoJsonSourceOptions(),
        arena,
      );
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_geojson_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          nativeOptions,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a GeoJSON source with inline data.
  BigInt addGeoJsonSourceData(
    String sourceId,
    Uint8List data, {
    GeoJsonSourceOptions? options,
  }) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeData = nativeBufferView(data, arena);
      final nativeOptions = _nativeGeoJsonSourceOptions(
        options ?? GeoJsonSourceOptions(),
        arena,
      );
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_geojson_source_data(
          _handle.raw,
          nativeId.value,
          nativeData,
          nativeOptions,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Updates one GeoJSON source to load from [url].
  BigInt setGeoJsonSourceUrl(String sourceId, String url) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_geojson_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Updates one GeoJSON source with inline data.
  BigInt setGeoJsonSourceData(String sourceId, Uint8List data) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeData = nativeBufferView(data, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_geojson_source_data(
          _handle.raw,
          nativeId.value,
          nativeData,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a vector source with a TileJSON URL.
  BigInt addVectorSourceUrl(
    String sourceId,
    String url, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_vector_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          _nativeTileSourceOptions(options, arena),
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a vector source with inline tile URL templates.
  BigInt addVectorSourceTiles(
    String sourceId,
    List<String> tiles, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_vector_source_tiles(
          _handle.raw,
          nativeId.value,
          _stringViewArray(tiles, arena),
          tiles.length,
          _nativeTileSourceOptions(options, arena),
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a raster source with a TileJSON URL.
  BigInt addRasterSourceUrl(
    String sourceId,
    String url, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_raster_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          _nativeTileSourceOptions(options, arena),
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a raster source with inline tile URL templates.
  BigInt addRasterSourceTiles(
    String sourceId,
    List<String> tiles, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_raster_source_tiles(
          _handle.raw,
          nativeId.value,
          _stringViewArray(tiles, arena),
          tiles.length,
          _nativeTileSourceOptions(options, arena),
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a raster DEM source with a TileJSON URL.
  BigInt addRasterDemSourceUrl(
    String sourceId,
    String url, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_raster_dem_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          _nativeTileSourceOptions(options, arena),
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a raster DEM source with inline tile URL templates.
  BigInt addRasterDemSourceTiles(
    String sourceId,
    List<String> tiles, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_raster_dem_source_tiles(
          _handle.raw,
          nativeId.value,
          _stringViewArray(tiles, arena),
          tiles.length,
          _nativeTileSourceOptions(options, arena),
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds an image source that loads its image from [url].
  BigInt addImageSourceUrl(
    String sourceId,
    List<LatLng> coordinates,
    String url,
  ) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_image_source_url(
          _handle.raw,
          nativeId.value,
          _latLngArray(coordinates, arena),
          coordinates.length,
          nativeUrl.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds an image source with inline image pixels.
  BigInt addImageSourceImage(
    String sourceId,
    List<LatLng> coordinates,
    PremultipliedRgba8Image image,
  ) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeImage = arena<raw.mln_premultiplied_rgba8_image>();
      nativeImage.ref = _premultipliedRgba8ImageToNative(image, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_image_source_image(
          _handle.raw,
          nativeId.value,
          _latLngArray(coordinates, arena),
          coordinates.length,
          nativeImage,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Updates an image source to load from [url].
  BigInt setImageSourceUrl(String sourceId, String url) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_image_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Updates an image source with inline image pixels.
  BigInt setImageSourceImage(String sourceId, PremultipliedRgba8Image image) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeImage = arena<raw.mln_premultiplied_rgba8_image>();
      nativeImage.ref = _premultipliedRgba8ImageToNative(image, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_image_source_image(
          _handle.raw,
          nativeId.value,
          nativeImage,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Updates image source coordinates.
  BigInt setImageSourceCoordinates(String sourceId, List<LatLng> coordinates) {
    return withNativeArena<BigInt>((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_image_source_coordinates(
          _handle.raw,
          nativeId.value,
          _latLngArray(coordinates, arena),
          coordinates.length,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies image source coordinates, or null when the source is missing.
  Future<List<LatLng>?> getImageSourceCoordinates(String sourceId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(sourceId, arena);
        _check(
          raw.mln_map_get_image_source_coordinates_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outCoordinates = arena<raw.mln_lat_lng>(4);
        final outCount = arena<Size>();
        final outFound = arena<Bool>();
        _check(
          raw.mln_map_get_image_source_coordinates_take_result(
            operation,
            outCoordinates,
            4,
            outCount,
            outFound,
          ),
        );
        return outFound.value
            ? [
                for (var index = 0; index < outCount.value; index += 1)
                  native_struct.latLngFromNative(outCoordinates[index]),
              ]
            : null;
      },
    );
  }

  /// Adds a custom geometry source with queued fetch/cancel notifications.
  ///
  /// The C API releases the source's callback root when the source is removed,
  /// when a style load drops it, or when this map is destroyed, so a host that
  /// adds a source subscribes to nothing to keep it alive.
  BigInt addCustomGeometrySource(
    String sourceId,
    CustomGeometrySourceOptions options,
  ) {
    final callbackState = _CustomGeometryCallbackState(
      options,
      () => _releaseCustomGeometryCallbacks(sourceId),
    );
    try {
      final commandId = withNativeArena((arena) {
        final nativeId = nativeStringView(sourceId, arena);
        final nativeOptions = arena<raw.mln_custom_geometry_source_options>();
        nativeOptions.ref = _customGeometrySourceOptionsToNative(
          options,
          callbackState,
        );
        final outCommandId = arena<Uint64>()..value = 0;
        _check(
          raw.mln_map_add_custom_geometry_source(
            _handle.raw,
            nativeId.value,
            nativeOptions,
            outCommandId,
          ),
        );
        return outCommandId.value;
      });
      _pendingStyleCallbacks[commandId] = (
        sourceId: sourceId,
        state: callbackState,
      );
      return uint64FromNative(commandId);
    } catch (_) {
      callbackState.close();
      rethrow;
    }
  }

  /// Sets custom geometry source data for one canonical tile.
  BigInt setCustomGeometrySourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Uint8List data,
  ) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeData = nativeBufferView(data, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_custom_geometry_source_tile_data(
          _handle.raw,
          nativeId.value,
          _canonicalTileIdToNative(tileId),
          nativeData,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Invalidates custom geometry source data for one canonical tile.
  BigInt invalidateCustomGeometrySourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_invalidate_custom_geometry_source_tile(
          _handle.raw,
          nativeId.value,
          _canonicalTileIdToNative(tileId),
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Invalidates custom geometry source data inside one geographic region.
  BigInt invalidateCustomGeometrySourceRegion(
    String sourceId,
    LatLngBounds bounds,
  ) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_invalidate_custom_geometry_source_region(
          _handle.raw,
          nativeId.value,
          native_struct.latLngBoundsToNative(bounds),
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Reports whether a style source ID exists after prior commands.
  Future<bool> styleSourceExists(String sourceId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(sourceId, arena);
        _check(
          raw.mln_map_style_source_exists_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outExists = arena<Bool>();
        _check(
          raw.mln_map_style_source_exists_take_result(operation, outExists),
        );
        return outExists.value;
      },
    );
  }

  /// Removes one style source by ID after prior commands.
  Future<bool> removeStyleSource(String sourceId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(sourceId, arena);
        _check(
          raw.mln_map_remove_style_source_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outRemoved = arena<Bool>();
        _check(
          raw.mln_map_remove_style_source_take_result(operation, outRemoved),
        );
        return outRemoved.value;
      },
    );
  }

  /// Reads one style source type after prior commands.
  Future<SourceType?> getStyleSourceType(String sourceId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(sourceId, arena);
        _check(
          raw.mln_map_get_style_source_type_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outType = arena<Uint32>();
        final outFound = arena<Bool>();
        _check(
          raw.mln_map_get_style_source_type_take_result(
            operation,
            outType,
            outFound,
          ),
        );
        return outFound.value ? SourceType.fromRaw(outType.value) : null;
      },
    );
  }

  /// Copies fixed style source metadata after prior commands.
  Future<SourceInfo?> getStyleSourceInfo(String sourceId) async {
    late int fields;
    late int type;
    late bool isVolatile;
    late bool hasAttribution;
    late double minZoom;
    late double maxZoom;
    late int scheme;
    late LatLngBounds bounds;
    late int tileSize;
    late int vectorEncoding;
    late int rasterEncoding;
    final found = await _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(sourceId, arena);
        _check(
          raw.mln_map_get_style_source_info_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outInfo = arena<raw.mln_style_source_info>();
        outInfo.ref.size = sizeOf<raw.mln_style_source_info>();
        final outFound = arena<Bool>();
        _check(
          raw.mln_map_get_style_source_info_take_result(
            operation,
            outInfo,
            outFound,
          ),
        );
        if (!outFound.value) {
          return false;
        }
        final info = outInfo.ref;
        fields = info.fields;
        type = info.type;
        isVolatile = info.is_volatile;
        hasAttribution = info.has_attribution;
        minZoom = info.min_zoom;
        maxZoom = info.max_zoom;
        scheme = info.scheme;
        bounds = native_struct.latLngBoundsFromNative(info.bounds);
        tileSize = info.tile_size;
        vectorEncoding = info.vector_encoding;
        rasterEncoding = info.raster_encoding;
        return true;
      },
    );
    if (!found) {
      return null;
    }
    final hasUrl =
        fields &
            raw.mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_URL.value !=
        0;
    final hasTileJson =
        fields &
            raw
                .mln_style_source_info_field
                .MLN_STYLE_SOURCE_INFO_TILEJSON
                .value !=
        0;
    final hasBounds =
        fields &
            raw
                .mln_style_source_info_field
                .MLN_STYLE_SOURCE_INFO_BOUNDS
                .value !=
        0;
    final hasTileSize =
        fields &
            raw
                .mln_style_source_info_field
                .MLN_STYLE_SOURCE_INFO_TILE_SIZE
                .value !=
        0;
    final hasVectorEncoding =
        fields &
            raw
                .mln_style_source_info_field
                .MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING
                .value !=
        0;
    final hasRasterEncoding =
        fields &
            raw
                .mln_style_source_info_field
                .MLN_STYLE_SOURCE_INFO_RASTER_ENCODING
                .value !=
        0;
    final attribution = hasAttribution
        ? await _copyStyleSourceText(
            sourceId,
            raw.mln_map_copy_style_source_attribution_start,
            raw.mln_map_copy_style_source_attribution_take_result,
          )
        : null;
    final url = hasUrl
        ? await _copyStyleSourceText(
            sourceId,
            raw.mln_map_copy_style_source_url_start,
            raw.mln_map_copy_style_source_url_take_result,
          )
        : null;
    final tileUrls = hasTileJson
        ? await _styleOperation<List<String>?>(
            (arena, outOperation) {
              final nativeId = nativeStringView(sourceId, arena);
              _check(
                raw.mln_map_get_style_source_tile_urls_start(
                  _handle.raw,
                  nativeId.value,
                  outOperation,
                ),
              );
            },
            (operation, arena) {
              final outList = arena<Uint64>()..value = 0;
              final outFound = arena<Bool>();
              _check(
                raw.mln_map_get_style_source_tile_urls_take_result(
                  operation,
                  outList,
                  outFound,
                ),
              );
              return outFound.value
                  ? _copyStyleStringList(NativeStyleStringList(outList.value))
                  : null;
            },
          )
        : null;
    return SourceInfo(
      type: SourceType.fromRaw(type),
      id: sourceId,
      isVolatile: isVolatile,
      attribution: attribution,
      url: url,
      tileJson: hasTileJson
          ? ParsedTileJson(
              tileUrls: tileUrls ?? const [],
              minZoom: minZoom,
              maxZoom: maxZoom,
              scheme: TileScheme.fromRaw(scheme),
              bounds: hasBounds ? bounds : null,
            )
          : null,
      tileSize: hasTileSize ? tileSize : null,
      vectorEncoding: hasVectorEncoding
          ? VectorTileEncoding.fromRaw(vectorEncoding)
          : null,
      rasterDemEncoding: hasRasterEncoding
          ? RasterDemEncoding.fromRaw(rasterEncoding)
          : null,
    );
  }

  /// Copies style source IDs in style order after prior commands.
  Future<List<String>> listStyleSourceIds() {
    return _styleOperation(
      (arena, outOperation) {
        _check(
          raw.mln_map_list_style_source_ids_start(_handle.raw, outOperation),
        );
      },
      (operation, arena) {
        final outList = arena<Uint64>()..value = 0;
        _check(
          raw.mln_map_list_style_source_ids_take_result(operation, outList),
        );
        return _copyStyleIdList(NativeStyleIdList(outList.value));
      },
    );
  }

  /// Adds a hillshade layer for a raster DEM source.
  BigInt addHillshadeLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) {
    return withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceId = nativeStringView(sourceId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_hillshade_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceId.value,
          nativeBeforeLayerId.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a color-relief layer for a raster DEM source.
  BigInt addColorReliefLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) {
    return withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceId = nativeStringView(sourceId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_color_relief_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceId.value,
          nativeBeforeLayerId.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds a source-free location indicator layer.
  BigInt addLocationIndicatorLayer(String layerId, {String? beforeLayerId}) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_location_indicator_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeBeforeLayerId.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Sets a location indicator layer location.
  BigInt setLocationIndicatorLocation(
    String layerId,
    LatLng coordinate, {
    double altitude = 0,
  }) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_location_indicator_location(
          _handle.raw,
          nativeLayerId.value,
          native_struct.latLngToNative(coordinate),
          altitude,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Sets a location indicator layer bearing in degrees.
  BigInt setLocationIndicatorBearing(String layerId, double bearing) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_location_indicator_bearing(
          _handle.raw,
          nativeLayerId.value,
          bearing,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Sets a location indicator layer accuracy radius in logical pixels.
  BigInt setLocationIndicatorAccuracyRadius(String layerId, double radius) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_location_indicator_accuracy_radius(
          _handle.raw,
          nativeLayerId.value,
          radius,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Sets one location indicator image-name property.
  BigInt setLocationIndicatorImageName(
    String layerId,
    LocationIndicatorImageKind imageKind,
    String imageId,
  ) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeImageId = nativeStringView(imageId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_location_indicator_image_name(
          _handle.raw,
          nativeLayerId.value,
          imageKind.rawValue,
          nativeImageId.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Adds one style layer from a full style-spec layer JSON object.
  BigInt addStyleLayerJson(Uint8List layerJson, {String? beforeLayerId}) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerJson = nativeBufferView(layerJson, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_add_style_layer_json(
          _handle.raw,
          nativeLayerJson,
          nativeBeforeLayerId.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies one style layer as a full style-spec layer JSON snapshot.
  Future<Uint8List?> getStyleLayerJson(String layerId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(layerId, arena);
        _check(
          raw.mln_map_get_style_layer_json_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outLayer = arena<Uint64>()..value = 0;
        final outFound = arena<Bool>();
        _check(
          raw.mln_map_get_style_layer_json_take_result(
            operation,
            outLayer,
            outFound,
          ),
        );
        return outFound.value
            ? copyOwnedBuffer(NativeOwnedBufferHandle(outLayer.value))
            : null;
      },
    );
  }

  /// Sets the style light from a style-spec light JSON object.
  BigInt setStyleLightJson(Uint8List lightJson) {
    return withNativeArena<BigInt>((arena) {
      final nativeLightJson = nativeBufferView(lightJson, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_style_light_json(
          _handle.raw,
          nativeLightJson,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Sets one style light property by style-spec property name.
  BigInt setStyleLightProperty(String propertyName, Uint8List value) {
    return withNativeArena<BigInt>((arena) {
      final nativePropertyName = nativeStringView(propertyName, arena);
      final nativeValue = nativeBufferView(value, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_style_light_property(
          _handle.raw,
          nativePropertyName.value,
          nativeValue,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies one style light property, or null when the property is undefined.
  Future<Uint8List?> getStyleLightProperty(String propertyName) {
    return _styleOperation(
      (arena, outOperation) {
        final nativePropertyName = nativeStringView(propertyName, arena);
        _check(
          raw.mln_map_get_style_light_property_start(
            _handle.raw,
            nativePropertyName.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outValue = arena<Uint64>()..value = 0;
        _check(
          raw.mln_map_get_style_light_property_take_result(operation, outValue),
        );
        final buffer = NativeOwnedBufferHandle(outValue.value);
        return buffer.isNull ? null : copyOwnedBuffer(buffer);
      },
    );
  }

  /// Sets the style's global transition options.
  ///
  /// This replaces the whole transition configuration rather than merging into
  /// it, and loading a style replaces it again with the style's own options, so
  /// apply an override after the style loads.
  BigInt setStyleTransitionOptions(StyleTransitionOptions options) {
    return withNativeArena<BigInt>((arena) {
      final nativeOptions = arena<raw.mln_style_transition_options>();
      nativeOptions.ref = _styleTransitionOptionsToNative(options);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_style_transition_options(
          _handle.raw,
          nativeOptions,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies the style's global transition options.
  Future<StyleTransitionOptions> getStyleTransitionOptions() {
    return _styleOperation(
      (arena, outOperation) {
        _check(
          raw.mln_map_get_style_transition_options_start(
            _handle.raw,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outOptions = arena<raw.mln_style_transition_options>();
        outOptions.ref = raw.mln_style_transition_options_default();
        _check(
          raw.mln_map_get_style_transition_options_take_result(
            operation,
            outOptions,
          ),
        );
        return _styleTransitionOptionsFromNative(outOptions.ref);
      },
    );
  }

  /// Sets one layer property by style-spec property name.
  BigInt setLayerProperty(
    String layerId,
    String propertyName,
    Uint8List value,
  ) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativePropertyName = nativeStringView(propertyName, arena);
      final nativeValue = nativeBufferView(value, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_layer_property(
          _handle.raw,
          nativeLayerId.value,
          nativePropertyName.value,
          nativeValue,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies one layer property, or null when the property is undefined.
  Future<Uint8List?> getLayerProperty(String layerId, String propertyName) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeLayerId = nativeStringView(layerId, arena);
        final nativePropertyName = nativeStringView(propertyName, arena);
        _check(
          raw.mln_map_get_layer_property_start(
            _handle.raw,
            nativeLayerId.value,
            nativePropertyName.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outValue = arena<Uint64>()..value = 0;
        _check(raw.mln_map_get_layer_property_take_result(operation, outValue));
        final buffer = NativeOwnedBufferHandle(outValue.value);
        return buffer.isNull ? null : copyOwnedBuffer(buffer);
      },
    );
  }

  /// Sets or clears one layer filter.
  BigInt setLayerFilter(String layerId, Uint8List? filter) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeFilter = filter == null
          ? nullptr.cast<raw.mln_buffer_view>()
          : (arena<raw.mln_buffer_view>()
              ..ref = nativeBufferView(filter, arena));
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_layer_filter(
          _handle.raw,
          nativeLayerId.value,
          nativeFilter,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies one layer filter, or null when the layer has no filter.
  Future<Uint8List?> getLayerFilter(String layerId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeLayerId = nativeStringView(layerId, arena);
        _check(
          raw.mln_map_get_layer_filter_start(
            _handle.raw,
            nativeLayerId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outFilter = arena<Uint64>()..value = 0;
        _check(raw.mln_map_get_layer_filter_take_result(operation, outFilter));
        final buffer = NativeOwnedBufferHandle(outFilter.value);
        return buffer.isNull ? null : copyOwnedBuffer(buffer);
      },
    );
  }

  /// Sets one layer's source-layer ID.
  ///
  /// Layer types that take no source, such as background, are rejected.
  BigInt setLayerSourceLayer(String layerId, String sourceLayer) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceLayer = nativeStringView(sourceLayer, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_layer_source_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceLayer.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies one layer's source-layer ID, empty when the layer carries none.
  Future<String> getLayerSourceLayer(String layerId) {
    return _copyLayerText(
      layerId,
      raw.mln_map_copy_layer_source_layer_start,
      raw.mln_map_copy_layer_source_layer_take_result,
    );
  }

  /// Sets one layer's source ID.
  ///
  /// Layer types that take no source, such as background, are rejected. The
  /// named source need not exist yet.
  BigInt setLayerSourceId(String layerId, String sourceId) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceId = nativeStringView(sourceId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_layer_source_id(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceId.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Copies one layer's source ID, empty when the layer carries none.
  Future<String> getLayerSourceId(String layerId) {
    return _copyLayerText(
      layerId,
      raw.mln_map_copy_layer_source_id_start,
      raw.mln_map_copy_layer_source_id_take_result,
    );
  }

  /// Sets the lowest zoom at which one layer draws.
  ///
  /// Pass `double.negativeInfinity` for no lower bound.
  BigInt setLayerMinZoom(String layerId, double minZoom) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_layer_min_zoom(
          _handle.raw,
          nativeLayerId.value,
          minZoom,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Reads the lowest zoom at which one layer draws.
  ///
  /// A layer with no lower bound reports `double.negativeInfinity`.
  Future<double> getLayerMinZoom(String layerId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeLayerId = nativeStringView(layerId, arena);
        _check(
          raw.mln_map_get_layer_min_zoom_start(
            _handle.raw,
            nativeLayerId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outZoom = arena<Double>();
        _check(raw.mln_map_get_layer_min_zoom_take_result(operation, outZoom));
        return outZoom.value;
      },
    );
  }

  /// Sets the highest zoom at which one layer draws.
  ///
  /// Pass `double.infinity` for no upper bound.
  BigInt setLayerMaxZoom(String layerId, double maxZoom) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_layer_max_zoom(
          _handle.raw,
          nativeLayerId.value,
          maxZoom,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Reads the highest zoom at which one layer draws.
  ///
  /// A layer with no upper bound reports `double.infinity`.
  Future<double> getLayerMaxZoom(String layerId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeLayerId = nativeStringView(layerId, arena);
        _check(
          raw.mln_map_get_layer_max_zoom_start(
            _handle.raw,
            nativeLayerId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outZoom = arena<Double>();
        _check(raw.mln_map_get_layer_max_zoom_take_result(operation, outZoom));
        return outZoom.value;
      },
    );
  }

  /// Sets whether one layer draws.
  BigInt setLayerVisibility(String layerId, StyleLayerVisibility visibility) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_set_layer_visibility(
          _handle.raw,
          nativeLayerId.value,
          visibility.rawValue,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Reads whether one layer draws.
  Future<StyleLayerVisibility> getLayerVisibility(String layerId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeLayerId = nativeStringView(layerId, arena);
        _check(
          raw.mln_map_get_layer_visibility_start(
            _handle.raw,
            nativeLayerId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outVisibility = arena<Uint32>();
        _check(
          raw.mln_map_get_layer_visibility_take_result(
            operation,
            outVisibility,
          ),
        );
        return StyleLayerVisibility.fromRawValue(outVisibility.value);
      },
    );
  }

  /// Reports whether a style layer ID exists.
  Future<bool> styleLayerExists(String layerId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(layerId, arena);
        _check(
          raw.mln_map_style_layer_exists_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outExists = arena<Bool>();
        _check(
          raw.mln_map_style_layer_exists_take_result(operation, outExists),
        );
        return outExists.value;
      },
    );
  }

  /// Copies one style layer type string, or returns null when absent.
  Future<String?> getStyleLayerType(String layerId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(layerId, arena);
        _check(
          raw.mln_map_get_style_layer_type_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outLayerType = arena<Uint64>()..value = 0;
        final outFound = arena<Bool>();
        _check(
          raw.mln_map_get_style_layer_type_take_result(
            operation,
            outLayerType,
            outFound,
          ),
        );
        return outFound.value
            ? utf8.decode(
                copyOwnedBuffer(NativeOwnedBufferHandle(outLayerType.value)),
              )
            : null;
      },
    );
  }

  /// Moves one style layer before another layer or to the top.
  BigInt moveStyleLayer(String layerId, {String? beforeLayerId}) {
    return withNativeArena<BigInt>((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      final outCommandId = arena<Uint64>()..value = 0;
      _check(
        raw.mln_map_move_style_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeBeforeLayerId.value,
          outCommandId,
        ),
      );
      return uint64FromNative(outCommandId.value);
    });
  }

  /// Removes one style layer by ID and returns whether one was removed.
  Future<bool> removeStyleLayer(String layerId) {
    return _styleOperation(
      (arena, outOperation) {
        final nativeId = nativeStringView(layerId, arena);
        _check(
          raw.mln_map_remove_style_layer_start(
            _handle.raw,
            nativeId.value,
            outOperation,
          ),
        );
      },
      (operation, arena) {
        final outRemoved = arena<Bool>();
        _check(
          raw.mln_map_remove_style_layer_take_result(operation, outRemoved),
        );
        return outRemoved.value;
      },
    );
  }

  /// Copies style layer IDs in style order.
  Future<List<String>> listStyleLayerIds() {
    return _styleOperation(
      (arena, outOperation) {
        _check(
          raw.mln_map_list_style_layer_ids_start(_handle.raw, outOperation),
        );
      },
      (operation, arena) {
        final outList = arena<Uint64>()..value = 0;
        _check(
          raw.mln_map_list_style_layer_ids_take_result(operation, outList),
        );
        return _copyStyleIdList(NativeStyleIdList(outList.value));
      },
    );
  }

  /// Closes this map after its queued work has finished.
  ///
  /// Callback roots remain alive until native close completion releases every
  /// custom-geometry source that the map still owns.
  Future<void> close() async {
    final id = _state.handleId;
    await _state.closeAsync((handle) async {
      final operation = withNativeArena((arena) {
        final outOperation = arena<Uint64>()..value = 0;
        _check(raw.mln_map_close_start(handle.raw, outOperation));
        return outOperation.value;
      });
      await _runtime._finishOperation(operation);
      // Native release callbacks are listener callbacks. Yield on this isolate
      // before retiring any root whose release message has not run yet.
      await Future<void>.delayed(Duration.zero);
      for (final state in _customGeometryCallbacks.values.toList()) {
        state._retire();
      }
      for (final pending in _pendingStyleCallbacks.values.toList()) {
        pending.state._retire();
      }
      await Future<void>.delayed(Duration.zero);
    });
    _runtime._unregisterMapId(id);
  }

  /// Drops [sourceId]'s callback root once the C API has released it.
  void _finishStyleCommand(int commandId, CommandDisposition disposition) {
    final pending = _pendingStyleCallbacks.remove(commandId);
    if (pending == null) {
      return;
    }
    if (disposition == CommandDisposition.committed) {
      _customGeometryCallbacks[pending.sourceId] = pending.state;
    } else {
      pending.state.close();
    }
  }

  void _releaseCustomGeometryCallbacks(String sourceId) {
    _customGeometryCallbacks.remove(sourceId);
  }
}
