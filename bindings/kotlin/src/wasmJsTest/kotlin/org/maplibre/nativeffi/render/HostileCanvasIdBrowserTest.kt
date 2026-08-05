package org.maplibre.nativeffi.render

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertTrue
import org.maplibre.nativeffi.PageCanvases
import org.maplibre.nativeffi.assertPresentedColor
import org.maplibre.nativeffi.assertRenderedColor
import org.maplibre.nativeffi.backgroundStyle
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.drain
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
  }
}
