package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertIs
import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.GeneratedValues
import org.maplibre.nativeffi.generated.RuntimeEventPayload
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_payload

@OptIn(ExperimentalForeignApi::class)
class RuntimeEventNativeTest {
  @Test
  fun unknownEventCopiesItsDiscriminantsAndOnlyThePayloadWindow() {
    val event = memScoped {
      val native = alloc<mln_runtime_event>()
      native.type = 900u
      native.source_type = 901u
      native.source = 0x5auL
      native.code = 902
      native.payload_type = 903u
      val payload = native.payload.ptr.reinterpret<ByteVar>()
      payload[0] = 1
      payload[1] = 2
      payload[2] = 3
      GeneratedValues.readRuntimeEvent(native, "future event").also { payload[0] = 9 }
    }
    assertEquals(900u, event.type.rawValue)
    assertEquals(901u, event.sourceType.rawValue)
    assertEquals(0x5auL, event.source)
    assertEquals(902, event.code)
    assertEquals("future event", event.message)
    val payload = assertIs<RuntimeEventPayload.Unknown>(event.payload)
    assertEquals(903u, payload.tag)
    assertEquals(sizeOf<mln_runtime_event_payload>().toInt(), payload.bytes.size)
    assertContentEquals(byteArrayOf(1, 2, 3), payload.bytes.take(3).toByteArray())
  }
}
