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
  final state = map._customGeometryCallbacks[sourceId];
  return state == null ? null : CustomGeometryCallbackLifecycleProbe._(state);
}

/// Dart resource provider callback run on the owner isolate.
typedef ResourceProviderCallback =
    void Function(ResourceRequest request, ResourceRequestHandle handle);

/// Owner-isolate resource provider definition.
final class ResourceProvider {
  /// Creates a resource provider with native-owned routing rules.
  ResourceProvider({
    required List<ResourceProviderRoute> routes,
    required this.callback,
  }) : routes = List.unmodifiable(routes);

  /// Exact routes handled by this provider.
  final List<ResourceProviderRoute> routes;

  /// Callback invoked on the owner isolate for matching requests.
  final ResourceProviderCallback callback;
}

/// Runtime creation options.
final class RuntimeOptions {
  /// Creates runtime options.
  const RuntimeOptions({this.assetPath, this.cachePath});

  /// Filesystem root for `asset://` URLs.
  final String? assetPath;

  /// Cache database path.
  final String? cachePath;
}

/// Owner-thread runtime handle for MapLibre Native work and event polling.
final class RuntimeHandle {
  RuntimeHandle._(NativeRuntime handle)
    : _state = NativeHandleState(handle, 'RuntimeHandle');

  final NativeHandleState<NativeRuntime> _state;
  final _maps = <int, WeakReference<MapHandle>>{};
  final _offlineOperations = <int, WeakReference<OfflineOperationHandle>>{};
  _ResourceTransformState? _resourceTransformState;
  _HttpHeaderTransformState? _httpHeaderTransformState;
  _ResourceProviderRulesState? _resourceProviderRulesState;
  _ResourceProviderCallbackState? _resourceProviderCallbackState;

  /// Creates a runtime on the current native thread.
  ///
  /// Do not `await` I/O on this isolate while the runtime or any handle under it
  /// is live. Owner-thread checks key on the OS thread, and the Dart VM may
  /// resume an isolate on a different one, after which every call on the handle
  /// fails with `wrongThread` — including [close].
  factory RuntimeHandle.create({
    RuntimeOptions options = const RuntimeOptions(),
  }) {
    // Check before the native call: an incompatible library would otherwise
    // create a runtime that the failed check leaves no way to destroy.
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeOptions = arena<raw.mln_runtime_options>();
      nativeOptions.ref = _runtimeOptionsToNative(options, arena);
      final outRuntime = arena<Uint64>();
      outRuntime.value = 0;
      _check(raw.mln_runtime_create(nativeOptions, outRuntime));
      return RuntimeHandle._(NativeRuntime(outRuntime.value));
    });
  }

  NativeRuntime get _handle => _state.handle;

  /// Whether this runtime has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  /// Advances this runtime, parking the owner isolate until there is work.
  ///
  /// The default zero [timeout] drains without parking; a null timeout parks
  /// until the wake flag is set; a negative timeout collapses to no wait.
  /// Native work, a queued runtime event, and [WakeSource.signal] each set the
  /// wake flag, which this call clears before returning. Drain events with
  /// [pollEvent] after every return.
  ///
  /// A non-null, non-zero [timeout] blocks the calling isolate's event loop for
  /// the duration of the park. Acquire a [WakeSource] with [acquireWakeSource]
  /// so another isolate can release this one.
  void pump({Duration? timeout = Duration.zero}) {
    _check(
      raw.mln_runtime_pump(_handle.raw, _pumpTimeoutMilliseconds(timeout)),
    );
  }

  /// Acquires a wake source that releases this runtime's parked owner isolate.
  ///
  /// Each call returns a distinct handle. A wake source holds its own reference
  /// to the runtime's wake state, so the two close in either order.
  WakeSource acquireWakeSource() {
    return withNativeArena((arena) {
      final outSource = arena<Uint64>();
      outSource.value = 0;
      _check(raw.mln_runtime_wake_source_acquire(_handle.raw, outSource));
      return WakeSource._(NativeWakeSource(outSource.value));
    });
  }

  /// Polls one queued runtime event and copies borrowed fields into Dart values.
  RuntimeEvent? pollEvent() {
    return withNativeArena((arena) {
      final event = arena<raw.mln_runtime_event>();
      event.ref.size = sizeOf<raw.mln_runtime_event>();
      final hasEvent = arena<Bool>();
      hasEvent.value = false;

      _check(raw.mln_runtime_poll_event(_handle.raw, event, hasEvent));
      if (!hasEvent.value) {
        return null;
      }

      final copiedEvent = RuntimeEvent._fromNative(event.ref, this);
      _handleRuntimeEvent(copiedEvent);
      return copiedEvent;
    });
  }

  /// Polls and discards queued runtime events until the queue is empty.
  int drainEvents() {
    var count = 0;
    while (pollEvent() != null) {
      count += 1;
    }
    return count;
  }

  /// Registers exact native-owned URL rewrite rules for network resources.
  void setResourceUrlRewriteRules(List<ResourceUrlRewriteRule> rules) {
    final state = _ResourceTransformState(rules);
    try {
      withNativeArena((arena) {
        final transform = arena<raw.mln_resource_transform>();
        transform.ref.size = sizeOf<raw.mln_resource_transform>();
        transform.ref.callback = _c.adapterResourceTransformRewriteCallback();
        transform.ref.user_data = state.pointer.cast<Void>();
        _check(raw.mln_runtime_set_resource_transform(_handle.raw, transform));
      });
      _resourceTransformState?.close();
      _resourceTransformState = state;
    } catch (_) {
      state.close();
      rethrow;
    }
  }

  /// Clears runtime-scoped URL rewrite rules.
  void clearResourceTransform() {
    _check(raw.mln_runtime_clear_resource_transform(_handle.raw));
    _resourceTransformState?.close();
    _resourceTransformState = null;
  }

  /// Registers native-owned HTTP header routes evaluated on network threads.
  void setHttpHeaderTransformRules(List<HttpHeaderTransformRule> rules) {
    final state = _HttpHeaderTransformState(rules);
    try {
      withNativeArena((arena) {
        final transform = arena<raw.mln_http_header_transform>();
        transform.ref.size = sizeOf<raw.mln_http_header_transform>();
        transform.ref.callback = _c.adapterHttpHeaderTransformCallback();
        transform.ref.user_data = state.pointer.cast<Void>();
        _check(
          raw.mln_runtime_set_http_header_transform(_handle.raw, transform),
        );
      });
      _httpHeaderTransformState?.close();
      _httpHeaderTransformState = state;
    } catch (_) {
      state.close();
      rethrow;
    }
  }

  /// Clears native-owned HTTP header transform routes.
  void clearHttpHeaderTransform() {
    _check(raw.mln_runtime_clear_http_header_transform(_handle.raw));
    _httpHeaderTransformState?.close();
    _httpHeaderTransformState = null;
  }

  /// Registers or replaces exact native-owned response rules.
  void setResourceProviderRules(List<ResourceProviderRule> rules) {
    final state = _ResourceProviderRulesState(rules);
    try {
      withNativeArena((arena) {
        final provider = arena<raw.mln_resource_provider>();
        provider.ref.size = sizeOf<raw.mln_resource_provider>();
        provider.ref.callback = _c.adapterResourceProviderRulesCallback();
        provider.ref.user_data = state.pointer.cast<Void>();
        _check(raw.mln_runtime_set_resource_provider(_handle.raw, provider));
      });
      _resourceProviderRulesState?.close();
      _resourceProviderRulesState = state;
      _resourceProviderCallbackState?.retire();
      _resourceProviderCallbackState = null;
    } catch (_) {
      state.close();
      rethrow;
    }
  }

  /// Registers or replaces a queued Dart resource provider callback.
  void setResourceProvider(ResourceProvider provider) {
    final state = _ResourceProviderCallbackState(provider);
    try {
      withNativeArena((arena) {
        final nativeProvider = arena<raw.mln_resource_provider>();
        nativeProvider.ref.size = sizeOf<raw.mln_resource_provider>();
        nativeProvider.ref.callback = _c
            .adapterQueuedResourceProviderCallback();
        nativeProvider.ref.user_data = state.pointer.cast<Void>();
        _check(
          raw.mln_runtime_set_resource_provider(_handle.raw, nativeProvider),
        );
      });
      _resourceProviderCallbackState?.retire();
      _resourceProviderCallbackState = state;
      _resourceProviderRulesState?.close();
      _resourceProviderRulesState = null;
    } catch (_) {
      state.close();
      rethrow;
    }
  }

  /// Clears the runtime-scoped network resource provider.
  void clearResourceProvider() {
    _check(raw.mln_runtime_clear_resource_provider(_handle.raw));
    _resourceProviderRulesState?.close();
    _resourceProviderRulesState = null;
    _resourceProviderCallbackState?.retire();
    _resourceProviderCallbackState = null;
  }

  /// Starts an ambient cache maintenance operation.
  OfflineOperationHandle runAmbientCacheOperation(
    AmbientCacheOperation operation,
  ) {
    return withNativeArena((arena) {
      final outOperationId = arena<Uint64>();
      _check(
        raw.mln_runtime_run_ambient_cache_operation_start(
          _handle.raw,
          operation.rawValue,
          outOperationId,
        ),
      );
      return OfflineOperationHandle._(
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
  OfflineOperationHandle setMaximumAmbientCacheSize(BigInt size) {
    return withNativeArena((arena) {
      final outOperationId = arena<Uint64>();
      _check(
        raw.mln_runtime_set_maximum_ambient_cache_size_start(
          _handle.raw,
          uint64ToNative(size, 'maximum ambient cache size'),
          outOperationId,
        ),
      );
      return OfflineOperationHandle._(
        this,
        outOperationId.value,
        _OfflineOperationKind.setMaximumAmbientCacheSize,
        _OfflineOperationResultKind.none,
      );
    });
  }

  /// Starts creating an offline region.
  OfflineOperationHandle createOfflineRegion(
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
      return OfflineOperationHandle._(
        this,
        outOperationId.value,
        _OfflineOperationKind.regionCreate,
        _OfflineOperationResultKind.region,
      );
    });
  }

  /// Starts getting an offline region snapshot by ID.
  OfflineOperationHandle getOfflineRegion(int regionId) =>
      _startOfflineOperation(
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
  OfflineOperationHandle listOfflineRegions() => _startOfflineOperation(
    _OfflineOperationKind.regionsList,
    _OfflineOperationResultKind.regionList,
    (outOperationId) {
      _check(
        raw.mln_runtime_offline_regions_list_start(_handle.raw, outOperationId),
      );
    },
  );

  /// Starts merging offline regions from another database path.
  OfflineOperationHandle mergeOfflineRegionDatabase(String sideDatabasePath) {
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
      return OfflineOperationHandle._(
        this,
        outOperationId.value,
        _OfflineOperationKind.regionsMergeDatabase,
        _OfflineOperationResultKind.regionList,
      );
    });
  }

  /// Starts updating opaque offline region metadata.
  OfflineOperationHandle updateOfflineRegionMetadata(
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
      return OfflineOperationHandle._(
        this,
        outOperationId.value,
        _OfflineOperationKind.regionUpdateMetadata,
        _OfflineOperationResultKind.region,
      );
    });
  }

  /// Starts getting the current offline region status.
  OfflineOperationHandle getOfflineRegionStatus(int regionId) =>
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
  OfflineOperationHandle setOfflineRegionObserved(
    int regionId,
    bool observed,
  ) => _startOfflineOperation(
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
  OfflineOperationHandle setOfflineRegionDownloadState(
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
  OfflineOperationHandle invalidateOfflineRegion(int regionId) =>
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
  OfflineOperationHandle deleteOfflineRegion(int regionId) =>
      _startOfflineOperation(
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

  OfflineOperationHandle _startOfflineOperation(
    _OfflineOperationKind kind,
    _OfflineOperationResultKind resultKind,
    void Function(Pointer<Uint64> outOperationId) start,
  ) {
    return withNativeArena((arena) {
      final outOperationId = arena<Uint64>();
      start(outOperationId);
      return OfflineOperationHandle._(
        this,
        outOperationId.value,
        kind,
        resultKind,
      );
    });
  }

  /// Creates a map owned by this runtime.
  MapHandle createMap({MapOptions options = const MapOptions()}) =>
      MapHandle.create(this, options: options);

  void _registerMap(MapHandle map) {
    _maps[map._handle.raw] = WeakReference(map);
  }

  void _unregisterMapId(int id) {
    _maps.remove(id);
  }

  void _registerOfflineOperation(OfflineOperationHandle operation) {
    _offlineOperations[operation._id] = WeakReference(operation);
  }

  void _unregisterOfflineOperation(int id) {
    _offlineOperations.remove(id);
  }

  void _handleRuntimeEvent(RuntimeEvent event) {
    if (event.sourceType !=
        raw.mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_MAP.value) {
      return;
    }
    if (event.type !=
        raw.mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_STYLE_LOADED.value) {
      return;
    }
    final reference = _maps[event._sourceId];
    final map = reference?.target;
    if (map == null) {
      _maps.remove(event._sourceId);
      return;
    }
    map._clearCustomGeometryCallbacksAfterUrlStyleLoad();
  }

  /// Explicitly destroys this runtime.
  void close() {
    final collectedOperationIds = _offlineOperations.entries
        .where((entry) => entry.value.target == null)
        .map((entry) => entry.key)
        .toList(growable: false);
    for (final operationId in collectedOperationIds) {
      _check(
        raw.mln_runtime_offline_operation_discard(_handle.raw, operationId),
      );
      _offlineOperations.remove(operationId);
    }
    if (_offlineOperations.isNotEmpty) {
      throwInvalidState(
        'RuntimeHandle has ${_offlineOperations.length} live offline '
        'operation(s); take or discard every result before closing',
      );
    }
    _state.close(
      (handle) => raw.mln_runtime_destroy(handle.raw),
      _c.threadLastErrorMessage,
    );
    _resourceTransformState?.close();
    _resourceTransformState = null;
    _httpHeaderTransformState?.close();
    _httpHeaderTransformState = null;
    _resourceProviderRulesState?.close();
    _resourceProviderRulesState = null;
    _resourceProviderCallbackState?.retire();
    _resourceProviderCallbackState = null;
  }
}

int _pumpTimeoutMilliseconds(Duration? timeout) {
  if (timeout == null) {
    return -1;
  }
  final milliseconds = timeout.inMilliseconds;
  return milliseconds < 0 ? 0 : milliseconds;
}

/// Releases a runtime owner isolate parked in [RuntimeHandle.pump].
///
/// [signal] runs from any isolate. Sending a wake source to another isolate
/// copies the wrapper around one shared native handle: every copy may signal,
/// and exactly one copy calls [close], after every signalling isolate has
/// finished.
///
/// A wake source outlives its runtime; signalling after the runtime closes
/// does nothing. It carries no native finalizer, because a [Finalizable] class
/// cannot cross an isolate boundary, so an unclosed wake source leaks
/// silently.
final class WakeSource {
  WakeSource._(this._handle);

  NativeWakeSource? _handle;

  /// Whether this wake source has released its native handle.
  bool get isClosed => _handle == null;

  /// Sets the runtime's wake flag and releases the parked owner isolate.
  ///
  /// A signal raised while the owner isolate is running sets the flag, so the
  /// next [RuntimeHandle.pump] returns without parking.
  void signal() {
    // Reachable from an isolate that has not yet run the ABI check.
    ensureAbiVersion();
    final handle = _handle;
    if (handle == null) {
      throwInvalidArgument('WakeSource is closed');
    }
    _check(raw.mln_wake_source_signal(handle.raw));
  }

  /// Releases the wake source.
  void close() {
    final handle = _handle;
    if (handle == null) {
      return;
    }
    _handle = null;
    // Reachable from an isolate that has not yet run the ABI check.
    ensureAbiVersion();
    raw.mln_wake_source_destroy(handle.raw);
  }
}

final class RuntimeEvent {
  RuntimeEvent._({
    required this.type,
    required this.eventType,
    required this.sourceType,
    required this.source,
    required int sourceId,
    required this.code,
    required this.payloadType,
    required this.payload,
    required this.payloadSize,
    required this.message,
  }) : _sourceId = sourceId;

  factory RuntimeEvent._fromNative(
    raw.mln_runtime_event event,
    RuntimeHandle runtime,
  ) {
    return RuntimeEvent._(
      type: event.type,
      eventType: RuntimeEventType.fromRawValue(event.type),
      sourceType: event.source_type,
      source: RuntimeEventSource._fromNative(event, runtime),
      sourceId: event.source,
      code: event.code,
      payloadType: event.payload_type,
      payload: RuntimeEventPayload._fromNative(event, runtime),
      payloadSize: event.payload_size,
      message: _copyNativeString(
        event.message.cast<Void>(),
        event.message_size,
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

  final int _sourceId;

  /// Native event code.
  final int code;

  /// Raw native payload type.
  final int payloadType;

  /// Typed event payload copied into Dart-owned values.
  final RuntimeEventPayload payload;

  /// Native payload byte size.
  final int payloadSize;

  /// Copied event message, when one was provided.
  final String? message;
}

/// Copies a raw runtime event through the production decoder for tests.
RuntimeEvent copyRuntimeEventForTesting(
  raw.mln_runtime_event event,
  RuntimeHandle runtime,
) => RuntimeEvent._fromNative(event, runtime);

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
  static const offlineOperationCompleted = RuntimeEventType._(
    22,
    'offlineOperationCompleted',
  );
  static const mapCameraTransitionFinished = RuntimeEventType._(
    23,
    'mapCameraTransitionFinished',
  );

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
    22 => offlineOperationCompleted,
    23 => mapCameraTransitionFinished,
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
sealed class RuntimeEventSource {
  const RuntimeEventSource(this.sourceType);

  factory RuntimeEventSource._fromNative(
    raw.mln_runtime_event event,
    RuntimeHandle runtime,
  ) {
    final sourceType = RuntimeEventSourceType.fromRawValue(event.source_type);
    final sourceId = event.source;
    if (sourceType == RuntimeEventSourceType.runtime) {
      return RuntimeRuntimeEventSource(runtime);
    }
    if (sourceType == RuntimeEventSourceType.map) {
      final map = runtime._maps[sourceId]?.target;
      return MapRuntimeEventSource(map);
    }
    return UnknownRuntimeEventSource(sourceType);
  }

  final RuntimeEventSourceType sourceType;
}

/// Runtime-scoped event source.
final class RuntimeRuntimeEventSource extends RuntimeEventSource {
  const RuntimeRuntimeEventSource(this.runtime)
    : super(RuntimeEventSourceType.runtime);

  final RuntimeHandle runtime;
}

/// Map-scoped event source.
final class MapRuntimeEventSource extends RuntimeEventSource {
  const MapRuntimeEventSource(this.map) : super(RuntimeEventSourceType.map);

  /// Map handle when still alive in this runtime.
  final MapHandle? map;
}

/// Unknown event source type.
final class UnknownRuntimeEventSource extends RuntimeEventSource {
  const UnknownRuntimeEventSource(super.sourceType);
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

/// Offline operation kind reported by runtime events.
final class OfflineOperationKind {
  const OfflineOperationKind._(this.rawValue, this.name);

  static const ambientCache = OfflineOperationKind._(1, 'ambientCache');
  static const regionCreate = OfflineOperationKind._(2, 'regionCreate');
  static const regionGet = OfflineOperationKind._(3, 'regionGet');
  static const regionsList = OfflineOperationKind._(4, 'regionsList');
  static const regionsMergeDatabase = OfflineOperationKind._(
    5,
    'regionsMergeDatabase',
  );
  static const regionUpdateMetadata = OfflineOperationKind._(
    6,
    'regionUpdateMetadata',
  );
  static const regionGetStatus = OfflineOperationKind._(7, 'regionGetStatus');
  static const regionSetObserved = OfflineOperationKind._(
    8,
    'regionSetObserved',
  );
  static const regionSetDownloadState = OfflineOperationKind._(
    9,
    'regionSetDownloadState',
  );
  static const regionInvalidate = OfflineOperationKind._(
    10,
    'regionInvalidate',
  );
  static const regionDelete = OfflineOperationKind._(11, 'regionDelete');
  static const setMaximumAmbientCacheSize = OfflineOperationKind._(
    12,
    'setMaximumAmbientCacheSize',
  );

  factory OfflineOperationKind.fromRawValue(int rawValue) => switch (rawValue) {
    1 => ambientCache,
    2 => regionCreate,
    3 => regionGet,
    4 => regionsList,
    5 => regionsMergeDatabase,
    6 => regionUpdateMetadata,
    7 => regionGetStatus,
    8 => regionSetObserved,
    9 => regionSetDownloadState,
    10 => regionInvalidate,
    11 => regionDelete,
    12 => setMaximumAmbientCacheSize,
    _ => OfflineOperationKind._(rawValue, 'unknown($rawValue)'),
  };

  final int rawValue;
  final String name;
}

/// Offline operation result kind reported by runtime events.
final class OfflineOperationResultKind {
  const OfflineOperationResultKind._(this.rawValue, this.name);

  static const none = OfflineOperationResultKind._(0, 'none');
  static const region = OfflineOperationResultKind._(1, 'region');
  static const optionalRegion = OfflineOperationResultKind._(
    2,
    'optionalRegion',
  );
  static const regionList = OfflineOperationResultKind._(3, 'regionList');
  static const regionStatus = OfflineOperationResultKind._(4, 'regionStatus');

  factory OfflineOperationResultKind.fromRawValue(int rawValue) =>
      switch (rawValue) {
        0 => none,
        1 => region,
        2 => optionalRegion,
        3 => regionList,
        4 => regionStatus,
        _ => OfflineOperationResultKind._(rawValue, 'unknown($rawValue)'),
      };

  final int rawValue;
  final String name;
}

/// Typed runtime event payload copied into Dart-owned values.
sealed class RuntimeEventPayload {
  const RuntimeEventPayload(this.rawPayloadType, this.payloadSize);

  factory RuntimeEventPayload._fromNative(
    raw.mln_runtime_event event,
    RuntimeHandle runtime,
  ) {
    final rawPayloadType = event.payload_type;
    final payloadSize = event.payload_size;
    if (rawPayloadType == 0) {
      return RuntimeEventPayloadNone(rawPayloadType, payloadSize);
    }
    if (event.payload == nullptr) {
      return RuntimeEventPayloadUnknown(
        rawPayloadType,
        payloadSize,
        Uint8List(0),
      );
    }
    final payload = event.payload;
    return switch (rawPayloadType) {
      1 => _runtimePayloadOrUnknown(
        event,
        sizeOf<raw.mln_runtime_event_render_frame>(),
        () {
          final value = payload.cast<raw.mln_runtime_event_render_frame>().ref;
          return RuntimeEventRenderFrame(
            rawPayloadType: rawPayloadType,
            payloadSize: payloadSize,
            mode: RenderMode.fromRawValue(value.mode),
            rawMode: value.mode,
            needsRepaint: value.needs_repaint,
            placementChanged: value.placement_changed,
            stats: RenderingStats._fromNative(value.stats),
          );
        },
      ),
      2 => _runtimePayloadOrUnknown(
        event,
        sizeOf<raw.mln_runtime_event_render_map>(),
        () {
          final value = payload.cast<raw.mln_runtime_event_render_map>().ref;
          return RuntimeEventRenderMap(
            rawPayloadType: rawPayloadType,
            payloadSize: payloadSize,
            mode: RenderMode.fromRawValue(value.mode),
            rawMode: value.mode,
          );
        },
      ),
      3 => _runtimePayloadOrUnknown(
        event,
        sizeOf<raw.mln_runtime_event_style_image_missing>(),
        () {
          final value = payload
              .cast<raw.mln_runtime_event_style_image_missing>()
              .ref;
          return RuntimeEventStyleImageMissing(
            rawPayloadType: rawPayloadType,
            payloadSize: payloadSize,
            imageId: _copyNativeString(
              value.image_id.cast<Void>(),
              value.image_id_size,
            ),
          );
        },
      ),
      4 => _runtimePayloadOrUnknown(
        event,
        sizeOf<raw.mln_runtime_event_tile_action>(),
        () {
          final value = payload.cast<raw.mln_runtime_event_tile_action>().ref;
          return RuntimeEventTileAction(
            rawPayloadType: rawPayloadType,
            payloadSize: payloadSize,
            operation: TileOperation.fromRawValue(value.operation),
            rawOperation: value.operation,
            tileId: TileId(
              overscaledZ: value.tile_id.overscaled_z,
              wrap: value.tile_id.wrap,
              canonicalZ: value.tile_id.canonical_z,
              canonicalX: value.tile_id.canonical_x,
              canonicalY: value.tile_id.canonical_y,
            ),
            sourceId: _copyNativeString(
              value.source_id.cast<Void>(),
              value.source_id_size,
            ),
          );
        },
      ),
      5 => _runtimePayloadOrUnknown(
        event,
        sizeOf<raw.mln_runtime_event_offline_region_status>(),
        () {
          final value = payload
              .cast<raw.mln_runtime_event_offline_region_status>()
              .ref;
          return RuntimeEventOfflineRegionStatus(
            rawPayloadType: rawPayloadType,
            payloadSize: payloadSize,
            regionId: value.region_id,
            status: _offlineRegionStatusFromNative(value.status),
          );
        },
      ),
      6 => _runtimePayloadOrUnknown(
        event,
        sizeOf<raw.mln_runtime_event_offline_region_response_error>(),
        () {
          final value = payload
              .cast<raw.mln_runtime_event_offline_region_response_error>()
              .ref;
          return RuntimeEventOfflineRegionResponseError(
            rawPayloadType: rawPayloadType,
            payloadSize: payloadSize,
            regionId: value.region_id,
            reason: ResourceErrorReason.fromRawValue(value.reason),
            rawReason: value.reason,
          );
        },
      ),
      7 => _runtimePayloadOrUnknown(
        event,
        sizeOf<raw.mln_runtime_event_offline_region_tile_count_limit>(),
        () {
          final value = payload
              .cast<raw.mln_runtime_event_offline_region_tile_count_limit>()
              .ref;
          return RuntimeEventOfflineRegionTileCountLimit(
            rawPayloadType: rawPayloadType,
            payloadSize: payloadSize,
            regionId: value.region_id,
            limit: uint64FromNative(value.limit),
          );
        },
      ),
      8 => _runtimePayloadOrUnknown(
        event,
        sizeOf<raw.mln_runtime_event_offline_operation_completed>(),
        () {
          final value = payload
              .cast<raw.mln_runtime_event_offline_operation_completed>()
              .ref;
          return RuntimeEventOfflineOperationCompleted(
            rawPayloadType: rawPayloadType,
            payloadSize: payloadSize,
            operation: runtime._offlineOperations[value.operation_id]?.target,
            operationKind: OfflineOperationKind.fromRawValue(
              value.operation_kind,
            ),
            rawOperationKind: value.operation_kind,
            resultKind: OfflineOperationResultKind.fromRawValue(
              value.result_kind,
            ),
            rawResultKind: value.result_kind,
            resultStatus: MaplibreStatus.fromNativeStatusCode(
              value.result_status,
            ),
            rawResultStatus: value.result_status,
            found: value.found,
          );
        },
      ),
      9 => _runtimePayloadOrUnknown(
        event,
        sizeOf<raw.mln_runtime_event_camera_transition_finished>(),
        () {
          final value = payload
              .cast<raw.mln_runtime_event_camera_transition_finished>()
              .ref;
          return RuntimeEventCameraTransitionFinished(
            rawPayloadType: rawPayloadType,
            payloadSize: payloadSize,
            transitionId: uint64FromNative(value.transition_id),
          );
        },
      ),
      _ => RuntimeEventPayloadUnknown(
        rawPayloadType,
        payloadSize,
        _copyRuntimePayloadBytes(event),
      ),
    };
  }

  final int rawPayloadType;
  final int payloadSize;
}

/// Runtime event with no payload.
final class RuntimeEventPayloadNone extends RuntimeEventPayload {
  const RuntimeEventPayloadNone(super.rawPayloadType, super.payloadSize);
}

/// Render-frame event payload.
final class RuntimeEventRenderFrame extends RuntimeEventPayload {
  const RuntimeEventRenderFrame({
    required int rawPayloadType,
    required int payloadSize,
    required this.mode,
    required this.rawMode,
    required this.needsRepaint,
    required this.placementChanged,
    required this.stats,
  }) : super(rawPayloadType, payloadSize);

  final RenderMode mode;
  final int rawMode;
  final bool needsRepaint;
  final bool placementChanged;
  final RenderingStats stats;
}

/// Render-map event payload.
final class RuntimeEventRenderMap extends RuntimeEventPayload {
  const RuntimeEventRenderMap({
    required int rawPayloadType,
    required int payloadSize,
    required this.mode,
    required this.rawMode,
  }) : super(rawPayloadType, payloadSize);

  final RenderMode mode;
  final int rawMode;
}

/// Style-image missing event payload.
final class RuntimeEventStyleImageMissing extends RuntimeEventPayload {
  const RuntimeEventStyleImageMissing({
    required int rawPayloadType,
    required int payloadSize,
    required this.imageId,
  }) : super(rawPayloadType, payloadSize);

  final String? imageId;
}

/// Tile-action event payload.
final class RuntimeEventTileAction extends RuntimeEventPayload {
  const RuntimeEventTileAction({
    required int rawPayloadType,
    required int payloadSize,
    required this.operation,
    required this.rawOperation,
    required this.tileId,
    required this.sourceId,
  }) : super(rawPayloadType, payloadSize);

  final TileOperation operation;
  final int rawOperation;
  final TileId tileId;
  final String? sourceId;
}

/// Offline-region status event payload.
final class RuntimeEventOfflineRegionStatus extends RuntimeEventPayload {
  const RuntimeEventOfflineRegionStatus({
    required int rawPayloadType,
    required int payloadSize,
    required this.regionId,
    required this.status,
  }) : super(rawPayloadType, payloadSize);

  final int regionId;
  final OfflineRegionStatus status;
}

/// Offline-region response error event payload.
final class RuntimeEventOfflineRegionResponseError extends RuntimeEventPayload {
  const RuntimeEventOfflineRegionResponseError({
    required int rawPayloadType,
    required int payloadSize,
    required this.regionId,
    required this.reason,
    required this.rawReason,
  }) : super(rawPayloadType, payloadSize);

  final int regionId;
  final ResourceErrorReason reason;
  final int rawReason;
}

/// Offline-region tile-count limit event payload.
final class RuntimeEventOfflineRegionTileCountLimit
    extends RuntimeEventPayload {
  const RuntimeEventOfflineRegionTileCountLimit({
    required int rawPayloadType,
    required int payloadSize,
    required this.regionId,
    required this.limit,
  }) : super(rawPayloadType, payloadSize);

  final int regionId;
  final BigInt limit;
}

/// Offline operation completion event payload.
final class RuntimeEventOfflineOperationCompleted extends RuntimeEventPayload {
  const RuntimeEventOfflineOperationCompleted({
    required int rawPayloadType,
    required int payloadSize,
    required this.operation,
    required this.operationKind,
    required this.rawOperationKind,
    required this.resultKind,
    required this.rawResultKind,
    required this.resultStatus,
    required this.rawResultStatus,
    required this.found,
  }) : super(rawPayloadType, payloadSize);

  /// Matching live operation, or null when it is no longer tracked.
  final OfflineOperationHandle? operation;
  final OfflineOperationKind operationKind;
  final int rawOperationKind;
  final OfflineOperationResultKind resultKind;
  final int rawResultKind;
  final MaplibreStatus resultStatus;
  final int rawResultStatus;
  final bool found;
}

/// Camera-transition-finished event payload.
final class RuntimeEventCameraTransitionFinished extends RuntimeEventPayload {
  const RuntimeEventCameraTransitionFinished({
    required int rawPayloadType,
    required int payloadSize,
    required this.transitionId,
  }) : super(rawPayloadType, payloadSize);

  /// Caller-chosen transition identity across the full `uint64_t` domain.
  final BigInt transitionId;
}

/// Unknown runtime event payload copied as raw bytes.
final class RuntimeEventPayloadUnknown extends RuntimeEventPayload {
  RuntimeEventPayloadUnknown(
    super.rawPayloadType,
    super.payloadSize,
    Uint8List bytes,
  ) : bytes = Uint8List.fromList(bytes).asUnmodifiableView();

  final Uint8List bytes;
}

RuntimeEventPayload _runtimePayloadOrUnknown(
  raw.mln_runtime_event event,
  int expectedPayloadSize,
  RuntimeEventPayload Function() copy,
) {
  if (event.payload_size < expectedPayloadSize) {
    return RuntimeEventPayloadUnknown(
      event.payload_type,
      event.payload_size,
      _copyRuntimePayloadBytes(event),
    );
  }
  return copy();
}

Uint8List _copyRuntimePayloadBytes(raw.mln_runtime_event event) {
  if (event.payload == nullptr || event.payload_size == 0) {
    return Uint8List(0);
  }
  return Uint8List.fromList(
    event.payload.cast<Uint8>().asTypedList(event.payload_size),
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
  });

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

  @override
  bool operator ==(Object other) =>
      other is MapOptions &&
      other.width == width &&
      other.height == height &&
      other.scaleFactor == scaleFactor &&
      other.mapMode == mapMode &&
      other.fastPforEnabled == fastPforEnabled;

  @override
  int get hashCode =>
      Object.hash(width, height, scaleFactor, mapMode, fastPforEnabled);
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

  /// Map pixel ratio, fixed for the map lifetime.
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

/// Owner-thread map handle bound to a retained runtime.
final class MapHandle {
  MapHandle._(this._runtime, NativeMap handle)
    : _state = NativeHandleState(handle, 'MapHandle');

  /// Creates a map owned by [runtime].
  factory MapHandle.create(
    RuntimeHandle runtime, {
    MapOptions options = const MapOptions(),
  }) {
    return withNativeArena((arena) {
      final nativeOptions = arena<raw.mln_map_options>();
      nativeOptions.ref = raw.mln_map_options_default();
      nativeOptions.ref.width = _positiveUint32(options.width, 'map width');
      nativeOptions.ref.height = _positiveUint32(options.height, 'map height');
      nativeOptions.ref.scale_factor = options.scaleFactor;
      nativeOptions.ref.map_mode = options.mapMode.rawValue;
      nativeOptions.ref.fast_pfor_enabled = options.fastPforEnabled;
      final outMap = arena<Uint64>();
      outMap.value = 0;

      _check(raw.mln_map_create(runtime._handle.raw, nativeOptions, outMap));
      final map = MapHandle._(runtime, NativeMap(outMap.value));
      runtime._registerMap(map);
      return map;
    });
  }

  final RuntimeHandle _runtime;
  final NativeHandleState<NativeMap> _state;
  final _customGeometryCallbacks = <String, _CustomGeometryCallbackState>{};
  final _pendingUrlStyleCallbacks = <Set<_CustomGeometryCallbackState>>[];

  /// Whether this map has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  NativeMap get _handle {
    // Touching the runtime's handle keeps its owner-isolate check on this path.
    final _ = _runtime._handle;
    return _state.handle;
  }

  /// Loads a style URL through MapLibre Native style APIs.
  void setStyleUrl(String url) {
    withNativeArena((arena) {
      final nativeUrl = nativeUtf8CString(url, arena);
      _check(
        raw.mln_map_set_style_url(_handle.raw, nativeUrl.pointer.cast<Char>()),
      );
    });
    _pendingUrlStyleCallbacks.add(_customGeometryCallbacks.values.toSet());
  }

  /// Loads inline style JSON through MapLibre Native style APIs.
  void setStyleJson(Uint8List json) {
    withNativeArena((arena) {
      final nativeJson = nativeBufferView(json, arena);
      _check(raw.mln_map_set_style_json(_handle.raw, nativeJson));
    });
    _clearCustomGeometryCallbacks();
  }

  /// Returns the style document this map's style was last parsed from.
  ///
  /// This is the loaded document, not a serialization of the live style:
  /// runtime mutations do not change it, and a failed parse leaves the
  /// previously parsed document in place. The result is empty when no document
  /// has been parsed.
  Uint8List getLoadedStyleJson() {
    return _copyMapData(_handle, raw.mln_map_copy_loaded_style_json);
  }

  /// Returns the URL this map's style was last requested from.
  ///
  /// [setStyleUrl] records the URL when the request is made, before the
  /// response arrives, and [setStyleJson] clears it, so this can disagree with
  /// [getLoadedStyleJson] while a load is in flight or after one fails. The
  /// result is empty when no URL bytes are available.
  String getStyleUrl() {
    return _copyMapText(_handle, raw.mln_map_copy_style_url);
  }

  /// Requests a repaint for a continuous map.
  void requestRepaint() {
    _check(raw.mln_map_request_repaint(_handle.raw));
  }

  /// Requests one still image for a static or tile map.
  void requestStillImage() {
    _check(raw.mln_map_request_still_image(_handle.raw));
  }

  /// Applies MapLibre debug overlay options.
  void setDebugOptions(MapDebugOptions options) {
    _check(raw.mln_map_set_debug_options(_handle.raw, options.bits));
  }

  /// Copies current MapLibre debug overlay options.
  MapDebugOptions debugOptions() {
    return withNativeArena((arena) {
      final outOptions = arena<Uint32>();
      _check(raw.mln_map_get_debug_options(_handle.raw, outOptions));
      return MapDebugOptions(outOptions.value);
    });
  }

  /// Dumps map debug logs through MapLibre Native logging.
  void dumpDebugLogs() {
    _check(raw.mln_map_dump_debug_logs(_handle.raw));
  }

  /// A reference to this map for attaching a render session, safe to send to
  /// another isolate.
  ///
  /// A render session belongs to the isolate that attached it, which need not
  /// be the map's; every other map call stays on the map's own isolate. The
  /// reference does not keep the map alive, and attaching after the map closes
  /// reports invalid argument.
  MapAttachRef attachRef() => MapAttachRef._(_handle.raw);

  /// Copies the current camera snapshot.
  CameraOptions camera() {
    return withNativeArena((arena) {
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      _check(raw.mln_map_get_camera(_handle.raw, outCamera));
      return native_struct.cameraOptionsFromNative(outCamera.ref);
    });
  }

  /// Applies a camera jump command.
  void jumpTo(CameraOptions camera) {
    withNativeArena((arena) {
      final nativeCamera = _nativeCamera(camera, arena);
      _check(raw.mln_map_jump_to(_handle.raw, nativeCamera));
    });
  }

  /// Applies a camera ease transition command.
  void easeTo(CameraOptions camera, {AnimationOptions? animation}) {
    withNativeArena((arena) {
      final nativeCamera = _nativeCamera(camera, arena);
      final nativeAnimation = _nativeAnimation(animation, arena);
      _check(raw.mln_map_ease_to(_handle.raw, nativeCamera, nativeAnimation));
    });
  }

  /// Applies a camera fly transition command.
  void flyTo(CameraOptions camera, {AnimationOptions? animation}) {
    withNativeArena((arena) {
      final nativeCamera = _nativeCamera(camera, arena);
      final nativeAnimation = _nativeAnimation(animation, arena);
      _check(raw.mln_map_fly_to(_handle.raw, nativeCamera, nativeAnimation));
    });
  }

  /// Applies a screen-space pan command.
  void moveBy(double deltaX, double deltaY, {AnimationOptions? animation}) {
    withNativeArena((arena) {
      final nativeAnimation = _nativeAnimation(animation, arena);
      _check(
        animation == null
            ? raw.mln_map_move_by(_handle.raw, deltaX, deltaY)
            : raw.mln_map_move_by_animated(
                _handle.raw,
                deltaX,
                deltaY,
                nativeAnimation,
              ),
      );
    });
  }

  /// Applies a screen-space zoom command.
  void scaleBy(
    double scale, {
    ScreenPoint? anchor,
    AnimationOptions? animation,
  }) {
    withNativeArena((arena) {
      final nativeAnchor = _nativeScreenPoint(anchor, arena);
      final nativeAnimation = _nativeAnimation(animation, arena);
      _check(
        animation == null
            ? raw.mln_map_scale_by(_handle.raw, scale, nativeAnchor)
            : raw.mln_map_scale_by_animated(
                _handle.raw,
                scale,
                nativeAnchor,
                nativeAnimation,
              ),
      );
    });
  }

  /// Applies a screen-space rotate command.
  void rotateBy(
    ScreenPoint first,
    ScreenPoint second, {
    AnimationOptions? animation,
  }) {
    withNativeArena((arena) {
      final nativeFirst = native_struct.screenPointToNative(first);
      final nativeSecond = native_struct.screenPointToNative(second);
      final nativeAnimation = _nativeAnimation(animation, arena);
      _check(
        animation == null
            ? raw.mln_map_rotate_by(_handle.raw, nativeFirst, nativeSecond)
            : raw.mln_map_rotate_by_animated(
                _handle.raw,
                nativeFirst,
                nativeSecond,
                nativeAnimation,
              ),
      );
    });
  }

  /// Applies a pitch delta command.
  void pitchBy(double pitch, {AnimationOptions? animation}) {
    withNativeArena((arena) {
      final nativeAnimation = _nativeAnimation(animation, arena);
      _check(
        animation == null
            ? raw.mln_map_pitch_by(_handle.raw, pitch)
            : raw.mln_map_pitch_by_animated(
                _handle.raw,
                pitch,
                nativeAnimation,
              ),
      );
    });
  }

  /// Cancels active camera transitions.
  void cancelTransitions() {
    _check(raw.mln_map_cancel_transitions(_handle.raw));
  }

  /// Marks whether a host-driven gesture is in progress.
  ///
  /// The flag stays set until the host clears it, so pair every `true` with a
  /// `false`.
  void setGestureInProgress(bool inProgress) {
    _check(raw.mln_map_set_gesture_in_progress(_handle.raw, inProgress));
  }

  /// Copies whether a host-driven gesture is currently in progress.
  bool isGestureInProgress() {
    return withNativeArena((arena) {
      final outInProgress = arena<Bool>();
      _check(raw.mln_map_is_gesture_in_progress(_handle.raw, outInProgress));
      return outInProgress.value;
    });
  }

  /// Enables or disables the rendering stats overlay.
  void setRenderingStatsViewEnabled(bool enabled) {
    _check(raw.mln_map_set_rendering_stats_view_enabled(_handle.raw, enabled));
  }

  /// Copies whether the rendering stats overlay is enabled.
  bool renderingStatsViewEnabled() {
    return withNativeArena((arena) {
      final outEnabled = arena<Bool>();
      _check(
        raw.mln_map_get_rendering_stats_view_enabled(_handle.raw, outEnabled),
      );
      return outEnabled.value;
    });
  }

  /// Copies whether MapLibre currently considers the map fully loaded.
  bool isFullyLoaded() {
    return withNativeArena((arena) {
      final outLoaded = arena<Bool>();
      _check(raw.mln_map_is_fully_loaded(_handle.raw, outLoaded));
      return outLoaded.value;
    });
  }

  /// Copies the current logical viewport size and map pixel ratio.
  MapSize size() {
    return withNativeArena((arena) {
      final outWidth = arena<Uint32>();
      final outHeight = arena<Uint32>();
      final outScaleFactor = arena<Double>();
      _check(
        raw.mln_map_get_size(_handle.raw, outWidth, outHeight, outScaleFactor),
      );
      return MapSize(
        width: outWidth.value,
        height: outHeight.value,
        scaleFactor: outScaleFactor.value,
      );
    });
  }

  /// Copies live map viewport and render-transform controls.
  MapViewportOptions viewportOptions() {
    return withNativeArena((arena) {
      final outOptions = arena<raw.mln_map_viewport_options>();
      outOptions.ref.size = sizeOf<raw.mln_map_viewport_options>();
      _check(raw.mln_map_get_viewport_options(_handle.raw, outOptions));
      return native_struct.mapViewportOptionsFromNative(outOptions.ref);
    });
  }

  /// Applies selected live map viewport and render-transform controls.
  void setViewportOptions(MapViewportOptions options) {
    withNativeArena((arena) {
      final nativeOptions = arena<raw.mln_map_viewport_options>();
      nativeOptions.ref = native_struct.mapViewportOptionsToNative(
        options,
        raw.mln_map_viewport_options_default(),
      );
      _check(raw.mln_map_set_viewport_options(_handle.raw, nativeOptions));
    });
  }

  /// Copies tile prefetch and LOD tuning controls.
  MapTileOptions tileOptions() {
    return withNativeArena((arena) {
      final outOptions = arena<raw.mln_map_tile_options>();
      outOptions.ref.size = sizeOf<raw.mln_map_tile_options>();
      _check(raw.mln_map_get_tile_options(_handle.raw, outOptions));
      return native_struct.mapTileOptionsFromNative(outOptions.ref);
    });
  }

  /// Applies selected tile prefetch and LOD tuning controls.
  void setTileOptions(MapTileOptions options) {
    withNativeArena((arena) {
      final nativeOptions = arena<raw.mln_map_tile_options>();
      nativeOptions.ref = native_struct.mapTileOptionsToNative(
        options,
        raw.mln_map_tile_options_default(),
      );
      _check(raw.mln_map_set_tile_options(_handle.raw, nativeOptions));
    });
  }

  /// Copies map camera constraint options.
  BoundOptions bounds() {
    return withNativeArena((arena) {
      final outOptions = arena<raw.mln_bound_options>();
      outOptions.ref.size = sizeOf<raw.mln_bound_options>();
      _check(raw.mln_map_get_bounds(_handle.raw, outOptions));
      return native_struct.boundOptionsFromNative(outOptions.ref);
    });
  }

  /// Applies selected map camera constraint options.
  void setBounds(BoundOptions options) {
    withNativeArena((arena) {
      final nativeOptions = arena<raw.mln_bound_options>();
      nativeOptions.ref = native_struct.boundOptionsToNative(
        options,
        raw.mln_bound_options_default(),
      );
      _check(raw.mln_map_set_bounds(_handle.raw, nativeOptions));
    });
  }

  /// Copies the current free camera position and orientation.
  FreeCameraOptions freeCameraOptions() {
    return withNativeArena((arena) {
      final outOptions = arena<raw.mln_free_camera_options>();
      outOptions.ref.size = sizeOf<raw.mln_free_camera_options>();
      _check(raw.mln_map_get_free_camera_options(_handle.raw, outOptions));
      return native_struct.freeCameraOptionsFromNative(outOptions.ref);
    });
  }

  /// Applies selected free camera position and orientation fields.
  void setFreeCameraOptions(FreeCameraOptions options) {
    withNativeArena((arena) {
      final nativeOptions = arena<raw.mln_free_camera_options>();
      nativeOptions.ref = native_struct.freeCameraOptionsToNative(
        options,
        raw.mln_free_camera_options_default(),
      );
      _check(raw.mln_map_set_free_camera_options(_handle.raw, nativeOptions));
    });
  }

  /// Copies the current axonometric rendering options.
  ProjectionModeOptions projectionMode() {
    return withNativeArena((arena) {
      final outMode = arena<raw.mln_projection_mode>();
      outMode.ref.size = sizeOf<raw.mln_projection_mode>();
      _check(raw.mln_map_get_projection_mode(_handle.raw, outMode));
      return native_struct.projectionModeOptionsFromNative(outMode.ref);
    });
  }

  /// Applies selected axonometric rendering option fields.
  void setProjectionMode(ProjectionModeOptions mode) {
    withNativeArena((arena) {
      final nativeMode = arena<raw.mln_projection_mode>();
      nativeMode.ref = native_struct.projectionModeOptionsToNative(
        mode,
        raw.mln_projection_mode_default(),
      );
      _check(raw.mln_map_set_projection_mode(_handle.raw, nativeMode));
    });
  }

  /// Computes a camera that fits geographic bounds in the current viewport.
  CameraOptions cameraForLatLngBounds(
    LatLngBounds bounds, {
    CameraFitOptions fitOptions = const CameraFitOptions(),
  }) {
    return withNativeArena((arena) {
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      final nativeFitOptions = arena<raw.mln_camera_fit_options>();
      nativeFitOptions.ref = native_struct.cameraFitOptionsToNative(
        fitOptions,
        raw.mln_camera_fit_options_default(),
      );
      _check(
        raw.mln_map_camera_for_lat_lng_bounds(
          _handle.raw,
          native_struct.latLngBoundsToNative(bounds),
          nativeFitOptions,
          outCamera,
        ),
      );
      return native_struct.cameraOptionsFromNative(outCamera.ref);
    });
  }

  /// Computes a camera that fits geographic coordinates in the current viewport.
  CameraOptions cameraForLatLngs(
    List<LatLng> coordinates, {
    CameraFitOptions fitOptions = const CameraFitOptions(),
  }) {
    return withNativeArena((arena) {
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      final nativeFitOptions = arena<raw.mln_camera_fit_options>();
      nativeFitOptions.ref = native_struct.cameraFitOptionsToNative(
        fitOptions,
        raw.mln_camera_fit_options_default(),
      );
      _check(
        raw.mln_map_camera_for_lat_lngs(
          _handle.raw,
          _latLngArray(coordinates, arena),
          coordinates.length,
          nativeFitOptions,
          outCamera,
        ),
      );
      return native_struct.cameraOptionsFromNative(outCamera.ref);
    });
  }

  /// Computes a camera that fits a geometry in the current viewport.
  CameraOptions cameraForGeometry(
    Uint8List geometry, {
    CameraFitOptions fitOptions = const CameraFitOptions(),
  }) {
    return withNativeArena((arena) {
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      final nativeFitOptions = arena<raw.mln_camera_fit_options>();
      nativeFitOptions.ref = native_struct.cameraFitOptionsToNative(
        fitOptions,
        raw.mln_camera_fit_options_default(),
      );
      final nativeGeometry = nativeBufferView(geometry, arena);
      _check(
        raw.mln_map_camera_for_geometry(
          _handle.raw,
          nativeGeometry,
          nativeFitOptions,
          outCamera,
        ),
      );
      return native_struct.cameraOptionsFromNative(outCamera.ref);
    });
  }

  /// Computes wrapped geographic bounds for a camera.
  LatLngBounds latLngBoundsForCamera(CameraOptions camera) =>
      _latLngBoundsForCamera(camera, unwrapped: false);

  /// Computes unwrapped geographic bounds for a camera.
  LatLngBounds latLngBoundsForCameraUnwrapped(CameraOptions camera) =>
      _latLngBoundsForCamera(camera, unwrapped: true);

  LatLngBounds _latLngBoundsForCamera(
    CameraOptions camera, {
    required bool unwrapped,
  }) {
    return withNativeArena((arena) {
      final nativeCamera = _nativeCamera(camera, arena);
      final outBounds = arena<raw.mln_lat_lng_bounds>();
      _check(
        unwrapped
            ? raw.mln_map_lat_lng_bounds_for_camera_unwrapped(
                _handle.raw,
                nativeCamera,
                outBounds,
              )
            : raw.mln_map_lat_lng_bounds_for_camera(
                _handle.raw,
                nativeCamera,
                outBounds,
              ),
      );
      return native_struct.latLngBoundsFromNative(outBounds.ref);
    });
  }

  /// Converts a geographic world coordinate to a screen point.
  ScreenPoint pixelForLatLng(LatLng coordinate) {
    return withNativeArena((arena) {
      final outPoint = arena<raw.mln_screen_point>();
      _check(
        raw.mln_map_pixel_for_lat_lng(
          _handle.raw,
          native_struct.latLngToNative(coordinate),
          outPoint,
        ),
      );
      return native_struct.screenPointFromNative(outPoint.ref);
    });
  }

  /// Converts a screen point to a geographic world coordinate.
  LatLng latLngForPixel(ScreenPoint point) {
    return withNativeArena((arena) {
      final outCoordinate = arena<raw.mln_lat_lng>();
      _check(
        raw.mln_map_lat_lng_for_pixel(
          _handle.raw,
          native_struct.screenPointToNative(point),
          outCoordinate,
        ),
      );
      return native_struct.latLngFromNative(outCoordinate.ref);
    });
  }

  /// Converts geographic coordinates to screen points.
  List<ScreenPoint> pixelsForLatLngs(List<LatLng> coordinates) {
    return withNativeArena((arena) {
      final outPoints = coordinates.isEmpty
          ? nullptr.cast<raw.mln_screen_point>()
          : arena<raw.mln_screen_point>(coordinates.length);
      _check(
        raw.mln_map_pixels_for_lat_lngs(
          _handle.raw,
          _latLngArray(coordinates, arena),
          coordinates.length,
          outPoints,
        ),
      );
      return [
        for (var index = 0; index < coordinates.length; index += 1)
          native_struct.screenPointFromNative(outPoints[index]),
      ];
    });
  }

  /// Converts screen points to geographic coordinates.
  List<LatLng> latLngsForPixels(List<ScreenPoint> points) {
    return withNativeArena((arena) {
      final nativePoints = points.isEmpty
          ? nullptr.cast<raw.mln_screen_point>()
          : arena<raw.mln_screen_point>(points.length);
      for (var index = 0; index < points.length; index += 1) {
        nativePoints[index] = native_struct.screenPointToNative(points[index]);
      }
      final outCoordinates = points.isEmpty
          ? nullptr.cast<raw.mln_lat_lng>()
          : arena<raw.mln_lat_lng>(points.length);
      _check(
        raw.mln_map_lat_lngs_for_pixels(
          _handle.raw,
          nativePoints,
          points.length,
          outCoordinates,
        ),
      );
      return [
        for (var index = 0; index < points.length; index += 1)
          native_struct.latLngFromNative(outCoordinates[index]),
      ];
    });
  }

  /// Creates a standalone projection helper from the current map transform.
  MapProjectionHandle createProjection() {
    return withNativeArena((arena) {
      final outProjection = arena<Uint64>();
      outProjection.value = 0;
      _check(raw.mln_map_projection_create(_handle.raw, outProjection));
      return MapProjectionHandle._(NativeMapProjection(outProjection.value));
    });
  }

  /// Sets or replaces one runtime style image.
  void setStyleImage(
    String imageId,
    PremultipliedRgba8Image image, {
    StyleImageOptions? options,
  }) {
    final resolvedOptions = options ?? StyleImageOptions();
    withNativeArena((arena) {
      final nativeId = nativeStringView(imageId, arena);
      final nativeImage = arena<raw.mln_premultiplied_rgba8_image>();
      nativeImage.ref = _premultipliedRgba8ImageToNative(image, arena);
      final nativeOptions = arena<raw.mln_style_image_options>();
      nativeOptions.ref = _styleImageOptionsToNative(resolvedOptions, arena);
      _check(
        raw.mln_map_set_style_image(
          _handle.raw,
          nativeId.value,
          nativeImage,
          nativeOptions,
        ),
      );
    });
  }

  /// Copies one runtime style image's stretchable intervals, or null when no
  /// image carries [imageId]. The record holds horizontal intervals first.
  ({List<ImageStretch> stretchX, List<ImageStretch> stretchY})?
  getStyleImageStretches(String imageId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(imageId, arena);
      final outXCount = arena<Size>();
      final outYCount = arena<Size>();
      final outFound = arena<Bool>();
      _check(
        raw.mln_map_copy_style_image_stretches(
          _handle.raw,
          nativeId.value,
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
        raw.mln_map_copy_style_image_stretches(
          _handle.raw,
          nativeId.value,
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
      return (stretchX: read(rawX, xCount), stretchY: read(rawY, yCount));
    });
  }

  /// Removes one runtime style image and returns whether one was removed.
  bool removeStyleImage(String imageId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(imageId, arena);
      final outRemoved = arena<Bool>();
      _check(
        raw.mln_map_remove_style_image(_handle.raw, nativeId.value, outRemoved),
      );
      return outRemoved.value;
    });
  }

  /// Reports whether one runtime style image exists.
  bool styleImageExists(String imageId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(imageId, arena);
      final outExists = arena<Bool>();
      _check(
        raw.mln_map_style_image_exists(_handle.raw, nativeId.value, outExists),
      );
      return outExists.value;
    });
  }

  /// Copies fixed metadata for one runtime style image.
  StyleImageInfo? getStyleImageInfo(String imageId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(imageId, arena);
      final outInfo = arena<raw.mln_style_image_info>();
      outInfo.ref = raw.mln_style_image_info_default();
      final outFound = arena<Bool>();
      _check(
        raw.mln_map_get_style_image_info(
          _handle.raw,
          nativeId.value,
          outInfo,
          outFound,
        ),
      );
      return outFound.value ? _styleImageInfoFromNative(outInfo.ref) : null;
    });
  }

  /// Copies one runtime style image as premultiplied RGBA8 pixels.
  StyleImage? copyStyleImagePremultipliedRgba8(String imageId) {
    final info = getStyleImageInfo(imageId);
    if (info == null) {
      return null;
    }
    return withNativeArena((arena) {
      final nativeId = nativeStringView(imageId, arena);
      final pixels = info.byteLength == 0
          ? nullptr.cast<Uint8>()
          : arena<Uint8>(info.byteLength);
      final outByteLength = arena<Size>();
      final outFound = arena<Bool>();
      _check(
        raw.mln_map_copy_style_image_premultiplied_rgba8(
          _handle.raw,
          nativeId.value,
          pixels,
          info.byteLength,
          outByteLength,
          outFound,
        ),
      );
      if (!outFound.value) {
        return null;
      }
      return StyleImage(
        info: info,
        bytes: Uint8List.fromList(pixels.asTypedList(outByteLength.value)),
      );
    });
  }

  /// Adds one style source from a style-spec source JSON object.
  void addStyleSourceJson(String sourceId, Uint8List sourceJson) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeSourceJson = nativeBufferView(sourceJson, arena);
      _check(
        raw.mln_map_add_style_source_json(
          _handle.raw,
          nativeId.value,
          nativeSourceJson,
        ),
      );
    });
  }

  /// Adds a GeoJSON source that loads from [url].
  void addGeoJsonSourceUrl(
    String sourceId,
    String url, {
    GeoJsonSourceOptions? options,
  }) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      final nativeOptions = _nativeGeoJsonSourceOptions(
        options ?? GeoJsonSourceOptions(),
        arena,
      );
      _check(
        raw.mln_map_add_geojson_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          nativeOptions,
        ),
      );
    });
  }

  /// Adds a GeoJSON source with inline data.
  void addGeoJsonSourceData(
    String sourceId,
    Uint8List data, {
    GeoJsonSourceOptions? options,
  }) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeData = nativeBufferView(data, arena);
      final nativeOptions = _nativeGeoJsonSourceOptions(
        options ?? GeoJsonSourceOptions(),
        arena,
      );
      _check(
        raw.mln_map_add_geojson_source_data(
          _handle.raw,
          nativeId.value,
          nativeData,
          nativeOptions,
        ),
      );
    });
  }

  /// Updates one GeoJSON source to load from [url].
  void setGeoJsonSourceUrl(String sourceId, String url) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_set_geojson_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
        ),
      );
    });
  }

  /// Updates one GeoJSON source with inline data.
  void setGeoJsonSourceData(String sourceId, Uint8List data) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeData = nativeBufferView(data, arena);
      _check(
        raw.mln_map_set_geojson_source_data(
          _handle.raw,
          nativeId.value,
          nativeData,
        ),
      );
    });
  }

  /// Adds a vector source with a TileJSON URL.
  void addVectorSourceUrl(
    String sourceId,
    String url, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_add_vector_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          _nativeTileSourceOptions(options, arena),
        ),
      );
    });
  }

  /// Adds a vector source with inline tile URL templates.
  void addVectorSourceTiles(
    String sourceId,
    List<String> tiles, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_add_vector_source_tiles(
          _handle.raw,
          nativeId.value,
          _stringViewArray(tiles, arena),
          tiles.length,
          _nativeTileSourceOptions(options, arena),
        ),
      );
    });
  }

  /// Adds a raster source with a TileJSON URL.
  void addRasterSourceUrl(
    String sourceId,
    String url, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_add_raster_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          _nativeTileSourceOptions(options, arena),
        ),
      );
    });
  }

  /// Adds a raster source with inline tile URL templates.
  void addRasterSourceTiles(
    String sourceId,
    List<String> tiles, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_add_raster_source_tiles(
          _handle.raw,
          nativeId.value,
          _stringViewArray(tiles, arena),
          tiles.length,
          _nativeTileSourceOptions(options, arena),
        ),
      );
    });
  }

  /// Adds a raster DEM source with a TileJSON URL.
  void addRasterDemSourceUrl(
    String sourceId,
    String url, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_add_raster_dem_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          _nativeTileSourceOptions(options, arena),
        ),
      );
    });
  }

  /// Adds a raster DEM source with inline tile URL templates.
  void addRasterDemSourceTiles(
    String sourceId,
    List<String> tiles, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_add_raster_dem_source_tiles(
          _handle.raw,
          nativeId.value,
          _stringViewArray(tiles, arena),
          tiles.length,
          _nativeTileSourceOptions(options, arena),
        ),
      );
    });
  }

  /// Adds an image source that loads its image from [url].
  void addImageSourceUrl(
    String sourceId,
    List<LatLng> coordinates,
    String url,
  ) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_add_image_source_url(
          _handle.raw,
          nativeId.value,
          _latLngArray(coordinates, arena),
          coordinates.length,
          nativeUrl.value,
        ),
      );
    });
  }

  /// Adds an image source with inline image pixels.
  void addImageSourceImage(
    String sourceId,
    List<LatLng> coordinates,
    PremultipliedRgba8Image image,
  ) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeImage = arena<raw.mln_premultiplied_rgba8_image>();
      nativeImage.ref = _premultipliedRgba8ImageToNative(image, arena);
      _check(
        raw.mln_map_add_image_source_image(
          _handle.raw,
          nativeId.value,
          _latLngArray(coordinates, arena),
          coordinates.length,
          nativeImage,
        ),
      );
    });
  }

  /// Updates an image source to load from [url].
  void setImageSourceUrl(String sourceId, String url) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_set_image_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
        ),
      );
    });
  }

  /// Updates an image source with inline image pixels.
  void setImageSourceImage(String sourceId, PremultipliedRgba8Image image) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeImage = arena<raw.mln_premultiplied_rgba8_image>();
      nativeImage.ref = _premultipliedRgba8ImageToNative(image, arena);
      _check(
        raw.mln_map_set_image_source_image(
          _handle.raw,
          nativeId.value,
          nativeImage,
        ),
      );
    });
  }

  /// Updates image source coordinates.
  void setImageSourceCoordinates(String sourceId, List<LatLng> coordinates) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_set_image_source_coordinates(
          _handle.raw,
          nativeId.value,
          _latLngArray(coordinates, arena),
          coordinates.length,
        ),
      );
    });
  }

  /// Copies image source coordinates, or null when the source is missing.
  List<LatLng>? getImageSourceCoordinates(String sourceId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outCoordinates = arena<raw.mln_lat_lng>(4);
      final outCount = arena<Size>();
      final outFound = arena<Bool>();
      _check(
        raw.mln_map_get_image_source_coordinates(
          _handle.raw,
          nativeId.value,
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
    });
  }

  /// Adds a custom geometry source with queued fetch/cancel notifications.
  void addCustomGeometrySource(
    String sourceId,
    CustomGeometrySourceOptions options,
  ) {
    final callbackState = _CustomGeometryCallbackState(options);
    try {
      withNativeArena((arena) {
        final nativeId = nativeStringView(sourceId, arena);
        final nativeOptions = arena<raw.mln_custom_geometry_source_options>();
        nativeOptions.ref = _customGeometrySourceOptionsToNative(
          options,
          callbackState,
        );
        _check(
          raw.mln_map_add_custom_geometry_source(
            _handle.raw,
            nativeId.value,
            nativeOptions,
          ),
        );
      });
      _customGeometryCallbacks.remove(sourceId)?.retire();
      _customGeometryCallbacks[sourceId] = callbackState;
    } catch (_) {
      callbackState.close();
      rethrow;
    }
  }

  /// Sets custom geometry source data for one canonical tile.
  void setCustomGeometrySourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Uint8List data,
  ) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeData = nativeBufferView(data, arena);
      _check(
        raw.mln_map_set_custom_geometry_source_tile_data(
          _handle.raw,
          nativeId.value,
          _canonicalTileIdToNative(tileId),
          nativeData,
        ),
      );
    });
  }

  /// Invalidates custom geometry source data for one canonical tile.
  void invalidateCustomGeometrySourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_invalidate_custom_geometry_source_tile(
          _handle.raw,
          nativeId.value,
          _canonicalTileIdToNative(tileId),
        ),
      );
    });
  }

  /// Invalidates custom geometry source data inside one geographic region.
  void invalidateCustomGeometrySourceRegion(
    String sourceId,
    LatLngBounds bounds,
  ) {
    withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_invalidate_custom_geometry_source_region(
          _handle.raw,
          nativeId.value,
          native_struct.latLngBoundsToNative(bounds),
        ),
      );
    });
  }

  /// Reports whether a style source ID exists.
  bool styleSourceExists(String sourceId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outExists = arena<Bool>();
      _check(
        raw.mln_map_style_source_exists(_handle.raw, nativeId.value, outExists),
      );
      return outExists.value;
    });
  }

  /// Removes one style source by ID and returns whether one was removed.
  bool removeStyleSource(String sourceId) {
    final removed = withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outRemoved = arena<Bool>();
      _check(
        raw.mln_map_remove_style_source(
          _handle.raw,
          nativeId.value,
          outRemoved,
        ),
      );
      return outRemoved.value;
    });
    if (removed) {
      _customGeometryCallbacks.remove(sourceId)?.retire();
    }
    return removed;
  }

  /// Copies fixed style source metadata, or returns null when the source is absent.
  SourceInfo? getStyleSourceInfo(String sourceId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      final outInfo = arena<raw.mln_style_source_info>();
      outInfo.ref.size = sizeOf<raw.mln_style_source_info>();
      final outFound = arena<Bool>();
      _check(
        raw.mln_map_get_style_source_info(
          _handle.raw,
          nativeId.value,
          outInfo,
          outFound,
        ),
      );
      if (!outFound.value) {
        return null;
      }
      final info = outInfo.ref;
      final hasUrl =
          info.fields &
              raw.mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_URL.value !=
          0;
      final hasTileJson =
          info.fields &
              raw
                  .mln_style_source_info_field
                  .MLN_STYLE_SOURCE_INFO_TILEJSON
                  .value !=
          0;
      final hasBounds =
          info.fields &
              raw
                  .mln_style_source_info_field
                  .MLN_STYLE_SOURCE_INFO_BOUNDS
                  .value !=
          0;
      final hasTileSize =
          info.fields &
              raw
                  .mln_style_source_info_field
                  .MLN_STYLE_SOURCE_INFO_TILE_SIZE
                  .value !=
          0;
      final hasVectorEncoding =
          info.fields &
              raw
                  .mln_style_source_info_field
                  .MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING
                  .value !=
          0;
      final hasRasterEncoding =
          info.fields &
              raw
                  .mln_style_source_info_field
                  .MLN_STYLE_SOURCE_INFO_RASTER_ENCODING
                  .value !=
          0;
      List<String>? tileUrls;
      if (hasTileJson) {
        final outTileUrls = arena<Uint64>();
        outTileUrls.value = 0;
        final outTileUrlsFound = arena<Bool>();
        _check(
          raw.mln_map_get_style_source_tile_urls(
            _handle.raw,
            nativeId.value,
            outTileUrls,
            outTileUrlsFound,
          ),
        );
        if (!outTileUrlsFound.value) {
          return null;
        }
        tileUrls = _copyStyleStringList(
          NativeStyleStringList(outTileUrls.value),
        );
      }
      return SourceInfo(
        type: SourceType.fromRaw(info.type),
        id: sourceId,
        isVolatile: info.is_volatile,
        attribution: _copyStyleSourceAttribution(
          _handle,
          nativeId.value,
          info.has_attribution,
          info.attribution_size,
          arena,
        ),
        url: _copyStyleSourceUrl(
          _handle,
          nativeId.value,
          hasUrl,
          info.url_size,
          arena,
        ),
        tileJson: hasTileJson
            ? ParsedTileJson(
                tileUrls: tileUrls!,
                minZoom: info.min_zoom,
                maxZoom: info.max_zoom,
                scheme: TileScheme.fromRaw(info.scheme),
                bounds: hasBounds
                    ? native_struct.latLngBoundsFromNative(info.bounds)
                    : null,
              )
            : null,
        tileSize: hasTileSize ? info.tile_size : null,
        vectorEncoding: hasVectorEncoding
            ? VectorTileEncoding.fromRaw(info.vector_encoding)
            : null,
        rasterDemEncoding: hasRasterEncoding
            ? RasterDemEncoding.fromRaw(info.raster_encoding)
            : null,
      );
    });
  }

  /// Copies style source IDs in style order.
  List<String> listStyleSourceIds() {
    return withNativeArena((arena) {
      final outList = arena<Uint64>();
      outList.value = 0;
      _check(raw.mln_map_list_style_source_ids(_handle.raw, outList));
      return _copyStyleIdList(NativeStyleIdList(outList.value));
    });
  }

  /// Adds a hillshade layer for a raster DEM source.
  void addHillshadeLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceId = nativeStringView(sourceId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_add_hillshade_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceId.value,
          nativeBeforeLayerId.value,
        ),
      );
    });
  }

  /// Adds a color-relief layer for a raster DEM source.
  void addColorReliefLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceId = nativeStringView(sourceId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_add_color_relief_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceId.value,
          nativeBeforeLayerId.value,
        ),
      );
    });
  }

  /// Adds a source-free location indicator layer.
  void addLocationIndicatorLayer(String layerId, {String? beforeLayerId}) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_add_location_indicator_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeBeforeLayerId.value,
        ),
      );
    });
  }

  /// Sets a location indicator layer location.
  void setLocationIndicatorLocation(
    String layerId,
    LatLng coordinate, {
    double altitude = 0,
  }) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_location_indicator_location(
          _handle.raw,
          nativeLayerId.value,
          native_struct.latLngToNative(coordinate),
          altitude,
        ),
      );
    });
  }

  /// Sets a location indicator layer bearing in degrees.
  void setLocationIndicatorBearing(String layerId, double bearing) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_location_indicator_bearing(
          _handle.raw,
          nativeLayerId.value,
          bearing,
        ),
      );
    });
  }

  /// Sets a location indicator layer accuracy radius in logical pixels.
  void setLocationIndicatorAccuracyRadius(String layerId, double radius) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_location_indicator_accuracy_radius(
          _handle.raw,
          nativeLayerId.value,
          radius,
        ),
      );
    });
  }

  /// Sets one location indicator image-name property.
  void setLocationIndicatorImageName(
    String layerId,
    LocationIndicatorImageKind imageKind,
    String imageId,
  ) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeImageId = nativeStringView(imageId, arena);
      _check(
        raw.mln_map_set_location_indicator_image_name(
          _handle.raw,
          nativeLayerId.value,
          imageKind.rawValue,
          nativeImageId.value,
        ),
      );
    });
  }

  /// Adds one style layer from a full style-spec layer JSON object.
  void addStyleLayerJson(Uint8List layerJson, {String? beforeLayerId}) {
    withNativeArena((arena) {
      final nativeLayerJson = nativeBufferView(layerJson, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_add_style_layer_json(
          _handle.raw,
          nativeLayerJson,
          nativeBeforeLayerId.value,
        ),
      );
    });
  }

  /// Copies one style layer as a full style-spec layer JSON snapshot.
  Uint8List? getStyleLayerJson(String layerId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(layerId, arena);
      final outLayer = arena<Uint64>();
      outLayer.value = 0;
      final outFound = arena<Bool>();
      _check(
        raw.mln_map_get_style_layer_json(
          _handle.raw,
          nativeId.value,
          outLayer,
          outFound,
        ),
      );
      if (!outFound.value) {
        return null;
      }
      return copyOwnedBuffer(NativeOwnedBufferHandle(outLayer.value));
    });
  }

  /// Sets the style light from a style-spec light JSON object.
  void setStyleLightJson(Uint8List lightJson) {
    withNativeArena((arena) {
      final nativeLightJson = nativeBufferView(lightJson, arena);
      _check(raw.mln_map_set_style_light_json(_handle.raw, nativeLightJson));
    });
  }

  /// Sets one style light property by style-spec property name.
  void setStyleLightProperty(String propertyName, Uint8List value) {
    withNativeArena((arena) {
      final nativePropertyName = nativeStringView(propertyName, arena);
      final nativeValue = nativeBufferView(value, arena);
      _check(
        raw.mln_map_set_style_light_property(
          _handle.raw,
          nativePropertyName.value,
          nativeValue,
        ),
      );
    });
  }

  /// Copies one style light property, or null when the property is undefined.
  Uint8List? getStyleLightProperty(String propertyName) {
    return withNativeArena((arena) {
      final nativePropertyName = nativeStringView(propertyName, arena);
      final outValue = arena<Uint64>();
      outValue.value = 0;
      _check(
        raw.mln_map_get_style_light_property(
          _handle.raw,
          nativePropertyName.value,
          outValue,
        ),
      );
      final buffer = NativeOwnedBufferHandle(outValue.value);
      return buffer.isNull ? null : copyOwnedBuffer(buffer);
    });
  }

  /// Sets the style's global transition options.
  ///
  /// This replaces the whole transition configuration rather than merging into
  /// it, and loading a style replaces it again with the style's own options, so
  /// apply an override after the style loads.
  void setStyleTransitionOptions(StyleTransitionOptions options) {
    withNativeArena((arena) {
      final nativeOptions = arena<raw.mln_style_transition_options>();
      nativeOptions.ref = _styleTransitionOptionsToNative(options);
      _check(
        raw.mln_map_set_style_transition_options(_handle.raw, nativeOptions),
      );
    });
  }

  /// Copies the style's global transition options.
  StyleTransitionOptions getStyleTransitionOptions() {
    return withNativeArena((arena) {
      final outOptions = arena<raw.mln_style_transition_options>();
      outOptions.ref = raw.mln_style_transition_options_default();
      _check(raw.mln_map_get_style_transition_options(_handle.raw, outOptions));
      return _styleTransitionOptionsFromNative(outOptions.ref);
    });
  }

  /// Sets one layer property by style-spec property name.
  void setLayerProperty(String layerId, String propertyName, Uint8List value) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativePropertyName = nativeStringView(propertyName, arena);
      final nativeValue = nativeBufferView(value, arena);
      _check(
        raw.mln_map_set_layer_property(
          _handle.raw,
          nativeLayerId.value,
          nativePropertyName.value,
          nativeValue,
        ),
      );
    });
  }

  /// Copies one layer property, or null when the property is undefined.
  Uint8List? getLayerProperty(String layerId, String propertyName) {
    return withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativePropertyName = nativeStringView(propertyName, arena);
      final outValue = arena<Uint64>();
      outValue.value = 0;
      _check(
        raw.mln_map_get_layer_property(
          _handle.raw,
          nativeLayerId.value,
          nativePropertyName.value,
          outValue,
        ),
      );
      final buffer = NativeOwnedBufferHandle(outValue.value);
      return buffer.isNull ? null : copyOwnedBuffer(buffer);
    });
  }

  /// Sets or clears one layer filter.
  void setLayerFilter(String layerId, Uint8List? filter) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeFilter = filter == null
          ? nullptr.cast<raw.mln_buffer_view>()
          : (arena<raw.mln_buffer_view>()
              ..ref = nativeBufferView(filter, arena));
      _check(
        raw.mln_map_set_layer_filter(
          _handle.raw,
          nativeLayerId.value,
          nativeFilter,
        ),
      );
    });
  }

  /// Copies one layer filter, or null when the layer has no filter.
  Uint8List? getLayerFilter(String layerId) {
    return withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outFilter = arena<Uint64>();
      outFilter.value = 0;
      _check(
        raw.mln_map_get_layer_filter(
          _handle.raw,
          nativeLayerId.value,
          outFilter,
        ),
      );
      final buffer = NativeOwnedBufferHandle(outFilter.value);
      return buffer.isNull ? null : copyOwnedBuffer(buffer);
    });
  }

  /// Sets one layer's source-layer ID.
  ///
  /// Layer types that take no source, such as background, are rejected.
  void setLayerSourceLayer(String layerId, String sourceLayer) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceLayer = nativeStringView(sourceLayer, arena);
      _check(
        raw.mln_map_set_layer_source_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceLayer.value,
        ),
      );
    });
  }

  /// Copies one layer's source-layer ID, empty when the layer carries none.
  String getLayerSourceLayer(String layerId) {
    return _copyLayerText(
      _handle,
      layerId,
      raw.mln_map_copy_layer_source_layer,
    );
  }

  /// Sets one layer's source ID.
  ///
  /// Layer types that take no source, such as background, are rejected. The
  /// named source need not exist yet.
  void setLayerSourceId(String layerId, String sourceId) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_set_layer_source_id(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceId.value,
        ),
      );
    });
  }

  /// Copies one layer's source ID, empty when the layer carries none.
  String getLayerSourceId(String layerId) {
    return _copyLayerText(_handle, layerId, raw.mln_map_copy_layer_source_id);
  }

  /// Sets the lowest zoom at which one layer draws.
  ///
  /// Pass `double.negativeInfinity` for no lower bound.
  void setLayerMinZoom(String layerId, double minZoom) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_layer_min_zoom(
          _handle.raw,
          nativeLayerId.value,
          minZoom,
        ),
      );
    });
  }

  /// Reads the lowest zoom at which one layer draws.
  ///
  /// A layer with no lower bound reports `double.negativeInfinity`.
  double getLayerMinZoom(String layerId) {
    return withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outZoom = arena<Double>();
      _check(
        raw.mln_map_get_layer_min_zoom(
          _handle.raw,
          nativeLayerId.value,
          outZoom,
        ),
      );
      return outZoom.value;
    });
  }

  /// Sets the highest zoom at which one layer draws.
  ///
  /// Pass `double.infinity` for no upper bound.
  void setLayerMaxZoom(String layerId, double maxZoom) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_layer_max_zoom(
          _handle.raw,
          nativeLayerId.value,
          maxZoom,
        ),
      );
    });
  }

  /// Reads the highest zoom at which one layer draws.
  ///
  /// A layer with no upper bound reports `double.infinity`.
  double getLayerMaxZoom(String layerId) {
    return withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outZoom = arena<Double>();
      _check(
        raw.mln_map_get_layer_max_zoom(
          _handle.raw,
          nativeLayerId.value,
          outZoom,
        ),
      );
      return outZoom.value;
    });
  }

  /// Sets whether one layer draws.
  void setLayerVisibility(String layerId, StyleLayerVisibility visibility) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_layer_visibility(
          _handle.raw,
          nativeLayerId.value,
          visibility.rawValue,
        ),
      );
    });
  }

  /// Reads whether one layer draws.
  StyleLayerVisibility getLayerVisibility(String layerId) {
    return withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final outVisibility = arena<Uint32>();
      _check(
        raw.mln_map_get_layer_visibility(
          _handle.raw,
          nativeLayerId.value,
          outVisibility,
        ),
      );
      return StyleLayerVisibility.fromRawValue(outVisibility.value);
    });
  }

  /// Reports whether a style layer ID exists.
  bool styleLayerExists(String layerId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(layerId, arena);
      final outExists = arena<Bool>();
      _check(
        raw.mln_map_style_layer_exists(_handle.raw, nativeId.value, outExists),
      );
      return outExists.value;
    });
  }

  /// Borrows one style layer type string, or returns null when absent.
  String? getStyleLayerType(String layerId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(layerId, arena);
      final outLayerType = arena<raw.mln_buffer_view>();
      final outFound = arena<Bool>();
      _check(
        raw.mln_map_get_style_layer_type(
          _handle.raw,
          nativeId.value,
          outLayerType,
          outFound,
        ),
      );
      if (!outFound.value) {
        return null;
      }
      return _copyStringView(outLayerType.ref);
    });
  }

  /// Moves one style layer before another layer or to the top.
  void moveStyleLayer(String layerId, {String? beforeLayerId}) {
    withNativeArena((arena) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_move_style_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeBeforeLayerId.value,
        ),
      );
    });
  }

  /// Removes one style layer by ID and returns whether one was removed.
  bool removeStyleLayer(String layerId) {
    return withNativeArena((arena) {
      final nativeId = nativeStringView(layerId, arena);
      final outRemoved = arena<Bool>();
      _check(
        raw.mln_map_remove_style_layer(_handle.raw, nativeId.value, outRemoved),
      );
      return outRemoved.value;
    });
  }

  /// Copies style layer IDs in style order.
  List<String> listStyleLayerIds() {
    return withNativeArena((arena) {
      final outList = arena<Uint64>();
      outList.value = 0;
      _check(raw.mln_map_list_style_layer_ids(_handle.raw, outList));
      return _copyStyleIdList(NativeStyleIdList(outList.value));
    });
  }

  /// Explicitly destroys this map.
  void close() {
    final id = _state.handleId;
    _state.close(
      (handle) => raw.mln_map_destroy(handle.raw),
      _c.threadLastErrorMessage,
    );
    _runtime._unregisterMapId(id);
    _clearCustomGeometryCallbacks();
  }

  void _clearCustomGeometryCallbacksAfterUrlStyleLoad() {
    if (_pendingUrlStyleCallbacks.isEmpty) {
      return;
    }
    final retired = _pendingUrlStyleCallbacks.removeAt(0);
    for (final state in retired) {
      final entries = _customGeometryCallbacks.entries
          .where((entry) => identical(entry.value, state))
          .toList(growable: false);
      for (final entry in entries) {
        _customGeometryCallbacks.remove(entry.key);
      }
      state.retire();
    }
  }

  void _clearCustomGeometryCallbacks() {
    for (final state in _customGeometryCallbacks.values) {
      state.retire();
    }
    _customGeometryCallbacks.clear();
    _pendingUrlStyleCallbacks.clear();
  }
}
