package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertNull
import org.maplibre.nativeffi.internal.loader.NativeAccess.JvmStructs
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

class RuntimeEventJvmTest {
  @Test
  fun unknownDomainsAndEventsFromReleasedMapsCopyWithoutResurrectingHandles() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
        },
      )
    val mapId = map.nativeHandleId()
    map.close()
    try {
      val orphan =
        runtime.copyEventForTesting(
          RuntimeEventType.MAP_STYLE_LOADED.nativeValue,
          RuntimeEventSourceType.MAP.nativeValue,
          mapId,
          0,
          RuntimeEventPayload.None,
          "",
        )
      assertNull(orphan.mapSource)
      assertNull(orphan.runtimeSource)

      val unknown =
        runtime.copyEventForTesting(
          900,
          901,
          0,
          902,
          JvmStructs.unknownRuntimePayload(903, byteArrayOf(1, 2, 3)),
          "future event",
        )
      assertEquals(900, unknown.type.nativeValue)
      assertEquals(901, unknown.sourceType.nativeValue)
      assertNull(unknown.mapSource)
      assertNull(unknown.runtimeSource)
      assertEquals("future event", unknown.message)
      val payload = unknown.payload as RuntimeEventPayload.Unknown
      assertEquals(903, payload.rawPayloadType)
      assertContentEquals(byteArrayOf(1, 2, 3), payload.payloadBytes)
    } finally {
      runtime.close()
    }
  }
}
