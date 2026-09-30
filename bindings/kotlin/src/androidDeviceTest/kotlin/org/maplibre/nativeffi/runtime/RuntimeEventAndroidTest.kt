package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertFailsWith
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.bytedeco.javacpp.PointerScope
import org.maplibre.nativeffi.generated.GeneratedValues
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC

/** The JavaCPP decoder for a drained event batch. */
class RuntimeEventAndroidTest {
  @Test
  fun aStridedBatchWithAnUnknownPayloadArmDecodesWithoutLoss() {
    val payloadSize = MaplibreNativeC.mln_runtime_event_payload().use { it.sizeof() }
    val events =
      PointerScope().use {
        val eventSize = MaplibreNativeC.mln_runtime_event().use { it.sizeof().toLong() }
        val stride = eventSize + STRIDE_PADDING
        val bytes = BytePointer(stride * STRIDED_EVENT_COUNT)
        Pointer.memset(bytes, PADDING_BYTE.toInt(), stride * STRIDED_EVENT_COUNT)
        repeat(STRIDED_EVENT_COUNT) { index ->
          val event =
            MaplibreNativeC.mln_runtime_event(
                JavaCppSupport.addressPointer(bytes.address() + index * stride)
              )
              .type(stridedEventType(index))
              .source_type(stridedSourceType(index))
              .source(stridedSource(index).toLong())
              .code(stridedCode(index))
              .payload_type(stridedPayloadType(index))
              .message_offset(stridedMessageOffset(index))
              .message_size(stridedMessageSize(index))
          val payload = BytePointer(event.payload())
          Pointer.memset(payload, 0, payloadSize.toLong())
          payload.put(0L, stridedPayloadFirstByte(index))
          payload.put(payloadSize - 1L, PAYLOAD_LAST_BYTE)
        }
        val messages = BytePointer(STRIDED_MESSAGES.size.toLong()).put(*STRIDED_MESSAGES)
        val view =
          MaplibreNativeC.mln_runtime_event_batch_view()
            .size(MaplibreNativeC.mln_runtime_event_batch_view().use { it.sizeof() })
            .event_size(stride.toInt())
            .events(MaplibreNativeC.mln_runtime_event(bytes))
            .event_count(STRIDED_EVENT_COUNT.toLong())
            .messages(messages)
            .messages_size(STRIDED_MESSAGES.size.toLong())
        // The decoded batch owns copies, so overwriting the source afterwards changes nothing.
        GeneratedValues.readRuntimeEventBatchView(view).events.also {
          Pointer.memset(bytes, 0, stride * STRIDED_EVENT_COUNT)
          Pointer.memset(messages, 0, STRIDED_MESSAGES.size.toLong())
        }
      }
    assertStridedBatchDecoded(events, payloadSize)
  }

  @Test
  fun aNativeCountPastTheListRangeIsRefusedBeforeDecoding() {
    PointerScope().use {
      val event = MaplibreNativeC.mln_runtime_event()
      val view =
        MaplibreNativeC.mln_runtime_event_batch_view()
          .size(MaplibreNativeC.mln_runtime_event_batch_view().use { it.sizeof() })
          .event_size(event.sizeof())
          .events(event)
          .event_count(UNLISTABLE_EVENT_COUNT)
      assertFailsWith<IllegalArgumentException> { GeneratedValues.readRuntimeEventBatchView(view) }
    }
  }
}
