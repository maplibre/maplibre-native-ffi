package org.maplibre.nativejni.runtime;

import java.lang.foreign.MemorySegment;
import java.lang.ref.WeakReference;
import java.nio.file.Path;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.bridge.OfflineNative;
import org.maplibre.nativejni.internal.bridge.RuntimeNative;
import org.maplibre.nativejni.internal.lifecycle.HandleState;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.internal.struct.RuntimeStructs;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.offline.OfflineRegionDefinition;
import org.maplibre.nativejni.offline.OfflineRegionDownloadState;
import org.maplibre.nativejni.offline.OfflineRegionInfo;
import org.maplibre.nativejni.offline.OfflineRegionStatus;
import org.maplibre.nativejni.resource.ResourceProviderCallback;
import org.maplibre.nativejni.resource.ResourceTransformCallback;

/** API-parity scaffold for the Java JNI binding. */
public final class RuntimeHandle implements AutoCloseable {
  private final HandleState state;
  private final ConcurrentHashMap<Long, WeakReference<MapHandle>> liveMaps =
      new ConcurrentHashMap<>();

  private RuntimeHandle(long handle) {
    this.state = new HandleState("RuntimeHandle", handle);
  }

  private static UnsupportedOperationException unsupported() {
    return new UnsupportedOperationException(
        "RuntimeHandle is not implemented by the JNI bridge yet");
  }

  public static RuntimeHandle create() {
    return create(new RuntimeOptions());
  }

  public static RuntimeHandle create(RuntimeOptions options) {
    Objects.requireNonNull(options, "options");
    NativeLibrary.ensureLoaded();
    var outRuntime = new long[1];
    Status.check(RuntimeNative.mln_runtime_create(outRuntime));
    return new RuntimeHandle(outRuntime[0]);
  }

  public void runOnce() {
    NativeLibrary.ensureLoaded();
    Status.check(RuntimeNative.mln_runtime_run_once(state.requireLiveAddress()));
  }

  private <T> OfflineOperationHandle<T> offlineOperation(
      long operationId, OfflineOperationKind kind, OfflineOperationResultKind resultKind) {
    return new OfflineOperationHandle<>(this, operationId, kind, resultKind);
  }

  public OfflineOperationHandle<Void> startAmbientCacheOperation(AmbientCacheOperation operation) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        RuntimeNative.mln_runtime_run_ambient_cache_operation_start(
            state.requireLiveAddress(),
            Objects.requireNonNull(operation, "operation").nativeValue(),
            outOperationId));
    return offlineOperation(
        outOperationId[0], OfflineOperationKind.AMBIENT_CACHE, OfflineOperationResultKind.NONE);
  }

  public OfflineOperationHandle<OfflineRegionInfo> startCreateOfflineRegion(
      OfflineRegionDefinition definition, byte[] metadata) {
    throw unsupported();
  }

  public OfflineOperationHandle<Optional<OfflineRegionInfo>> startOfflineRegion(long id) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        OfflineNative.mln_runtime_offline_region_get_start(
            state.requireLiveAddress(), id, outOperationId));
    return offlineOperation(
        outOperationId[0],
        OfflineOperationKind.REGION_GET,
        OfflineOperationResultKind.OPTIONAL_REGION);
  }

  public OfflineOperationHandle<List<OfflineRegionInfo>> startOfflineRegions() {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        OfflineNative.mln_runtime_offline_regions_list_start(
            state.requireLiveAddress(), outOperationId));
    return offlineOperation(
        outOperationId[0],
        OfflineOperationKind.REGIONS_LIST,
        OfflineOperationResultKind.REGION_LIST);
  }

  public OfflineOperationHandle<List<OfflineRegionInfo>> startMergeOfflineRegionsDatabase(
      Path path) {
    return startMergeOfflineRegionsDatabase(Objects.requireNonNull(path, "path").toString());
  }

  public OfflineOperationHandle<List<OfflineRegionInfo>> startMergeOfflineRegionsDatabase(
      String path) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        OfflineNative.mln_runtime_offline_regions_merge_database_start(
            state.requireLiveAddress(), Objects.requireNonNull(path, "path"), outOperationId));
    return offlineOperation(
        outOperationId[0],
        OfflineOperationKind.REGIONS_MERGE_DATABASE,
        OfflineOperationResultKind.REGION_LIST);
  }

  public OfflineOperationHandle<OfflineRegionInfo> startUpdateOfflineRegionMetadata(
      long id, byte[] metadata) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        OfflineNative.mln_runtime_offline_region_update_metadata_start(
            state.requireLiveAddress(),
            id,
            Objects.requireNonNull(metadata, "metadata"),
            outOperationId));
    return offlineOperation(
        outOperationId[0],
        OfflineOperationKind.REGION_UPDATE_METADATA,
        OfflineOperationResultKind.REGION);
  }

  public OfflineOperationHandle<OfflineRegionStatus> startOfflineRegionStatus(long id) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        OfflineNative.mln_runtime_offline_region_get_status_start(
            state.requireLiveAddress(), id, outOperationId));
    return offlineOperation(
        outOperationId[0],
        OfflineOperationKind.REGION_GET_STATUS,
        OfflineOperationResultKind.REGION_STATUS);
  }

  public OfflineOperationHandle<Void> startSetOfflineRegionObserved(long id, boolean observed) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        OfflineNative.mln_runtime_offline_region_set_observed_start(
            state.requireLiveAddress(), id, observed, outOperationId));
    return offlineOperation(
        outOperationId[0],
        OfflineOperationKind.REGION_SET_OBSERVED,
        OfflineOperationResultKind.NONE);
  }

  public OfflineOperationHandle<Void> startSetOfflineRegionDownloadState(
      long id, OfflineRegionDownloadState downloadState) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        OfflineNative.mln_runtime_offline_region_set_download_state_start(
            state.requireLiveAddress(),
            id,
            Objects.requireNonNull(downloadState, "downloadState").nativeValue(),
            outOperationId));
    return offlineOperation(
        outOperationId[0],
        OfflineOperationKind.REGION_SET_DOWNLOAD_STATE,
        OfflineOperationResultKind.NONE);
  }

  public OfflineOperationHandle<Void> startInvalidateOfflineRegion(long id) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        OfflineNative.mln_runtime_offline_region_invalidate_start(
            state.requireLiveAddress(), id, outOperationId));
    return offlineOperation(
        outOperationId[0], OfflineOperationKind.REGION_INVALIDATE, OfflineOperationResultKind.NONE);
  }

  public OfflineOperationHandle<Void> startDeleteOfflineRegion(long id) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        OfflineNative.mln_runtime_offline_region_delete_start(
            state.requireLiveAddress(), id, outOperationId));
    return offlineOperation(
        outOperationId[0], OfflineOperationKind.REGION_DELETE, OfflineOperationResultKind.NONE);
  }

  public OfflineRegionInfo takeCreateOfflineRegionResult(
      OfflineOperationHandle<OfflineRegionInfo> operation) {
    throw unsupported();
  }

  public Optional<OfflineRegionInfo> takeOfflineRegionResult(
      OfflineOperationHandle<Optional<OfflineRegionInfo>> operation) {
    throw unsupported();
  }

  public List<OfflineRegionInfo> takeOfflineRegionsResult(
      OfflineOperationHandle<List<OfflineRegionInfo>> operation) {
    throw unsupported();
  }

  public List<OfflineRegionInfo> takeMergeOfflineRegionsDatabaseResult(
      OfflineOperationHandle<List<OfflineRegionInfo>> operation) {
    throw unsupported();
  }

  public OfflineRegionInfo takeUpdateOfflineRegionMetadataResult(
      OfflineOperationHandle<OfflineRegionInfo> operation) {
    throw unsupported();
  }

  public OfflineRegionStatus takeOfflineRegionStatusResult(
      OfflineOperationHandle<OfflineRegionStatus> operation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    var longs = new long[6];
    var ints = new int[1];
    var booleans = new boolean[2];
    var operationId =
        operation.requireLive(
            this, OfflineOperationKind.REGION_GET_STATUS, OfflineOperationResultKind.REGION_STATUS);
    Status.check(
        OfflineNative.mln_runtime_offline_region_get_status_take_result(
            state.requireLiveAddress(), operationId, longs, ints, booleans));
    operation.markConsumed();
    var rawDownloadState = ints[0];
    return new OfflineRegionStatus(
        OfflineRegionDownloadState.fromNative(rawDownloadState),
        rawDownloadState,
        longs[0],
        longs[1],
        longs[2],
        longs[3],
        longs[4],
        longs[5],
        booleans[0],
        booleans[1]);
  }

  public void discardOfflineOperation(OfflineOperationHandle<?> operation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    if (operation.isClosed()) {
      return;
    }
    var operationId = operation.requireLive(this);
    Status.check(
        RuntimeNative.mln_runtime_offline_operation_discard(
            state.requireLiveAddress(), operationId));
    operation.markConsumed();
  }

  public void setResourceTransform(ResourceTransformCallback callback) {
    throw unsupported();
  }

  public void clearResourceTransform() {
    throw unsupported();
  }

  public void setResourceProvider(ResourceProviderCallback callback) {
    throw unsupported();
  }

  public Optional<RuntimeEvent> pollEvent() {
    NativeLibrary.ensureLoaded();
    var longs = new long[RuntimeStructs.LONG_COUNT];
    var ints = new int[RuntimeStructs.INT_COUNT];
    var booleans = new boolean[RuntimeStructs.BOOLEAN_COUNT];
    var doubles = new double[RuntimeStructs.DOUBLE_COUNT];
    var strings = new String[RuntimeStructs.STRING_COUNT];
    Status.check(
        RuntimeNative.mln_runtime_poll_event(
            state.requireLiveAddress(), longs, ints, booleans, doubles, strings));
    if (!booleans[RuntimeStructs.BOOLEAN_HAS_EVENT]) {
      return Optional.empty();
    }
    var sourceType = RuntimeEventSourceType.fromNative(ints[RuntimeStructs.INT_SOURCE_TYPE]);
    var runtimeSource =
        sourceType == RuntimeEventSourceType.RUNTIME
            ? Optional.of(this)
            : Optional.<RuntimeHandle>empty();
    var mapSource =
        sourceType == RuntimeEventSourceType.MAP
            ? Optional.ofNullable(mapFor(longs[RuntimeStructs.LONG_SOURCE_ADDRESS]))
            : Optional.<MapHandle>empty();
    var event =
        RuntimeStructs.runtimeEvent(
            longs, ints, booleans, doubles, strings, runtimeSource, mapSource);
    if (event.type() == RuntimeEventType.MAP_STYLE_LOADED) {
      event
          .mapSource()
          .ifPresent(map -> map.releaseDetachedCustomGeometrySources(InternalAccess.INSTANCE));
    }
    return Optional.of(event);
  }

  public void close() {
    state.closeOnce(RuntimeNative::mln_runtime_destroy);
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public MemorySegment nativeHandle(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    return state.requireLiveSegment();
  }

  MemorySegment nativeHandle() {
    return state.requireLiveSegment();
  }

  long nativeAddress() {
    return state.requireLiveAddress();
  }

  public void registerMap(InternalAccess access, MapHandle map) {
    Objects.requireNonNull(access, "access");
    Objects.requireNonNull(map, "map");
    liveMaps.put(map.nativeAddress(InternalAccess.INSTANCE), new WeakReference<>(map));
  }

  public void unregisterMap(InternalAccess access, MapHandle map) {
    Objects.requireNonNull(access, "access");
    Objects.requireNonNull(map, "map");
    liveMaps.entrySet().removeIf(entry -> entry.getValue().get() == map);
  }

  private MapHandle mapFor(long sourceAddress) {
    if (sourceAddress == 0) {
      return null;
    }
    var reference = liveMaps.get(sourceAddress);
    var map = reference == null ? null : reference.get();
    if (reference != null && map == null) {
      liveMaps.remove(sourceAddress, reference);
    }
    return map;
  }

  static MemorySegment offlineOperationCompletedPayload(MemorySegment event) {
    throw unsupported();
  }
}
