package org.maplibre.nativejni.internal.bridge;

import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.PointerPointer;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.struct.ResourceStructs;
import org.maplibre.nativejni.internal.struct.RuntimeStructs;
import org.maplibre.nativejni.resource.ResourceKind;
import org.maplibre.nativejni.resource.ResourceLoadingMethod;
import org.maplibre.nativejni.resource.ResourcePriority;
import org.maplibre.nativejni.resource.ResourceProviderCallback;
import org.maplibre.nativejni.resource.ResourceProviderDecision;
import org.maplibre.nativejni.resource.ResourceRequest;
import org.maplibre.nativejni.resource.ResourceRequest.ByteRange;
import org.maplibre.nativejni.resource.ResourceRequestHandle;
import org.maplibre.nativejni.resource.ResourceStoragePolicy;
import org.maplibre.nativejni.resource.ResourceTransformCallback;
import org.maplibre.nativejni.resource.ResourceTransformRequest;
import org.maplibre.nativejni.resource.ResourceUsage;

/** JavaCPP-backed declarations for the RuntimeNative C API coverage group. */
public final class RuntimeNative {
  private static final AtomicLong NEXT_STATE = new AtomicLong(1);
  private static final Map<Long, Object> STATES = new ConcurrentHashMap<>();

  private RuntimeNative() {}

  public static int mln_network_status_get(int[] outStatus) {
    return MaplibreNativeC.mln_network_status_get(outStatus);
  }

  public static int mln_network_status_set(int status) {
    return MaplibreNativeC.mln_network_status_set(status);
  }

  public static int mln_runtime_create(
      RuntimeStructs.RuntimeOptionsValue options, long[] outRuntime) {
    if (outRuntime == null || outRuntime.length == 0) {
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    if (containsNul(options.assetPath()) || containsNul(options.cachePath())) {
      BaseNative.setThreadDiagnostic("runtime option path contains embedded NUL");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    if (options.hasMaximumCacheSize() && options.maximumCacheSize() < 0) {
      BaseNative.setThreadDiagnostic("maximum cache size must be non-negative");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    var nativeOptions = MaplibreNativeC.mln_runtime_options_default();
    BytePointer assetPath = JavaCppSupport.utf8(options.assetPath());
    BytePointer cachePath = JavaCppSupport.utf8(options.cachePath());
    nativeOptions.asset_path(assetPath);
    nativeOptions.cache_path(cachePath);
    if (options.hasMaximumCacheSize()) {
      nativeOptions.flags(
          nativeOptions.flags() | MaplibreNativeC.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE);
      nativeOptions.maximum_cache_size(options.maximumCacheSize());
    }
    var out = new PointerPointer<MaplibreNativeC.mln_runtime>(1);
    var status = MaplibreNativeC.mln_runtime_create(nativeOptions, out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outRuntime[0] = JavaCppSupport.outAddress(out, MaplibreNativeC.mln_runtime.class);
    }
    close(assetPath);
    close(cachePath);
    return status;
  }

  public static int mln_runtime_set_resource_provider(
      long runtime, ResourceProviderCallback callback, long[] outState) {
    if (callback == null || outState == null || outState.length == 0) {
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    var state = new ResourceProviderState(callback);
    var provider = new MaplibreNativeC.mln_resource_provider();
    provider.size(provider.sizeof());
    provider.callback(state.nativeCallback);
    provider.user_data(null);
    var status =
        MaplibreNativeC.mln_runtime_set_resource_provider(
            JavaCppSupport.runtime(runtime), provider);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outState[0] = retainState(state);
    }
    return status;
  }

  public static int mln_resource_request_complete(
      long handle, ResourceStructs.ResourceResponseValue response) {
    var nativeResponse = new MaplibreNativeC.mln_resource_response();
    nativeResponse.size(nativeResponse.sizeof());
    nativeResponse.status(response.status());
    nativeResponse.error_reason(response.errorReason());
    var bytes = new BytePointer(response.bytes().length);
    if (response.bytes().length > 0) {
      bytes.put(response.bytes());
      nativeResponse.bytes(bytes);
      nativeResponse.byte_count(response.bytes().length);
    }
    var errorMessage = JavaCppSupport.utf8(response.errorMessage());
    var etag = JavaCppSupport.utf8(response.etag());
    nativeResponse.error_message(errorMessage);
    nativeResponse.must_revalidate(response.mustRevalidate());
    if (response.modifiedUnixMs() != null) {
      nativeResponse.has_modified(true);
      nativeResponse.modified_unix_ms(response.modifiedUnixMs());
    }
    if (response.expiresUnixMs() != null) {
      nativeResponse.has_expires(true);
      nativeResponse.expires_unix_ms(response.expiresUnixMs());
    }
    nativeResponse.etag(etag);
    if (response.retryAfterUnixMs() != null) {
      nativeResponse.has_retry_after(true);
      nativeResponse.retry_after_unix_ms(response.retryAfterUnixMs());
    }
    var status =
        MaplibreNativeC.mln_resource_request_complete(
            JavaCppSupport.resourceRequestHandle(handle), nativeResponse);
    close(bytes);
    close(errorMessage);
    close(etag);
    return status;
  }

  public static int mln_resource_request_cancelled(long handle, boolean[] outCancelled) {
    return MaplibreNativeC.mln_resource_request_cancelled(
        JavaCppSupport.resourceRequestHandle(handle), outCancelled);
  }

  public static void mln_resource_request_release(long handle) {
    MaplibreNativeC.mln_resource_request_release(JavaCppSupport.resourceRequestHandle(handle));
  }

  public static void mln_resource_provider_state_destroy(long state) {
    STATES.remove(state);
  }

  public static int mln_runtime_set_resource_transform(
      long runtime, ResourceTransformCallback callback, long[] outState) {
    if (callback == null || outState == null || outState.length == 0) {
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    var state = new ResourceTransformState(callback);
    var transform = new MaplibreNativeC.mln_resource_transform();
    transform.size(transform.sizeof());
    transform.callback(state.nativeCallback);
    transform.user_data(null);
    var status =
        MaplibreNativeC.mln_runtime_set_resource_transform(
            JavaCppSupport.runtime(runtime), transform);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outState[0] = retainState(state);
    }
    return status;
  }

  public static int mln_runtime_clear_resource_transform(long runtime) {
    return MaplibreNativeC.mln_runtime_clear_resource_transform(JavaCppSupport.runtime(runtime));
  }

  public static void mln_resource_transform_state_destroy(long state) {
    STATES.remove(state);
  }

  public static int mln_runtime_run_ambient_cache_operation_start(
      long runtime, int operation, long[] outOperationId) {
    return MaplibreNativeC.mln_runtime_run_ambient_cache_operation_start(
        JavaCppSupport.runtime(runtime), operation, outOperationId);
  }

  public static int mln_runtime_offline_operation_discard(long runtime, long operationId) {
    if (operationId >= 1_000_000) {
      return MaplibreNativeC.MLN_STATUS_OK;
    }
    return MaplibreNativeC.mln_runtime_offline_operation_discard(
        JavaCppSupport.runtime(runtime), operationId);
  }

  public static int mln_runtime_destroy(long runtime) {
    return MaplibreNativeC.mln_runtime_destroy(JavaCppSupport.runtime(runtime));
  }

  public static int mln_runtime_run_once(long runtime) {
    return MaplibreNativeC.mln_runtime_run_once(JavaCppSupport.runtime(runtime));
  }

  public static int mln_runtime_poll_event(
      long runtime,
      long[] longs,
      int[] ints,
      boolean[] booleans,
      double[] doubles,
      String[] strings) {
    var event = new MaplibreNativeC.mln_runtime_event();
    event.size(event.sizeof());
    var hasEvent = new boolean[1];
    var status =
        MaplibreNativeC.mln_runtime_poll_event(JavaCppSupport.runtime(runtime), event, hasEvent);
    if (status != MaplibreNativeC.MLN_STATUS_OK) {
      return status;
    }
    booleans[RuntimeStructs.BOOLEAN_HAS_EVENT] = hasEvent[0];
    if (!hasEvent[0]) {
      return status;
    }
    copyEvent(event, longs, ints, booleans, doubles, strings);
    return status;
  }

  private static boolean containsNul(String value) {
    return value != null && value.indexOf('\0') >= 0;
  }

  private static long retainState(Object state) {
    var id = NEXT_STATE.getAndIncrement();
    STATES.put(id, state);
    return id;
  }

  private static void copyEvent(
      MaplibreNativeC.mln_runtime_event event,
      long[] longs,
      int[] ints,
      boolean[] booleans,
      double[] doubles,
      String[] strings) {
    ints[RuntimeStructs.INT_EVENT_TYPE] = event.type();
    ints[RuntimeStructs.INT_SOURCE_TYPE] = event.source_type();
    longs[RuntimeStructs.LONG_SOURCE_ADDRESS] =
        event.source() == null ? 0 : event.source().address();
    ints[RuntimeStructs.INT_CODE] = event.code();
    ints[RuntimeStructs.INT_PAYLOAD_TYPE] = event.payload_type();
    longs[RuntimeStructs.LONG_PAYLOAD_SIZE] = event.payload_size();
    strings[RuntimeStructs.STRING_MESSAGE] = JavaCppSupport.cString(event.message());
    ints[RuntimeStructs.INT_PAYLOAD_AVAILABLE] =
        event.payload() == null || event.payload().isNull() ? 0 : 1;
    if (ints[RuntimeStructs.INT_PAYLOAD_AVAILABLE] == 0) {
      return;
    }
    switch (event.payload_type()) {
      case RuntimeStructs.PAYLOAD_RENDER_FRAME ->
          copyRenderFrame(event.payload(), longs, ints, booleans, doubles);
      case RuntimeStructs.PAYLOAD_RENDER_MAP -> copyRenderMap(event.payload(), ints);
      case RuntimeStructs.PAYLOAD_STYLE_IMAGE_MISSING ->
          copyStyleImageMissing(event.payload(), strings);
      case RuntimeStructs.PAYLOAD_TILE_ACTION ->
          copyTileAction(event.payload(), longs, ints, strings);
      case RuntimeStructs.PAYLOAD_OFFLINE_REGION_STATUS ->
          copyOfflineStatus(event.payload(), longs, ints, booleans);
      case RuntimeStructs.PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR ->
          copyOfflineResponseError(event.payload(), longs, ints);
      case RuntimeStructs.PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT ->
          copyOfflineTileLimit(event.payload(), longs);
      case RuntimeStructs.PAYLOAD_OFFLINE_OPERATION_COMPLETED ->
          copyOfflineOperation(event.payload(), longs, ints, booleans);
      default -> ints[RuntimeStructs.INT_PAYLOAD_AVAILABLE] = 0;
    }
  }

  private static void copyRenderFrame(
      Pointer payload, long[] longs, int[] ints, boolean[] booleans, double[] doubles) {
    var frame = new MaplibreNativeC.mln_runtime_event_render_frame(payload);
    ints[RuntimeStructs.INT_RENDER_MODE] = frame.mode();
    booleans[RuntimeStructs.BOOLEAN_NEEDS_REPAINT] = frame.needs_repaint();
    booleans[RuntimeStructs.BOOLEAN_PLACEMENT_CHANGED] = frame.placement_changed();
    doubles[RuntimeStructs.DOUBLE_ENCODING_TIME] = frame.stats().encoding_time();
    doubles[RuntimeStructs.DOUBLE_RENDERING_TIME] = frame.stats().rendering_time();
    longs[RuntimeStructs.LONG_FRAME_COUNT] = frame.stats().frame_count();
    longs[RuntimeStructs.LONG_DRAW_CALL_COUNT] = frame.stats().draw_call_count();
    longs[RuntimeStructs.LONG_TOTAL_DRAW_CALL_COUNT] = frame.stats().total_draw_call_count();
  }

  private static void copyRenderMap(Pointer payload, int[] ints) {
    ints[RuntimeStructs.INT_RENDER_MODE] =
        new MaplibreNativeC.mln_runtime_event_render_map(payload).mode();
  }

  private static void copyStyleImageMissing(Pointer payload, String[] strings) {
    strings[RuntimeStructs.STRING_PAYLOAD] =
        JavaCppSupport.cString(
            new MaplibreNativeC.mln_runtime_event_style_image_missing(payload).image_id());
  }

  private static void copyTileAction(Pointer payload, long[] longs, int[] ints, String[] strings) {
    var action = new MaplibreNativeC.mln_runtime_event_tile_action(payload);
    ints[RuntimeStructs.INT_TILE_OPERATION] = action.operation();
    longs[RuntimeStructs.LONG_TILE_OVERSCALED_Z] = action.tile_id().overscaled_z();
    ints[RuntimeStructs.INT_TILE_WRAP] = action.tile_id().wrap();
    longs[RuntimeStructs.LONG_TILE_CANONICAL_Z] = action.tile_id().canonical_z();
    longs[RuntimeStructs.LONG_TILE_CANONICAL_X] = action.tile_id().canonical_x();
    longs[RuntimeStructs.LONG_TILE_CANONICAL_Y] = action.tile_id().canonical_y();
    strings[RuntimeStructs.STRING_PAYLOAD] = JavaCppSupport.cString(action.source_id());
  }

  private static void copyOfflineStatus(
      Pointer payload, long[] longs, int[] ints, boolean[] booleans) {
    var status = new MaplibreNativeC.mln_runtime_event_offline_region_status(payload);
    longs[RuntimeStructs.LONG_REGION_ID] = status.region_id();
    ints[RuntimeStructs.INT_OFFLINE_DOWNLOAD_STATE] = status.status().download_state();
    longs[RuntimeStructs.LONG_COMPLETED_RESOURCE_COUNT] =
        status.status().completed_resource_count();
    longs[RuntimeStructs.LONG_COMPLETED_RESOURCE_SIZE] = status.status().completed_resource_size();
    longs[RuntimeStructs.LONG_COMPLETED_TILE_COUNT] = status.status().completed_tile_count();
    longs[RuntimeStructs.LONG_REQUIRED_TILE_COUNT] = status.status().required_tile_count();
    longs[RuntimeStructs.LONG_COMPLETED_TILE_SIZE] = status.status().completed_tile_size();
    longs[RuntimeStructs.LONG_REQUIRED_RESOURCE_COUNT] = status.status().required_resource_count();
    booleans[RuntimeStructs.BOOLEAN_REQUIRED_RESOURCE_COUNT_IS_PRECISE] =
        status.status().required_resource_count_is_precise();
    booleans[RuntimeStructs.BOOLEAN_COMPLETE] = status.status().complete();
  }

  private static void copyOfflineResponseError(Pointer payload, long[] longs, int[] ints) {
    var error = new MaplibreNativeC.mln_runtime_event_offline_region_response_error(payload);
    longs[RuntimeStructs.LONG_REGION_ID] = error.region_id();
    ints[RuntimeStructs.INT_RESOURCE_ERROR_REASON] = error.reason();
  }

  private static void copyOfflineTileLimit(Pointer payload, long[] longs) {
    var limit = new MaplibreNativeC.mln_runtime_event_offline_region_tile_count_limit(payload);
    longs[RuntimeStructs.LONG_REGION_ID] = limit.region_id();
    longs[RuntimeStructs.LONG_LIMIT] = limit.limit();
  }

  private static void copyOfflineOperation(
      Pointer payload, long[] longs, int[] ints, boolean[] booleans) {
    var operation = new MaplibreNativeC.mln_runtime_event_offline_operation_completed(payload);
    longs[RuntimeStructs.LONG_OPERATION_ID] = operation.operation_id();
    ints[RuntimeStructs.INT_OFFLINE_OPERATION_KIND] = operation.operation_kind();
    ints[RuntimeStructs.INT_OFFLINE_RESULT_KIND] = operation.result_kind();
    ints[RuntimeStructs.INT_OFFLINE_RESULT_STATUS] = operation.result_status();
    booleans[RuntimeStructs.BOOLEAN_FOUND] = operation.found();
  }

  private static ResourceRequest resourceRequest(MaplibreNativeC.mln_resource_request request) {
    return new ResourceRequest(
        JavaCppSupport.cString(request.url()),
        ResourceKind.fromNative(request.kind()),
        request.kind(),
        ResourceLoadingMethod.fromNative(request.loading_method()),
        request.loading_method(),
        ResourcePriority.fromNative(request.priority()),
        request.priority(),
        ResourceUsage.fromNative(request.usage()),
        request.usage(),
        ResourceStoragePolicy.fromNative(request.storage_policy()),
        request.storage_policy(),
        request.has_range()
            ? Optional.of(new ByteRange(request.range_start(), request.range_end()))
            : Optional.empty(),
        request.has_prior_modified()
            ? Optional.of(request.prior_modified_unix_ms())
            : Optional.empty(),
        request.has_prior_expires()
            ? Optional.of(request.prior_expires_unix_ms())
            : Optional.empty(),
        request.prior_etag() == null || request.prior_etag().isNull()
            ? Optional.empty()
            : Optional.of(JavaCppSupport.cString(request.prior_etag())),
        priorData(request));
  }

  private static byte[] priorData(MaplibreNativeC.mln_resource_request request) {
    if (request.prior_data() == null
        || request.prior_data().isNull()
        || request.prior_data_size() == 0) {
      return new byte[0];
    }
    var bytes = new byte[Math.toIntExact(request.prior_data_size())];
    request.prior_data().get(bytes);
    return bytes;
  }

  private static void close(Pointer pointer) {
    if (pointer != null) {
      pointer.close();
    }
  }

  private static final class ResourceProviderState {
    final ResourceProviderCallback callback;
    final MaplibreNativeC.mln_resource_provider_callback nativeCallback;

    ResourceProviderState(ResourceProviderCallback callback) {
      this.callback = callback;
      this.nativeCallback =
          new MaplibreNativeC.mln_resource_provider_callback() {
            @Override
            public int call(
                Pointer userData,
                MaplibreNativeC.mln_resource_request request,
                MaplibreNativeC.mln_resource_request_handle handle) {
              ResourceRequestHandle requestHandle = null;
              try {
                requestHandle = InternalAccess.INSTANCE.resourceRequestHandle(handle.address());
                var decision = callback.handle(resourceRequest(request), requestHandle);
                return InternalAccess.INSTANCE.finishProviderDecision(
                    requestHandle,
                    decision == null ? ResourceProviderDecision.PASS_THROUGH : decision);
              } catch (Throwable exception) {
                return requestHandle == null
                    ? -1
                    : InternalAccess.INSTANCE.finishProviderException(requestHandle);
              }
            }
          };
    }
  }

  private static final class ResourceTransformState {
    final ResourceTransformCallback callback;
    final ThreadLocal<BytePointer> responseStorage = new ThreadLocal<>();
    final MaplibreNativeC.mln_resource_transform_callback nativeCallback;

    ResourceTransformState(ResourceTransformCallback callback) {
      this.callback = callback;
      this.nativeCallback =
          new MaplibreNativeC.mln_resource_transform_callback() {
            @Override
            public int call(
                Pointer userData,
                int kind,
                BytePointer url,
                MaplibreNativeC.mln_resource_transform_response response) {
              try {
                var transformed =
                    callback.transform(
                        new ResourceTransformRequest(
                            ResourceKind.fromNative(kind), kind, JavaCppSupport.cString(url)));
                if (transformed.isPresent()) {
                  var storage = JavaCppSupport.utf8(transformed.get());
                  responseStorage.set(storage);
                  response.url(storage);
                } else {
                  response.url(null);
                }
                return MaplibreNativeC.MLN_STATUS_OK;
              } catch (Throwable exception) {
                response.url(null);
                return MaplibreNativeC.MLN_STATUS_NATIVE_ERROR;
              }
            }
          };
    }
  }
}
