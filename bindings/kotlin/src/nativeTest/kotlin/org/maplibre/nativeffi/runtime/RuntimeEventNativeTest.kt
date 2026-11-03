package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlinx.cinterop.*
import org.maplibre.nativeffi.RuntimeEvent
import org.maplibre.nativeffi.RuntimeEventPayload
import org.maplibre.nativeffi.internal.cinterop.mln_event_batch_view
import org.maplibre.nativeffi.internal.cinterop.mln_runtime_event
import org.maplibre.nativeffi.internal.cinterop.mln_runtime_event_payload
import org.maplibre.nativeffi.readEventBatchView
import platform.posix.memset

/**
 * The common decoder for a drained event batch, over a batch that cinterop lays out the way the C
 * compiler does.
 */
@OptIn(ExperimentalForeignApi::class)
class RuntimeEventNativeTest {
  @Test
  fun aStridedBatchWithAnUnknownPayloadArmDecodesWithoutLoss() {
    val payloadSize = sizeOf<mln_runtime_event_payload>()
    val events = memScoped {
      val stride = sizeOf<mln_runtime_event>() + STRIDE_PADDING
      val bytes = allocArray<ByteVar>(stride * STRIDED_EVENT_COUNT)
      memset(bytes, PADDING_BYTE.toInt(), (stride * STRIDED_EVENT_COUNT).convert())
      val messages = allocArray<ByteVar>(STRIDED_MESSAGES.size)
      STRIDED_MESSAGES.forEachIndexed { index, byte -> messages[index] = byte }
      repeat(STRIDED_EVENT_COUNT) { index ->
        val event = (bytes + index * stride)!!.reinterpret<mln_runtime_event>().pointed
        event.type = stridedEventType(index).toUInt()
        event.source_type = stridedSourceType(index).toUInt()
        event.source = stridedSource(index)
        event.generation = stridedGeneration(index)
        event.code = stridedCode(index)
        event.payload_type = stridedPayloadType(index).toUInt()
        event.message.data = messages + stridedMessageOffset(index)
        event.message.size = stridedMessageSize(index).convert()
        val payload = event.payload.ptr.reinterpret<ByteVar>()
        memset(payload, 0, payloadSize.convert())
        payload[0] = stridedPayloadFirstByte(index)
        payload[payloadSize - 1] = PAYLOAD_LAST_BYTE
      }
      val view = alloc<mln_event_batch_view>()
      view.size = sizeOf<mln_event_batch_view>().toUInt()
      view.event_size = stride.toUInt()
      view.events = bytes.reinterpret()
      view.event_count = STRIDED_EVENT_COUNT.convert()
      // The decoded batch owns copies, so overwriting the source afterwards changes nothing.
      readEventBatchView(view.ptr.toLong()).also {
        memset(bytes, 0, (stride * STRIDED_EVENT_COUNT).convert())
        memset(messages, 0, STRIDED_MESSAGES.size.convert())
      }
    }
    assertStridedBatchDecoded(events, payloadSize.toInt())
  }

  @Test
  fun aNativeCountPastTheListRangeIsRefusedBeforeDecoding() {
    memScoped {
      val view = alloc<mln_event_batch_view>()
      view.size = sizeOf<mln_event_batch_view>().toUInt()
      view.event_size = sizeOf<mln_runtime_event>().toUInt()
      view.events = alloc<mln_runtime_event>().ptr
      view.event_count = UNLISTABLE_EVENT_COUNT.convert()
      assertFailsWith<IllegalArgumentException> { readEventBatchView(view.ptr.toLong()) }
    }
  }
}

/**
 * The number of events in the batch that [RuntimeEventNativeTest] builds by hand. The events lie a
 * [STRIDE_PADDING]-byte gap apart, as a later header with a wider payload would lay them out, and
 * each has discriminants and a payload tag that this binding does not know.
 */
private const val STRIDED_EVENT_COUNT = 2

/** The bytes between one event's end and the next event's start. */
private const val STRIDE_PADDING = 24L

/** What fills the padding, which no decoded field may pick up. */
private const val PADDING_BYTE: Byte = 0x7f

/** The last byte of each event's payload window, which the decoded payload must keep. */
private const val PAYLOAD_LAST_BYTE: Byte = 0x55

/** The message arena: each message followed by its terminator. */
private val STRIDED_MESSAGES: ByteArray = "first\u0000second\u0000".encodeToByteArray()

private fun stridedEventType(index: Int): Int = 900 + index

private fun stridedSourceType(index: Int): Int = 910 + index

/** A source whose top byte is set, as a handle's kind byte is, so a 64-bit carrier must keep it. */
private fun stridedSource(index: Int): ULong = 0xff00_0000_0000_005auL + index.toULong()

/** A generation past 32 bits, so a narrowed carrier would lose it. */
private fun stridedGeneration(index: Int): ULong = 0x1_0000_0000uL + index.toULong()

private fun stridedCode(index: Int): UInt = 920u + index.toUInt()

private fun stridedPayloadType(index: Int): Int = 930 + index

private fun stridedMessageOffset(index: Int): Long = if (index == 0) 0L else 6L

private fun stridedMessageSize(index: Int): Int = if (index == 0) 5 else 6

/** The first byte of each event's payload window. */
private fun stridedPayloadFirstByte(index: Int): Byte = (index + 1).toByte()

/** Checks that every field of the hand-built batch decoded, whatever the stride and tags. */
private fun assertStridedBatchDecoded(events: List<RuntimeEvent>, payloadSize: Int) {
  assertEquals(STRIDED_EVENT_COUNT, events.size)
  events.forEachIndexed { index, event ->
    assertEquals(stridedEventType(index).toUInt(), event.type.rawValue)
    assertEquals(stridedSourceType(index).toUInt(), event.sourceType.rawValue)
    assertEquals(stridedSource(index), event.source)
    assertEquals(stridedGeneration(index), event.generation)
    assertEquals(stridedCode(index), event.code)
    assertEquals(listOf("first", "second")[index], event.message)
    val payload = assertIs<RuntimeEventPayload.Unknown>(event.payload)
    assertEquals(stridedPayloadType(index).toUInt(), payload.tag)
    // The whole payload window is copied, and nothing past it.
    assertEquals(payloadSize, payload.bytes.size)
    assertEquals(stridedPayloadFirstByte(index), payload.bytes.first())
    assertEquals(PAYLOAD_LAST_BYTE, payload.bytes.last())
  }
}

/** An event count past what a Kotlin list can index, which decoding refuses before reading. */
private const val UNLISTABLE_EVENT_COUNT: Long = Int.MAX_VALUE.toLong() + 1
