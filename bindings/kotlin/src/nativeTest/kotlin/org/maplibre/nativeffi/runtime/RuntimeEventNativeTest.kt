package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.set
import kotlinx.cinterop.sizeOf
import org.maplibre.nativeffi.NativeTestBase
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

@OptIn(ExperimentalForeignApi::class)
class RuntimeEventNativeTest : NativeTestBase() {
  @Test
  fun unknownDomainsAndEventsFromReleasedMapsCopyWithoutResurrectingHandles(): Unit =
    runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val map =
          MapHandle.create(
              runtime,
              MapOptions().apply {
                width = 64
                height = 64
              },
            )
            .await()
        val mapId = map.nativeHandleId()
        map.close().await()

        memScoped {
          val orphan = alloc<mln_runtime_event>()
          orphan.type = RuntimeEventType.MAP_STYLE_LOADED.nativeValue.toUInt()
          orphan.source_type = RuntimeEventSourceType.MAP.nativeValue.toUInt()
          orphan.source = mapId.toULong()
          val copiedOrphan = runtime.copyEventForTesting(orphan, null)
          assertNull(copiedOrphan.mapSource)
          assertNull(copiedOrphan.runtimeSource)
          assertEquals(mapId, copiedOrphan.sourceId)

          val unknown = alloc<mln_runtime_event>()
          unknown.type = 900U
          unknown.source_type = 901U
          unknown.source = 0x5AUL
          unknown.code = 902
          unknown.payload_type = 903U
          val payload = unknown.payload.ptr.reinterpret<ByteVar>()
          payload[0] = 1
          payload[1] = 2
          payload[2] = 3
          val copiedUnknown =
            runtime.copyEventForTesting(unknown, null, sizeOf<mln_runtime_event>())

          assertEquals(900, copiedUnknown.type.nativeValue)
          assertEquals(901, copiedUnknown.sourceType.nativeValue)
          assertEquals(0x5AL, copiedUnknown.sourceId)
          assertNull(copiedUnknown.mapSource)
          assertNull(copiedUnknown.runtimeSource)
          assertEquals("", copiedUnknown.message)
          val decoded = copiedUnknown.payload as RuntimeEventPayload.Unknown
          assertEquals(903, decoded.rawPayloadType)
          assertContentEquals(byteArrayOf(1, 2, 3), decoded.payloadBytes.take(3).toByteArray())
        }
      }
    }
}
