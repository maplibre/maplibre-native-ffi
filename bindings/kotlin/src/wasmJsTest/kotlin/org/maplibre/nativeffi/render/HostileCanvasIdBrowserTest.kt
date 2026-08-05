package org.maplibre.nativeffi.render

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.PageCanvases
import org.maplibre.nativeffi.assertPresentedColor
import org.maplibre.nativeffi.assertRenderedColor
import org.maplibre.nativeffi.backgroundStyle
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.drain
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.waitForMapEvent
import org.maplibre.nativeffi.withMap

/**
 * Presents a frame to a page canvas whose id is valid HTML and invalid CSS.
 *
 * The public API takes an element `id`, and the module reaches the element through two different
 * mechanisms that have to agree on which one it means. `pthread_create` resolves a canvas with
 * `document.querySelector`, so the id travels there as a selector; creating the context looks the
 * canvas up in Emscripten's own registry, which is keyed by the element's `id` unchanged. An id may
 * be any string without ASCII whitespace, and a CSS identifier may not, so a selector built by
 * concatenation goes wrong for a large family of perfectly ordinary ids —
 * `#9mln:test.hostile[canvas]` parses as a number, a pseudo-class, a class, and an attribute
 * selector, and matches nothing at all.
 *
 * So this renders through [PageCanvases.HOSTILE] end to end: reserving it transfers the element,
 * creating a context finds it, and the page's `<canvas>` shows what the owner thread drew. Each
 * step exercises a different one of the two spellings, and only the pair being right makes the
 * colour arrive.
 *
 * The reservation is not asserted here because it cannot fail quietly. Emscripten refuses the whole
 * `pthread_create` when any named canvas does not resolve, so an unescaped selector takes down
 * every test in this suite; this one is what names the reason.
 */
class HostileCanvasIdBrowserTest {
  @Test
  fun presentsToACanvasWhoseIdIsNotACssIdentifier(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap(WIDTH, HEIGHT) { runtime, map ->
        val context = WebglContext.createForCanvas(PageCanvases.HOSTILE, WIDTH, HEIGHT)
        try {
          val session =
            map.attachOpenGLSurface(
              OpenGLSurfaceDescriptor(
                RenderTargetExtent(WIDTH, HEIGHT, 1.0),
                context.descriptor(),
                // A WebGL context is already bound to its canvas, so there is no drawable to name.
                NativePointer.NULL,
              )
            )
          try {
            drain(runtime)
            map.setStyleJson(backgroundStyle(COLOR))
            waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
            assertTrue(
              renderUntilSettled(runtime, session),
              "the session rendered no frame for the $COLOR background",
            )

            // The readback says the map drew the right colour into the right context; the page
            // assertion says that context's canvas is the element the document holds under this id.
            // The second is the claim, and the first is what makes its failure diagnosable.
            val pixels = context.readPixels(DEFAULT_FRAMEBUFFER, WIDTH, HEIGHT)
            assertRenderedColor(pixels, RED, GREEN, BLUE)
            assertPresentedColor(PageCanvases.HOSTILE, RED, GREEN, BLUE)
          } finally {
            session.close()
          }
        } finally {
          context.close()
        }
      }
    }
  }

  /**
   * The id limits, refused where a host can still pick a different id.
   *
   * The module carries a canvas id in a fixed-size record and refuses a longer one, and the two
   * calls that name a canvas sit either side of something irreversible: reserving transfers the
   * `<canvas>` element to the owner thread, and Emscripten transfers only at `pthread_create`, so a
   * host that got as far as a context failing would have given the element away for the page's
   * whole life with nothing left able to draw into it. Refusing at both, and saying what the limit
   * is, is what leaves the host a move.
   */
  @Test
  fun anIdTooLongForTheModuleIsRefusedByBothCallsThatNameACanvas(): Promise<JsAny?> = browserTest {
    // One byte past what the module's record holds, so it is the first id that cannot work.
    val tooLong = "m".repeat(MAX_ID_BYTES)

    val reserving =
      assertFailsWith<InvalidArgumentException> { WebglContext.reserveCanvas(tooLong) }
    assertEquals(MaplibreStatus.INVALID_ARGUMENT, reserving.status)
    assertTrue(
      reserving.diagnostic.contains("${MAX_ID_BYTES - 1} bytes"),
      "the refusal did not name the limit: ${reserving.diagnostic}",
    )

    val creating =
      assertFailsWith<InvalidArgumentException> {
        WebglContext.createForCanvas(tooLong, WIDTH, HEIGHT)
      }
    assertTrue(
      creating.diagnostic.contains("${MAX_ID_BYTES - 1} bytes"),
      "the refusal did not name the limit: ${creating.diagnostic}",
    )

    // And the longest id that fits gets past the check, so what is refused above is the length
    // rather than long ids in general. It fails at the canvas instead, because nothing reserved it.
    maplibreScope {
      val longest = "m".repeat(MAX_ID_BYTES - 1)
      val error =
        assertFailsWith<InvalidStateException> {
          WebglContext.createForCanvas(longest, WIDTH, HEIGHT)
        }
      assertTrue(
        error.diagnostic.contains(longest),
        "the refusal was not the canvas's: ${error.diagnostic}",
      )
    }
  }

  /**
   * An id with whitespace around it, which the module's id list would trim off one call and not the
   * other.
   *
   * The ids cross to the owner thread as one comma-separated list, and the module trims ASCII
   * whitespace from each entry, so reserving `" x "` transfers the element `x`. Creating a context
   * asks Emscripten's registry for the id unchanged, finds no `" x "`, and fails — with the element
   * already transferred. HTML forbids whitespace in an `id` anyway, so both calls refuse it rather
   * than one of them silently meaning something else.
   */
  @Test
  fun anIdWithSurroundingWhitespaceIsRefusedRatherThanTrimmed(): Promise<JsAny?> = browserTest {
    val padded = " ${PageCanvases.HOSTILE} "
    for (diagnostic in
      listOf(
        assertFailsWith<InvalidArgumentException> { WebglContext.reserveCanvas(padded) }.diagnostic,
        assertFailsWith<InvalidArgumentException> {
            WebglContext.createForCanvas(padded, WIDTH, HEIGHT)
          }
          .diagnostic,
      )) {
      assertTrue(
        diagnostic.contains("whitespace"),
        "the refusal did not name the reason: $diagnostic",
      )
    }
  }

  /**
   * Renders until the session has nothing left to draw, pumping the runtime in between.
   *
   * The first render after a new style still paints the old one, because a render draws whatever
   * parameters the map last handed the renderer. The frame that matters is the last, and pumping is
   * what gives the MapLibre worker that parsed the style a chance to make the map renderable at
   * all.
   */
  private fun renderUntilSettled(runtime: RuntimeHandle, session: RenderSessionHandle): Boolean {
    var rendered = false
    repeat(ATTEMPTS) {
      if (session.renderUpdate()) {
        rendered = true
      } else if (rendered) {
        return true
      }
      runtime.pump(PUMP_MILLIS)
      while (runtime.pollEvent() != null) {}
    }
    return rendered
  }

  private companion object {
    const val WIDTH = PageCanvases.WIDTH
    const val HEIGHT = PageCanvases.HEIGHT

    // Framebuffer zero, which is the canvas's own and what a surface target renders into.
    const val DEFAULT_FRAMEBUFFER = 0

    // Three distinct channels, and a colour no other test in this suite presents, so a canvas
    // showing another test's frame fails rather than passing.
    const val COLOR = "#2060a0"
    const val RED = 0x20
    const val GREEN = 0x60
    const val BLUE = 0xA0

    const val ATTEMPTS = 200
    const val PUMP_MILLIS = 2L

    // MLN_BROWSER_WEBGL_CANVAS_ID_BYTES in src/browser/webgl_context.c, terminator included, so an
    // id of this many bytes is the first one too long to carry.
    const val MAX_ID_BYTES = 64
  }
}
