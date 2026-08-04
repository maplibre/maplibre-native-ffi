package org.maplibre.nativeffi.internal.lifecycle

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.withRuntime

/**
 * What native does with a handle id the safe API can no longer produce.
 *
 * A handle is a generational integer, so the safe wrappers cannot express either case here: a
 * released wrapper refuses before it reaches the module, and the kinds are distinct value classes.
 * Both are replayed through the dispatch path directly, because what is being checked is that
 * native's own generation and kind tags still catch them — the last line of defence under the
 * binding's own guards.
 */
class HandleIdentityBrowserTest {
  // Spec coverage: BND-045, BND-047.

  @Test
  fun aReleasedMapIdIsStaleEvenOnceANewMapHasTakenItsSlot(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withRuntime { runtime ->
        val first = MapHandle.create(runtime, mapOptions())
        val released = first.nativeHandle().raw
        first.close()

        // The released slot is the one the next map takes, so the replayed id names a retired
        // generation of a slot that is live again. A native side that compared only the slot would
        // answer for the new map.
        val second = MapHandle.create(runtime, mapOptions())
        try {
          val error = assertFailsWith<InvalidArgumentException> { mapSize(released) }

          assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
          assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, error.nativeStatusCode)
          // Native's own words for it: the id names a retired generation rather than an id it
          // never issued. The completion carries the message back from the owner thread, which is
          // the only way it reaches the page at all.
          assertTrue(error.diagnostic.contains("mln_map"), error.diagnostic)
          assertTrue(error.diagnostic.contains("stale"), error.diagnostic)

          // The live map is unaffected by the replay.
          mapSize(second.nativeHandle().raw)
        } finally {
          second.close()
        }
      }
    }
  }

  @Test
  fun aMapIdHandedToARuntimeOperationIsRejectedOnItsKind(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withRuntime { runtime ->
        val map = MapHandle.create(runtime, mapOptions())
        try {
          // `NativeMap` and `NativeRuntime` are distinct value classes, so this call has no
          // expression in the safe API at all and needs the raw id.
          val error =
            assertFailsWith<InvalidArgumentException> {
              Dispatcher.call(
                "mln_runtime_pump",
                2,
                { slots ->
                  slots.setLong(0, map.nativeHandle().raw)
                  slots.setLong(1, 0L)
                },
                { Status.check(Heap.loadInt(it)) },
              )
            }

          assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
          assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, error.nativeStatusCode)
          // Native names both kinds, so the message says which handle was passed as well as which
          // one was wanted.
          assertTrue(error.diagnostic.contains("mln_map"), error.diagnostic)
          assertTrue(error.diagnostic.contains("mln_runtime"), error.diagnostic)

          // The runtime still works, so the rejection was of the argument and not of the call.
          runtime.pump(0)
        } finally {
          map.close()
        }
      }
    }
  }

  private fun mapOptions() =
    MapOptions().apply {
      width = 64
      height = 64
    }

  /** Reads a map's extent through the dispatch path, so the handle id can be chosen. */
  private fun mapSize(rawHandle: Long) {
    Heap.withScratch(SIZE_SCRATCH_BYTES) { scratch ->
      Dispatcher.call(
        "mln_map_get_size",
        4,
        { slots ->
          slots.setLong(0, rawHandle)
          slots.setPointer(1, scratch)
          slots.setPointer(2, scratch + 4)
          slots.setPointer(3, scratch + 8)
        },
        { Status.check(Heap.loadInt(it)) },
      )
    }
  }

  private companion object {
    /** Two `uint32_t` and a `double`, with the double at the eight-byte offset it needs. */
    const val SIZE_SCRATCH_BYTES = 16
  }
}
