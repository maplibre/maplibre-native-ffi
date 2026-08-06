package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.withRuntime

/**
 * A map exercised through the calls whose descriptors are optional.
 *
 * The C API reads a null descriptor as its own default — a null anchor is the screen centre, a null
 * animation is a zero-duration change — so passing null is ordinary use rather than an edge case.
 * On this target that is the one shape where a call places *nothing* in the module's heap, which is
 * why it has its own coverage: the binding measures before it allocates, and a measure of zero has
 * to stay a legal answer rather than becoming a refused allocation.
 */
class MapHandleBrowserTest {
  // Spec coverage: BND-024, BND-042, BND-100, BND-108.

  @Test
  fun aMapReportsTheExtentAndModeItWasCreatedWith() {
    withRuntime { runtime ->
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 512
            height = 256
            scaleFactor = 2.0
            mapMode = MapMode.STATIC
            fastPforEnabled = true
          },
        )
      try {
        val size = map.size
        assertEquals(512, size.width)
        assertEquals(256, size.height)
        assertEquals(2.0, size.scaleFactor)
        assertEquals(runtime, map.runtime())
        assertEquals(false, map.isClosed)
      } finally {
        map.close()
      }

      // Release runs through the runtime that parented it: closed once, closed idempotently, and
      // refusing later use before anything crosses into the module.
      assertEquals(true, map.isClosed)
      map.close()
      assertFailsWith<InvalidStateException> { map.setStyleJson(EMPTY_STYLE_JSON) }

      // The parent outlived its child and is still usable.
      runtime.pump(0)
    }
  }

  @Test
  fun theLoadedStyleDocumentAndTheRequestedUrlReadBackSeparately() {
    withRuntime { runtime ->
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
      try {
        // Nothing parsed and nothing requested yet.
        assertEquals("", map.loadedStyleJson())
        assertEquals("", map.styleUrl())

        // The document reads back byte for byte, so it can be handed straight back.
        map.setStyleJson(STYLE_WITH_UNICODE)
        assertEquals(STYLE_WITH_UNICODE, map.loadedStyleJson())
        // Inline JSON clears the URL.
        assertEquals("", map.styleUrl())

        // The URL is request state, recorded before the load can succeed, while the document
        // still reports the style that last parsed.
        map.setStyleUrl("https://example.com/style.json")
        assertEquals("https://example.com/style.json", map.styleUrl())
        assertEquals(STYLE_WITH_UNICODE, map.loadedStyleJson())
      } finally {
        map.close()
      }
    }
  }

  @Test
  fun aNullTerminatedInputCarryingAnEmbeddedNulIsRefused() {
    withRuntime { runtime ->
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
      try {
        // These two take a null-terminated C string rather than a string view, so a NUL in the
        // middle would truncate the value instead of being carried as a byte.
        val url =
          assertFailsWith<InvalidArgumentException> {
            map.setStyleUrl("https://example.com/a" + NUL + "b.json")
          }
        assertEquals(MaplibreStatus.INVALID_ARGUMENT, url.status)
        assertEquals("url cannot contain embedded NUL characters", url.diagnostic)

        val json = assertFailsWith<InvalidArgumentException> { map.setStyleJson("{" + NUL + "}") }
        assertEquals(MaplibreStatus.INVALID_ARGUMENT, json.status)
        assertEquals("json cannot contain embedded NUL characters", json.diagnostic)

        // Refused before the call, so nothing was requested and nothing parsed.
        assertEquals("", map.styleUrl())
        assertEquals("", map.loadedStyleJson())
      } finally {
        map.close()
      }
    }
  }

  private fun <T> withMap(body: (MapHandle) -> T): T {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    try {
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
      try {
        return body(map)
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun aCameraChangeAcceptsEveryOptionalDescriptorAsNull() {
    withMap { map ->
      // Each of these measures zero bytes, because the only descriptor it could place is absent.
      map.scaleBy(2.0, null)
      map.moveByAnimated(10.0, 10.0, null)
      map.pitchByAnimated(5.0, null)
      map.scaleByAnimated(2.0, null, null)

      // The map still answers afterwards, so the calls reached native rather than being refused
      // before they crossed into the module.
      assertEquals(false, map.isClosed)
    }
  }

  @Test
  fun aCameraChangeAcceptsTheSameDescriptorsWhenPresent() {
    withMap { map ->
      // The other half of the pair: the same entry points with a descriptor to place, so the
      // zero-byte path above is shown to be a real branch rather than the only one that works.
      map.scaleBy(2.0, org.maplibre.nativeffi.geo.ScreenPoint(16.0, 16.0))
      map.moveByAnimated(10.0, 10.0, AnimationOptions().also { it.durationMs = 0.0 })
      assertEquals(false, map.isClosed)
    }
  }

  private companion object {
    /** The character C reads as the end of a string, which no null-terminated input may carry. */
    val NUL: Char = Char(0)

    /**
     * A document with non-ASCII text, so the readback is checked for byte fidelity rather than only
     * for ASCII surviving a UTF-8 round trip through two heaps.
     */
    const val STYLE_WITH_UNICODE =
      """{"version":8,"name":"caf\u00e9 \u2014 \u5730\u56fe","sources":{},"layers":[]}"""
  }
}
