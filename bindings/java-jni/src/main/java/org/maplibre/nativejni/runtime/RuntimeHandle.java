package org.maplibre.nativejni.runtime;

import java.lang.ref.WeakReference;
import java.nio.file.Path;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicReference;
import org.bytedeco.javacpp.BoolPointer;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.callback.ResourceTransformState;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.lifecycle.HandleState;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.internal.struct.OfflineStructs;
import org.maplibre.nativejni.internal.struct.RuntimeStructs;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.offline.OfflineRegionDefinition;
import org.maplibre.nativejni.offline.OfflineRegionDownloadState;
import org.maplibre.nativejni.offline.OfflineRegionInfo;
import org.maplibre.nativejni.offline.OfflineRegionStatus;
import org.maplibre.nativejni.resource.ResourceProviderCallback;
import org.maplibre.nativejni.resource.ResourceTransformCallback;

/** Owned native runtime handle. Close it on the owner thread. */
public final class RuntimeHandle implements AutoCloseable {
  private static final AtomicReference<RuntimeException> RESOURCE_TRANSFORM_INSTALL_FAILURE =
      new AtomicReference<>();
  private static final AtomicReference<RuntimeException> RESOURCE_PROVIDER_INSTALL_FAILURE =
      new AtomicReference<>();

  private final HandleState state;
  private final ConcurrentHashMap<Long, WeakReference<MapHandle>> liveMaps =
      new ConcurrentHashMap<>();
  private ResourceProviderState resourceProvider;
  private ResourceTransformState resourceTransform;
  private ResourceProviderState failedResourceProviderForTesting;
  private ResourceTransformState failedResourceTransformForTesting;

  private RuntimeHandle(long handle) {
    this.state = new HandleState("RuntimeHandle", handle);
  }

  public static RuntimeHandle create() {
    return create(new RuntimeOptions());
  }

  public static RuntimeHandle create(RuntimeOptions options) {
    Objects.requireNonNull(options, "options");
    NativeLibrary.ensureLoaded();
    validateRuntimeOptions(options);
    try (var nativeOptions = RuntimeStructs.nativeRuntimeOptions(options)) {
      var outRuntime = JavaCppSupport.outPointer(MaplibreNativeC.mln_runtime.class);
      Status.check(MaplibreNativeC.mln_runtime_create(nativeOptions.options(), outRuntime));
      return new RuntimeHandle(
          JavaCppSupport.outAddress(outRuntime, MaplibreNativeC.mln_runtime.class));
    }
  }

  private static void validateRuntimeOptions(RuntimeOptions options) {
    var value = RuntimeStructs.runtimeOptions(options);
    if ((value.assetPath() != null && value.assetPath().indexOf('\0') >= 0)
        || (value.cachePath() != null && value.cachePath().indexOf('\0') >= 0)) {
      JavaCppSupport.setThreadDiagnostic("runtime option path contains embedded NUL");
      Status.check(MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT);
    }
    if (value.hasMaximumCacheSize() && value.maximumCacheSize() < 0) {
      JavaCppSupport.setThreadDiagnostic("maximum cache size must be non-negative");
      Status.check(MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT);
    }
  }

  private static void validateOfflineRegionDefinition(OfflineRegionDefinition definition) {
    switch (definition) {
      case OfflineRegionDefinition.TilePyramid tilePyramid ->
          Status.checkNoEmbeddedNul(tilePyramid.styleUrl(), "offline region style URL");
      case OfflineRegionDefinition.GeometryRegion geometryRegion ->
          Status.checkNoEmbeddedNul(geometryRegion.styleUrl(), "offline region style URL");
    }
  }

  public void runOnce() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_runtime_run_once(JavaCppSupport.runtime(state.requireLiveAddress())));
  }

  private <T> OfflineOperationHandle<T> offlineOperation(
      long operationId, OfflineOperationKind kind, OfflineOperationResultKind resultKind) {
    return new OfflineOperationHandle<>(this, operationId, kind, resultKind);
  }

  public OfflineOperationHandle<Void> startAmbientCacheOperation(AmbientCacheOperation operation) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        MaplibreNativeC.mln_runtime_run_ambient_cache_operation_start(
            JavaCppSupport.runtime(state.requireLiveAddress()),
            Objects.requireNonNull(operation, "operation").nativeValue(),
            outOperationId));
    return offlineOperation(
        outOperationId[0], OfflineOperationKind.AMBIENT_CACHE, OfflineOperationResultKind.NONE);
  }

  public OfflineOperationHandle<OfflineRegionInfo> startCreateOfflineRegion(
      OfflineRegionDefinition definition, byte[] metadata) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(definition, "definition");
    validateOfflineRegionDefinition(definition);
    var outOperationId = new long[1];
    try (var nativeDefinition = new OfflineStructs.DefinitionScope(definition)) {
      Status.check(
          MaplibreNativeC.mln_runtime_offline_region_create_start(
              JavaCppSupport.runtime(state.requireLiveAddress()),
              nativeDefinition.definition(),
              Objects.requireNonNull(metadata, "metadata"),
              metadata.length,
              outOperationId));
    }
    return offlineOperation(
        outOperationId[0], OfflineOperationKind.REGION_CREATE, OfflineOperationResultKind.REGION);
  }

  public OfflineOperationHandle<Optional<OfflineRegionInfo>> startOfflineRegion(long id) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        MaplibreNativeC.mln_runtime_offline_region_get_start(
            JavaCppSupport.runtime(state.requireLiveAddress()), id, outOperationId));
    return offlineOperation(
        outOperationId[0],
        OfflineOperationKind.REGION_GET,
        OfflineOperationResultKind.OPTIONAL_REGION);
  }

  public OfflineOperationHandle<List<OfflineRegionInfo>> startOfflineRegions() {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        MaplibreNativeC.mln_runtime_offline_regions_list_start(
            JavaCppSupport.runtime(state.requireLiveAddress()), outOperationId));
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
    var pathValue = Objects.requireNonNull(path, "path");
    if (pathValue.indexOf('\0') >= 0) {
      JavaCppSupport.setThreadDiagnostic("side database path contains embedded NUL");
      Status.check(MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT);
    }
    Status.check(
        MaplibreNativeC.mln_runtime_offline_regions_merge_database_start(
            JavaCppSupport.runtime(state.requireLiveAddress()), pathValue, outOperationId));
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
        MaplibreNativeC.mln_runtime_offline_region_update_metadata_start(
            JavaCppSupport.runtime(state.requireLiveAddress()),
            id,
            Objects.requireNonNull(metadata, "metadata"),
            metadata.length,
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
        MaplibreNativeC.mln_runtime_offline_region_get_status_start(
            JavaCppSupport.runtime(state.requireLiveAddress()), id, outOperationId));
    return offlineOperation(
        outOperationId[0],
        OfflineOperationKind.REGION_GET_STATUS,
        OfflineOperationResultKind.REGION_STATUS);
  }

  public OfflineOperationHandle<Void> startSetOfflineRegionObserved(long id, boolean observed) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        MaplibreNativeC.mln_runtime_offline_region_set_observed_start(
            JavaCppSupport.runtime(state.requireLiveAddress()), id, observed, outOperationId));
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
        MaplibreNativeC.mln_runtime_offline_region_set_download_state_start(
            JavaCppSupport.runtime(state.requireLiveAddress()),
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
        MaplibreNativeC.mln_runtime_offline_region_invalidate_start(
            JavaCppSupport.runtime(state.requireLiveAddress()), id, outOperationId));
    return offlineOperation(
        outOperationId[0], OfflineOperationKind.REGION_INVALIDATE, OfflineOperationResultKind.NONE);
  }

  public OfflineOperationHandle<Void> startDeleteOfflineRegion(long id) {
    NativeLibrary.ensureLoaded();
    var outOperationId = new long[1];
    Status.check(
        MaplibreNativeC.mln_runtime_offline_region_delete_start(
            JavaCppSupport.runtime(state.requireLiveAddress()), id, outOperationId));
    return offlineOperation(
        outOperationId[0], OfflineOperationKind.REGION_DELETE, OfflineOperationResultKind.NONE);
  }

  public OfflineRegionInfo takeCreateOfflineRegionResult(
      OfflineOperationHandle<OfflineRegionInfo> operation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    var operationId =
        operation.requireLive(
            this, OfflineOperationKind.REGION_CREATE, OfflineOperationResultKind.REGION);
    var outSnapshot = JavaCppSupport.outPointer(MaplibreNativeC.mln_offline_region_snapshot.class);
    Status.check(
        MaplibreNativeC.mln_runtime_offline_region_create_take_result(
            JavaCppSupport.runtime(state.requireLiveAddress()), operationId, outSnapshot));
    try {
      return OfflineStructs.offlineRegionSnapshot(outSnapshot);
    } finally {
      operation.markConsumed();
    }
  }

  public Optional<OfflineRegionInfo> takeOfflineRegionResult(
      OfflineOperationHandle<Optional<OfflineRegionInfo>> operation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    var operationId =
        operation.requireLive(
            this, OfflineOperationKind.REGION_GET, OfflineOperationResultKind.OPTIONAL_REGION);
    try (var found = new BoolPointer(1)) {
      var outSnapshot =
          JavaCppSupport.outPointer(MaplibreNativeC.mln_offline_region_snapshot.class);
      Status.check(
          MaplibreNativeC.mln_runtime_offline_region_get_take_result(
              JavaCppSupport.runtime(state.requireLiveAddress()), operationId, outSnapshot, found));
      try {
        return found.get()
            ? Optional.of(OfflineStructs.offlineRegionSnapshot(outSnapshot))
            : Optional.<OfflineRegionInfo>empty();
      } finally {
        operation.markConsumed();
      }
    }
  }

  public List<OfflineRegionInfo> takeOfflineRegionsResult(
      OfflineOperationHandle<List<OfflineRegionInfo>> operation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    var operationId =
        operation.requireLive(
            this, OfflineOperationKind.REGIONS_LIST, OfflineOperationResultKind.REGION_LIST);
    var outList = JavaCppSupport.outPointer(MaplibreNativeC.mln_offline_region_list.class);
    Status.check(
        MaplibreNativeC.mln_runtime_offline_regions_list_take_result(
            JavaCppSupport.runtime(state.requireLiveAddress()), operationId, outList));
    try {
      return OfflineStructs.offlineRegionList(outList);
    } finally {
      operation.markConsumed();
    }
  }

  public List<OfflineRegionInfo> takeMergeOfflineRegionsDatabaseResult(
      OfflineOperationHandle<List<OfflineRegionInfo>> operation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    var operationId =
        operation.requireLive(
            this,
            OfflineOperationKind.REGIONS_MERGE_DATABASE,
            OfflineOperationResultKind.REGION_LIST);
    var outList = JavaCppSupport.outPointer(MaplibreNativeC.mln_offline_region_list.class);
    Status.check(
        MaplibreNativeC.mln_runtime_offline_regions_merge_database_take_result(
            JavaCppSupport.runtime(state.requireLiveAddress()), operationId, outList));
    try {
      return OfflineStructs.offlineRegionList(outList);
    } finally {
      operation.markConsumed();
    }
  }

  public OfflineRegionInfo takeUpdateOfflineRegionMetadataResult(
      OfflineOperationHandle<OfflineRegionInfo> operation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    var operationId =
        operation.requireLive(
            this, OfflineOperationKind.REGION_UPDATE_METADATA, OfflineOperationResultKind.REGION);
    var outSnapshot = JavaCppSupport.outPointer(MaplibreNativeC.mln_offline_region_snapshot.class);
    Status.check(
        MaplibreNativeC.mln_runtime_offline_region_update_metadata_take_result(
            JavaCppSupport.runtime(state.requireLiveAddress()), operationId, outSnapshot));
    try {
      return OfflineStructs.offlineRegionSnapshot(outSnapshot);
    } finally {
      operation.markConsumed();
    }
  }

  public OfflineRegionStatus takeOfflineRegionStatusResult(
      OfflineOperationHandle<OfflineRegionStatus> operation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    var operationId =
        operation.requireLive(
            this, OfflineOperationKind.REGION_GET_STATUS, OfflineOperationResultKind.REGION_STATUS);
    try (var status = new MaplibreNativeC.mln_offline_region_status()) {
      status.size(status.sizeof());
      Status.check(
          MaplibreNativeC.mln_runtime_offline_region_get_status_take_result(
              JavaCppSupport.runtime(state.requireLiveAddress()), operationId, status));
      try {
        var rawDownloadState = status.download_state();
        return new OfflineRegionStatus(
            OfflineRegionDownloadState.fromNative(rawDownloadState),
            status.completed_resource_count(),
            status.completed_resource_size(),
            status.completed_tile_count(),
            status.required_tile_count(),
            status.completed_tile_size(),
            status.required_resource_count(),
            status.required_resource_count_is_precise(),
            status.complete());
      } finally {
        operation.markConsumed();
      }
    }
  }

  void discardOfflineOperation(OfflineOperationHandle<?> operation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(operation, "operation");
    if (operation.isClosed()) {
      return;
    }
    var operationId = operation.requireLive(this);
    long runtimeAddress;
    try {
      runtimeAddress = state.requireLiveAddress();
    } catch (InvalidStateException error) {
      operation.markConsumed();
      throw error;
    }
    var status =
        MaplibreNativeC.mln_runtime_offline_operation_discard(
            JavaCppSupport.runtime(runtimeAddress), operationId);
    if (status == MaplibreStatus.WRONG_THREAD.nativeCode()) {
      Status.check(status);
    }
    Status.check(status);
    operation.markConsumed();
  }

  public void setResourceTransform(ResourceTransformCallback callback) {
    NativeLibrary.ensureLoaded();
    var replacement = new ResourceTransformState(Objects.requireNonNull(callback, "callback"));
    try {
      throwResourceTransformInstallFailureForTesting();
      Status.check(
          MaplibreNativeC.mln_runtime_set_resource_transform(
              JavaCppSupport.runtime(state.requireLiveAddress()), replacement.transform()));
    } catch (RuntimeException | Error error) {
      closeQuietly(replacement);
      failedResourceTransformForTesting = replacement;
      throw error;
    }
    closeQuietly(resourceTransform);
    resourceTransform = replacement;
  }

  public void clearResourceTransform() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_runtime_clear_resource_transform(
            JavaCppSupport.runtime(state.requireLiveAddress())));
    closeQuietly(resourceTransform);
    resourceTransform = null;
  }

  public void setResourceProvider(ResourceProviderCallback callback) {
    NativeLibrary.ensureLoaded();
    var replacement = new ResourceProviderState(Objects.requireNonNull(callback, "callback"));
    try {
      throwResourceProviderInstallFailureForTesting();
      Status.check(
          MaplibreNativeC.mln_runtime_set_resource_provider(
              JavaCppSupport.runtime(state.requireLiveAddress()), replacement.provider()));
    } catch (RuntimeException | Error error) {
      closeQuietly(replacement);
      failedResourceProviderForTesting = replacement;
      throw error;
    }
    closeQuietly(resourceProvider);
    resourceProvider = replacement;
  }

  static void failNextResourceTransformInstallForTesting(RuntimeException failure) {
    if (!RESOURCE_TRANSFORM_INSTALL_FAILURE.compareAndSet(null, Objects.requireNonNull(failure))) {
      throw new IllegalStateException("resource transform install failure is already armed");
    }
  }

  static void failNextResourceProviderInstallForTesting(RuntimeException failure) {
    if (!RESOURCE_PROVIDER_INSTALL_FAILURE.compareAndSet(null, Objects.requireNonNull(failure))) {
      throw new IllegalStateException("resource provider install failure is already armed");
    }
  }

  static void resetInstallFailuresForTesting() {
    RESOURCE_TRANSFORM_INSTALL_FAILURE.set(null);
    RESOURCE_PROVIDER_INSTALL_FAILURE.set(null);
  }

  ResourceTransformState resourceTransformForTesting() {
    return resourceTransform;
  }

  ResourceTransformState failedResourceTransformForTesting() {
    return failedResourceTransformForTesting;
  }

  ResourceProviderState resourceProviderForTesting() {
    return resourceProvider;
  }

  ResourceProviderState failedResourceProviderForTesting() {
    return failedResourceProviderForTesting;
  }

  private static void throwResourceTransformInstallFailureForTesting() {
    var failure = RESOURCE_TRANSFORM_INSTALL_FAILURE.getAndSet(null);
    if (failure != null) {
      throw failure;
    }
  }

  private static void throwResourceProviderInstallFailureForTesting() {
    var failure = RESOURCE_PROVIDER_INSTALL_FAILURE.getAndSet(null);
    if (failure != null) {
      throw failure;
    }
  }

  public Optional<RuntimeEvent> pollEvent() {
    NativeLibrary.ensureLoaded();
    var longs = new long[RuntimeStructs.LONG_COUNT];
    var ints = new int[RuntimeStructs.INT_COUNT];
    var booleans = new boolean[RuntimeStructs.BOOLEAN_COUNT];
    var doubles = new double[RuntimeStructs.DOUBLE_COUNT];
    var strings = new String[RuntimeStructs.STRING_COUNT];
    var bytes = new byte[RuntimeStructs.BYTES_COUNT][];
    try (var nativeEvent = new MaplibreNativeC.mln_runtime_event()) {
      nativeEvent.size(nativeEvent.sizeof());
      var hasEvent = new boolean[1];
      Status.check(
          MaplibreNativeC.mln_runtime_poll_event(
              JavaCppSupport.runtime(state.requireLiveAddress()), nativeEvent, hasEvent));
      booleans[RuntimeStructs.BOOLEAN_HAS_EVENT] = hasEvent[0];
      if (hasEvent[0]) {
        RuntimeStructs.copyEvent(nativeEvent, longs, ints, booleans, doubles, strings, bytes);
      }
    }
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
            longs, ints, booleans, doubles, strings, bytes, runtimeSource, mapSource);
    if (event.type() == RuntimeEventType.MAP_STYLE_LOADED) {
      event
          .mapSource()
          .ifPresent(map -> map.releaseDetachedCustomGeometrySources(InternalAccess.INSTANCE));
    }
    return Optional.of(event);
  }

  public void close() {
    state.closeOnce(
        address -> MaplibreNativeC.mln_runtime_destroy(JavaCppSupport.runtime(address)),
        () -> {
          closeQuietly(resourceTransform);
          resourceTransform = null;
          closeQuietly(resourceProvider);
          resourceProvider = null;
        });
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public long nativeAddress(InternalAccess access) {
    Objects.requireNonNull(access, "access").checkCaller();
    return nativeAddress();
  }

  public HandleState.ChildRetention retainChild(InternalAccess access, String childTypeName) {
    Objects.requireNonNull(access, "access").checkCaller();
    return state.retainChild(childTypeName);
  }

  long nativeAddress() {
    return state.requireLiveAddress();
  }

  public void registerMap(InternalAccess access, MapHandle map) {
    Objects.requireNonNull(access, "access").checkCaller();
    Objects.requireNonNull(map, "map");
    liveMaps.put(map.nativeAddress(InternalAccess.INSTANCE), new WeakReference<>(map));
  }

  public void unregisterMap(InternalAccess access, MapHandle map) {
    Objects.requireNonNull(access, "access").checkCaller();
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

  private static void closeQuietly(AutoCloseable closeable) {
    if (closeable == null) {
      return;
    }
    try {
      closeable.close();
    } catch (Exception ignored) {
      // Closing callback state is best-effort during runtime teardown/replacement.
    }
  }
}
