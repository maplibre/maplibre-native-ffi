package org.maplibre.nativejni.runtime;

import java.lang.foreign.MemorySegment;
import java.lang.ref.WeakReference;
import java.nio.file.Path;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import org.maplibre.nativejni.internal.access.InternalAccess;
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

  public OfflineOperationHandle<Void> startAmbientCacheOperation(AmbientCacheOperation operation) {
    throw unsupported();
  }

  public OfflineOperationHandle<OfflineRegionInfo> startCreateOfflineRegion(
      OfflineRegionDefinition definition, byte[] metadata) {
    throw unsupported();
  }

  public OfflineOperationHandle<Optional<OfflineRegionInfo>> startOfflineRegion(long id) {
    throw unsupported();
  }

  public OfflineOperationHandle<List<OfflineRegionInfo>> startOfflineRegions() {
    throw unsupported();
  }

  public OfflineOperationHandle<List<OfflineRegionInfo>> startMergeOfflineRegionsDatabase(
      Path path) {
    throw unsupported();
  }

  public OfflineOperationHandle<List<OfflineRegionInfo>> startMergeOfflineRegionsDatabase(
      String path) {
    throw unsupported();
  }

  public OfflineOperationHandle<OfflineRegionInfo> startUpdateOfflineRegionMetadata(
      long id, byte[] metadata) {
    throw unsupported();
  }

  public OfflineOperationHandle<OfflineRegionStatus> startOfflineRegionStatus(long id) {
    throw unsupported();
  }

  public OfflineOperationHandle<Void> startSetOfflineRegionObserved(long id, boolean observed) {
    throw unsupported();
  }

  public OfflineOperationHandle<Void> startSetOfflineRegionDownloadState(
      long id, OfflineRegionDownloadState downloadState) {
    throw unsupported();
  }

  public OfflineOperationHandle<Void> startInvalidateOfflineRegion(long id) {
    throw unsupported();
  }

  public OfflineOperationHandle<Void> startDeleteOfflineRegion(long id) {
    throw unsupported();
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
    throw unsupported();
  }

  public void discardOfflineOperation(OfflineOperationHandle<?> operation) {
    throw unsupported();
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
