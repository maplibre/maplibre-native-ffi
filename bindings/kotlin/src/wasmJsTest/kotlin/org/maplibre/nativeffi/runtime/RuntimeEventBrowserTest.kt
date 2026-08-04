package org.maplibre.nativeffi.runtime

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertIs
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.RuntimeEventMarshal
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEvent
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventCameraTransitionFinished
import org.maplibre.nativeffi.internal.wasm.generated.MlnRuntimeEventPayloadType
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.pumpUntil
import org.maplibre.nativeffi.waitForMapEvent
import org.maplibre.nativeffi.withMap
import org.maplibre.nativeffi.withRuntime

/**
 * The events a runtime hands back, and how much of them survives the next poll.
 *
 * A poll writes into one runtime-owned block that the next poll overwrites, so everything a public
 * event carries has to be copied out before the frame that read it returns. The two events built by
 * hand are the cases no module can be made to produce: a domain from a later revision of the C API,
 * and a map-originated event whose map the host has already closed.
 */
class RuntimeEventBrowserTest {
  // Spec coverage: BND-081, BND-082, BND-083, BND-086, BND-087.

  @Test
  fun aStyleLoadReportsItsOwnMapAndKeepsItsMessageAcrossTheNextPoll(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap { runtime, map ->
          map.setStyleJson(EMPTY_STYLE_JSON)

          val event = waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
          val copiedMessage = event.message

          assertEquals(RuntimeEventSourceType.MAP, event.sourceType)
          assertEquals(map, event.mapSource)
          assertNull(event.runtimeSource)
          assertEquals(RuntimeEventPayload.None, event.payload)

          // The next poll reuses the storage the message was read out of, so a view rather than a
          // copy would change here.
          runtime.pollEvent()
          assertEquals(copiedMessage, event.message)

          // And polling reaches an empty queue rather than repeating the last event forever.
          var polls = 0
          while (runtime.pollEvent() != null && polls < POLL_LIMIT) {
            polls++
          }
          assertTrue(polls < POLL_LIMIT, "the event queue never emptied")
          assertNull(runtime.pollEvent())
        }
      }
    }

  @Test
  fun twoMapsAreEachNamedByTheirOwnStyleLoadEvent(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withRuntime { runtime ->
        val first = MapHandle.create(runtime, mapOptions())
        val second = MapHandle.create(runtime, mapOptions())
        try {
          first.setStyleJson(EMPTY_STYLE_JSON)
          second.setStyleJson(EMPTY_STYLE_JSON)

          val loaded = mutableMapOf<MapHandle, Int>()
          pumpUntil(
            runtime,
            onEvent = {
              if (it.type == RuntimeEventType.MAP_STYLE_LOADED) {
                val source = it.mapSource
                if (source != null) loaded[source] = (loaded[source] ?: 0) + 1
              }
            },
          ) {
            loaded.containsKey(first) && loaded.containsKey(second)
          }

          // Both maps loaded, and each event named the map that raised it rather than whichever
          // map happened to be looked up first.
          assertEquals(setOf(first, second), loaded.keys)
        } finally {
          first.close()
          second.close()
        }
      }
    }
  }

  @Test
  fun anEventFromAFutureDomainKeepsItsRawValuesAndCopiedPayload(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withRuntime { runtime ->
        val message = "future event"
        val messageBytes = Heap.utf8Size(message)
        val copied =
          Heap.withScratch(MlnRuntimeEvent.SIZEOF + PAYLOAD_BYTES + messageBytes) { base ->
            val payload = base + MlnRuntimeEvent.SIZEOF
            val text = payload + PAYLOAD_BYTES
            Heap.storeByte(payload, 1)
            Heap.storeByte(payload + 1, 2)
            Heap.storeByte(payload + 2, 3)
            Heap.storeUtf8(text, message)

            MlnRuntimeEvent.setSize(base, MlnRuntimeEvent.SIZEOF)
            MlnRuntimeEvent.setType(base, FUTURE_TYPE)
            MlnRuntimeEvent.setSourceType(base, FUTURE_SOURCE_TYPE)
            MlnRuntimeEvent.setSource(base, 0L)
            MlnRuntimeEvent.setCode(base, FUTURE_CODE)
            MlnRuntimeEvent.setPayloadType(base, FUTURE_PAYLOAD_TYPE)
            MlnRuntimeEvent.setPayload(base, payload)
            MlnRuntimeEvent.setPayloadSize(base, PAYLOAD_BYTES)
            MlnRuntimeEvent.setMessage(base, text)
            MlnRuntimeEvent.setMessageSize(base, message.length)

            val event = RuntimeEventMarshal.readEvent(base, runtime)
            // Overwritten after the read, so a payload that was a view rather than a copy would
            // show it below.
            Heap.storeByte(payload, 9)
            event
          }

        assertEquals(RuntimeEventType(FUTURE_TYPE), copied.type)
        assertEquals(FUTURE_TYPE, copied.type.nativeValue)
        assertEquals(RuntimeEventSourceType(FUTURE_SOURCE_TYPE), copied.sourceType)
        assertEquals(FUTURE_SOURCE_TYPE, copied.sourceType.nativeValue)
        assertNull(copied.runtimeSource)
        assertNull(copied.mapSource)
        assertEquals(FUTURE_CODE, copied.code)
        assertEquals(message, copied.message)

        val payload = assertIs<RuntimeEventPayload.Unknown>(copied.payload)
        assertEquals(FUTURE_PAYLOAD_TYPE, payload.rawPayloadType)
        assertEquals(PAYLOAD_BYTES.toLong(), payload.payloadSize)
        assertContentEquals(byteArrayOf(1, 2, 3), payload.payloadBytes)
      }
    }
  }

  @Test
  fun aKnownPayloadShorterThanItsStructIsReadAsUnknown(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withRuntime { runtime ->
        val full =
          readSyntheticCameraTransition(runtime, MlnRuntimeEventCameraTransitionFinished.SIZEOF)
        // A camera transition really does carry its id, so the full-size case is the control.
        assertEquals(
          TRANSITION_ID,
          assertIs<RuntimeEventPayload.CameraTransitionFinished>(full).transitionId,
        )

        // A module built from other headers can report a shorter payload. The fields past the size
        // it declares belong to that module, so they are not read as this binding's struct.
        val truncated =
          readSyntheticCameraTransition(runtime, MlnRuntimeEventCameraTransitionFinished.SIZEOF - 1)
        val unknown = assertIs<RuntimeEventPayload.Unknown>(truncated)
        assertEquals(
          MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED,
          unknown.rawPayloadType,
        )
        assertEquals(
          (MlnRuntimeEventCameraTransitionFinished.SIZEOF - 1).toLong(),
          unknown.payloadSize,
        )
      }
    }
  }

  @Test
  fun aMapEventWhoseMapHasBeenClosedNamesNoMap(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withRuntime { runtime ->
        val map = MapHandle.create(runtime, mapOptions())
        val closedMapHandle = map.nativeHandle().raw
        map.close()

        val copied =
          Heap.withScratch(MlnRuntimeEvent.SIZEOF) { base ->
            MlnRuntimeEvent.setSize(base, MlnRuntimeEvent.SIZEOF)
            MlnRuntimeEvent.setType(base, RuntimeEventType.MAP_STYLE_LOADED.nativeValue)
            MlnRuntimeEvent.setSourceType(base, RuntimeEventSourceType.MAP.nativeValue)
            MlnRuntimeEvent.setSource(base, closedMapHandle)
            RuntimeEventMarshal.readEvent(base, runtime)
          }

        // The event still says a map raised it; there is simply no live public map to name.
        assertEquals(RuntimeEventType.MAP_STYLE_LOADED, copied.type)
        assertEquals(RuntimeEventSourceType.MAP, copied.sourceType)
        assertNull(copied.mapSource)
        assertNull(copied.runtimeSource)
        assertEquals(RuntimeEventPayload.None, copied.payload)

        // A live map is still resolved, so the lookup missed rather than being switched off.
        val live = MapHandle.create(runtime, mapOptions())
        try {
          val resolved =
            Heap.withScratch(MlnRuntimeEvent.SIZEOF) { base ->
              MlnRuntimeEvent.setSize(base, MlnRuntimeEvent.SIZEOF)
              MlnRuntimeEvent.setType(base, RuntimeEventType.MAP_STYLE_LOADED.nativeValue)
              MlnRuntimeEvent.setSourceType(base, RuntimeEventSourceType.MAP.nativeValue)
              MlnRuntimeEvent.setSource(base, live.nativeHandle().raw)
              RuntimeEventMarshal.readEvent(base, runtime)
            }
          assertEquals(live, assertNotNull(resolved.mapSource))
        } finally {
          live.close()
        }
      }
    }
  }

  private fun readSyntheticCameraTransition(
    runtime: RuntimeHandle,
    declaredPayloadSize: Int,
  ): RuntimeEventPayload =
    Heap.withScratch(MlnRuntimeEvent.SIZEOF + MlnRuntimeEventCameraTransitionFinished.SIZEOF) { base
      ->
      val payload: HeapPointer = base + MlnRuntimeEvent.SIZEOF
      MlnRuntimeEventCameraTransitionFinished.setSize(
        payload,
        MlnRuntimeEventCameraTransitionFinished.SIZEOF,
      )
      MlnRuntimeEventCameraTransitionFinished.setTransitionId(payload, TRANSITION_ID)

      MlnRuntimeEvent.setSize(base, MlnRuntimeEvent.SIZEOF)
      MlnRuntimeEvent.setType(base, RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED.nativeValue)
      MlnRuntimeEvent.setSourceType(base, RuntimeEventSourceType.MAP.nativeValue)
      MlnRuntimeEvent.setPayloadType(
        base,
        MlnRuntimeEventPayloadType.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED,
      )
      MlnRuntimeEvent.setPayload(base, payload)
      MlnRuntimeEvent.setPayloadSize(base, declaredPayloadSize)
      RuntimeEventMarshal.readEvent(base, runtime).payload
    }

  private fun mapOptions() =
    MapOptions().apply {
      width = 64
      height = 64
    }

  private companion object {
    const val POLL_LIMIT = 4_096
    const val PAYLOAD_BYTES = 3
    const val FUTURE_TYPE = 900
    const val FUTURE_SOURCE_TYPE = 901
    const val FUTURE_CODE = 902
    const val FUTURE_PAYLOAD_TYPE = 903
    const val TRANSITION_ID = 21L
  }
}
