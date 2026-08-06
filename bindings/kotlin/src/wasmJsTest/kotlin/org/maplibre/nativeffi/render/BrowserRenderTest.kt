package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/**
 * Renders a real map frame in a real browser and looks at the pixels.
 *
 * This is the test the whole browser render path exists for. Everything below it — the module, the
 * WebGL context, the descriptors, the readback — can be exercised without a GPU ever being asked to
 * draw anything, and each of those pieces can be right while the frame is still blank. So the
 * assertion is on the image: a background layer of a known colour fills the viewport, and the
 * readback has to come back as that colour rather than as the zeroed buffer a target that never
 * rendered would leave.
 *
 * A style with only a background layer is deliberate. It needs no network, no tiles, and no glyphs,
 * so a failure here is a rendering failure rather than a resource one.
 */
class BrowserRenderTest {
  @Test
  fun rendersABackgroundFrameAndReadsItBack() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    try {
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = WIDTH
            height = HEIGHT
          },
        )
      try {
        // The context has to exist before the target that borrows it, and it outlives the
        // session: the C API borrows the handle for the target's lifetime.
        val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
        try {
          map.setStyleJson(BACKGROUND_STYLE_JSON)
          val session =
            map.attachOpenGLOwnedTexture(
              OpenGLOwnedTextureDescriptor(
                extent = RenderTargetExtent(WIDTH, HEIGHT, 1.0),
                context = context.descriptor(),
              )
            )
          try {
            assertTrue(
              renderUntilFrame(runtime, session),
              "the session never reported a rendered frame",
            )

            val info = session.textureImageInfo()
            assertEquals(WIDTH, info.width)
            assertEquals(HEIGHT, info.height)
            assertEquals(WIDTH * 4, info.stride)

            val pixels =
              NativeBuffer.allocate(info.byteLength).use { buffer ->
                assertEquals(info, session.readPremultipliedRgba8(buffer))
                buffer.toByteArray()
              }
            assertBackgroundImage(pixels, info)
          } finally {
            session.close()
          }
        } finally {
          context.close()
        }
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  /**
   * Renders until the session reports a frame, pumping the runtime in between.
   *
   * The first render has nothing to draw yet: the style is still parsing on a MapLibre worker, and
   * the map only becomes renderable once the update it produces has been pumped through. Pumping
   * blocks this thread, which is legal here and is what gives that worker a chance to run.
   */
  private fun renderUntilFrame(runtime: RuntimeHandle, session: RenderSessionHandle): Boolean {
    repeat(ATTEMPTS) {
      if (session.renderUpdate()) return true
      runtime.pump(PUMP_MILLIS)
      // Drained so the queue does not grow without bound while this waits. What the events say does
      // not matter here; the rendered frame is the thing being waited for.
      while (runtime.pollEvent() != null) {}
    }
    return false
  }

  /**
   * Asserts the readback is the background this style paints, and not an empty buffer.
   *
   * Checked as a whole image rather than a sampled pixel: a background layer covers the viewport,
   * so every pixel is the same colour, and a frame that rendered only part of the target would show
   * up here where a single sample would miss it.
   *
   * The comparison has a tolerance because the colour makes a round trip through a float shader and
   * an eight-bit render target, and a software rasteriser is allowed to land a step either side.
   */
  private fun assertBackgroundImage(pixels: ByteArray, info: TextureImageInfo) {
    assertEquals(info.byteLength.toInt(), pixels.size)
    assertTrue(
      pixels.any { it != ZERO_BYTE },
      "the readback was entirely zero, so nothing rendered",
    )
    for (y in 0 until info.height) {
      for (x in 0 until info.width) {
        val offset = y * info.stride + x * 4
        assertChannel(pixels, offset, BACKGROUND_RED, "red", x, y)
        assertChannel(pixels, offset + 1, BACKGROUND_GREEN, "green", x, y)
        assertChannel(pixels, offset + 2, BACKGROUND_BLUE, "blue", x, y)
        // Opaque, and exactly so: the background is fully opaque, and premultiplied readback of a
        // partly transparent frame would darken the channels above rather than only this one.
        assertChannel(pixels, offset + 3, 255, "alpha", x, y)
      }
    }
  }

  private fun assertChannel(
    pixels: ByteArray,
    offset: Int,
    expected: Int,
    channel: String,
    x: Int,
    y: Int,
  ) {
    val actual = pixels[offset].toInt() and 0xFF
    assertTrue(
      actual in (expected - TOLERANCE)..(expected + TOLERANCE),
      "pixel ($x, $y) has $channel $actual, but the background is $expected",
    )
  }

  private companion object {
    const val WIDTH = 64
    const val HEIGHT = 32

    // #4080c0, chosen so that no two channels share a value: a readback that swapped or duplicated
    // channels would still pass against a grey or a primary.
    const val BACKGROUND_RED = 0x40
    const val BACKGROUND_GREEN = 0x80
    const val BACKGROUND_BLUE = 0xC0
    const val TOLERANCE = 2

    const val ATTEMPTS = 200
    const val PUMP_MILLIS = 2L
    const val ZERO_BYTE: Byte = 0

    const val BACKGROUND_STYLE_JSON =
      """
      {
        "version": 8,
        "name": "kotlin-browser-render-test",
        "sources": {},
        "layers": [
          {"id": "background", "type": "background", "paint": {"background-color": "#4080c0"}}
        ]
      }
      """
  }
}
