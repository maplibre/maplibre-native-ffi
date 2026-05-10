package org.maplibre.nativeffi;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.lang.ref.WeakReference;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import org.maplibre.nativeffi.internal.HandleState;
import org.maplibre.nativeffi.internal.MemoryUtil;
import org.maplibre.nativeffi.internal.NativeAccess;
import org.maplibre.nativeffi.internal.ResourceTransformState;
import org.maplibre.nativeffi.internal.RuntimeStructs;
import org.maplibre.nativeffi.internal.Status;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_runtime_event;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_response_error;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_status;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_tile_count_limit;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_frame;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_map;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_style_image_missing;
import org.maplibre.nativeffi.internal.c.mln_runtime_event_tile_action;

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

  public static RuntimeHandle create() {
    return create(new RuntimeOptions());
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

  public void runAmbientCacheOperation(AmbientCacheOperation operation) {
    NativeAccess.ensureLoaded();
    Status.check(
        MapLibreNativeC.mln_runtime_run_ambient_cache_operation(
            state.requireLive(), Objects.requireNonNull(operation, "operation").nativeValue()));
  }

  public void setResourceTransform(ResourceTransformCallback callback) {
    NativeAccess.ensureLoaded();
    var replacement = new ResourceTransformState(Objects.requireNonNull(callback, "callback"));
    ResourceTransformState previous;
    try {
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

  public void setResourceProvider(ResourceProviderCallback callback) {
    NativeAccess.ensureLoaded();
    var replacement = new ResourceProviderState(Objects.requireNonNull(callback, "callback"));
    ResourceProviderState previous;
    try {
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

  MemorySegment nativeHandle() {
    return state.requireLive();
  }

  long nativeAddress() {
    return state.address();
  }

  void registerMap(MapHandle map) {
    liveMaps.put(map.nativeAddress(), new WeakReference<>(map));
  }

  void unregisterMap(MapHandle map) {
    liveMaps.computeIfPresent(
        map.nativeAddress(), (address, reference) -> reference.get() == map ? null : reference);
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
    var sourceType = RuntimeEventSourceType.fromNative(rawSourceType);
    var runtimeSource =
        sourceType == RuntimeEventSourceType.RUNTIME
            ? Optional.of(this)
            : Optional.<RuntimeHandle>empty();
    var mapSource =
        sourceType == RuntimeEventSourceType.MAP
            ? Optional.ofNullable(mapFor(source))
            : Optional.<MapHandle>empty();
    var rawPayloadType = mln_runtime_event.payload_type(event);
    return new RuntimeEvent(
        RuntimeEventType.fromNative(rawType),
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
  }

  private RuntimeEventPayload readPayload(
      int rawPayloadType, MemorySegment payload, long payloadSize) {
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_NONE()) {
      return RuntimeEventPayload.NONE;
    }
    if (MemoryUtil.isNull(payload) || payloadSize == 0) {
      return new RuntimeEventPayload.Unknown(rawPayloadType, payloadSize);
    }

    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME()) {
      return payloadSize >= mln_runtime_event_render_frame.sizeof()
          ? readRenderFrame(payload)
          : new RuntimeEventPayload.Unknown(rawPayloadType, payloadSize);
    }
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP()) {
      return payloadSize >= mln_runtime_event_render_map.sizeof()
          ? readRenderMap(payload)
          : new RuntimeEventPayload.Unknown(rawPayloadType, payloadSize);
    }
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING()) {
      return payloadSize >= mln_runtime_event_style_image_missing.sizeof()
          ? readStyleImageMissing(payload)
          : new RuntimeEventPayload.Unknown(rawPayloadType, payloadSize);
    }
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION()) {
      return payloadSize >= mln_runtime_event_tile_action.sizeof()
          ? readTileAction(payload)
          : new RuntimeEventPayload.Unknown(rawPayloadType, payloadSize);
    }
    if (rawPayloadType == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS()) {
      return payloadSize >= mln_runtime_event_offline_region_status.sizeof()
          ? readOfflineRegionStatus(payload)
          : new RuntimeEventPayload.Unknown(rawPayloadType, payloadSize);
    }
    if (rawPayloadType
        == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR()) {
      return payloadSize >= mln_runtime_event_offline_region_response_error.sizeof()
          ? readOfflineRegionResponseError(payload)
          : new RuntimeEventPayload.Unknown(rawPayloadType, payloadSize);
    }
    if (rawPayloadType
        == MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT()) {
      return payloadSize >= mln_runtime_event_offline_region_tile_count_limit.sizeof()
          ? readOfflineRegionTileCountLimit(payload)
          : new RuntimeEventPayload.Unknown(rawPayloadType, payloadSize);
    }
    return new RuntimeEventPayload.Unknown(rawPayloadType, payloadSize);
  }

  private RuntimeEventPayload.RenderFrame readRenderFrame(MemorySegment payload) {
    var frame = payload.reinterpret(mln_runtime_event_render_frame.sizeof());
    var rawMode = mln_runtime_event_render_frame.mode(frame);
    return new RuntimeEventPayload.RenderFrame(
        RenderMode.fromNative(rawMode),
        rawMode,
        mln_runtime_event_render_frame.needs_repaint(frame),
        mln_runtime_event_render_frame.placement_changed(frame),
        RuntimeStructs.renderingStats(mln_runtime_event_render_frame.stats(frame)));
  }

  private RuntimeEventPayload.RenderMap readRenderMap(MemorySegment payload) {
    var renderMap = payload.reinterpret(mln_runtime_event_render_map.sizeof());
    var rawMode = mln_runtime_event_render_map.mode(renderMap);
    return new RuntimeEventPayload.RenderMap(RenderMode.fromNative(rawMode), rawMode);
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
        TileOperation.fromNative(rawOperation),
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
        ResourceErrorReason.fromNative(rawReason),
        rawReason);
  }

  private RuntimeEventPayload.OfflineRegionTileCountLimit readOfflineRegionTileCountLimit(
      MemorySegment payload) {
    var limit = payload.reinterpret(mln_runtime_event_offline_region_tile_count_limit.sizeof());
    return new RuntimeEventPayload.OfflineRegionTileCountLimit(
        mln_runtime_event_offline_region_tile_count_limit.region_id(limit),
        mln_runtime_event_offline_region_tile_count_limit.limit(limit));
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
