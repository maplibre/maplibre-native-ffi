package org.maplibre.nativejni.internal.struct;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertInstanceOf;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.Optional;
import org.bytedeco.javacpp.BytePointer;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.map.TileOperation;
import org.maplibre.nativejni.offline.OfflineRegionDownloadState;
import org.maplibre.nativejni.render.RenderMode;
import org.maplibre.nativejni.resource.ResourceErrorReason;
import org.maplibre.nativejni.runtime.OfflineOperationKind;
import org.maplibre.nativejni.runtime.OfflineOperationResultKind;
import org.maplibre.nativejni.runtime.RuntimeEventPayload;
import org.maplibre.nativejni.runtime.RuntimeEventSourceType;
import org.maplibre.nativejni.runtime.RuntimeEventType;

final class RuntimeEventStructsTest {
  @Test
  void renderFramePayloadCopiesStatsAndFlags() {
    var longs = longs();
    var ints = ints(RuntimeStructs.PAYLOAD_RENDER_FRAME);
    var booleans = booleans();
    var doubles = doubles();
    ints[RuntimeStructs.INT_RENDER_MODE] = 1;
    booleans[RuntimeStructs.BOOLEAN_NEEDS_REPAINT] = true;
    booleans[RuntimeStructs.BOOLEAN_PLACEMENT_CHANGED] = true;
    doubles[RuntimeStructs.DOUBLE_ENCODING_TIME] = 1.5;
    doubles[RuntimeStructs.DOUBLE_RENDERING_TIME] = 2.5;
    longs[RuntimeStructs.LONG_FRAME_COUNT] = 3;
    longs[RuntimeStructs.LONG_DRAW_CALL_COUNT] = 4;
    longs[RuntimeStructs.LONG_TOTAL_DRAW_CALL_COUNT] = 5;
    var payload =
        assertInstanceOf(
            RuntimeEventPayload.RenderFrame.class,
            RuntimeStructs.runtimeEventPayload(longs, ints, booleans, doubles, strings()));
    assertSame(RenderMode.FULL, payload.mode());
    assertTrue(payload.needsRepaint());
    assertEquals(1.5, payload.stats().encodingTime(), 0.0);
    assertEquals(5, payload.stats().totalDrawCallCount());
  }

  @Test
  void stringAndTilePayloadsCopyBorrowedStrings() {
    var strings = strings();
    strings[RuntimeStructs.STRING_PAYLOAD] = "missing-image";
    var imagePayload =
        assertInstanceOf(
            RuntimeEventPayload.StyleImageMissing.class,
            RuntimeStructs.runtimeEventPayload(
                longs(),
                ints(RuntimeStructs.PAYLOAD_STYLE_IMAGE_MISSING),
                booleans(),
                doubles(),
                strings));
    assertEquals("missing-image", imagePayload.imageId());
    var longs = longs();
    var ints = ints(RuntimeStructs.PAYLOAD_TILE_ACTION);
    strings[RuntimeStructs.STRING_PAYLOAD] = "source";
    ints[RuntimeStructs.INT_TILE_OPERATION] = 0;
    ints[RuntimeStructs.INT_TILE_WRAP] = -1;
    longs[RuntimeStructs.LONG_TILE_OVERSCALED_Z] = 6;
    longs[RuntimeStructs.LONG_TILE_CANONICAL_Z] = 5;
    longs[RuntimeStructs.LONG_TILE_CANONICAL_X] = 4;
    longs[RuntimeStructs.LONG_TILE_CANONICAL_Y] = 3;
    var tilePayload =
        assertInstanceOf(
            RuntimeEventPayload.TileAction.class,
            RuntimeStructs.runtimeEventPayload(longs, ints, booleans(), doubles(), strings));
    assertSame(TileOperation.REQUESTED_FROM_CACHE, tilePayload.operation());
    assertEquals(-1, tilePayload.tileId().wrap());
    assertEquals("source", tilePayload.sourceId());
  }

  @Test
  void offlinePayloadsCopyStatusErrorLimitAndCompletion() {
    var statusLongs = longs();
    var statusInts = ints(RuntimeStructs.PAYLOAD_OFFLINE_REGION_STATUS);
    var statusBooleans = booleans();
    statusLongs[RuntimeStructs.LONG_REGION_ID] = 9;
    statusInts[RuntimeStructs.INT_OFFLINE_DOWNLOAD_STATE] =
        OfflineRegionDownloadState.ACTIVE.nativeValue();
    statusLongs[RuntimeStructs.LONG_COMPLETED_RESOURCE_COUNT] = 1;
    statusLongs[RuntimeStructs.LONG_COMPLETED_RESOURCE_SIZE] = 2;
    statusLongs[RuntimeStructs.LONG_COMPLETED_TILE_COUNT] = 3;
    statusLongs[RuntimeStructs.LONG_REQUIRED_TILE_COUNT] = 4;
    statusLongs[RuntimeStructs.LONG_COMPLETED_TILE_SIZE] = 5;
    statusLongs[RuntimeStructs.LONG_REQUIRED_RESOURCE_COUNT] = 6;
    statusBooleans[RuntimeStructs.BOOLEAN_REQUIRED_RESOURCE_COUNT_IS_PRECISE] = true;
    statusBooleans[RuntimeStructs.BOOLEAN_COMPLETE] = true;
    var statusPayload =
        assertInstanceOf(
            RuntimeEventPayload.OfflineRegionStatusChanged.class,
            RuntimeStructs.runtimeEventPayload(
                statusLongs, statusInts, statusBooleans, doubles(), strings()));
    assertEquals(9, statusPayload.regionId());
    assertSame(OfflineRegionDownloadState.ACTIVE, statusPayload.status().downloadState());
    assertTrue(statusPayload.status().complete());
    var errorLongs = longs();
    var errorInts = ints(RuntimeStructs.PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR);
    errorLongs[RuntimeStructs.LONG_REGION_ID] = 10;
    errorInts[RuntimeStructs.INT_RESOURCE_ERROR_REASON] =
        ResourceErrorReason.NOT_FOUND.nativeValue();
    var errorPayload =
        assertInstanceOf(
            RuntimeEventPayload.OfflineRegionResponseError.class,
            RuntimeStructs.runtimeEventPayload(
                errorLongs, errorInts, booleans(), doubles(), strings()));
    assertSame(ResourceErrorReason.NOT_FOUND, errorPayload.reason());
    var limitLongs = longs();
    limitLongs[RuntimeStructs.LONG_REGION_ID] = 11;
    limitLongs[RuntimeStructs.LONG_LIMIT] = 12;
    var limitPayload =
        assertInstanceOf(
            RuntimeEventPayload.OfflineRegionTileCountLimit.class,
            RuntimeStructs.runtimeEventPayload(
                limitLongs,
                ints(RuntimeStructs.PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT),
                booleans(),
                doubles(),
                strings()));
    assertEquals(12, limitPayload.limit());
    var completedLongs = longs();
    var completedInts = ints(RuntimeStructs.PAYLOAD_OFFLINE_OPERATION_COMPLETED);
    var completedBooleans = booleans();
    completedLongs[RuntimeStructs.LONG_OPERATION_ID] = 13;
    completedInts[RuntimeStructs.INT_OFFLINE_OPERATION_KIND] =
        OfflineOperationKind.REGION_GET.nativeValue();
    completedInts[RuntimeStructs.INT_OFFLINE_RESULT_KIND] =
        OfflineOperationResultKind.OPTIONAL_REGION.nativeValue();
    completedInts[RuntimeStructs.INT_OFFLINE_RESULT_STATUS] = 0;
    completedBooleans[RuntimeStructs.BOOLEAN_FOUND] = true;
    var completedPayload =
        assertInstanceOf(
            RuntimeEventPayload.OfflineOperationCompleted.class,
            RuntimeStructs.runtimeEventPayload(
                completedLongs, completedInts, completedBooleans, doubles(), strings()));
    assertEquals(13, completedPayload.operationId());
    assertSame(OfflineOperationKind.REGION_GET, completedPayload.operationKind());
    assertSame(OfflineOperationResultKind.OPTIONAL_REGION, completedPayload.resultKind());
    assertTrue(completedPayload.found());
  }

  @Test
  void copyEventTreatsUndersizedPayloadAsUnknown() {
    try (var event = new MaplibreNativeC.mln_runtime_event();
        var payload = new BytePointer(1)) {
      event.payload_type(RuntimeStructs.PAYLOAD_RENDER_MAP);
      event.payload(payload);
      event.payload_size(0);
      var longs = longs();
      var ints = new int[RuntimeStructs.INT_COUNT];
      RuntimeStructs.copyEvent(event, longs, ints, booleans(), doubles(), strings());
      assertEquals(0, ints[RuntimeStructs.INT_PAYLOAD_AVAILABLE]);
      var unknown =
          assertInstanceOf(
              RuntimeEventPayload.Unknown.class,
              RuntimeStructs.runtimeEventPayload(longs, ints, booleans(), doubles(), strings()));
      assertEquals(RuntimeStructs.PAYLOAD_RENDER_MAP, unknown.rawPayloadType());
      assertEquals(0, unknown.payloadSize());
    }
  }

  @Test
  void copyEventUsesExplicitStringSizes() {
    var imageId = new byte[] {'i', 0, 'd'};
    var message = new byte[] {'m', 0, 'g'};
    try (var event = new MaplibreNativeC.mln_runtime_event();
        var payload = new MaplibreNativeC.mln_runtime_event_style_image_missing();
        var imageBytes = new BytePointer(imageId.length);
        var messageBytes = new BytePointer(message.length)) {
      imageBytes.put(imageId, 0, imageId.length).position(0);
      messageBytes.put(message, 0, message.length).position(0);
      payload.image_id(imageBytes);
      payload.image_id_size(imageId.length);
      event.payload_type(RuntimeStructs.PAYLOAD_STYLE_IMAGE_MISSING);
      event.payload(payload);
      event.payload_size(payload.sizeof());
      event.message(messageBytes);
      event.message_size(message.length);
      var strings = strings();
      RuntimeStructs.copyEvent(
          event, longs(), new int[RuntimeStructs.INT_COUNT], booleans(), doubles(), strings);
      assertEquals("i\0d", strings[RuntimeStructs.STRING_PAYLOAD]);
      assertEquals("m\0g", strings[RuntimeStructs.STRING_MESSAGE]);
    }
  }

  @Test
  void eventCopiesHeaderSourcesMessageAndUnknownPayload() {
    var longs = longs();
    var ints = ints(99);
    var strings = strings();
    ints[RuntimeStructs.INT_EVENT_TYPE] = 7;
    ints[RuntimeStructs.INT_SOURCE_TYPE] = 0;
    ints[RuntimeStructs.INT_CODE] = 7;
    longs[RuntimeStructs.LONG_PAYLOAD_SIZE] = 123;
    strings[RuntimeStructs.STRING_MESSAGE] = "boom";
    var event =
        RuntimeStructs.runtimeEvent(
            longs, ints, booleans(), doubles(), strings, Optional.empty(), Optional.empty());
    assertSame(RuntimeEventType.MAP_LOADING_FAILED, event.type());
    assertSame(RuntimeEventSourceType.RUNTIME, event.sourceType());
    assertEquals(7, event.code());
    assertEquals("boom", event.message());
    var unknown = assertInstanceOf(RuntimeEventPayload.Unknown.class, event.payload());
    assertEquals(99, unknown.rawPayloadType());
    assertEquals(123, unknown.payloadSize());
  }

  private static long[] longs() {
    return new long[RuntimeStructs.LONG_COUNT];
  }

  private static int[] ints(int payloadType) {
    var ints = new int[RuntimeStructs.INT_COUNT];
    ints[RuntimeStructs.INT_PAYLOAD_TYPE] = payloadType;
    ints[RuntimeStructs.INT_PAYLOAD_AVAILABLE] = 1;
    return ints;
  }

  private static boolean[] booleans() {
    return new boolean[RuntimeStructs.BOOLEAN_COUNT];
  }

  private static double[] doubles() {
    return new double[RuntimeStructs.DOUBLE_COUNT];
  }

  private static String[] strings() {
    return new String[RuntimeStructs.STRING_COUNT];
  }
}
