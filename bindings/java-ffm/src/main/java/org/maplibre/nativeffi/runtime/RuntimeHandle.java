package org.maplibre.nativeffi.runtime;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.lang.ref.WeakReference;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.internal.access.InternalAccess;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_runtime_event;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_operation_completed;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_response_error;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_status;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_tile_count_limit;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_frame;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_map;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_style_image_missing;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_tile_action;
import org.maplibre.nativeffi.internal.callback.ResourceTransformState;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.internal.lifecycle.HandleState;
import org.maplibre.nativeffi.internal.loader.NativeAccess;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.internal.struct.RuntimeStructs;
import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.offline.OfflineRegionDefinition;
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState;
import org.maplibre.nativeffi.offline.OfflineRegionInfo;
import org.maplibre.nativeffi.offline.OfflineRegionStatus;
import org.maplibre.nativeffi.resource.ResourceProviderCallback;
import org.maplibre.nativeffi.resource.ResourceTransformCallback;

/** Owned native runtime handle. Close it on the owner thread. */
public final class RuntimeHandle implements AutoCloseable {
  private final HandleState state;
  private final ConcurrentHashMap<Long, WeakReference<MapHandle>> liveMaps =
      new ConcurrentHashMap<>();
  private ResourceTransformState resourceTransformState;
  private ResourceProviderState resourceProviderState;

  private RuntimeHandle(MemorySegment handle) {
    this.state = new HandleState("RuntimeHandle", handle);
  }

  public static RuntimeHandle create(RuntimeOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      var outRuntime = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_runtime_create(
              RuntimeStructs.runtimeOptions(options, arena), outRuntime));
      return new RuntimeHandle(outRuntime.get(ValueLayout.ADDRESS, 0));
    }
  }

  public void runOnce() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_runtime_run_once(state.requireLive()));
  }

  private <T> OfflineOperationHandle<T> offlineOperation(
      long operationId, OfflineOperationKind kind, OfflineOperationResultKind resultKind) {
    return new OfflineOperationHandle<>(this, operationId, kind, resultKind);
  }

  public OfflineOperationHandle<Void> startAmbientCacheOperation(AmbientCacheOperation operation) {
    NativeAccess.ensureLoaded();
    var runtime = state.requireLive();
    try (var arena = Arena.ofConfined()) {
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_run_ambient_cache_operation_start(
              runtime,
              NativeValues.nativeValue(Objects.requireNonNull(operation, "operation")),
              outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.AMBIENT_CACHE,
          OfflineOperationResultKind.NONE);
    }
  }

  public OfflineOperationHandle<OfflineRegionInfo> startCreateOfflineRegion(
      OfflineRegionDefinition definition, byte[] metadata) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(definition, "definition");
    Objects.requireNonNull(metadata, "metadata");
    try (var arena = Arena.ofConfined()) {
      var runtime = state.requireLive();
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_offline_region_create_start(
              runtime,
              RuntimeStructs.offlineRegionDefinition(definition, arena),
              RuntimeStructs.metadata(metadata, arena),
              metadata.length,
              outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGION_CREATE,
          OfflineOperationResultKind.REGION);
    }
  }

  public OfflineOperationHandle<Optional<OfflineRegionInfo>> startOfflineRegion(long id) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var runtime = state.requireLive();
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_offline_region_get_start(runtime, id, outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGION_GET,
          OfflineOperationResultKind.OPTIONAL_REGION);
    }
  }

  public OfflineOperationHandle<List<OfflineRegionInfo>> startOfflineRegions() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var runtime = state.requireLive();
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(MapLibreNativeC.mln_runtime_offline_regions_list_start(runtime, outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGIONS_LIST,
          OfflineOperationResultKind.REGION_LIST);
    }
  }

  public OfflineOperationHandle<List<OfflineRegionInfo>> startMergeOfflineRegionsDatabase(
      String path) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var runtime = state.requireLive();
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_offline_regions_merge_database_start(
              runtime,
              MemoryUtil.allocateCString(arena, Objects.requireNonNull(path, "path")),
              outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGIONS_MERGE_DATABASE,
          OfflineOperationResultKind.REGION_LIST);
    }
  }

  public OfflineOperationHandle<OfflineRegionInfo> startUpdateOfflineRegionMetadata(
      long id, byte[] metadata) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(metadata, "metadata");
    try (var arena = Arena.ofConfined()) {
      var runtime = state.requireLive();
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_offline_region_update_metadata_start(
              runtime,
              id,
              RuntimeStructs.metadata(metadata, arena),
              metadata.length,
              outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGION_UPDATE_METADATA,
          OfflineOperationResultKind.REGION);
    }
  }

  public OfflineOperationHandle<OfflineRegionStatus> startOfflineRegionStatus(long id) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var runtime = state.requireLive();
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_offline_region_get_status_start(runtime, id, outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGION_GET_STATUS,
          OfflineOperationResultKind.REGION_STATUS);
    }
  }

  public OfflineOperationHandle<Void> startSetOfflineRegionObserved(long id, boolean observed) {
    NativeAccess.ensureLoaded();
    var runtime = state.requireLive();
    try (var arena = Arena.ofConfined()) {
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_offline_region_set_observed_start(
              runtime, id, observed, outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGION_SET_OBSERVED,
          OfflineOperationResultKind.NONE);
    }
  }

  public OfflineOperationHandle<Void> startSetOfflineRegionDownloadState(
      long id, OfflineRegionDownloadState downloadState) {
    NativeAccess.ensureLoaded();
    var stateValue =
        NativeValues.nativeValue(Objects.requireNonNull(downloadState, "downloadState"));
    var runtime = state.requireLive();
    try (var arena = Arena.ofConfined()) {
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_offline_region_set_download_state_start(
              runtime, id, stateValue, outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGION_SET_DOWNLOAD_STATE,
          OfflineOperationResultKind.NONE);
    }
  }

  public OfflineOperationHandle<Void> startInvalidateOfflineRegion(long id) {
    NativeAccess.ensureLoaded();
    var runtime = state.requireLive();
    try (var arena = Arena.ofConfined()) {
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_offline_region_invalidate_start(runtime, id, outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGION_INVALIDATE,
          OfflineOperationResultKind.NONE);
    }
  }

  public OfflineOperationHandle<Void> startDeleteOfflineRegion(long id) {
    NativeAccess.ensureLoaded();
    var runtime = state.requireLive();
    try (var arena = Arena.ofConfined()) {
      var outOperationId = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(
          MapLibreNativeC.mln_runtime_offline_region_delete_start(runtime, id, outOperationId));
      return offlineOperation(
          outOperationId.get(ValueLayout.JAVA_LONG, 0),
          OfflineOperationKind.REGION_DELETE,
          OfflineOperationResultKind.NONE);
    }
  }

  public OfflineRegionInfo takeCreateOfflineRegionResult(
      OfflineOperationHandle<OfflineRegionInfo> operation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    try (var arena = Arena.ofConfined()) {
      var outRegion = MemoryUtil.allocatePointer(arena);
      var operationId =
          operation.requireLive(
              this, OfflineOperationKind.REGION_CREATE, OfflineOperationResultKind.REGION);
      var nativeStatus =
          MapLibreNativeC.mln_runtime_offline_region_create_take_result(
              state.requireLive(), operationId, outRegion);
      Status.check(nativeStatus);
      operation.markConsumed();
      return RuntimeStructs.offlineRegionSnapshot(outRegion.get(ValueLayout.ADDRESS, 0));
    }
  }

  public Optional<OfflineRegionInfo> takeOfflineRegionResult(
      OfflineOperationHandle<Optional<OfflineRegionInfo>> operation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    try (var arena = Arena.ofConfined()) {
      var outRegion = MemoryUtil.allocatePointer(arena);
      var outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      var operationId =
          operation.requireLive(
              this, OfflineOperationKind.REGION_GET, OfflineOperationResultKind.OPTIONAL_REGION);
      var nativeStatus =
          MapLibreNativeC.mln_runtime_offline_region_get_take_result(
              state.requireLive(), operationId, outRegion, outFound);
      Status.check(nativeStatus);
      operation.markConsumed();
      if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        return Optional.empty();
      }
      return Optional.of(
          RuntimeStructs.offlineRegionSnapshot(outRegion.get(ValueLayout.ADDRESS, 0)));
    }
  }

  public List<OfflineRegionInfo> takeOfflineRegionsResult(
      OfflineOperationHandle<List<OfflineRegionInfo>> operation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    try (var arena = Arena.ofConfined()) {
      var outRegions = MemoryUtil.allocatePointer(arena);
      var operationId =
          operation.requireLive(
              this, OfflineOperationKind.REGIONS_LIST, OfflineOperationResultKind.REGION_LIST);
      var nativeStatus =
          MapLibreNativeC.mln_runtime_offline_regions_list_take_result(
              state.requireLive(), operationId, outRegions);
      Status.check(nativeStatus);
      operation.markConsumed();
      return RuntimeStructs.offlineRegionList(outRegions.get(ValueLayout.ADDRESS, 0));
    }
  }

  public List<OfflineRegionInfo> takeMergeOfflineRegionsDatabaseResult(
      OfflineOperationHandle<List<OfflineRegionInfo>> operation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    try (var arena = Arena.ofConfined()) {
      var outRegions = MemoryUtil.allocatePointer(arena);
      var operationId =
          operation.requireLive(
              this,
              OfflineOperationKind.REGIONS_MERGE_DATABASE,
              OfflineOperationResultKind.REGION_LIST);
      var nativeStatus =
          MapLibreNativeC.mln_runtime_offline_regions_merge_database_take_result(
              state.requireLive(), operationId, outRegions);
      Status.check(nativeStatus);
      operation.markConsumed();
      return RuntimeStructs.offlineRegionList(outRegions.get(ValueLayout.ADDRESS, 0));
    }
  }

  public OfflineRegionInfo takeUpdateOfflineRegionMetadataResult(
      OfflineOperationHandle<OfflineRegionInfo> operation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    try (var arena = Arena.ofConfined()) {
      var outRegion = MemoryUtil.allocatePointer(arena);
      var operationId =
          operation.requireLive(
              this, OfflineOperationKind.REGION_UPDATE_METADATA, OfflineOperationResultKind.REGION);
      var nativeStatus =
          MapLibreNativeC.mln_runtime_offline_region_update_metadata_take_result(
              state.requireLive(), operationId, outRegion);
      Status.check(nativeStatus);
      operation.markConsumed();
      return RuntimeStructs.offlineRegionSnapshot(outRegion.get(ValueLayout.ADDRESS, 0));
    }
  }

  public OfflineRegionStatus takeOfflineRegionStatusResult(
      OfflineOperationHandle<OfflineRegionStatus> operation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    try (var arena = Arena.ofConfined()) {
      var status = RuntimeStructs.offlineRegionStatus(arena);
      var operationId =
          operation.requireLive(
              this,
              OfflineOperationKind.REGION_GET_STATUS,
              OfflineOperationResultKind.REGION_STATUS);
      var nativeStatus =
          MapLibreNativeC.mln_runtime_offline_region_get_status_take_result(
              state.requireLive(), operationId, status);
      Status.check(nativeStatus);
      operation.markConsumed();
      return RuntimeStructs.offlineRegionStatus(status);
    }
  }

  void discardOfflineOperation(OfflineOperationHandle<?> operation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    if (operation.isClosed()) {
      return;
    }
    var operationId = operation.requireLive(this);
    MemorySegment runtime;
    try {
      runtime = state.requireLive();
    } catch (InvalidStateException error) {
      operation.markConsumed();
      throw error;
    }
    Status.check(MapLibreNativeC.mln_runtime_offline_operation_discard(runtime, operationId));
    operation.markConsumed();
  }

  public void setResourceTransform(ResourceTransformCallback callback) {
    NativeAccess.ensureLoaded();
    var replacement = new ResourceTransformState(Objects.requireNonNull(callback, "callback"));
    ResourceTransformState previous;
    try {
      if (resourceTransformState != null && resourceTransformState.isCurrentThreadInCallback()) {
        throw Status.callbackReentry("Resource transform");
      }
      Status.check(
          MapLibreNativeC.mln_runtime_set_resource_transform(
              state.requireLive(), replacement.descriptor()));
      previous = resourceTransformState;
      resourceTransformState = replacement;
    } catch (RuntimeException | Error error) {
      closeQuietly(replacement);
      throw error;
    }
    closeQuietly(previous);
  }

  public void clearResourceTransform() {
    NativeAccess.ensureLoaded();
    if (resourceTransformState != null && resourceTransformState.isCurrentThreadInCallback()) {
      throw Status.callbackReentry("Resource transform");
    }
    Status.check(MapLibreNativeC.mln_runtime_clear_resource_transform(state.requireLive()));
    var previous = resourceTransformState;
    resourceTransformState = null;
    closeQuietly(previous);
  }

  public void setResourceProvider(ResourceProviderCallback callback) {
    NativeAccess.ensureLoaded();
    var replacement = new ResourceProviderState(Objects.requireNonNull(callback, "callback"));
    ResourceProviderState previous;
    try {
      if (resourceProviderState != null && resourceProviderState.isCurrentThreadInCallback()) {
        throw Status.callbackReentry("Resource provider");
      }
      Status.check(
          MapLibreNativeC.mln_runtime_set_resource_provider(
              state.requireLive(), replacement.descriptor()));
      previous = resourceProviderState;
      resourceProviderState = replacement;
    } catch (RuntimeException | Error error) {
      closeQuietly(replacement);
      throw error;
    }
    closeQuietly(previous);
  }

  public Optional<RuntimeEvent> pollEvent() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var event = mln_runtime_event.allocate(arena);
      mln_runtime_event.size(event, (int) mln_runtime_event.sizeof());
      var hasEvent = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(MapLibreNativeC.mln_runtime_poll_event(state.requireLive(), event, hasEvent));
      if (!hasEvent.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        return Optional.empty();
      }
      return Optional.of(readEvent(event));
    }
  }

  @Override
  public void close() {
    NativeAccess.ensureLoaded();
    state.closeOnce(MapLibreNativeC::mln_runtime_destroy, this::closeResourceCallbacks);
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public MemorySegment nativeHandle(InternalAccess access) {
    Objects.requireNonNull(access, "access").checkCaller();
    return nativeHandle();
  }

  MemorySegment nativeHandle() {
    return state.requireLive();
  }

  public long nativeAddress(InternalAccess access) {
    Objects.requireNonNull(access, "access").checkCaller();
    return nativeAddress();
  }

  long nativeAddress() {
    return state.address();
  }

  public void registerMap(InternalAccess access, MapHandle map) {
    Objects.requireNonNull(access, "access").checkCaller();
    registerMap(map);
  }

  void registerMap(MapHandle map) {
    liveMaps.put(map.nativeAddress(InternalAccess.INSTANCE), new WeakReference<>(map));
  }

  public void unregisterMap(InternalAccess access, MapHandle map) {
    Objects.requireNonNull(access, "access").checkCaller();
    unregisterMap(map);
  }

  void unregisterMap(MapHandle map) {
    liveMaps.computeIfPresent(
        map.nativeAddress(InternalAccess.INSTANCE),
        (address, reference) -> reference.get() == map ? null : reference);
  }

  private MapHandle mapFor(MemorySegment source) {
    if (MemoryUtil.isNull(source)) {
      return null;
    }
    var address = source.address();
    var reference = liveMaps.get(address);
    var map = reference == null ? null : reference.get();
    if (reference != null && map == null) {
      liveMaps.remove(address, reference);
    }
    return map;
  }

  private RuntimeEvent readEvent(MemorySegment event) {
    var rawType = mln_runtime_event.type(event);
    var rawSourceType = mln_runtime_event.source_type(event);
    var source = mln_runtime_event.source(event);
    var sourceType = NativeValues.runtimeEventSourceType(rawSourceType);
    var runtimeSource =
        RuntimeEventSourceType.RUNTIME.equals(sourceType)
            ? Optional.of(this)
            : Optional.<RuntimeHandle>empty();
    var mapSource =
        RuntimeEventSourceType.MAP.equals(sourceType)
            ? Optional.ofNullable(mapFor(source))
            : Optional.<MapHandle>empty();
    var rawPayloadType = mln_runtime_event.payload_type(event);
    var eventType = NativeValues.runtimeEventType(rawType);
    var copied =
        new RuntimeEvent(
            eventType,
            rawType,
            sourceType,
            rawSourceType,
            runtimeSource,
            mapSource,
            mln_runtime_event.code(event),
            rawPayloadType,
            readPayload(
                rawPayloadType,
                mln_runtime_event.payload(event),
                mln_runtime_event.payload_size(event)),
            MemoryUtil.copyStringView(
                mln_runtime_event.message(event), mln_runtime_event.message_size(event)));
    if (RuntimeEventType.MAP_STYLE_LOADED.equals(eventType)) {
      mapSource.ifPresent(map -> map.releaseDetachedCustomGeometrySources(InternalAccess.INSTANCE));
    }
    return copied;
  }

  private RuntimeEventPayload readPayload(
      int rawPayloadType, MemorySegment payload, long payloadSize) {
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_NONE()) {
      return RuntimeEventPayload.NONE;
    }
    if (MemoryUtil.isNull(payload) || payloadSize == 0) {
      return unknownPayload(rawPayloadType, payload, payloadSize);
    }

    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME()) {
      return payloadSize >= mln_runtime_event_render_frame.sizeof()
          ? readRenderFrame(payload)
          : unknownPayload(rawPayloadType, payload, payloadSize);
    }
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP()) {
      return payloadSize >= mln_runtime_event_render_map.sizeof()
          ? readRenderMap(payload)
          : unknownPayload(rawPayloadType, payload, payloadSize);
    }
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING()) {
      return payloadSize >= mln_runtime_event_style_image_missing.sizeof()
          ? readStyleImageMissing(payload)
          : unknownPayload(rawPayloadType, payload, payloadSize);
    }
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION()) {
      return payloadSize >= mln_runtime_event_tile_action.sizeof()
          ? readTileAction(payload)
          : unknownPayload(rawPayloadType, payload, payloadSize);
    }
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS()) {
      return payloadSize >= mln_runtime_event_offline_region_status.sizeof()
          ? readOfflineRegionStatus(payload)
          : unknownPayload(rawPayloadType, payload, payloadSize);
    }
    if (rawPayloadType
        == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR()) {
      return payloadSize >= mln_runtime_event_offline_region_response_error.sizeof()
          ? readOfflineRegionResponseError(payload)
          : unknownPayload(rawPayloadType, payload, payloadSize);
    }
    if (rawPayloadType
        == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT()) {
      return payloadSize >= mln_runtime_event_offline_region_tile_count_limit.sizeof()
          ? readOfflineRegionTileCountLimit(payload)
          : unknownPayload(rawPayloadType, payload, payloadSize);
    }
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED()) {
      return readOfflineOperationCompletedPayload(rawPayloadType, payload, payloadSize);
    }
    return unknownPayload(rawPayloadType, payload, payloadSize);
  }

  static RuntimeEventPayload.Unknown unknownPayload(
      int rawPayloadType, MemorySegment payload, long payloadSize) {
    return new RuntimeEventPayload.Unknown(
        rawPayloadType, payloadSize, MemoryUtil.copyBytes(payload, payloadSize));
  }

  private RuntimeEventPayload.RenderFrame readRenderFrame(MemorySegment payload) {
    var frame = payload.reinterpret(mln_runtime_event_render_frame.sizeof());
    var rawMode = mln_runtime_event_render_frame.mode(frame);
    return new RuntimeEventPayload.RenderFrame(
        NativeValues.renderMode(rawMode),
        rawMode,
        mln_runtime_event_render_frame.needs_repaint(frame),
        mln_runtime_event_render_frame.placement_changed(frame),
        RuntimeStructs.renderingStats(mln_runtime_event_render_frame.stats(frame)));
  }

  private RuntimeEventPayload.RenderMap readRenderMap(MemorySegment payload) {
    var renderMap = payload.reinterpret(mln_runtime_event_render_map.sizeof());
    var rawMode = mln_runtime_event_render_map.mode(renderMap);
    return new RuntimeEventPayload.RenderMap(NativeValues.renderMode(rawMode), rawMode);
  }

  private RuntimeEventPayload.StyleImageMissing readStyleImageMissing(MemorySegment payload) {
    var missing = payload.reinterpret(mln_runtime_event_style_image_missing.sizeof());
    return new RuntimeEventPayload.StyleImageMissing(
        MemoryUtil.copyStringView(
            mln_runtime_event_style_image_missing.image_id(missing),
            mln_runtime_event_style_image_missing.image_id_size(missing)));
  }

  private RuntimeEventPayload.TileAction readTileAction(MemorySegment payload) {
    var action = payload.reinterpret(mln_runtime_event_tile_action.sizeof());
    var rawOperation = mln_runtime_event_tile_action.operation(action);
    return new RuntimeEventPayload.TileAction(
        NativeValues.tileOperation(rawOperation),
        rawOperation,
        RuntimeStructs.tileId(mln_runtime_event_tile_action.tile_id(action)),
        MemoryUtil.copyStringView(
            mln_runtime_event_tile_action.source_id(action),
            mln_runtime_event_tile_action.source_id_size(action)));
  }

  private RuntimeEventPayload.OfflineRegionStatusChanged readOfflineRegionStatus(
      MemorySegment payload) {
    var status = payload.reinterpret(mln_runtime_event_offline_region_status.sizeof());
    return new RuntimeEventPayload.OfflineRegionStatusChanged(
        mln_runtime_event_offline_region_status.region_id(status),
        RuntimeStructs.offlineRegionStatus(mln_runtime_event_offline_region_status.status(status)));
  }

  private RuntimeEventPayload.OfflineRegionResponseError readOfflineRegionResponseError(
      MemorySegment payload) {
    var error = payload.reinterpret(mln_runtime_event_offline_region_response_error.sizeof());
    var rawReason = mln_runtime_event_offline_region_response_error.reason(error);
    return new RuntimeEventPayload.OfflineRegionResponseError(
        mln_runtime_event_offline_region_response_error.region_id(error),
        NativeValues.resourceErrorReason(rawReason),
        rawReason);
  }

  private RuntimeEventPayload.OfflineRegionTileCountLimit readOfflineRegionTileCountLimit(
      MemorySegment payload) {
    var limit = payload.reinterpret(mln_runtime_event_offline_region_tile_count_limit.sizeof());
    return new RuntimeEventPayload.OfflineRegionTileCountLimit(
        mln_runtime_event_offline_region_tile_count_limit.region_id(limit),
        mln_runtime_event_offline_region_tile_count_limit.limit(limit));
  }

  private RuntimeEventPayload readOfflineOperationCompletedPayload(
      int rawPayloadType, MemorySegment payload, long payloadSize) {
    var requiredSize = mln_runtime_event_offline_operation_completed.sizeof();
    if (MemoryUtil.isNull(payload) || payloadSize < requiredSize) {
      return unknownPayload(rawPayloadType, payload, payloadSize);
    }
    return readOfflineOperationCompleted(payload);
  }

  static MemorySegment offlineOperationCompletedPayload(MemorySegment event) {
    var payload = mln_runtime_event.payload(event);
    var payloadSize = mln_runtime_event.payload_size(event);
    var requiredSize = mln_runtime_event_offline_operation_completed.sizeof();
    if (MemoryUtil.isNull(payload) || payloadSize < requiredSize) {
      throw NativeValues.exceptionForStatus(
          MaplibreStatus.INVALID_ARGUMENT,
          MapLibreNativeC.MLN_STATUS_INVALID_ARGUMENT(),
          "offline operation completion payload is invalid");
    }
    return payload.reinterpret(requiredSize);
  }

  private RuntimeEventPayload.OfflineOperationCompleted readOfflineOperationCompleted(
      MemorySegment payload) {
    var completed = payload.reinterpret(mln_runtime_event_offline_operation_completed.sizeof());
    var rawOperationKind = mln_runtime_event_offline_operation_completed.operation_kind(completed);
    var rawResultKind = mln_runtime_event_offline_operation_completed.result_kind(completed);
    return new RuntimeEventPayload.OfflineOperationCompleted(
        mln_runtime_event_offline_operation_completed.operation_id(completed),
        NativeValues.offlineOperationKind(rawOperationKind),
        rawOperationKind,
        NativeValues.offlineOperationResultKind(rawResultKind),
        rawResultKind,
        mln_runtime_event_offline_operation_completed.result_status(completed),
        mln_runtime_event_offline_operation_completed.found(completed));
  }

  private void closeResourceCallbacks() {
    closeQuietly(resourceProviderState);
    closeQuietly(resourceTransformState);
    resourceProviderState = null;
    resourceTransformState = null;
  }

  private static void closeQuietly(AutoCloseable closeable) {
    if (closeable == null) {
      return;
    }
    try {
      closeable.close();
    } catch (Exception ignored) {
      // Releasing callback arenas is best-effort after native callbacks are disabled.
    }
  }
}
