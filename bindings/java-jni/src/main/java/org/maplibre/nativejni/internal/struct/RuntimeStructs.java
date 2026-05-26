package org.maplibre.nativejni.internal.struct;

import java.util.Objects;
import java.util.Optional;
import java.util.OptionalLong;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.geo.TileId;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.map.RenderingStats;
import org.maplibre.nativejni.map.TileOperation;
import org.maplibre.nativejni.offline.OfflineRegionDownloadState;
import org.maplibre.nativejni.offline.OfflineRegionStatus;
import org.maplibre.nativejni.render.RenderMode;
import org.maplibre.nativejni.resource.ResourceErrorReason;
import org.maplibre.nativejni.runtime.OfflineOperationKind;
import org.maplibre.nativejni.runtime.OfflineOperationResultKind;
import org.maplibre.nativejni.runtime.RuntimeEvent;
import org.maplibre.nativejni.runtime.RuntimeEventPayload;
import org.maplibre.nativejni.runtime.RuntimeEventSourceType;
import org.maplibre.nativejni.runtime.RuntimeEventType;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.runtime.RuntimeOptions;

/** Internal materializers for runtime options, events, and offline operation data. */
public final class RuntimeStructs {
  public static final int LONG_SOURCE_ADDRESS = 0;
  public static final int LONG_PAYLOAD_SIZE = 1;
  public static final int LONG_TILE_OVERSCALED_Z = 2;
  public static final int LONG_TILE_CANONICAL_Z = 3;
  public static final int LONG_TILE_CANONICAL_X = 4;
  public static final int LONG_TILE_CANONICAL_Y = 5;
  public static final int LONG_REGION_ID = 6;
  public static final int LONG_LIMIT = 7;
  public static final int LONG_OPERATION_ID = 8;
  public static final int LONG_COMPLETED_RESOURCE_COUNT = 9;
  public static final int LONG_COMPLETED_RESOURCE_SIZE = 10;
  public static final int LONG_COMPLETED_TILE_COUNT = 11;
  public static final int LONG_REQUIRED_TILE_COUNT = 12;
  public static final int LONG_COMPLETED_TILE_SIZE = 13;
  public static final int LONG_REQUIRED_RESOURCE_COUNT = 14;
  public static final int LONG_FRAME_COUNT = 15;
  public static final int LONG_DRAW_CALL_COUNT = 16;
  public static final int LONG_TOTAL_DRAW_CALL_COUNT = 17;
  public static final int LONG_COUNT = 18;

  public static final int INT_EVENT_TYPE = 0;
  public static final int INT_SOURCE_TYPE = 1;
  public static final int INT_CODE = 2;
  public static final int INT_PAYLOAD_TYPE = 3;
  public static final int INT_RENDER_MODE = 4;
  public static final int INT_TILE_OPERATION = 5;
  public static final int INT_TILE_WRAP = 6;
  public static final int INT_RESOURCE_ERROR_REASON = 7;
  public static final int INT_OFFLINE_DOWNLOAD_STATE = 8;
  public static final int INT_OFFLINE_OPERATION_KIND = 9;
  public static final int INT_OFFLINE_RESULT_KIND = 10;
  public static final int INT_OFFLINE_RESULT_STATUS = 11;
  public static final int INT_PAYLOAD_AVAILABLE = 12;
  public static final int INT_COUNT = 13;

  public static final int BOOLEAN_HAS_EVENT = 0;
  public static final int BOOLEAN_NEEDS_REPAINT = 1;
  public static final int BOOLEAN_PLACEMENT_CHANGED = 2;
  public static final int BOOLEAN_REQUIRED_RESOURCE_COUNT_IS_PRECISE = 3;
  public static final int BOOLEAN_COMPLETE = 4;
  public static final int BOOLEAN_FOUND = 5;
  public static final int BOOLEAN_COUNT = 6;

  public static final int DOUBLE_ENCODING_TIME = 0;
  public static final int DOUBLE_RENDERING_TIME = 1;
  public static final int DOUBLE_COUNT = 2;

  public static final int STRING_MESSAGE = 0;
  public static final int STRING_PAYLOAD = 1;
  public static final int STRING_COUNT = 2;

  public static final int PAYLOAD_NONE = 0;
  public static final int PAYLOAD_RENDER_FRAME = 1;
  public static final int PAYLOAD_RENDER_MAP = 2;
  public static final int PAYLOAD_STYLE_IMAGE_MISSING = 3;
  public static final int PAYLOAD_TILE_ACTION = 4;
  public static final int PAYLOAD_OFFLINE_REGION_STATUS = 5;
  public static final int PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR = 6;
  public static final int PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT = 7;
  public static final int PAYLOAD_OFFLINE_OPERATION_COMPLETED = 8;

  private RuntimeStructs() {}

  public record RuntimeOptionsValue(
      String assetPath, String cachePath, boolean hasMaximumCacheSize, long maximumCacheSize) {}

  public static RuntimeOptionsValue runtimeOptions(RuntimeOptions options) {
    Objects.requireNonNull(options, "options");
    OptionalLong maximumCacheSize = options.maximumCacheSize();
    return new RuntimeOptionsValue(
        options.assetPath(),
        options.cachePath(),
        maximumCacheSize.isPresent(),
        maximumCacheSize.orElse(0));
  }

  public static NativeRuntimeOptionsScope nativeRuntimeOptions(RuntimeOptions options) {
    return new NativeRuntimeOptionsScope(runtimeOptions(options));
  }

  public static void copyEvent(
      MaplibreNativeC.mln_runtime_event event,
      long[] longs,
      int[] ints,
      boolean[] booleans,
      double[] doubles,
      String[] strings) {
    ints[INT_EVENT_TYPE] = event.type();
    ints[INT_SOURCE_TYPE] = event.source_type();
    longs[LONG_SOURCE_ADDRESS] = event.source() == null ? 0 : event.source().address();
    ints[INT_CODE] = event.code();
    ints[INT_PAYLOAD_TYPE] = event.payload_type();
    longs[LONG_PAYLOAD_SIZE] = event.payload_size();
    strings[STRING_MESSAGE] = JavaCppSupport.utf8String(event.message(), event.message_size());
    ints[INT_PAYLOAD_AVAILABLE] = event.payload() == null || event.payload().isNull() ? 0 : 1;
    if (ints[INT_PAYLOAD_AVAILABLE] == 0) {
      return;
    }
    switch (event.payload_type()) {
      case PAYLOAD_RENDER_FRAME -> {
        if (payloadSizeAtLeast(event, PayloadSizes.RENDER_FRAME, ints)) {
          copyRenderFrame(event.payload(), longs, ints, booleans, doubles);
        }
      }
      case PAYLOAD_RENDER_MAP -> {
        if (payloadSizeAtLeast(event, PayloadSizes.RENDER_MAP, ints)) {
          copyRenderMap(event.payload(), ints);
        }
      }
      case PAYLOAD_STYLE_IMAGE_MISSING -> {
        if (payloadSizeAtLeast(event, PayloadSizes.STYLE_IMAGE_MISSING, ints)) {
          copyStyleImageMissing(event.payload(), strings);
        }
      }
      case PAYLOAD_TILE_ACTION -> {
        if (payloadSizeAtLeast(event, PayloadSizes.TILE_ACTION, ints)) {
          copyTileAction(event.payload(), longs, ints, strings);
        }
      }
      case PAYLOAD_OFFLINE_REGION_STATUS -> {
        if (payloadSizeAtLeast(event, PayloadSizes.OFFLINE_REGION_STATUS, ints)) {
          copyOfflineStatus(event.payload(), longs, ints, booleans);
        }
      }
      case PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR -> {
        if (payloadSizeAtLeast(event, PayloadSizes.OFFLINE_REGION_RESPONSE_ERROR, ints)) {
          copyOfflineResponseError(event.payload(), longs, ints);
        }
      }
      case PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT -> {
        if (payloadSizeAtLeast(event, PayloadSizes.OFFLINE_REGION_TILE_COUNT_LIMIT, ints)) {
          copyOfflineTileLimit(event.payload(), longs);
        }
      }
      case PAYLOAD_OFFLINE_OPERATION_COMPLETED -> {
        if (payloadSizeAtLeast(event, PayloadSizes.OFFLINE_OPERATION_COMPLETED, ints)) {
          copyOfflineOperation(event.payload(), longs, ints, booleans);
        }
      }
      default -> ints[INT_PAYLOAD_AVAILABLE] = 0;
    }
  }

  private static boolean payloadSizeAtLeast(
      MaplibreNativeC.mln_runtime_event event, long expectedSize, int[] ints) {
    if (event.payload_size() >= expectedSize) {
      return true;
    }
    ints[INT_PAYLOAD_AVAILABLE] = 0;
    return false;
  }

  private static void copyRenderFrame(
      Pointer payload, long[] longs, int[] ints, boolean[] booleans, double[] doubles) {
    var frame = new MaplibreNativeC.mln_runtime_event_render_frame(payload);
    ints[INT_RENDER_MODE] = frame.mode();
    booleans[BOOLEAN_NEEDS_REPAINT] = frame.needs_repaint();
    booleans[BOOLEAN_PLACEMENT_CHANGED] = frame.placement_changed();
    doubles[DOUBLE_ENCODING_TIME] = frame.stats().encoding_time();
    doubles[DOUBLE_RENDERING_TIME] = frame.stats().rendering_time();
    longs[LONG_FRAME_COUNT] = frame.stats().frame_count();
    longs[LONG_DRAW_CALL_COUNT] = frame.stats().draw_call_count();
    longs[LONG_TOTAL_DRAW_CALL_COUNT] = frame.stats().total_draw_call_count();
  }

  private static void copyRenderMap(Pointer payload, int[] ints) {
    ints[INT_RENDER_MODE] = new MaplibreNativeC.mln_runtime_event_render_map(payload).mode();
  }

  private static void copyStyleImageMissing(Pointer payload, String[] strings) {
    var missing = new MaplibreNativeC.mln_runtime_event_style_image_missing(payload);
    strings[STRING_PAYLOAD] =
        JavaCppSupport.utf8String(missing.image_id(), missing.image_id_size());
  }

  private static void copyTileAction(Pointer payload, long[] longs, int[] ints, String[] strings) {
    var action = new MaplibreNativeC.mln_runtime_event_tile_action(payload);
    ints[INT_TILE_OPERATION] = action.operation();
    longs[LONG_TILE_OVERSCALED_Z] = action.tile_id().overscaled_z();
    ints[INT_TILE_WRAP] = action.tile_id().wrap();
    longs[LONG_TILE_CANONICAL_Z] = action.tile_id().canonical_z();
    longs[LONG_TILE_CANONICAL_X] = action.tile_id().canonical_x();
    longs[LONG_TILE_CANONICAL_Y] = action.tile_id().canonical_y();
    strings[STRING_PAYLOAD] =
        JavaCppSupport.utf8String(action.source_id(), action.source_id_size());
  }

  private static void copyOfflineStatus(
      Pointer payload, long[] longs, int[] ints, boolean[] booleans) {
    var status = new MaplibreNativeC.mln_runtime_event_offline_region_status(payload);
    longs[LONG_REGION_ID] = status.region_id();
    ints[INT_OFFLINE_DOWNLOAD_STATE] = status.status().download_state();
    longs[LONG_COMPLETED_RESOURCE_COUNT] = status.status().completed_resource_count();
    longs[LONG_COMPLETED_RESOURCE_SIZE] = status.status().completed_resource_size();
    longs[LONG_COMPLETED_TILE_COUNT] = status.status().completed_tile_count();
    longs[LONG_REQUIRED_TILE_COUNT] = status.status().required_tile_count();
    longs[LONG_COMPLETED_TILE_SIZE] = status.status().completed_tile_size();
    longs[LONG_REQUIRED_RESOURCE_COUNT] = status.status().required_resource_count();
    booleans[BOOLEAN_REQUIRED_RESOURCE_COUNT_IS_PRECISE] =
        status.status().required_resource_count_is_precise();
    booleans[BOOLEAN_COMPLETE] = status.status().complete();
  }

  private static void copyOfflineResponseError(Pointer payload, long[] longs, int[] ints) {
    var error = new MaplibreNativeC.mln_runtime_event_offline_region_response_error(payload);
    longs[LONG_REGION_ID] = error.region_id();
    ints[INT_RESOURCE_ERROR_REASON] = error.reason();
  }

  private static void copyOfflineTileLimit(Pointer payload, long[] longs) {
    var limit = new MaplibreNativeC.mln_runtime_event_offline_region_tile_count_limit(payload);
    longs[LONG_REGION_ID] = limit.region_id();
    longs[LONG_LIMIT] = limit.limit();
  }

  private static void copyOfflineOperation(
      Pointer payload, long[] longs, int[] ints, boolean[] booleans) {
    var operation = new MaplibreNativeC.mln_runtime_event_offline_operation_completed(payload);
    longs[LONG_OPERATION_ID] = operation.operation_id();
    ints[INT_OFFLINE_OPERATION_KIND] = operation.operation_kind();
    ints[INT_OFFLINE_RESULT_KIND] = operation.result_kind();
    ints[INT_OFFLINE_RESULT_STATUS] = operation.result_status();
    booleans[BOOLEAN_FOUND] = operation.found();
  }

  public static RuntimeEvent runtimeEvent(
      long[] longs,
      int[] ints,
      boolean[] booleans,
      double[] doubles,
      String[] strings,
      Optional<RuntimeHandle> runtimeSource,
      Optional<MapHandle> mapSource) {
    Objects.requireNonNull(longs, "longs");
    Objects.requireNonNull(ints, "ints");
    Objects.requireNonNull(booleans, "booleans");
    Objects.requireNonNull(doubles, "doubles");
    Objects.requireNonNull(strings, "strings");
    requireLength(longs.length, LONG_COUNT, "longs");
    requireLength(ints.length, INT_COUNT, "ints");
    requireLength(booleans.length, BOOLEAN_COUNT, "booleans");
    requireLength(doubles.length, DOUBLE_COUNT, "doubles");
    requireLength(strings.length, STRING_COUNT, "strings");

    var rawType = ints[INT_EVENT_TYPE];
    var rawSourceType = ints[INT_SOURCE_TYPE];
    var rawPayloadType = ints[INT_PAYLOAD_TYPE];
    return new RuntimeEvent(
        RuntimeEventType.fromNative(rawType),
        rawType,
        RuntimeEventSourceType.fromNative(rawSourceType),
        rawSourceType,
        runtimeSource,
        mapSource,
        ints[INT_CODE],
        rawPayloadType,
        runtimeEventPayload(longs, ints, booleans, doubles, strings),
        nullToEmpty(strings[STRING_MESSAGE]));
  }

  public static RuntimeEventPayload runtimeEventPayload(
      long[] longs, int[] ints, boolean[] booleans, double[] doubles, String[] strings) {
    var rawPayloadType = ints[INT_PAYLOAD_TYPE];
    if (rawPayloadType == PAYLOAD_NONE) {
      return RuntimeEventPayload.NONE;
    }
    if (ints[INT_PAYLOAD_AVAILABLE] == 0) {
      return new RuntimeEventPayload.Unknown(rawPayloadType, longs[LONG_PAYLOAD_SIZE]);
    }

    return switch (rawPayloadType) {
      case PAYLOAD_RENDER_FRAME -> renderFrame(longs, ints, booleans, doubles);
      case PAYLOAD_RENDER_MAP -> renderMap(ints);
      case PAYLOAD_STYLE_IMAGE_MISSING ->
          new RuntimeEventPayload.StyleImageMissing(nullToEmpty(strings[STRING_PAYLOAD]));
      case PAYLOAD_TILE_ACTION -> tileAction(longs, ints, strings);
      case PAYLOAD_OFFLINE_REGION_STATUS -> offlineRegionStatus(longs, ints, booleans);
      case PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR -> offlineRegionResponseError(longs, ints);
      case PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT ->
          new RuntimeEventPayload.OfflineRegionTileCountLimit(
              longs[LONG_REGION_ID], longs[LONG_LIMIT]);
      case PAYLOAD_OFFLINE_OPERATION_COMPLETED -> offlineOperationCompleted(longs, ints, booleans);
      default -> new RuntimeEventPayload.Unknown(rawPayloadType, longs[LONG_PAYLOAD_SIZE]);
    };
  }

  private static RuntimeEventPayload.RenderFrame renderFrame(
      long[] longs, int[] ints, boolean[] booleans, double[] doubles) {
    var rawMode = ints[INT_RENDER_MODE];
    return new RuntimeEventPayload.RenderFrame(
        RenderMode.fromNative(rawMode),
        rawMode,
        booleans[BOOLEAN_NEEDS_REPAINT],
        booleans[BOOLEAN_PLACEMENT_CHANGED],
        new RenderingStats(
            doubles[DOUBLE_ENCODING_TIME],
            doubles[DOUBLE_RENDERING_TIME],
            longs[LONG_FRAME_COUNT],
            longs[LONG_DRAW_CALL_COUNT],
            longs[LONG_TOTAL_DRAW_CALL_COUNT]));
  }

  private static RuntimeEventPayload.RenderMap renderMap(int[] ints) {
    var rawMode = ints[INT_RENDER_MODE];
    return new RuntimeEventPayload.RenderMap(RenderMode.fromNative(rawMode), rawMode);
  }

  private static RuntimeEventPayload.TileAction tileAction(
      long[] longs, int[] ints, String[] strings) {
    var rawOperation = ints[INT_TILE_OPERATION];
    return new RuntimeEventPayload.TileAction(
        TileOperation.fromNative(rawOperation),
        rawOperation,
        new TileId(
            longs[LONG_TILE_OVERSCALED_Z],
            ints[INT_TILE_WRAP],
            longs[LONG_TILE_CANONICAL_Z],
            longs[LONG_TILE_CANONICAL_X],
            longs[LONG_TILE_CANONICAL_Y]),
        nullToEmpty(strings[STRING_PAYLOAD]));
  }

  private static RuntimeEventPayload.OfflineRegionStatusChanged offlineRegionStatus(
      long[] longs, int[] ints, boolean[] booleans) {
    var rawDownloadState = ints[INT_OFFLINE_DOWNLOAD_STATE];
    return new RuntimeEventPayload.OfflineRegionStatusChanged(
        longs[LONG_REGION_ID],
        new OfflineRegionStatus(
            OfflineRegionDownloadState.fromNative(rawDownloadState),
            rawDownloadState,
            longs[LONG_COMPLETED_RESOURCE_COUNT],
            longs[LONG_COMPLETED_RESOURCE_SIZE],
            longs[LONG_COMPLETED_TILE_COUNT],
            longs[LONG_REQUIRED_TILE_COUNT],
            longs[LONG_COMPLETED_TILE_SIZE],
            longs[LONG_REQUIRED_RESOURCE_COUNT],
            booleans[BOOLEAN_REQUIRED_RESOURCE_COUNT_IS_PRECISE],
            booleans[BOOLEAN_COMPLETE]));
  }

  private static RuntimeEventPayload.OfflineRegionResponseError offlineRegionResponseError(
      long[] longs, int[] ints) {
    var rawReason = ints[INT_RESOURCE_ERROR_REASON];
    return new RuntimeEventPayload.OfflineRegionResponseError(
        longs[LONG_REGION_ID], ResourceErrorReason.fromNative(rawReason), rawReason);
  }

  private static RuntimeEventPayload.OfflineOperationCompleted offlineOperationCompleted(
      long[] longs, int[] ints, boolean[] booleans) {
    var rawOperationKind = ints[INT_OFFLINE_OPERATION_KIND];
    var rawResultKind = ints[INT_OFFLINE_RESULT_KIND];
    return new RuntimeEventPayload.OfflineOperationCompleted(
        longs[LONG_OPERATION_ID],
        OfflineOperationKind.fromNative(rawOperationKind),
        rawOperationKind,
        OfflineOperationResultKind.fromNative(rawResultKind),
        rawResultKind,
        ints[INT_OFFLINE_RESULT_STATUS],
        booleans[BOOLEAN_FOUND]);
  }

  private static final class PayloadSizes {
    private static final long RENDER_FRAME =
        sizeof(new MaplibreNativeC.mln_runtime_event_render_frame());
    private static final long RENDER_MAP =
        sizeof(new MaplibreNativeC.mln_runtime_event_render_map());
    private static final long STYLE_IMAGE_MISSING =
        sizeof(new MaplibreNativeC.mln_runtime_event_style_image_missing());
    private static final long TILE_ACTION =
        sizeof(new MaplibreNativeC.mln_runtime_event_tile_action());
    private static final long OFFLINE_REGION_STATUS =
        sizeof(new MaplibreNativeC.mln_runtime_event_offline_region_status());
    private static final long OFFLINE_REGION_RESPONSE_ERROR =
        sizeof(new MaplibreNativeC.mln_runtime_event_offline_region_response_error());
    private static final long OFFLINE_REGION_TILE_COUNT_LIMIT =
        sizeof(new MaplibreNativeC.mln_runtime_event_offline_region_tile_count_limit());
    private static final long OFFLINE_OPERATION_COMPLETED =
        sizeof(new MaplibreNativeC.mln_runtime_event_offline_operation_completed());

    private PayloadSizes() {}

    private static long sizeof(Pointer pointer) {
      try (pointer) {
        return pointer.sizeof();
      }
    }
  }

  private static void requireLength(int actual, int expected, String name) {
    if (actual < expected) {
      throw new IllegalArgumentException(name + " must contain at least " + expected + " entries");
    }
  }

  private static String nullToEmpty(String value) {
    return value == null ? "" : value;
  }

  public static final class NativeRuntimeOptionsScope implements AutoCloseable {
    private final MaplibreNativeC.mln_runtime_options options;
    private final BytePointer assetPath;
    private final BytePointer cachePath;

    private NativeRuntimeOptionsScope(RuntimeOptionsValue value) {
      if (containsNul(value.assetPath()) || containsNul(value.cachePath())) {
        JavaCppSupport.setThreadDiagnostic("runtime option path contains embedded NUL");
        throw new IllegalArgumentException("runtime option path contains embedded NUL");
      }
      if (value.hasMaximumCacheSize() && value.maximumCacheSize() < 0) {
        JavaCppSupport.setThreadDiagnostic("maximum cache size must be non-negative");
        throw new IllegalArgumentException("maximum cache size must be non-negative");
      }
      options = MaplibreNativeC.mln_runtime_options_default();
      assetPath = JavaCppSupport.utf8(value.assetPath());
      cachePath = JavaCppSupport.utf8(value.cachePath());
      options.asset_path(assetPath);
      options.cache_path(cachePath);
      if (value.hasMaximumCacheSize()) {
        options.flags(options.flags() | MaplibreNativeC.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE);
        options.maximum_cache_size(value.maximumCacheSize());
      }
    }

    public MaplibreNativeC.mln_runtime_options options() {
      return options;
    }

    @Override
    public void close() {
      options.close();
      close(assetPath);
      close(cachePath);
    }

    private static boolean containsNul(String value) {
      return value != null && value.indexOf('\0') >= 0;
    }

    private static void close(Pointer pointer) {
      if (pointer != null) {
        pointer.close();
      }
    }
  }
}
