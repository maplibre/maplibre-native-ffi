package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertIs
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.PointerScope
import org.maplibre.nativeffi.generated.GeneratedValues
import org.maplibre.nativeffi.generated.RuntimeEventPayload
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC

class RuntimeEventAndroidTest {
  @Test
  fun unknownEventCopiesItsDiscriminantsAndOnlyThePayloadWindow() {
    val event =
      PointerScope().use {
        val native =
          MaplibreNativeC.mln_runtime_event()
            .type(900)
            .source_type(901)
            .source(0x5aL)
            .code(902)
            .payload_type(903)
        val payload = BytePointer(native.payload())
        payload.put(0L, 1.toByte()).put(1L, 2.toByte()).put(2L, 3.toByte())
        GeneratedValues.readRuntimeEvent(native, "future event").also {
          payload.put(0L, 9.toByte())
        }
      }
    assertEquals(900u, event.type.rawValue)
    assertEquals(901u, event.sourceType.rawValue)
    assertEquals(0x5auL, event.source)
    assertEquals(902, event.code)
    assertEquals("future event", event.message)
    val payload = assertIs<RuntimeEventPayload.Unknown>(event.payload)
    assertEquals(903u, payload.tag)
    MaplibreNativeC.mln_runtime_event_payload().use {
      assertEquals(it.sizeof(), payload.bytes.size)
    }
    assertContentEquals(byteArrayOf(1, 2, 3), payload.bytes.take(3).toByteArray())
  }
}
