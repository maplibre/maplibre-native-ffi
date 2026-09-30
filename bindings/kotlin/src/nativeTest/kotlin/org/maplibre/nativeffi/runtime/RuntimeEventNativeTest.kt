package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.GeneratedValues
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_batch_view
import org.maplibre.nativeffi.internal.c.mln_runtime_event_payload
import platform.posix.memset

/** The cinterop decoder for a drained event batch. */
@OptIn(ExperimentalForeignApi::class)
class RuntimeEventNativeTest {
  @Test
  fun aStridedBatchWithAnUnknownPayloadArmDecodesWithoutLoss() {
    val payloadSize = sizeOf<mln_runtime_event_payload>()
    val events = memScoped {
      val stride = sizeOf<mln_runtime_event>() + STRIDE_PADDING
      val bytes = allocArray<ByteVar>(stride * STRIDED_EVENT_COUNT)
      memset(bytes, PADDING_BYTE.toInt(), (stride * STRIDED_EVENT_COUNT).convert())
      repeat(STRIDED_EVENT_COUNT) { index ->
        val event = (bytes + index * stride)!!.reinterpret<mln_runtime_event>().pointed
        event.type = stridedEventType(index).toUInt()
        event.source_type = stridedSourceType(index).toUInt()
        event.source = stridedSource(index)
        event.code = stridedCode(index)
        event.payload_type = stridedPayloadType(index).toUInt()
        event.message_offset = stridedMessageOffset(index).toULong()
        event.message_size = stridedMessageSize(index).toUInt()
        val payload = event.payload.ptr.reinterpret<ByteVar>()
        memset(payload, 0, payloadSize.convert())
        payload[0] = stridedPayloadFirstByte(index)
        payload[payloadSize - 1] = PAYLOAD_LAST_BYTE
      }
      val messages = allocArray<ByteVar>(STRIDED_MESSAGES.size)
      STRIDED_MESSAGES.forEachIndexed { index, byte -> messages[index] = byte }
      val view = alloc<mln_runtime_event_batch_view>()
      view.size = sizeOf<mln_runtime_event_batch_view>().toUInt()
      view.event_size = stride.toUInt()
      view.events = bytes.reinterpret()
      view.event_count = STRIDED_EVENT_COUNT.convert()
      view.messages = messages
      view.messages_size = STRIDED_MESSAGES.size.convert()
      // The decoded batch owns copies, so overwriting the source afterwards changes nothing.
      GeneratedValues.readRuntimeEventBatchView(view).events.also {
        memset(bytes, 0, (stride * STRIDED_EVENT_COUNT).convert())
        memset(messages, 0, STRIDED_MESSAGES.size.convert())
      }
    }
    assertStridedBatchDecoded(events, payloadSize.toInt())
  }

  @Test
  fun aNativeCountPastTheListRangeIsRefusedBeforeDecoding() {
    memScoped {
      val view = alloc<mln_runtime_event_batch_view>()
      view.size = sizeOf<mln_runtime_event_batch_view>().toUInt()
      view.event_size = sizeOf<mln_runtime_event>().toUInt()
      view.events = alloc<mln_runtime_event>().ptr
      view.event_count = UNLISTABLE_EVENT_COUNT.convert()
      assertFailsWith<IllegalArgumentException> { GeneratedValues.readRuntimeEventBatchView(view) }
    }
  }
}
