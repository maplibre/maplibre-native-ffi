import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../camera/camera.dart';
import '../error/maplibre_exception.dart';
import '../geo/geo.dart';
import '../internal/callback/callback_state.dart';
import '../internal/callback/completion.dart';
import '../internal/callback/wake.dart';
import '../internal/c/maplibre_native_c.dart';
import '../internal/c/maplibre_native_c.g.dart' as raw;
import '../internal/lifecycle/lifecycle.dart';
import '../internal/lifecycle/native_handles.dart';
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

/// Read-only view of one custom-MVT-vector callback root for lifecycle tests.
final class CustomMvtVectorCallbackLifecycleProbe {
  CustomMvtVectorCallbackLifecycleProbe._(this._state);

  final _CustomMvtVectorCallbackState _state;

  bool get retirementQueued => _state.retirementQueuedForTesting;
  bool get closed => _state.closedForTesting;
}

/// Returns the callback root currently owned by [map] for lifecycle tests.
CustomMvtVectorCallbackLifecycleProbe? customMvtVectorCallbackProbeForTesting(
  MapHandle map,
  String sourceId,
) {
  final state = map._customMvtVectorCallbacks[sourceId];
  return state == null ? null : CustomMvtVectorCallbackLifecycleProbe._(state);
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
  RuntimeHandle._(NativeRuntime handle, this._eventWake)
    : _state = NativeHandleState(handle, 'RuntimeHandle') {
    _callbackReleaseListener =
        NativeCallable<raw.mln_runtime_callback_releaseFunction>.listener(
          _releaseNativeCallback,
        );
  }
  final NativeHandleState<NativeRuntime> _state;
  final NativeWakeState _eventWake;
  late final NativeCallable<raw.mln_runtime_callback_releaseFunction>
  _callbackReleaseListener;
  final _maps = <int, WeakReference<MapHandle>>{};
  final _queuedRuntimeEvents = <RuntimeEvent>[];
  final _nativeCallbackReleases = <int, void Function()>{};
  final _resourceProviderQueues = <int, _ResourceProviderCallbackState>{};

  void _releaseNativeCallback(Pointer<Void> userData) {
    _nativeCallbackReleases.remove(userData.address)?.call();
  }

  void _registerNativeCallback(
    Pointer<Void> userData,
    void Function() release,
  ) {
    _nativeCallbackReleases[userData.address] = release;
  }

  void _cancelNativeCallback(Pointer<Void> userData) {
    _nativeCallbackReleases.remove(userData.address)?.call();
  }

  Future<CommandCompletion> _startCommand(
    NativeCompletionStart start, {
    void Function()? onRejected,
  }) => startNativeCompletion(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: start,
    onRejected: onRejected,
    acceptErrorStatus: true,
    decode: (result) => CommandCompletion(
      disposition: CommandDisposition.fromRawValue(result.disposition),
      generation: uint64FromNative(result.generation),
      status: MaplibreStatus.fromNativeStatusCode(result.status),
      diagnostic: copyCompletionDiagnostic(result.diagnostic),
    ),
  );

  Future<T> _startValue<T>({
    required raw.mln_adapter_completion_copy_kind copyKind,
    required int elementSize,
    required NativeCompletionStart start,
    required NativeCompletionDecoder<T> decode,
    void Function()? onRejected,
  }) => startNativeCompletion(
    copyKind: copyKind,
    elementSize: elementSize,
    start: start,
    decode: decode,
    onRejected: onRejected,
  );

  /// Creates a runtime.
  static RuntimeHandle create({
    RuntimeOptions options = const RuntimeOptions(),
  }) {
    ensureAbiVersion();
    RuntimeHandle? runtime;
    final eventWake = NativeWakeState(() {
      scheduleMicrotask(() {
        final target = runtime;
        if (target != null && !target.isClosed) {
          target._queuedRuntimeEvents.addAll(
            target._drainNativeEvents().events,
          );
        }
      });
    });
    var runtimeHandle = 0;
    try {
      runtimeHandle = withNativeArena((arena) {
        final nativeOptions = arena<raw.mln_runtime_options>();
        nativeOptions.ref = _runtimeOptionsToNative(options, arena);
        eventWake.writeTo(nativeOptions.ref.event_wake);
        final outRuntime = arena<Uint64>()..value = 0;
        _check(raw.mln_runtime_create(nativeOptions, outRuntime));
        return outRuntime.value;
      });
      final created = RuntimeHandle._(NativeRuntime(runtimeHandle), eventWake);
      runtime = created;
      runtimeHandle = 0;
      return created;
    } catch (_) {
      if (runtimeHandle != 0) {
        // No wrapper owns this handle, so nothing waits for its teardown.
        _releaseRuntimeHandle(runtimeHandle).ignore();
      }
      eventWake.reject();
      rethrow;
    }
  }

  /// Starts native teardown of an owned runtime handle.
  static Future<void> _releaseRuntimeHandle(int handle) =>
      startNativeCompletion(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        start: (completion) => raw.mln_runtime_release(handle, completion),
        decode: (_) {},
      );

  NativeRuntime get _handle => _state.handle;

  /// Whether this runtime has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  /// Drains queued runtime events into one batch of Dart-owned copies.
  ///
  /// Events arrive in queue order. Every field, message, and payload is copied
  /// before this returns, so a batch stays readable for as long as the host
  /// keeps it.
  RuntimeEventBatch drainEvents() {
    final _ = _handle;
    _queuedRuntimeEvents.addAll(_drainNativeEvents().events);
    final events = List<RuntimeEvent>.of(_queuedRuntimeEvents);
    _queuedRuntimeEvents.clear();
    return RuntimeEventBatch._(events: events);
  }

  RuntimeEventBatch _drainNativeEvents() {
    return withNativeArena((arena) {
      final outBatch = arena<Uint64>()..value = 0;
      _check(raw.mln_runtime_drain_events(_handle.raw, outBatch));
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
  Future<CommandCompletion> setResourceUrlRewriteRules(
    List<ResourceUrlRewriteRule> rules,
  ) {
    final state = _ResourceTransformState(rules);
    final userData = state.pointer.cast<Void>();
    _registerNativeCallback(userData, state.close);
    return _startCommand(
      (completion) => withNativeArena((arena) {
        final transform = arena<raw.mln_resource_transform>();
        transform.ref.size = sizeOf<raw.mln_resource_transform>();
        transform.ref.callback = _c.adapterResourceTransformRewriteCallback();
        transform.ref.user_data = userData;
        transform.ref.release_user_data =
            _callbackReleaseListener.nativeFunction;
        return raw.mln_runtime_set_resource_transform(
          _handle.raw,
          transform,
          completion,
        );
      }),
      onRejected: () => _cancelNativeCallback(userData),
    );
  }

  /// Clears runtime-scoped URL rewrite rules.
  ///
  Future<CommandCompletion> clearResourceTransform() => _startCommand(
    (completion) =>
        raw.mln_runtime_clear_resource_transform(_handle.raw, completion),
  );

  /// Registers native-owned HTTP header routes evaluated on network threads.
  ///
  Future<CommandCompletion> setHttpHeaderTransformRules(
    List<HttpHeaderTransformRule> rules,
  ) {
    final state = _HttpHeaderTransformState(rules);
    final userData = state.pointer.cast<Void>();
    _registerNativeCallback(userData, state.close);
    return _startCommand(
      (completion) => withNativeArena((arena) {
        final transform = arena<raw.mln_http_header_transform>();
        transform.ref.size = sizeOf<raw.mln_http_header_transform>();
        transform.ref.callback = _c.adapterHttpHeaderTransformCallback();
        transform.ref.user_data = userData;
        transform.ref.release_user_data =
            _callbackReleaseListener.nativeFunction;
        return raw.mln_runtime_set_http_header_transform(
          _handle.raw,
          transform,
          completion,
        );
      }),
      onRejected: () => _cancelNativeCallback(userData),
    );
  }

  /// Clears native-owned HTTP header transform routes.
  ///
  Future<CommandCompletion> clearHttpHeaderTransform() => _startCommand(
    (completion) =>
        raw.mln_runtime_clear_http_header_transform(_handle.raw, completion),
  );

  /// Registers or replaces exact native-owned response rules.
  ///
  Future<CommandCompletion> setResourceProviderRules(
    List<ResourceProviderRule> rules,
  ) {
    final state = _ResourceProviderRulesState(rules);
    final userData = state.pointer.cast<Void>();
    _registerNativeCallback(userData, state.close);
    return _startCommand(
      (completion) => withNativeArena((arena) {
        final provider = arena<raw.mln_resource_provider>();
        provider.ref.size = sizeOf<raw.mln_resource_provider>();
        provider.ref.callback = _c.adapterResourceProviderRulesCallback();
        provider.ref.user_data = userData;
        provider.ref.release_user_data =
            _callbackReleaseListener.nativeFunction;
        return raw.mln_runtime_set_resource_provider(
          _handle.raw,
          provider,
          completion,
        );
      }),
      onRejected: () => _cancelNativeCallback(userData),
    );
  }

  /// Registers or replaces a queued Dart resource provider callback.
  ///
  Future<CommandCompletion> setResourceProvider(ResourceProvider provider) {
    final state = _ResourceProviderCallbackState(provider);
    _resourceProviderQueues[state.queue] = state;
    final userData = state.pointer.cast<Void>();
    _registerNativeCallback(
      userData,
      () => _closeResourceProviderCallback(state),
    );
    return _startCommand(
      (completion) => withNativeArena((arena) {
        final nativeProvider = arena<raw.mln_resource_provider>();
        nativeProvider.ref.size = sizeOf<raw.mln_resource_provider>();
        nativeProvider.ref.callback = _c
            .adapterQueuedResourceProviderCallback();
        nativeProvider.ref.user_data = userData;
        nativeProvider.ref.release_user_data =
            _callbackReleaseListener.nativeFunction;
        return raw.mln_runtime_set_resource_provider(
          _handle.raw,
          nativeProvider,
          completion,
        );
      }),
      onRejected: () => _cancelNativeCallback(userData),
    );
  }

  /// Clears the runtime-scoped network resource provider.
  ///
  Future<CommandCompletion> clearResourceProvider() => _startCommand(
    (completion) =>
        raw.mln_runtime_clear_resource_provider(_handle.raw, completion),
  );

  void _closeResourceProviderCallback(_ResourceProviderCallbackState state) {
    _resourceProviderQueues.remove(state.queue);
    state.retire();
  }

  /// Starts an ambient cache maintenance operation.
  Future<void> runAmbientCacheOperation(AmbientCacheOperation operation) =>
      _startUnit(
        (completion) => raw.mln_runtime_run_ambient_cache_operation(
          _handle.raw,
          operation.rawValue,
          completion,
        ),
      );

  /// Starts a change to this runtime's maximum ambient cache size.
  ///
  /// MapLibre evicts ambient resources to fit the new budget, so lowering it
  /// discards cached resources. Offline regions are unaffected.
  Future<void> setMaximumAmbientCacheSize(BigInt size) => _startUnit(
    (completion) => raw.mln_runtime_set_maximum_ambient_cache_size(
      _handle.raw,
      uint64ToNative(size, 'maximum ambient cache size'),
      completion,
    ),
  );

  /// Starts creating an offline region.
  Future<OfflineRegionInfo> createOfflineRegion(
    OfflineRegionDefinition definition, {
    Uint8List? metadata,
  }) {
    return _startOfflineValue(
      copyKind: raw
          .mln_adapter_completion_copy_kind
          .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGIONS,
      elementSize: sizeOf<raw.mln_offline_region_info>(),
      start: (completion) => withNativeArena((arena) {
        final nativeDefinition = arena<raw.mln_offline_region_definition>();
        nativeDefinition.ref = _offlineRegionDefinitionToNative(
          definition,
          arena,
        );
        final nativeMetadata = _nativeBytes(metadata, arena);
        return raw.mln_runtime_offline_region_create(
          _handle.raw,
          nativeDefinition,
          nativeMetadata,
          metadata?.length ?? 0,
          completion,
        );
      }),
      decode: (result) => _offlineRegionInfoFromNative(
        result.value.cast<raw.mln_offline_region_info>().ref,
      ),
    );
  }

  /// Starts getting an offline region snapshot by ID.
  Future<OfflineRegionInfo?> getOfflineRegion(int regionId) =>
      _startOfflineValue(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGIONS,
        elementSize: sizeOf<raw.mln_offline_region_info>(),
        start: (completion) => raw.mln_runtime_offline_region_get(
          _handle.raw,
          regionId,
          completion,
        ),
        decode: (result) => result.value_count == 0
            ? null
            : _offlineRegionInfoFromNative(
                result.value.cast<raw.mln_offline_region_info>().ref,
              ),
      );

  /// Starts listing offline region snapshots.
  Future<List<OfflineRegionInfo>> listOfflineRegions() => _startOfflineRegions(
    (completion) =>
        raw.mln_runtime_offline_regions_list(_handle.raw, completion),
  );

  /// Starts merging offline regions from another database path.
  Future<List<OfflineRegionInfo>> mergeOfflineRegionDatabase(
    String sideDatabasePath,
  ) => _startOfflineRegions(
    (completion) => withNativeArena((arena) {
      final nativePath = nativeUtf8CString(sideDatabasePath, arena);
      return raw.mln_runtime_offline_regions_merge_database(
        _handle.raw,
        nativePath.pointer.cast<Char>(),
        completion,
      );
    }),
  );

  /// Starts updating opaque offline region metadata.
  Future<OfflineRegionInfo> updateOfflineRegionMetadata(
    int regionId,
    Uint8List metadata,
  ) {
    return _startOfflineValue(
      copyKind: raw
          .mln_adapter_completion_copy_kind
          .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGIONS,
      elementSize: sizeOf<raw.mln_offline_region_info>(),
      start: (completion) => withNativeArena((arena) {
        final nativeMetadata = _nativeBytes(metadata, arena);
        return raw.mln_runtime_offline_region_update_metadata(
          _handle.raw,
          regionId,
          nativeMetadata,
          metadata.length,
          completion,
        );
      }),
      decode: (result) => _offlineRegionInfoFromNative(
        result.value.cast<raw.mln_offline_region_info>().ref,
      ),
    );
  }

  /// Starts getting the current offline region status.
  Future<OfflineRegionStatus> getOfflineRegionStatus(int regionId) =>
      _startOfflineValue(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: sizeOf<raw.mln_offline_region_status>(),
        start: (completion) => raw.mln_runtime_offline_region_get_status(
          _handle.raw,
          regionId,
          completion,
        ),
        decode: (result) => _offlineRegionStatusFromNative(
          result.value.cast<raw.mln_offline_region_status>().ref,
        ),
      );

  /// Starts enabling or disabling offline region observation.
  Future<void> setOfflineRegionObserved(int regionId, bool observed) =>
      _startUnit(
        (completion) => raw.mln_runtime_offline_region_set_observed(
          _handle.raw,
          regionId,
          observed,
          completion,
        ),
      );

  /// Starts changing an offline region's download state.
  Future<void> setOfflineRegionDownloadState(
    int regionId,
    OfflineRegionDownloadState state,
  ) => _startUnit(
    (completion) => raw.mln_runtime_offline_region_set_download_state(
      _handle.raw,
      regionId,
      state.rawValue,
      completion,
    ),
  );

  /// Starts invalidating cached resources for an offline region.
  Future<void> invalidateOfflineRegion(int regionId) => _startUnit(
    (completion) => raw.mln_runtime_offline_region_invalidate(
      _handle.raw,
      regionId,
      completion,
    ),
  );

  /// Starts deleting an offline region.
  Future<void> deleteOfflineRegion(int regionId) => _startUnit(
    (completion) => raw.mln_runtime_offline_region_delete(
      _handle.raw,
      regionId,
      completion,
    ),
  );

  Future<void> _startUnit(NativeCompletionStart start) => _startValue(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: start,
    decode: (_) {},
  );

  Future<T> _startOfflineValue<T>({
    required raw.mln_adapter_completion_copy_kind copyKind,
    required int elementSize,
    required NativeCompletionStart start,
    required NativeCompletionDecoder<T> decode,
  }) => _startValue(
    copyKind: copyKind,
    elementSize: elementSize,
    start: start,
    decode: decode,
  );

  Future<List<OfflineRegionInfo>> _startOfflineRegions(
    NativeCompletionStart start,
  ) => _startOfflineValue(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGIONS,
    elementSize: sizeOf<raw.mln_offline_region_info>(),
    start: start,
    decode: (result) => [
      for (var index = 0; index < result.value_count; index += 1)
        _offlineRegionInfoFromNative(
          result.value.cast<raw.mln_offline_region_info>()[index],
        ),
    ],
  );

  /// Creates a map without blocking the calling isolate.
  Future<MapHandle> createMap({MapOptions options = const MapOptions()}) =>
      MapHandle.create(this, options: options);

  void _registerMap(MapHandle map) {
    _maps[map._handle.raw] = WeakReference(map);
  }

  void _unregisterMapId(int id) {
    _maps.remove(id);
  }

  /// Completes after every previously accepted runtime command.
  Future<void> barrier() => _startUnit(
    (completion) => raw.mln_runtime_barrier(_handle.raw, completion),
  );

  /// Releases this runtime's public native handle and waits for native
  /// teardown.
  ///
  /// The native handle is consumed before this returns to the event loop, so a
  /// rejected release throws and leaves the runtime open. The returned future
  /// completes after every earlier accepted submission, including released
  /// maps' teardown, has finished and the runtime's threads and resources are
  /// gone, so a host that awaits it may exit the process.
  Future<void> close() async {
    await _state.closeAsync((handle) => _releaseRuntimeHandle(handle.raw));
    while (_nativeCallbackReleases.isNotEmpty) {
      await Future<void>.delayed(Duration.zero);
    }
    await _eventWake.released;
    _callbackReleaseListener.close();
  }
}

/// One batch of runtime events copied out of the native event arena.
final class RuntimeEventBatch {
  RuntimeEventBatch._({required List<RuntimeEvent> events})
    : events = List.unmodifiable(events);

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
    }
    return RuntimeEventBatch._(events: events);
  }

  /// Drained events in queue order.
  final List<RuntimeEvent> events;
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

  static const mapCameraTransitionFinished = RuntimeEventMask(1 << 22);

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
      (1 << 22);
  static const _runtimeEventBits = (1 << 19) | (1 << 20) | (1 << 21);

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
    22,
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

    22 => mapCameraTransitionFinished,
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

/// Terminal outcome of an ordered native command.
final class CommandCompletion {
  const CommandCompletion({
    required this.disposition,
    required this.generation,
    required this.status,
    required this.diagnostic,
  });

  final CommandDisposition disposition;
  final BigInt generation;
  final MaplibreStatus status;
  final String diagnostic;
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
      8 => _cameraTransitionFinishedPayload(payload.camera_transition_finished),
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

/// Immutable map state copied from the latest published generation.
///
/// Every committed map command publishes a new map snapshot. A snapshot whose
/// [generation] is at or past a command completion's generation observes that
/// commit.
final class MapSnapshot {
  /// Creates a copied map snapshot.
  const MapSnapshot({
    required this.generation,
    required this.debugOptions,
    required this.camera,
    required this.size,
    required this.projectionMode,
    required this.viewportOptions,
    required this.fullyLoaded,
    required this.renderingStatsViewEnabled,
    required this.repaintDemand,
    required this.eventMask,
    required this.latestRenderUpdateGeneration,
    required this.tileOptions,
    required this.bounds,
    required this.freeCameraOptions,
  });

  /// Generation of every field in this snapshot.
  final BigInt generation;

  /// Debug overlay options copied from the snapshot.
  final MapDebugOptions debugOptions;

  /// Camera copied from the snapshot.
  final CameraOptions camera;

  /// Logical extent copied from the snapshot.
  final MapSize size;

  /// Projection mode copied from the snapshot.
  final ProjectionModeOptions projectionMode;

  /// Viewport options copied from the snapshot.
  final MapViewportOptions viewportOptions;

  /// Whether every requested style and tile resource finished loading.
  final bool fullyLoaded;

  /// Whether the rendering stats overlay is enabled.
  final bool renderingStatsViewEnabled;

  /// Whether the map currently requests a repaint.
  final bool repaintDemand;

  /// Map event types selected in this snapshot.
  final RuntimeEventMask eventMask;

  /// Generation of the latest render update.
  final BigInt latestRenderUpdateGeneration;

  /// Tile prefetch and LOD tuning controls copied from the snapshot.
  final MapTileOptions tileOptions;

  /// Map camera constraints copied from the snapshot.
  final BoundOptions bounds;

  /// Free camera position and orientation copied from the snapshot.
  final FreeCameraOptions freeCameraOptions;
}

/// Owned prepared GeoJSON source data.
///
/// [prepare] parses one complete UTF-8 GeoJSON document and tiles or clusters
/// it into an immutable prepared index with its options baked in. Preparation
/// needs no runtime or map. Its native id is copiable identity only, so a raw
/// id sent to another isolate cannot become an operable handle there.
///
/// Install calls borrow this handle, so one prepared value may be installed on
/// any number of sources and closed at any time afterward; [close] never
/// invalidates a source the data was installed on.
final class GeoJsonSourceDataHandle {
  GeoJsonSourceDataHandle._(NativeGeoJsonSourceData handle)
    : _state = NativeHandleState(handle, 'GeoJsonSourceDataHandle');

  /// Parses and tiles one complete UTF-8 GeoJSON document with [options].
  ///
  /// Cluster validation happens here: clustering accepts only a feature
  /// collection whose every feature carries point geometry.
  factory GeoJsonSourceDataHandle.prepare(
    Uint8List data, {
    GeoJsonSourceOptions? options,
  }) {
    return withNativeArena((arena) {
      final nativeData = nativeBufferView(data, arena);
      final nativeOptions = _nativeGeoJsonSourceOptions(
        options ?? GeoJsonSourceOptions(),
        arena,
      );
      final outData = arena<Uint64>();
      outData.value = 0;
      _check(
        raw.mln_geojson_source_data_create(nativeData, nativeOptions, outData),
      );
      return GeoJsonSourceDataHandle._(NativeGeoJsonSourceData(outData.value));
    });
  }

  final NativeHandleState<NativeGeoJsonSourceData> _state;

  /// Whether this prepared data has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  /// Explicitly releases this prepared data.
  ///
  /// Sources the data was installed on keep their own reference, so this never
  /// invalidates a source.
  void close() {
    _state.close((handle) {
      raw.mln_geojson_source_data_destroy(handle.raw);
      return 0;
    }, _c.threadLastErrorMessage);
  }
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
    final map = await runtime._startValue(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: sizeOf<Uint64>(),
      start: (completion) => withNativeArena((arena) {
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
        return raw.mln_map_create(
          runtime._handle.raw,
          nativeOptions,
          completion,
        );
      }),
      decode: (result) => MapHandle._(
        runtime,
        NativeMap(result.value.cast<Uint64>().value),
        options.eventMask,
      ),
    );
    runtime._registerMap(map);
    return map;
  }

  final RuntimeHandle _runtime;
  final NativeHandleState<NativeMap> _state;
  RuntimeEventMask _acceptedEventMask;

  /// Callback roots of the custom-geometry sources this map still holds, each
  /// released by the C API's own release callback.
  final _customGeometryCallbacks = <String, _CustomGeometryCallbackState>{};
  final _customMvtVectorCallbacks = <String, _CustomMvtVectorCallbackState>{};

  /// Whether this map has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  NativeMap get _handle {
    final _ = _runtime._handle;
    return _state.handle;
  }

  Future<CommandCompletion> _startCommand(NativeCompletionStart start) =>
      _runtime._startCommand(start);

  Future<CommandCompletion> _startCommandInArena(
    void Function(Arena, Pointer<raw.mln_completion>) start,
  ) => _startCommand(
    (completion) => withNativeArena((arena) {
      start(arena, completion);
      return nativeStatusOk;
    }),
  );

  Future<T> _startMapValue<T>({
    required raw.mln_adapter_completion_copy_kind copyKind,
    required int elementSize,
    required NativeCompletionStart start,
    required NativeCompletionDecoder<T> decode,
  }) => _runtime._startValue(
    copyKind: copyKind,
    elementSize: elementSize,
    start: start,
    decode: decode,
  );

  /// Loads a style URL.
  Future<CommandCompletion> setStyleUrl(String url) => _startCommand(
    (completion) => withNativeArena((arena) {
      final nativeUrl = nativeUtf8CString(url, arena);
      return raw.mln_map_set_style_url(
        _handle.raw,
        nativeUrl.pointer.cast<Char>(),
        completion,
      );
    }),
  );

  /// Loads inline style JSON.
  Future<CommandCompletion> setStyleJson(Uint8List json) => _startCommand(
    (completion) => withNativeArena((arena) {
      final nativeJson = nativeBufferView(json, arena);
      return raw.mln_map_set_style_json(_handle.raw, nativeJson, completion);
    }),
  );

  /// Sets per-feature state on this map.
  Future<CommandCompletion> setFeatureState(
    FeatureStateSelector selector,
    Uint8List state,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeSelector = _featureStateSelectorToNative(selector, arena);
      final nativeState = nativeBufferView(state, arena);
      _check(
        raw.mln_map_set_feature_state(
          _handle.raw,
          nativeSelector,
          nativeState,
          completion,
        ),
      );
    });
  }

  /// Copies per-feature state from this map after prior commands.
  ///
  /// Missing feature state is reported as an empty JSON object.
  Future<Uint8List> getFeatureState(FeatureStateSelector selector) {
    return _startMapValue(
      copyKind: raw
          .mln_adapter_completion_copy_kind
          .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEWS,
      elementSize: sizeOf<raw.mln_buffer_view>(),
      start: (completion) => withNativeArena((arena) {
        final nativeSelector = _featureStateSelectorToNative(selector, arena);
        return raw.mln_map_get_feature_state(
          _handle.raw,
          nativeSelector,
          completion,
        );
      }),
      decode: (result) =>
          _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    );
  }

  /// Removes per-feature state from this map.
  Future<CommandCompletion> removeFeatureState(FeatureStateSelector selector) {
    return _startCommandInArena((arena, completion) {
      final nativeSelector = _featureStateSelectorToNative(selector, arena);
      _check(
        raw.mln_map_remove_feature_state(
          _handle.raw,
          nativeSelector,
          completion,
        ),
      );
    });
  }

  /// Returns the style document this map's style was last parsed from.
  ///
  /// This is the loaded document, not a serialization of the live style:
  /// runtime mutations do not change it, and a failed parse leaves the
  /// previously parsed document in place. The result is empty when no document
  /// has been parsed.
  Future<Uint8List> getLoadedStyleJson() {
    return _startMapValue(
      copyKind: raw
          .mln_adapter_completion_copy_kind
          .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEWS,
      elementSize: sizeOf<raw.mln_buffer_view>(),
      start: (completion) =>
          raw.mln_map_loaded_style_json(_handle.raw, completion),
      decode: (result) =>
          _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    );
  }

  /// Returns the URL this map's style was last requested from.
  ///
  /// [setStyleUrl] records the URL when the request is made, before the
  /// response arrives, and [setStyleJson] clears it, so this can disagree with
  /// [getLoadedStyleJson] while a load is in flight or after one fails. The
  /// result is empty when no URL bytes are available.
  Future<String> getStyleUrl() async {
    final bytes = await _startMapValue(
      copyKind: raw
          .mln_adapter_completion_copy_kind
          .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEWS,
      elementSize: sizeOf<raw.mln_buffer_view>(),
      start: (completion) => raw.mln_map_style_url(_handle.raw, completion),
      decode: (result) =>
          _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    );
    return utf8.decode(bytes);
  }

  /// Selects map-originated events.
  Future<CommandCompletion> setEventMask(RuntimeEventMask mask) {
    final completion = _startCommand(
      (completion) =>
          raw.mln_map_set_event_mask(_handle.raw, mask.value, completion),
    );
    _acceptedEventMask = mask;
    return completion;
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
        debugOptions: MapDebugOptions(value.debug_options),
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
        fullyLoaded: value.fully_loaded,
        renderingStatsViewEnabled: value.rendering_stats_view_enabled,
        repaintDemand: value.repaint_demand,
        eventMask: RuntimeEventMask(value.event_mask),
        latestRenderUpdateGeneration: uint64FromNative(
          value.latest_render_update_generation,
        ),
        tileOptions: native_struct.mapTileOptionsFromNative(value.tile),
        bounds: native_struct.boundOptionsFromNative(value.bounds),
        freeCameraOptions: native_struct.freeCameraOptionsFromNative(
          value.free_camera,
        ),
      );
    });
  }

  /// Resizes the map.
  Future<CommandCompletion> resize(MapSize size) => _startCommand(
    (completion) => withNativeArena((arena) {
      final extent = arena<raw.mln_logical_extent>();
      extent.ref.width = _positiveUint32(size.width, 'map width');
      extent.ref.height = _positiveUint32(size.height, 'map height');
      extent.ref.scale_factor = size.scaleFactor;
      return raw.mln_map_resize(_handle.raw, extent.ref, completion);
    }),
  );

  /// Requests a repaint.
  Future<CommandCompletion> requestRepaint() => _startCommand(
    (completion) => raw.mln_map_request_repaint(_handle.raw, completion),
  );

  /// Requests one still image without blocking the calling isolate.
  Future<void> requestStillImage() => _runtime._startUnit(
    (completion) => raw.mln_map_request_still_image(_handle.raw, completion),
  );

  /// Applies MapLibre debug overlay options.
  Future<CommandCompletion> setDebugOptions(MapDebugOptions options) =>
      _startCommand(
        (completion) => raw.mln_map_set_debug_options(
          _handle.raw,
          options.bits,
          completion,
        ),
      );

  /// Dumps map debug logs through MapLibre Native logging.
  Future<CommandCompletion> dumpDebugLogs() => _startCommand(
    (completion) => raw.mln_map_dump_debug_logs(_handle.raw, completion),
  );

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
  Future<CameraSnapshot> queryCamera() => _startMapValue(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: sizeOf<raw.mln_camera_query_result>(),
    start: (completion) => raw.mln_map_camera_query(_handle.raw, completion),
    decode: (result) {
      final value = result.value.cast<raw.mln_camera_query_result>().ref;
      return CameraSnapshot(
        camera: native_struct.cameraOptionsFromNative(value.camera),
        generation: uint64FromNative(value.generation),
      );
    },
  );

  /// Submits an atomic camera update.
  Future<CommandCompletion> updateCamera(
    CameraOptions camera, {
    CameraUpdateMode mode = CameraUpdateMode.jump,
    AnimationOptions? animation,
    int gesturePhase = 0,
  }) {
    return _startCommand(
      (completion) => withNativeArena((arena) {
        final update = arena<raw.mln_camera_update>();
        update.ref = raw.mln_camera_update_default();
        update.ref.mode = mode.rawValue;
        update.ref.camera = _nativeCamera(camera, arena).ref;
        if (animation != null) {
          update.ref.animation = _nativeAnimation(animation, arena).ref;
        }
        update.ref.gesture_phase = gesturePhase;
        return raw.mln_map_update_camera(_handle.raw, update, completion);
      }),
    );
  }

  /// Submits one relative camera operation.
  Future<CommandCompletion> applyCameraDelta(CameraDelta delta) {
    return _startCommand(
      (completion) => withNativeArena((arena) {
        final native = arena<raw.mln_camera_delta>();
        native.ref = raw.mln_camera_delta_default();
        native.ref.kind = delta.kind.rawValue;
        native.ref.offset = native_struct.screenPointToNative(delta.offset);
        native.ref.amount = delta.amount;
        if (delta.anchor case final anchor?) {
          native.ref.has_anchor = true;
          native.ref.anchor = native_struct.screenPointToNative(anchor);
        }
        native.ref.animation = _nativeAnimation(delta.animation, arena).ref;
        return raw.mln_map_apply_camera_delta(_handle.raw, native, completion);
      }),
    );
  }

  /// Enables or disables the rendering stats overlay.
  Future<CommandCompletion> setRenderingStatsViewEnabled(bool enabled) =>
      _startCommand(
        (completion) => raw.mln_map_set_rendering_stats_view_enabled(
          _handle.raw,
          enabled,
          completion,
        ),
      );

  /// Applies selected live map viewport and render-transform controls.
  Future<CommandCompletion> setViewportOptions(MapViewportOptions options) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          final nativeOptions = arena<raw.mln_map_viewport_options>();
          nativeOptions.ref = native_struct.mapViewportOptionsToNative(
            options,
            raw.mln_map_viewport_options_default(),
          );
          return raw.mln_map_set_viewport_options(
            _handle.raw,
            nativeOptions,
            completion,
          );
        }),
      );

  /// Applies selected tile prefetch and LOD tuning controls.
  Future<CommandCompletion> setTileOptions(MapTileOptions options) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          final nativeOptions = arena<raw.mln_map_tile_options>();
          nativeOptions.ref = native_struct.mapTileOptionsToNative(
            options,
            raw.mln_map_tile_options_default(),
          );
          return raw.mln_map_set_tile_options(
            _handle.raw,
            nativeOptions,
            completion,
          );
        }),
      );

  /// Applies selected map camera constraint options.
  Future<CommandCompletion> setBounds(BoundOptions options) => _startCommand(
    (completion) => withNativeArena((arena) {
      final nativeOptions = arena<raw.mln_bound_options>();
      nativeOptions.ref = native_struct.boundOptionsToNative(
        options,
        raw.mln_bound_options_default(),
      );
      return raw.mln_map_set_bounds(_handle.raw, nativeOptions, completion);
    }),
  );

  /// Applies selected free camera position and orientation fields.
  Future<CommandCompletion> setFreeCameraOptions(FreeCameraOptions options) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          final nativeOptions = arena<raw.mln_free_camera_options>();
          nativeOptions.ref = native_struct.freeCameraOptionsToNative(
            options,
            raw.mln_free_camera_options_default(),
          );
          return raw.mln_map_set_free_camera_options(
            _handle.raw,
            nativeOptions,
            completion,
          );
        }),
      );

  /// Copies axonometric rendering options from the latest map snapshot.
  ProjectionModeOptions projectionMode() => snapshot().projectionMode;

  /// Applies selected axonometric rendering option fields.
  Future<CommandCompletion> setProjectionMode(ProjectionModeOptions mode) =>
      _startCommand(
        (completion) => withNativeArena((arena) {
          final nativeMode = arena<raw.mln_projection_mode>();
          nativeMode.ref = native_struct.projectionModeOptionsToNative(
            mode,
            raw.mln_projection_mode_default(),
          );
          return raw.mln_map_set_projection_mode(
            _handle.raw,
            nativeMode,
            completion,
          );
        }),
      );

  /// Computes a camera that fits geographic bounds after prior commands.
  Future<CameraOptions> cameraForLatLngBounds(
    LatLngBounds bounds, {
    CameraFitOptions fitOptions = const CameraFitOptions(),
  }) => _cameraFitQuery((arena, completion) {
    final nativeFitOptions = arena<raw.mln_camera_fit_options>();
    nativeFitOptions.ref = native_struct.cameraFitOptionsToNative(
      fitOptions,
      raw.mln_camera_fit_options_default(),
    );
    return raw.mln_map_camera_for_lat_lng_bounds(
      _handle.raw,
      native_struct.latLngBoundsToNative(bounds),
      nativeFitOptions,
      completion,
    );
  });

  /// Computes a camera that fits geographic coordinates after prior commands.
  Future<CameraOptions> cameraForLatLngs(
    List<LatLng> coordinates, {
    CameraFitOptions fitOptions = const CameraFitOptions(),
  }) => _cameraFitQuery((arena, completion) {
    final nativeFitOptions = arena<raw.mln_camera_fit_options>();
    nativeFitOptions.ref = native_struct.cameraFitOptionsToNative(
      fitOptions,
      raw.mln_camera_fit_options_default(),
    );
    return raw.mln_map_camera_for_lat_lngs(
      _handle.raw,
      _latLngArray(coordinates, arena),
      coordinates.length,
      nativeFitOptions,
      completion,
    );
  });

  /// Computes a camera that fits geometry after prior commands.
  Future<CameraOptions> cameraForGeometry(
    Uint8List geometry, {
    CameraFitOptions fitOptions = const CameraFitOptions(),
  }) => _cameraFitQuery((arena, completion) {
    final nativeFitOptions = arena<raw.mln_camera_fit_options>();
    nativeFitOptions.ref = native_struct.cameraFitOptionsToNative(
      fitOptions,
      raw.mln_camera_fit_options_default(),
    );
    return raw.mln_map_camera_for_geometry(
      _handle.raw,
      nativeBufferView(geometry, arena),
      nativeFitOptions,
      completion,
    );
  });

  Future<CameraOptions> _cameraFitQuery(
    int Function(Arena, Pointer<raw.mln_completion>) start,
  ) => _startMapValue(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: sizeOf<raw.mln_camera_options>(),
    start: (completion) => withNativeArena((arena) => start(arena, completion)),
    decode: (result) => native_struct.cameraOptionsFromNative(
      result.value.cast<raw.mln_camera_options>().ref,
    ),
  );

  /// Computes geographic bounds for a camera from two viewport corners.
  ///
  /// The box is the hull of the top-left and bottom-right screen corners for
  /// that camera in the current viewport. When bearing and pitch are zero, the
  /// box equals the visible area. Those corners are the northwest and southeast
  /// of the viewport. Longitudes stay in -180 to 180.
  Future<LatLngBounds> latLngBoundsForCamera(CameraOptions camera) =>
      _latLngBoundsForCamera(camera, unwrapped: false);

  /// Computes geographic bounds for a camera from the four viewport corners.
  ///
  /// The axis-aligned hull of all four screen corners and the center encompasses
  /// the projected viewport. Longitudes unwrap onto the shortest path through
  /// the center. A viewport that crosses the antimeridian reports values outside
  /// -180 to 180.
  Future<LatLngBounds> latLngBoundsForCameraUnwrapped(CameraOptions camera) =>
      _latLngBoundsForCamera(camera, unwrapped: true);

  Future<LatLngBounds> _latLngBoundsForCamera(
    CameraOptions camera, {
    required bool unwrapped,
  }) => _startMapValue(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: sizeOf<raw.mln_lat_lng_bounds>(),
    start: (completion) => withNativeArena((arena) {
      final nativeCamera = _nativeCamera(camera, arena);
      return unwrapped
          ? raw.mln_map_lat_lng_bounds_for_camera_unwrapped(
              _handle.raw,
              nativeCamera,
              completion,
            )
          : raw.mln_map_lat_lng_bounds_for_camera(
              _handle.raw,
              nativeCamera,
              completion,
            );
    }),
    decode: (result) => native_struct.latLngBoundsFromNative(
      result.value.cast<raw.mln_lat_lng_bounds>().ref,
    ),
  );

  /// Converts a geographic coordinate after prior commands.
  Future<ScreenPoint> pixelForLatLng(LatLng coordinate) => _startMapValue(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: sizeOf<raw.mln_screen_point>(),
    start: (completion) => raw.mln_map_pixel_for_lat_lng(
      _handle.raw,
      native_struct.latLngToNative(coordinate),
      completion,
    ),
    decode: (result) => native_struct.screenPointFromNative(
      result.value.cast<raw.mln_screen_point>().ref,
    ),
  );

  /// Converts a screen point after prior commands.
  ///
  /// The longitude wraps to the range from -180 to 180 degrees.
  Future<LatLng> latLngForPixel(ScreenPoint point) =>
      _latLngForPixel(point, unwrapped: false);

  /// Converts a screen point after prior commands without wrapping longitude.
  ///
  /// The longitude preserves the visible world copy and may fall outside
  /// -180 to 180.
  Future<LatLng> latLngForPixelUnwrapped(ScreenPoint point) =>
      _latLngForPixel(point, unwrapped: true);

  Future<LatLng> _latLngForPixel(
    ScreenPoint point, {
    required bool unwrapped,
  }) => _startMapValue(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: sizeOf<raw.mln_lat_lng>(),
    start: (completion) {
      final nativePoint = native_struct.screenPointToNative(point);
      return unwrapped
          ? raw.mln_map_lat_lng_for_pixel_unwrapped(
              _handle.raw,
              nativePoint,
              completion,
            )
          : raw.mln_map_lat_lng_for_pixel(_handle.raw, nativePoint, completion);
    },
    decode: (result) => native_struct.latLngFromNative(
      result.value.cast<raw.mln_lat_lng>().ref,
    ),
  );

  /// Converts geographic coordinates after prior commands.
  Future<List<ScreenPoint>> pixelsForLatLngs(List<LatLng> coordinates) =>
      _coordinateListQuery<ScreenPoint>(
        sizeOf<raw.mln_screen_point>(),
        (arena, completion) => raw.mln_map_pixels_for_lat_lngs(
          _handle.raw,
          _latLngArray(coordinates, arena),
          coordinates.length,
          completion,
        ),
        (values, index) => native_struct.screenPointFromNative(
          values.cast<raw.mln_screen_point>()[index],
        ),
      );

  /// Converts screen points after prior commands.
  ///
  /// Longitudes wrap to the range from -180 to 180 degrees.
  Future<List<LatLng>> latLngsForPixels(List<ScreenPoint> points) =>
      _latLngsForPixels(points, unwrapped: false);

  /// Converts screen points after prior commands without wrapping longitudes.
  ///
  /// Each longitude preserves its visible world copy and may fall outside
  /// -180 to 180.
  Future<List<LatLng>> latLngsForPixelsUnwrapped(List<ScreenPoint> points) =>
      _latLngsForPixels(points, unwrapped: true);

  Future<List<LatLng>> _latLngsForPixels(
    List<ScreenPoint> points, {
    required bool unwrapped,
  }) => _coordinateListQuery<LatLng>(
    sizeOf<raw.mln_lat_lng>(),
    (arena, completion) {
      // Keep argument storage alive until native copies it below.
      final nativePoints = points.isEmpty
          ? nullptr.cast<raw.mln_screen_point>()
          : arena<raw.mln_screen_point>(points.length);
      for (var index = 0; index < points.length; index += 1) {
        nativePoints[index] = native_struct.screenPointToNative(points[index]);
      }
      return unwrapped
          ? raw.mln_map_lat_lngs_for_pixels_unwrapped(
              _handle.raw,
              nativePoints,
              points.length,
              completion,
            )
          : raw.mln_map_lat_lngs_for_pixels(
              _handle.raw,
              nativePoints,
              points.length,
              completion,
            );
    },
    (values, index) =>
        native_struct.latLngFromNative(values.cast<raw.mln_lat_lng>()[index]),
  );

  Future<List<T>> _coordinateListQuery<T>(
    int elementSize,
    int Function(Arena, Pointer<raw.mln_completion>) start,
    T Function(Pointer<Void>, int) convert,
  ) => _startMapValue(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: elementSize,
    start: (completion) => withNativeArena((arena) => start(arena, completion)),
    decode: (result) => [
      for (var index = 0; index < result.value_count; index += 1)
        convert(result.value, index),
    ],
  );

  /// Creates a projection helper without blocking the calling isolate.
  ///
  /// The projection copies this map's transform state after every earlier map
  /// command and never observes map changes made after its creation. Every
  /// later projection call is synchronous.
  Future<MapProjectionHandle> createProjection() => _startMapValue(
    copyKind:
        raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: sizeOf<Uint64>(),
    start: (completion) =>
        raw.mln_map_projection_create(_handle.raw, completion),
    decode: (result) => MapProjectionHandle._(
      NativeMapProjection(result.value.cast<Uint64>().value),
    ),
  );

  /// Sets or replaces one runtime style image.
  Future<CommandCompletion> setStyleImage(
    String imageId,
    PremultipliedRgba8Image image, {
    StyleImageOptions? options,
  }) {
    final resolvedOptions = options ?? StyleImageOptions();
    return _startCommandInArena((arena, completion) {
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
          completion,
        ),
      );
    });
  }

  /// Copies one runtime style image's stretchable intervals, or null when no
  /// image carries [imageId]. The record holds horizontal intervals first.
  Future<({List<ImageStretch> stretchX, List<ImageStretch> stretchY})?>
  getStyleImageStretches(String imageId) {
    return _startMapValue(
      copyKind: raw
          .mln_adapter_completion_copy_kind
          .MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE_STRETCHES,
      elementSize: sizeOf<raw.mln_style_image_stretches_result>(),
      start: (completion) => withNativeArena((arena) {
        final nativeId = nativeStringView(imageId, arena);
        return raw.mln_map_copy_style_image_stretches(
          _handle.raw,
          nativeId.value,
          completion,
        );
      }),
      decode: (result) {
        if (result.value_count == 0) return null;
        final value = result.value
            .cast<raw.mln_style_image_stretches_result>()
            .ref;
        List<ImageStretch> read(
          Pointer<raw.mln_image_stretch> array,
          int count,
        ) => List<ImageStretch>.generate(
          count,
          (index) => ImageStretch(array[index].from, array[index].to),
        );
        return (
          stretchX: read(value.stretch_x, value.stretch_x_count),
          stretchY: read(value.stretch_y, value.stretch_y_count),
        );
      },
    );
  }

  /// Removes one runtime style image.
  ///
  /// The command fails with [MaplibreStatus.notFound] when no runtime style
  /// image has [imageId].
  Future<CommandCompletion> removeStyleImage(String imageId) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(imageId, arena);
      _check(
        raw.mln_map_remove_style_image(_handle.raw, nativeId.value, completion),
      );
    });
  }

  /// Copies fixed metadata for one runtime style image.
  Future<StyleImageInfo?> getStyleImageInfo(String imageId) {
    return _startStyleImage(imageId).then((image) => image?.info);
  }

  Future<StyleImage?> _startStyleImage(String imageId) => _startMapValue(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE,
    elementSize: sizeOf<raw.mln_style_image_result>(),
    start: (completion) => withNativeArena((arena) {
      final nativeId = nativeStringView(imageId, arena);
      return raw.mln_map_get_style_image_info(
        _handle.raw,
        nativeId.value,
        completion,
      );
    }),
    decode: (result) {
      if (result.value_count == 0) return null;
      final value = result.value.cast<raw.mln_style_image_result>().ref;
      return StyleImage(
        info: _styleImageInfoFromNative(value.info),
        bytes: _copyBufferView(value.pixels),
      );
    },
  );

  /// Copies one runtime style image as premultiplied RGBA8 pixels.
  Future<StyleImage?> copyStyleImagePremultipliedRgba8(String imageId) =>
      _startStyleImage(imageId);

  Future<String> _copyLayerText(
    String layerId,
    int Function(int, raw.mln_buffer_view, Pointer<raw.mln_completion>) start,
  ) => _startMapValue(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEWS,
    elementSize: sizeOf<raw.mln_buffer_view>(),
    start: (completion) => withNativeArena((arena) {
      final nativeId = nativeStringView(layerId, arena);
      return start(_handle.raw, nativeId.value, completion);
    }),
    decode: (result) => utf8.decode(
      _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
    ),
  );

  /// Adds one style source from a style-spec source JSON object.
  ///
  /// Resolves with the command's terminal disposition and snapshot generation.
  Future<CommandCompletion> addStyleSourceJson(
    String sourceId,
    Uint8List sourceJson,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeSourceJson = nativeBufferView(sourceJson, arena);
      _check(
        raw.mln_map_add_style_source_json(
          _handle.raw,
          nativeId.value,
          nativeSourceJson,
          completion,
        ),
      );
    });
  }

  /// Adds a GeoJSON source that loads from [url].
  Future<CommandCompletion> addGeoJsonSourceUrl(
    String sourceId,
    String url, {
    GeoJsonSourceOptions? options,
  }) {
    return _startCommandInArena((arena, completion) {
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
          completion,
        ),
      );
    });
  }

  /// Adds a GeoJSON source with prepared inline [data].
  ///
  /// The command borrows [data]'s handle before returning and the source
  /// retains the prepared index, adopting the options the data was prepared
  /// with, fixed for the lifetime of the source, so the data may be closed
  /// as soon as this returns.
  Future<CommandCompletion> addGeoJsonSourceData(
    String sourceId,
    GeoJsonSourceDataHandle data,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_add_geojson_source_data(
          _handle.raw,
          nativeId.value,
          data._state.handle.raw,
          completion,
        ),
      );
    });
  }

  /// Updates one GeoJSON source to load from [url].
  Future<CommandCompletion> setGeoJsonSourceUrl(String sourceId, String url) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_set_geojson_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          completion,
        ),
      );
    });
  }

  /// Updates one GeoJSON source with prepared inline [data].
  ///
  /// The command borrows [data]'s handle before returning. Installing is
  /// cheap: the heavy parse and tiling already ran in
  /// [GeoJsonSourceDataHandle.prepare]. Data whose baked-in options differ
  /// from the source's fails the command as an invalid argument, with cluster
  /// properties excepted from the comparison.
  Future<CommandCompletion> setGeoJsonSourceData(
    String sourceId,
    GeoJsonSourceDataHandle data,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_set_geojson_source_data(
          _handle.raw,
          nativeId.value,
          data._state.handle.raw,
          completion,
        ),
      );
    });
  }

  /// Overrides one GeoJSON source's synchronous-tiling behavior at runtime.
  ///
  /// Tiles are sliced inline during the update pass when either the source's
  /// baked-in [GeoJsonSourceOptions.synchronousTiling] option or this override
  /// enables it.
  Future<CommandCompletion> setGeoJsonSourceSynchronousTiling(
    String sourceId,
    bool enabled,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_set_geojson_source_synchronous_tiling(
          _handle.raw,
          nativeId.value,
          enabled,
          completion,
        ),
      );
    });
  }

  /// Adds a vector source with a TileJSON URL.
  Future<CommandCompletion> addVectorSourceUrl(
    String sourceId,
    String url, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_add_vector_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          _nativeTileSourceOptions(options, arena),
          completion,
        ),
      );
    });
  }

  /// Adds a vector source with inline tile URL templates.
  Future<CommandCompletion> addVectorSourceTiles(
    String sourceId,
    List<String> tiles, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_add_vector_source_tiles(
          _handle.raw,
          nativeId.value,
          _stringViewArray(tiles, arena),
          tiles.length,
          _nativeTileSourceOptions(options, arena),
          completion,
        ),
      );
    });
  }

  /// Adds a raster source with a TileJSON URL.
  Future<CommandCompletion> addRasterSourceUrl(
    String sourceId,
    String url, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_add_raster_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          _nativeTileSourceOptions(options, arena),
          completion,
        ),
      );
    });
  }

  /// Adds a raster source with inline tile URL templates.
  Future<CommandCompletion> addRasterSourceTiles(
    String sourceId,
    List<String> tiles, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_add_raster_source_tiles(
          _handle.raw,
          nativeId.value,
          _stringViewArray(tiles, arena),
          tiles.length,
          _nativeTileSourceOptions(options, arena),
          completion,
        ),
      );
    });
  }

  /// Adds a raster DEM source with a TileJSON URL.
  Future<CommandCompletion> addRasterDemSourceUrl(
    String sourceId,
    String url, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_add_raster_dem_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          _nativeTileSourceOptions(options, arena),
          completion,
        ),
      );
    });
  }

  /// Adds a raster DEM source with inline tile URL templates.
  Future<CommandCompletion> addRasterDemSourceTiles(
    String sourceId,
    List<String> tiles, {
    TileSourceOptions options = const TileSourceOptions(),
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_add_raster_dem_source_tiles(
          _handle.raw,
          nativeId.value,
          _stringViewArray(tiles, arena),
          tiles.length,
          _nativeTileSourceOptions(options, arena),
          completion,
        ),
      );
    });
  }

  /// Adds an image source that loads its image from [url].
  Future<CommandCompletion> addImageSourceUrl(
    String sourceId,
    List<LatLng> coordinates,
    String url,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_add_image_source_url(
          _handle.raw,
          nativeId.value,
          _latLngArray(coordinates, arena),
          coordinates.length,
          nativeUrl.value,
          completion,
        ),
      );
    });
  }

  /// Adds an image source with inline image pixels.
  Future<CommandCompletion> addImageSourceImage(
    String sourceId,
    List<LatLng> coordinates,
    PremultipliedRgba8Image image,
  ) {
    return _startCommandInArena((arena, completion) {
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
          completion,
        ),
      );
    });
  }

  /// Updates an image source to load from [url].
  Future<CommandCompletion> setImageSourceUrl(String sourceId, String url) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeUrl = nativeStringView(url, arena);
      _check(
        raw.mln_map_set_image_source_url(
          _handle.raw,
          nativeId.value,
          nativeUrl.value,
          completion,
        ),
      );
    });
  }

  /// Updates an image source with inline image pixels.
  Future<CommandCompletion> setImageSourceImage(
    String sourceId,
    PremultipliedRgba8Image image,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeImage = arena<raw.mln_premultiplied_rgba8_image>();
      nativeImage.ref = _premultipliedRgba8ImageToNative(image, arena);
      _check(
        raw.mln_map_set_image_source_image(
          _handle.raw,
          nativeId.value,
          nativeImage,
          completion,
        ),
      );
    });
  }

  /// Updates image source coordinates.
  Future<CommandCompletion> setImageSourceCoordinates(
    String sourceId,
    List<LatLng> coordinates,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_set_image_source_coordinates(
          _handle.raw,
          nativeId.value,
          _latLngArray(coordinates, arena),
          coordinates.length,
          completion,
        ),
      );
    });
  }

  /// Copies image source coordinates, or null when the source is missing.
  Future<List<LatLng>?> getImageSourceCoordinates(String sourceId) {
    return _startMapValue(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: sizeOf<raw.mln_lat_lng>(),
      start: (completion) => withNativeArena((arena) {
        final nativeId = nativeStringView(sourceId, arena);
        return raw.mln_map_get_image_source_coordinates(
          _handle.raw,
          nativeId.value,
          completion,
        );
      }),
      decode: (result) => result.value_count == 0
          ? null
          : [
              for (var index = 0; index < result.value_count; index += 1)
                native_struct.latLngFromNative(
                  result.value.cast<raw.mln_lat_lng>()[index],
                ),
            ],
    );
  }

  /// Adds a custom geometry source with queued fetch/cancel notifications.
  ///
  /// The C API releases the source's callback root when the source is removed,
  /// when a style load drops it, or when this map is destroyed, so a host that
  /// adds a source subscribes to nothing to keep it alive.
  Future<CommandCompletion> addCustomGeometrySource(
    String sourceId,
    CustomGeometrySourceOptions options,
  ) {
    final callbackState = _CustomGeometryCallbackState(
      options,
      () => _releaseCustomGeometryCallbacks(sourceId),
    );
    final future = _runtime._startCommand(
      (completion) => withNativeArena((arena) {
        final nativeId = nativeStringView(sourceId, arena);
        final nativeOptions = arena<raw.mln_custom_geometry_source_options>();
        nativeOptions.ref = _customGeometrySourceOptionsToNative(
          options,
          callbackState,
        );
        return raw.mln_map_add_custom_geometry_source(
          _handle.raw,
          nativeId.value,
          nativeOptions,
          completion,
        );
      }),
      onRejected: callbackState.close,
    );
    future.then((completion) {
      if (completion.disposition == CommandDisposition.committed) {
        _customGeometryCallbacks[sourceId] = callbackState;
      } else {
        callbackState.close();
      }
    }, onError: (_, _) => callbackState.close());
    return future;
  }

  /// Sets custom geometry source data for one canonical tile.
  Future<CommandCompletion> setCustomGeometrySourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Uint8List data,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeData = nativeBufferView(data, arena);
      _check(
        raw.mln_map_set_custom_geometry_source_tile_data(
          _handle.raw,
          nativeId.value,
          _canonicalTileIdToNative(tileId),
          nativeData,
          completion,
        ),
      );
    });
  }

  /// Invalidates custom geometry source data for one canonical tile.
  Future<CommandCompletion> invalidateCustomGeometrySourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_invalidate_custom_geometry_source_tile(
          _handle.raw,
          nativeId.value,
          _canonicalTileIdToNative(tileId),
          completion,
        ),
      );
    });
  }

  /// Invalidates custom geometry source data inside one geographic region.
  Future<CommandCompletion> invalidateCustomGeometrySourceRegion(
    String sourceId,
    LatLngBounds bounds,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_invalidate_custom_geometry_source_region(
          _handle.raw,
          nativeId.value,
          native_struct.latLngBoundsToNative(bounds),
          completion,
        ),
      );
    });
  }

  /// Adds a custom MVT vector source with queued fetch/cancel notifications.
  ///
  /// The C API releases the source's callback root when the source is removed,
  /// when a style load drops it, or when this map is destroyed, so a host that
  /// adds a source subscribes to nothing to keep it alive.
  Future<CommandCompletion> addCustomMvtVectorSource(
    String sourceId,
    CustomMvtVectorSourceOptions options,
  ) {
    final callbackState = _CustomMvtVectorCallbackState(
      options,
      () => _releaseCustomMvtVectorCallbacks(sourceId),
    );
    final future = _runtime._startCommand(
      (completion) => withNativeArena((arena) {
        final nativeId = nativeStringView(sourceId, arena);
        final nativeOptions = arena<raw.mln_custom_mvt_vector_source_options>();
        nativeOptions.ref = _customMvtVectorSourceOptionsToNative(
          options,
          callbackState,
        );
        return raw.mln_map_add_custom_mvt_vector_source(
          _handle.raw,
          nativeId.value,
          nativeOptions,
          completion,
        );
      }),
      onRejected: callbackState.close,
    );
    future.then((completion) {
      if (completion.disposition == CommandDisposition.committed) {
        _customMvtVectorCallbacks[sourceId] = callbackState;
      } else {
        callbackState.close();
      }
    }, onError: (_, _) => callbackState.close());
    return future;
  }

  /// Sets custom MVT vector source data for one canonical tile.
  Future<CommandCompletion> setCustomMvtVectorSourceTileData(
    String sourceId,
    CanonicalTileId tileId,
    Uint8List data,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeData = nativeBufferView(data, arena);
      _check(
        raw.mln_map_set_custom_mvt_vector_source_tile_data(
          _handle.raw,
          nativeId.value,
          _canonicalTileIdToNative(tileId),
          nativeData,
          completion,
        ),
      );
    });
  }

  /// Reports a custom MVT vector source error for one canonical tile.
  Future<CommandCompletion> setCustomMvtVectorSourceTileError(
    String sourceId,
    CanonicalTileId tileId,
    String message,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      final nativeMessage = nativeStringView(message, arena);
      _check(
        raw.mln_map_set_custom_mvt_vector_source_tile_error(
          _handle.raw,
          nativeId.value,
          _canonicalTileIdToNative(tileId),
          nativeMessage.value,
          completion,
        ),
      );
    });
  }

  /// Invalidates custom MVT vector source data for one canonical tile.
  Future<CommandCompletion> invalidateCustomMvtVectorSourceTile(
    String sourceId,
    CanonicalTileId tileId,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_invalidate_custom_mvt_vector_source_tile(
          _handle.raw,
          nativeId.value,
          _canonicalTileIdToNative(tileId),
          completion,
        ),
      );
    });
  }

  /// Sets whether one style source stores fetched tiles in the persistent
  /// cache.
  ///
  /// When [isVolatile] is true, source implementations that fetch tiles stop
  /// storing them in persistent storage, and other source types keep the value
  /// for inspection. [getStyleSourceInfo] reports the committed value as
  /// [SourceInfo.isVolatile]. The command fails with
  /// [MaplibreStatus.notFound] when no style source has [sourceId].
  Future<CommandCompletion> setStyleSourceVolatile(
    String sourceId,
    bool isVolatile,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_set_style_source_volatile(
          _handle.raw,
          nativeId.value,
          isVolatile,
          completion,
        ),
      );
    });
  }

  /// Removes one style source by ID.
  ///
  /// The command fails with [MaplibreStatus.notFound] when no style source
  /// has [sourceId], and with [MaplibreStatus.invalidState] when a layer
  /// still uses the source.
  Future<CommandCompletion> removeStyleSource(String sourceId) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_remove_style_source(
          _handle.raw,
          nativeId.value,
          completion,
        ),
      );
    });
  }

  /// Copies fixed style source metadata after prior commands.
  Future<SourceInfo?> getStyleSourceInfo(String sourceId) => _startMapValue(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE,
    elementSize: sizeOf<raw.mln_style_source_result>(),
    start: (completion) => withNativeArena((arena) {
      final nativeId = nativeStringView(sourceId, arena);
      return raw.mln_map_get_style_source_info(
        _handle.raw,
        nativeId.value,
        completion,
      );
    }),
    decode: (result) {
      if (result.value_count == 0) return null;
      final value = result.value.cast<raw.mln_style_source_result>().ref;
      final info = value.info;
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
      return SourceInfo(
        type: SourceType.fromRaw(info.type),
        id: sourceId,
        isVolatile: info.is_volatile,
        attribution: info.has_attribution
            ? utf8.decode(_copyBufferView(value.attribution))
            : null,
        url: hasUrl ? utf8.decode(_copyBufferView(value.url)) : null,
        tileJson: hasTileJson
            ? ParsedTileJson(
                tileUrls: [
                  for (var index = 0; index < value.tile_url_count; index += 1)
                    utf8.decode(_copyBufferView(value.tile_urls[index])),
                ],
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
    },
  );

  /// Copies style source IDs in style order after prior commands.
  Future<List<String>> listStyleSourceIds() => _startStringList(
    (completion) => raw.mln_map_list_style_source_ids(_handle.raw, completion),
  );

  Future<List<String>> _startStringList(NativeCompletionStart start) =>
      _startMapValue(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEWS,
        elementSize: sizeOf<raw.mln_buffer_view>(),
        start: start,
        decode: (result) => [
          for (var index = 0; index < result.value_count; index += 1)
            utf8.decode(
              _copyBufferView(result.value.cast<raw.mln_buffer_view>()[index]),
            ),
        ],
      );

  Future<Uint8List?> _startOptionalBuffer(NativeCompletionStart start) =>
      _startMapValue(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEWS,
        elementSize: sizeOf<raw.mln_buffer_view>(),
        start: start,
        decode: (result) => result.value_count == 0
            ? null
            : _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
      );

  /// Adds a hillshade layer for a raster DEM source.
  Future<CommandCompletion> addHillshadeLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceId = nativeStringView(sourceId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_add_hillshade_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceId.value,
          nativeBeforeLayerId.value,
          completion,
        ),
      );
    });
  }

  /// Adds a color-relief layer for a raster DEM source.
  Future<CommandCompletion> addColorReliefLayer(
    String layerId,
    String sourceId, {
    String? beforeLayerId,
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceId = nativeStringView(sourceId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_add_color_relief_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceId.value,
          nativeBeforeLayerId.value,
          completion,
        ),
      );
    });
  }

  /// Adds a source-free location indicator layer.
  Future<CommandCompletion> addLocationIndicatorLayer(
    String layerId, {
    String? beforeLayerId,
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_add_location_indicator_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeBeforeLayerId.value,
          completion,
        ),
      );
    });
  }

  /// Sets a location indicator layer location.
  Future<CommandCompletion> setLocationIndicatorLocation(
    String layerId,
    LatLng coordinate, {
    double altitude = 0,
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_location_indicator_location(
          _handle.raw,
          nativeLayerId.value,
          native_struct.latLngToNative(coordinate),
          altitude,
          completion,
        ),
      );
    });
  }

  /// Sets a location indicator layer bearing in degrees.
  Future<CommandCompletion> setLocationIndicatorBearing(
    String layerId,
    double bearing,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_location_indicator_bearing(
          _handle.raw,
          nativeLayerId.value,
          bearing,
          completion,
        ),
      );
    });
  }

  /// Sets a location indicator layer accuracy radius in meters.
  Future<CommandCompletion> setLocationIndicatorAccuracyRadius(
    String layerId,
    double radius,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_location_indicator_accuracy_radius(
          _handle.raw,
          nativeLayerId.value,
          radius,
          completion,
        ),
      );
    });
  }

  /// Sets one location indicator image-name property.
  Future<CommandCompletion> setLocationIndicatorImageName(
    String layerId,
    LocationIndicatorImageKind imageKind,
    String imageId,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeImageId = nativeStringView(imageId, arena);
      _check(
        raw.mln_map_set_location_indicator_image_name(
          _handle.raw,
          nativeLayerId.value,
          imageKind.rawValue,
          nativeImageId.value,
          completion,
        ),
      );
    });
  }

  /// Adds one style layer from a full style-spec layer JSON object.
  Future<CommandCompletion> addStyleLayerJson(
    Uint8List layerJson, {
    String? beforeLayerId,
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerJson = nativeBufferView(layerJson, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_add_style_layer_json(
          _handle.raw,
          nativeLayerJson,
          nativeBeforeLayerId.value,
          completion,
        ),
      );
    });
  }

  /// Copies one style layer as a full style-spec layer JSON snapshot.
  Future<Uint8List?> getStyleLayerJson(String layerId) {
    return _startOptionalBuffer(
      (completion) => withNativeArena((arena) {
        final nativeId = nativeStringView(layerId, arena);
        return raw.mln_map_get_style_layer_json(
          _handle.raw,
          nativeId.value,
          completion,
        );
      }),
    );
  }

  /// Sets the style light from a style-spec light JSON object.
  Future<CommandCompletion> setStyleLightJson(Uint8List lightJson) {
    return _startCommandInArena((arena, completion) {
      final nativeLightJson = nativeBufferView(lightJson, arena);
      _check(
        raw.mln_map_set_style_light_json(
          _handle.raw,
          nativeLightJson,
          completion,
        ),
      );
    });
  }

  /// Sets one style light property by style-spec property name.
  Future<CommandCompletion> setStyleLightProperty(
    String propertyName,
    Uint8List value,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativePropertyName = nativeStringView(propertyName, arena);
      final nativeValue = nativeBufferView(value, arena);
      _check(
        raw.mln_map_set_style_light_property(
          _handle.raw,
          nativePropertyName.value,
          nativeValue,
          completion,
        ),
      );
    });
  }

  /// Copies one style light property, or null when the property is undefined.
  Future<Uint8List?> getStyleLightProperty(String propertyName) {
    return _startOptionalBuffer(
      (completion) => withNativeArena((arena) {
        final nativePropertyName = nativeStringView(propertyName, arena);
        return raw.mln_map_get_style_light_property(
          _handle.raw,
          nativePropertyName.value,
          completion,
        );
      }),
    );
  }

  /// Sets the style's global transition options.
  ///
  /// This replaces the whole transition configuration rather than merging into
  /// it, and loading a style replaces it again with the style's own options, so
  /// apply an override after the style loads.
  Future<CommandCompletion> setStyleTransitionOptions(
    StyleTransitionOptions options,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeOptions = arena<raw.mln_style_transition_options>();
      nativeOptions.ref = _styleTransitionOptionsToNative(options);
      _check(
        raw.mln_map_set_style_transition_options(
          _handle.raw,
          nativeOptions,
          completion,
        ),
      );
    });
  }

  /// Copies the style's global transition options.
  Future<StyleTransitionOptions> getStyleTransitionOptions() {
    return _startMapValue(
      copyKind:
          raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
      elementSize: sizeOf<raw.mln_style_transition_options>(),
      start: (completion) =>
          raw.mln_map_get_style_transition_options(_handle.raw, completion),
      decode: (result) => _styleTransitionOptionsFromNative(
        result.value.cast<raw.mln_style_transition_options>().ref,
      ),
    );
  }

  /// Sets one layer property by style-spec property name.
  Future<CommandCompletion> setLayerProperty(
    String layerId,
    String propertyName,
    Uint8List value,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativePropertyName = nativeStringView(propertyName, arena);
      final nativeValue = nativeBufferView(value, arena);
      _check(
        raw.mln_map_set_layer_property(
          _handle.raw,
          nativeLayerId.value,
          nativePropertyName.value,
          nativeValue,
          completion,
        ),
      );
    });
  }

  /// Copies one layer property, or null when the property is undefined.
  Future<Uint8List?> getLayerProperty(String layerId, String propertyName) {
    return _startOptionalBuffer(
      (completion) => withNativeArena((arena) {
        final nativeLayerId = nativeStringView(layerId, arena);
        final nativePropertyName = nativeStringView(propertyName, arena);
        return raw.mln_map_get_layer_property(
          _handle.raw,
          nativeLayerId.value,
          nativePropertyName.value,
          completion,
        );
      }),
    );
  }

  /// Sets or clears one layer filter.
  Future<CommandCompletion> setLayerFilter(String layerId, Uint8List? filter) {
    return _startCommandInArena((arena, completion) {
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
          completion,
        ),
      );
    });
  }

  /// Copies one layer filter, or null when the layer has no filter.
  Future<Uint8List?> getLayerFilter(String layerId) {
    return _startOptionalBuffer(
      (completion) => withNativeArena((arena) {
        final nativeLayerId = nativeStringView(layerId, arena);
        return raw.mln_map_get_layer_filter(
          _handle.raw,
          nativeLayerId.value,
          completion,
        );
      }),
    );
  }

  /// Sets one layer's source-layer ID.
  ///
  /// Layer types that take no source, such as background, are rejected.
  Future<CommandCompletion> setLayerSourceLayer(
    String layerId,
    String sourceLayer,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceLayer = nativeStringView(sourceLayer, arena);
      _check(
        raw.mln_map_set_layer_source_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceLayer.value,
          completion,
        ),
      );
    });
  }

  /// Copies one layer's source-layer ID, empty when the layer carries none.
  Future<String> getLayerSourceLayer(String layerId) {
    return _copyLayerText(layerId, raw.mln_map_copy_layer_source_layer);
  }

  /// Sets one layer's source ID.
  ///
  /// Layer types that take no source, such as background, are rejected. The
  /// named source need not exist yet.
  Future<CommandCompletion> setLayerSourceId(String layerId, String sourceId) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeSourceId = nativeStringView(sourceId, arena);
      _check(
        raw.mln_map_set_layer_source_id(
          _handle.raw,
          nativeLayerId.value,
          nativeSourceId.value,
          completion,
        ),
      );
    });
  }

  /// Copies one layer's source ID, empty when the layer carries none.
  Future<String> getLayerSourceId(String layerId) {
    return _copyLayerText(layerId, raw.mln_map_copy_layer_source_id);
  }

  /// Sets the lowest zoom at which one layer draws.
  ///
  /// Pass `double.negativeInfinity` for no lower bound.
  Future<CommandCompletion> setLayerMinZoom(String layerId, double minZoom) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_layer_min_zoom(
          _handle.raw,
          nativeLayerId.value,
          minZoom,
          completion,
        ),
      );
    });
  }

  /// Sets the highest zoom at which one layer draws.
  ///
  /// Pass `double.infinity` for no upper bound.
  Future<CommandCompletion> setLayerMaxZoom(String layerId, double maxZoom) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_layer_max_zoom(
          _handle.raw,
          nativeLayerId.value,
          maxZoom,
          completion,
        ),
      );
    });
  }

  /// Sets whether one layer draws.
  Future<CommandCompletion> setLayerVisibility(
    String layerId,
    StyleLayerVisibility visibility,
  ) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_set_layer_visibility(
          _handle.raw,
          nativeLayerId.value,
          visibility.rawValue,
          completion,
        ),
      );
    });
  }

  /// Copies fixed style layer metadata, or null when no layer has [layerId].
  Future<LayerInfo?> getStyleLayerInfo(String layerId) => _startMapValue(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_STYLE_LAYER,
    elementSize: sizeOf<raw.mln_style_layer_result>(),
    start: (completion) => withNativeArena((arena) {
      final nativeId = nativeStringView(layerId, arena);
      return raw.mln_map_get_style_layer_info(
        _handle.raw,
        nativeId.value,
        completion,
      );
    }),
    decode: (result) {
      if (result.value_count == 0) return null;
      final value = result.value.cast<raw.mln_style_layer_result>().ref;
      final info = value.info;
      final hasSourceId =
          info.fields &
              raw
                  .mln_style_layer_info_field
                  .MLN_STYLE_LAYER_INFO_SOURCE_ID
                  .value !=
          0;
      final hasSourceLayer =
          info.fields &
              raw
                  .mln_style_layer_info_field
                  .MLN_STYLE_LAYER_INFO_SOURCE_LAYER
                  .value !=
          0;
      return LayerInfo(
        type: _copyStringView(info.type) ?? '',
        minZoom: info.min_zoom,
        maxZoom: info.max_zoom,
        visibility: StyleLayerVisibility.fromRawValue(info.visibility),
        sourceId: hasSourceId
            ? utf8.decode(_copyBufferView(value.source_id))
            : null,
        sourceLayer: hasSourceLayer
            ? utf8.decode(_copyBufferView(value.source_layer))
            : null,
      );
    },
  );

  /// Moves one style layer before another layer or to the top.
  Future<CommandCompletion> moveStyleLayer(
    String layerId, {
    String? beforeLayerId,
  }) {
    return _startCommandInArena((arena, completion) {
      final nativeLayerId = nativeStringView(layerId, arena);
      final nativeBeforeLayerId = nativeStringView(beforeLayerId ?? '', arena);
      _check(
        raw.mln_map_move_style_layer(
          _handle.raw,
          nativeLayerId.value,
          nativeBeforeLayerId.value,
          completion,
        ),
      );
    });
  }

  /// Removes one style layer by ID.
  ///
  /// The command fails with [MaplibreStatus.notFound] when no style layer has
  /// [layerId].
  Future<CommandCompletion> removeStyleLayer(String layerId) {
    return _startCommandInArena((arena, completion) {
      final nativeId = nativeStringView(layerId, arena);
      _check(
        raw.mln_map_remove_style_layer(_handle.raw, nativeId.value, completion),
      );
    });
  }

  /// Copies style layer IDs in style order.
  Future<List<String>> listStyleLayerIds() {
    return _startStringList(
      (completion) => raw.mln_map_list_style_layer_ids(_handle.raw, completion),
    );
  }

  /// Releases this map's public native handle.
  ///
  /// Callback roots remain alive until native teardown releases every
  /// custom-geometry source that the map still owns.
  Future<void> close() async {
    final id = _state.handleId;
    await _state.closeAsync((handle) async {
      await _runtime._startUnit(
        (completion) => raw.mln_map_release(handle.raw, completion),
      );
      // Native release callbacks are listener callbacks. Yield on this isolate
      // before retiring any root whose release message has not run yet.
      await Future<void>.delayed(Duration.zero);
      for (final state in _customGeometryCallbacks.values.toList()) {
        state._retire();
      }
      await Future<void>.delayed(Duration.zero);
    });
    _runtime._unregisterMapId(id);
  }

  void _releaseCustomGeometryCallbacks(String sourceId) {
    _customGeometryCallbacks.remove(sourceId);
  }

  /// Drops [sourceId]'s callback root once the C API has released it.
  void _releaseCustomMvtVectorCallbacks(String sourceId) {
    _customMvtVectorCallbacks.remove(sourceId);
  }
}
