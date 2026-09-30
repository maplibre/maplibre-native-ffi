package org.maplibre.nativeffi.runtime

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import kotlin.test.Test
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.generated.GeneratedValues
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_batch_view
import org.maplibre.nativeffi.internal.c.mln_runtime_event_payload

/** The FFM decoder for a drained event batch. */
class RuntimeEventJvmTest {
  @Test
  fun aStridedBatchWithAnUnknownPayloadArmDecodesWithoutLoss() {
    val payloadSize = mln_runtime_event_payload.sizeof()
    val events =
      Arena.ofConfined().use { arena ->
        val stride = mln_runtime_event.sizeof() + STRIDE_PADDING
        val events = arena.allocate(stride * STRIDED_EVENT_COUNT, 8).fill(PADDING_BYTE)
        repeat(STRIDED_EVENT_COUNT) { index ->
          val event = events.asSlice(index * stride, mln_runtime_event.sizeof())
          mln_runtime_event.type(event, stridedEventType(index))
          mln_runtime_event.source_type(event, stridedSourceType(index))
          mln_runtime_event.source(event, stridedSource(index).toLong())
          mln_runtime_event.code(event, stridedCode(index))
          mln_runtime_event.payload_type(event, stridedPayloadType(index))
          mln_runtime_event.message_offset(event, stridedMessageOffset(index))
          mln_runtime_event.message_size(event, stridedMessageSize(index))
          val payload = mln_runtime_event.payload(event).fill(0)
          payload.set(ValueLayout.JAVA_BYTE, 0, stridedPayloadFirstByte(index))
          payload.set(ValueLayout.JAVA_BYTE, payloadSize - 1, PAYLOAD_LAST_BYTE)
        }
        val messages =
          arena
            .allocate(STRIDED_MESSAGES.size.toLong())
            .copyFrom(MemorySegment.ofArray(STRIDED_MESSAGES))
        val view = mln_runtime_event_batch_view.allocate(arena)
        mln_runtime_event_batch_view.size(view, mln_runtime_event_batch_view.sizeof().toInt())
        mln_runtime_event_batch_view.event_size(view, stride.toInt())
        mln_runtime_event_batch_view.events(view, events)
        mln_runtime_event_batch_view.event_count(view, STRIDED_EVENT_COUNT.toLong())
        mln_runtime_event_batch_view.messages(view, messages)
        mln_runtime_event_batch_view.messages_size(view, STRIDED_MESSAGES.size.toLong())
        // The decoded batch owns copies, so overwriting the source afterwards changes nothing.
        GeneratedValues.readRuntimeEventBatchView(view).events.also {
          events.fill(0)
          messages.fill(0)
        }
      }
    assertStridedBatchDecoded(events, payloadSize.toInt())
  }

  @Test
  fun aNativeCountPastTheListRangeIsRefusedBeforeDecoding() {
    Arena.ofConfined().use { arena ->
      val view = mln_runtime_event_batch_view.allocate(arena)
      mln_runtime_event_batch_view.size(view, mln_runtime_event_batch_view.sizeof().toInt())
      mln_runtime_event_batch_view.event_size(view, mln_runtime_event.sizeof().toInt())
      mln_runtime_event_batch_view.events(view, arena.allocate(mln_runtime_event.sizeof()))
      mln_runtime_event_batch_view.event_count(view, UNLISTABLE_EVENT_COUNT)
      assertFailsWith<IllegalArgumentException> { GeneratedValues.readRuntimeEventBatchView(view) }
    }
  }
}
