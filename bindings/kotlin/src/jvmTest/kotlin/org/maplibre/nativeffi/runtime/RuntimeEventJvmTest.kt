package org.maplibre.nativeffi.runtime

import java.lang.foreign.Arena
import java.lang.foreign.ValueLayout
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertIs
import org.maplibre.nativeffi.generated.GeneratedValues
import org.maplibre.nativeffi.generated.RuntimeEventPayload
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_payload

class RuntimeEventJvmTest {
  @Test
  fun unknownEventCopiesItsDiscriminantsAndOnlyThePayloadWindow() {
    val event =
      Arena.ofConfined().use { arena ->
        val native = mln_runtime_event.allocate(arena)
        mln_runtime_event.type(native, 900)
        mln_runtime_event.source_type(native, 901)
        mln_runtime_event.source(native, 0x5aL)
        mln_runtime_event.code(native, 902)
        mln_runtime_event.payload_type(native, 903)
        val payload = mln_runtime_event.payload(native)
        payload.set(ValueLayout.JAVA_BYTE, 0, 1)
        payload.set(ValueLayout.JAVA_BYTE, 1, 2)
        payload.set(ValueLayout.JAVA_BYTE, 2, 3)
        GeneratedValues.readRuntimeEvent(native, "future event").also {
          payload.set(ValueLayout.JAVA_BYTE, 0, 9)
        }
      }
    assertEquals(900u, event.type.rawValue)
    assertEquals(901u, event.sourceType.rawValue)
    assertEquals(0x5auL, event.source)
    assertEquals(902, event.code)
    assertEquals("future event", event.message)
    val payload = assertIs<RuntimeEventPayload.Unknown>(event.payload)
    assertEquals(903u, payload.tag)
    assertEquals(mln_runtime_event_payload.sizeof().toInt(), payload.bytes.size)
    assertContentEquals(byteArrayOf(1, 2, 3), payload.bytes.take(3).toByteArray())
  }
}
