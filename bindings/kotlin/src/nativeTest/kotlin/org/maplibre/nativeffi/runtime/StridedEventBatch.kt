package org.maplibre.nativeffi.runtime

import kotlin.test.assertEquals
import kotlin.test.assertIs
import org.maplibre.nativeffi.generated.RuntimeEvent
import org.maplibre.nativeffi.generated.RuntimeEventPayload

/**
 * The number of events in the batch that [RuntimeEventNativeTest] builds by hand. The events lie a
 * [STRIDE_PADDING]-byte gap apart, as a later header with a wider payload would lay them out, and
 * each has discriminants and a payload tag that this binding does not know.
 */
internal const val STRIDED_EVENT_COUNT = 2

/** The bytes between one event's end and the next event's start. */
internal const val STRIDE_PADDING = 24L

/** What fills the padding, which no decoded field may pick up. */
internal const val PADDING_BYTE: Byte = 0x7f

/** The last byte of each event's payload window, which the decoded payload must keep. */
internal const val PAYLOAD_LAST_BYTE: Byte = 0x55

/** The message arena: each message followed by its terminator. */
internal val STRIDED_MESSAGES: ByteArray = "first\u0000second\u0000".encodeToByteArray()

internal fun stridedEventType(index: Int): Int = 900 + index

internal fun stridedSourceType(index: Int): Int = 910 + index

/** A source whose top byte is set, as a handle's kind byte is, so a 64-bit carrier must keep it. */
internal fun stridedSource(index: Int): ULong = 0xff00_0000_0000_005auL + index.toULong()

/** A generation past 32 bits, so a narrowed carrier would lose it. */
internal fun stridedGeneration(index: Int): ULong = 0x1_0000_0000uL + index.toULong()

internal fun stridedCode(index: Int): Int = 920 + index

internal fun stridedPayloadType(index: Int): Int = 930 + index

internal fun stridedMessageOffset(index: Int): Long = if (index == 0) 0L else 6L

internal fun stridedMessageSize(index: Int): Int = if (index == 0) 5 else 6

/** The first byte of each event's payload window. */
internal fun stridedPayloadFirstByte(index: Int): Byte = (index + 1).toByte()

/** Checks that every field of the hand-built batch decoded, whatever the stride and tags. */
internal fun assertStridedBatchDecoded(events: List<RuntimeEvent>, payloadSize: Int) {
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
internal const val UNLISTABLE_EVENT_COUNT: Long = Int.MAX_VALUE.toLong() + 1
