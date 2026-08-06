package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.PageCanvas
import org.maplibre.nativeffi.assertPresentedColor
import org.maplibre.nativeffi.assertRenderedColor
import org.maplibre.nativeffi.backgroundStyle
import org.maplibre.nativeffi.drain
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.waitForMapEvent
import org.maplibre.nativeffi.withMap

/**
 * Puts a frame on the page's canvas, for each of the three render target families.
 *
 * This is a different claim from the one `BrowserRenderTest` makes. That test renders into a
 * private surface and reads it back, which proves the map drew the right pixels and nothing about
 * where they went. Here the target is the `<canvas>` element the host page owns, transferred to
 * this thread as it was created, and every assertion is against framebuffer zero of that element: a
 * surface session draws straight into it, and a texture session's frame reaches it only by being
 * blitted there.
 *
 * Presentation is zero-copy in all three cases. Nothing here reads a pixel back to show one; the
 * readbacks are assertions about what a target holds, not steps in getting it onto the canvas.
 *
 * All three share one context, because the module's link names one element id and a canvas hands
 * out one WebGL context. A page showing more than one map at once is a documented limitation of
 * this binding rather than something a test could reach.
 *
 * The specification's test table has no row for presenting, because every other platform's host
 * does it with its own graphics API and outside the binding entirely. The one row these tests do
 * close is BND-171: a texture native can look up has to belong to the render thread's context
 * table, and until a host could run its own GL work there it could not supply one.
 */
class BrowserPresentationTest {
  @Test
  fun presentsASurfaceSessionToThePageCanvas() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      val context = PageCanvas.context()
      val session = map.attachOpenGLSurface(surfaceDescriptor(context, WIDTH, HEIGHT))
      try {
        renderStyle(runtime, map, session, SURFACE_COLOR)
        assertPresentedColor(context, SURFACE_RED, SURFACE_GREEN, SURFACE_BLUE)

        // Resizing is two steps, and both are the host's. The canvas's drawing buffer is what a
        // frame has room to land in, and the session's extent is what MapLibre lays a frame out
        // for; neither implies the other, and a canvas can only be sized on the thread holding it.
        // The colour changes with the size so that what arrives is provably the new frame rather
        // than the old one still sitting in a preserved drawing buffer.
        context.resizeCanvas(HALF_WIDTH, HALF_HEIGHT)
        session.resize(HALF_WIDTH, HALF_HEIGHT, 1.0)
        renderStyle(runtime, map, session, RESIZED_COLOR)
        assertPresentedColor(
          context,
          RESIZED_RED,
          RESIZED_GREEN,
          RESIZED_BLUE,
          HALF_WIDTH,
          HALF_HEIGHT,
        )

        // Detaching gives the map back without closing the session, and the canvas stays with this
        // thread rather than with the session, so the same context serves a second one. That is the
        // whole reason a canvas is transferred at thread creation instead of at attach.
        session.detach()
      } finally {
        session.close()
      }

      val reattached = map.attachOpenGLSurface(surfaceDescriptor(context, HALF_WIDTH, HALF_HEIGHT))
      try {
        renderStyle(runtime, map, reattached, REATTACHED_COLOR)
        assertPresentedColor(
          context,
          REATTACHED_RED,
          REATTACHED_GREEN,
          REATTACHED_BLUE,
          HALF_WIDTH,
          HALF_HEIGHT,
        )
      } finally {
        reattached.close()
      }
    }
  }

  @Test
  fun presentsAnOwnedTextureToThePageCanvas() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      val context = PageCanvas.context()
      val session =
        map.attachOpenGLOwnedTexture(
          OpenGLOwnedTextureDescriptor(RenderTargetExtent(WIDTH, HEIGHT, 1.0), context.descriptor())
        )
      try {
        renderStyle(runtime, map, session, OWNED_COLOR)

        // The frame is what names the texture, and it is borrowed for as long as the handle is
        // open,
        // so the blit happens while it is held rather than after it is given back.
        val frame = session.acquireOpenGLOwnedTextureFrame()
        try {
          val rendered = frame.frame()
          assertNotEquals(0, rendered.texture())
          assertEquals(WIDTH, rendered.width())
          assertEquals(HEIGHT, rendered.height())
          // What the target holds, then what the canvas holds. The second is the claim; the first
          // is
          // what makes a failure of the second say which half broke.
          assertRenderedColor(
            context.readPixels(rendered.texture(), WIDTH, HEIGHT),
            OWNED_RED,
            OWNED_GREEN,
            OWNED_BLUE,
          )
          context.presentTexture(rendered.texture(), rendered.width(), rendered.height())
        } finally {
          frame.close()
        }

        assertPresentedColor(context, OWNED_RED, OWNED_GREEN, OWNED_BLUE)
      } finally {
        session.close()
      }
    }
  }

  // Spec coverage: BND-171.
  @Test
  fun presentsABorrowedTextureAndThenASecondOne() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      val context = PageCanvas.context()
      // Created in the session's own context, because WebGL shares no objects between contexts: a
      // texture made through another context names nothing the session could attach.
      val first = context.createTexture(WIDTH, HEIGHT)
      val second = context.createTexture(WIDTH, HEIGHT)
      assertNotEquals(first, second)
      try {
        val session = map.attachOpenGLBorrowedTexture(borrowedDescriptor(context, first))
        try {
          renderStyle(runtime, map, session, BORROWED_COLOR)
          context.presentTexture(first, WIDTH, HEIGHT)
          assertPresentedColor(context, BORROWED_RED, BORROWED_GREEN, BORROWED_BLUE)

          // The same session, a different texture, and a different colour. Presenting the second
          // texture is what proves the target really moved: presenting the first one would keep
          // showing the colour it was left holding.
          session.setOpenGLBorrowedTextureTarget(borrowedDescriptor(context, second))
          renderStyle(runtime, map, session, RETARGETED_COLOR)
          assertRenderedColor(
            context.readPixels(second, WIDTH, HEIGHT),
            RETARGETED_RED,
            RETARGETED_GREEN,
            RETARGETED_BLUE,
          )
          context.presentTexture(second, WIDTH, HEIGHT)
          assertPresentedColor(context, RETARGETED_RED, RETARGETED_GREEN, RETARGETED_BLUE)
          session.close()

          // Closing the session left both textures alone. A caller-owned target only borrows what
          // it
          // is given, so presenting the first one still puts its own colour on the canvas — which
          // it
          // could not do if the session had deleted it, or drawn over it, on the way out.
          context.presentTexture(first, WIDTH, HEIGHT)
          assertPresentedColor(context, BORROWED_RED, BORROWED_GREEN, BORROWED_BLUE)
        } finally {
          session.close()
        }
      } finally {
        // Released only once no target borrows them: a session naming a destroyed texture renders
        // into nothing.
        context.destroyTexture(second)
        context.destroyTexture(first)
      }
    }
  }

  private fun surfaceDescriptor(
    context: WebglContext,
    width: Int,
    height: Int,
  ): OpenGLSurfaceDescriptor =
    OpenGLSurfaceDescriptor(
      RenderTargetExtent(width, height, 1.0),
      context.descriptor(),
      // A WebGL context is already bound to its canvas, so there is no drawable to name where every
      // other OpenGL provider names one, and native refuses anything else here.
      NativePointer.NULL,
    )

  private fun borrowedDescriptor(context: WebglContext, texture: Int) =
    OpenGLBorrowedTextureDescriptor(
      RenderTargetExtent(WIDTH, HEIGHT, 1.0),
      WIDTH,
      HEIGHT,
      context.descriptor(),
      texture,
      TEXTURE_2D,
    )

  /**
   * Loads a background style and renders one frame of it.
   *
   * Waiting for the style is what makes the colour mean anything. A map that is already renderable
   * renders on the first ask, with whatever style it still has, so setting a new style and
   * rendering immediately presents the *previous* colour. The queue is drained first so that the
   * load being waited for is this one rather than a load already reported.
   */
  private fun renderStyle(
    runtime: RuntimeHandle,
    map: MapHandle,
    session: RenderSessionHandle,
    color: String,
  ) {
    drain(runtime)
    map.setStyleJson(backgroundStyle(color))
    waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
    assertTrue(
      renderUntilSettled(runtime, session),
      "the session rendered no frame for the $color background",
    )
  }

  /**
   * Renders until the session has nothing left to draw, pumping the runtime in between.
   *
   * Rendering once is not enough after a style change, and that is MapLibre's shape rather than
   * this binding's: a render draws the parameters the map last handed the renderer, so the first
   * one after a new style still paints the old one. A render can also find nothing to draw at all,
   * because the style is parsed on a MapLibre worker and the map only becomes renderable once the
   * update that produced has been pumped through. Pumping blocks this thread, which is legal here
   * and is what gives that worker a chance to run.
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
    const val WIDTH = PageCanvas.WIDTH
    const val HEIGHT = PageCanvas.HEIGHT
    const val HALF_WIDTH = PageCanvas.WIDTH / 2
    const val HALF_HEIGHT = PageCanvas.HEIGHT / 2

    // GL_TEXTURE_2D. The C API takes the GL enum unchanged, and this is the only target a render
    // target can be attached to.
    const val TEXTURE_2D = 3553

    // Every colour has three distinct channels, so a path that swapped or duplicated one would
    // still
    // be caught, and no two of these share a colour, so a canvas still holding an earlier frame
    // fails rather than passing.
    const val SURFACE_COLOR = "#4080c0"
    const val SURFACE_RED = 0x40
    const val SURFACE_GREEN = 0x80
    const val SURFACE_BLUE = 0xC0

    const val RESIZED_COLOR = "#c08040"
    const val RESIZED_RED = 0xC0
    const val RESIZED_GREEN = 0x80
    const val RESIZED_BLUE = 0x40

    const val REATTACHED_COLOR = "#8040c0"
    const val REATTACHED_RED = 0x80
    const val REATTACHED_GREEN = 0x40
    const val REATTACHED_BLUE = 0xC0

    const val OWNED_COLOR = "#20a060"
    const val OWNED_RED = 0x20
    const val OWNED_GREEN = 0xA0
    const val OWNED_BLUE = 0x60

    const val BORROWED_COLOR = "#a02060"
    const val BORROWED_RED = 0xA0
    const val BORROWED_GREEN = 0x20
    const val BORROWED_BLUE = 0x60

    const val RETARGETED_COLOR = "#60a020"
    const val RETARGETED_RED = 0x60
    const val RETARGETED_GREEN = 0xA0
    const val RETARGETED_BLUE = 0x20

    const val ATTEMPTS = 200
    const val PUMP_MILLIS = 2L
  }
}
