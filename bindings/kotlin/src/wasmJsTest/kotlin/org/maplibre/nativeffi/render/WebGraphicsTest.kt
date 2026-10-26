@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.render

import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotEquals
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import kotlinx.coroutines.await
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapFixture
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.NativePointer
import org.maplibre.nativeffi.OpenGlContextDescriptor
import org.maplibre.nativeffi.OpenGlContextDescriptorData
import org.maplibre.nativeffi.OpenGlContextOwnership
import org.maplibre.nativeffi.OpenGlOwnedTextureDescriptor
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderResult
import org.maplibre.nativeffi.WebGlContextDescriptor
import org.maplibre.nativeffi.WebGlContextKind
import org.maplibre.nativeffi.WebGpuBorrowedTexture
import org.maplibre.nativeffi.WebGpuBorrowedTextureDescriptor
import org.maplibre.nativeffi.WebGpuContextDescriptor
import org.maplibre.nativeffi.WebGraphics
import org.maplibre.nativeffi.attachWebGpuBorrowedTexture
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.withMap
import org.maplibre.nativeffi.withWebGpuTexture

/**
 * Graphics objects that the page creates, imported through [WebGraphics], render a caller-driven
 * frame on the build's backend: a `GPUDevice` into a borrowed ring of the page's `GPUTexture` and
 * `GPUTextureView` on WebGPU, and an owned texture over a `WebGL2RenderingContext` on WebGL. Each
 * import closes after its session, and its handle then refuses use.
 */
class WebGraphicsTest {
  @Test
  fun aGraphicsObjectThatThePageCreatedRendersAFrame() = runSuspendTest {
    withMap(MapMode.STATIC, options = smallMapOptions(MapMode.STATIC, WIDTH, HEIGHT)) {
      when (TestGraphics.backend) {
        TestBackend.WEBGPU -> renderWithPageDevice()
        TestBackend.WEBGL -> renderWithPageContext()
        else -> error("a browser build renders with WebGL or WebGPU, not ${TestGraphics.backend}")
      }
    }
  }

  private suspend fun MapFixture.renderWithPageDevice() {
    val pageDevice =
      checkNotNull(requestPageWebGpuDevice().await<JsAny?>()) {
        "the browser provided no WebGPU device; see the console for why"
      }
    val pageTexture = createRingTexture(pageDevice, WIDTH, HEIGHT)
    val device = WebGraphics.importWebGpuDevice(pageDevice)
    val texture = WebGraphics.importWebGpuTexture(pageTexture)
    val view = WebGraphics.importWebGpuTextureView(createTextureView(pageTexture))
    val pointer = device.pointer
    val descriptor =
      WebGpuBorrowedTextureDescriptor(
        extent,
        physicalWidth = WIDTH.toUInt(),
        physicalHeight = HEIGHT.toUInt(),
        context =
          WebGpuContextDescriptor(NativePointer.NULL_POINTER, pointer, NativePointer.NULL_POINTER),
        textures = listOf(WebGpuBorrowedTexture(texture.pointer, view.pointer)),
        format = WebGraphics.webGpuTextureFormat("rgba8unorm"),
      )
    withRenderSession(
      RenderDriverKind.CALLER_GRAPHICS_THREAD,
      attach = { options -> map.attachWebGpuBorrowedTexture(descriptor, options) },
    ) {
      setStyle(RED_BACKGROUND_STYLE)
      renderStill()
      // One more frame, left for the test to acquire.
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)

      val frame = assertNotNull(session.acquireFrame())
      // The frame names the page's own texture, which the page reads through its own device. The
      // copy is submitted while the frame is borrowed, and the queue orders it after the session's
      // rendering.
      val center = frame.withWebGpuTexture { frameTexture ->
        val named = WebGraphics.webGpuObject(frameTexture.texture)
        assertTrue(isSameObject(named, pageTexture), "the frame names another texture")
        readCenterPixel(pageDevice, named, WIDTH, HEIGHT)
      }
      assertEquals("255,0,0,255", center.await<JsString>().toString())
      frame.release(null)

      complete(session.detach())
    }

    view.close()
    texture.close()
    device.close()
    assertFailsWith<IllegalStateException> { device.pointer }
    assertFailsWith<IllegalStateException> { pointer.address }
    destroyWebGpuDevice(pageDevice)
  }

  private suspend fun MapFixture.renderWithPageContext() {
    val pageContext =
      checkNotNull(pageWebGlContext(WIDTH)) { "creating the page's WebGL context failed" }
    val context = WebGraphics.registerWebGlContext(pageContext)
    val descriptor =
      OpenGlOwnedTextureDescriptor(
        extent,
        OpenGlContextDescriptor(
          OpenGlContextOwnership.SHARED,
          OpenGlContextDescriptorData.WebGl(
            WebGlContextDescriptor(WebGlContextKind.EXISTING, context.context),
          ),
        ),
      )
    withRenderSession(
      RenderDriverKind.CALLER_GRAPHICS_THREAD,
      attach = { options -> map.attachOpenGlOwnedTexture(descriptor, options) },
    ) {
      setStyle(RED_BACKGROUND_STYLE)
      renderStill()
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)

      val readback = complete(session.readTexture())
      val center = HEIGHT / 2 * readback.info.stride.toInt() + WIDTH / 2 * 4
      assertEquals(
        listOf(255, 0, 0, 255),
        readback.data.copyOfRange(center, center + 4).map { it.toInt() and 0xff },
      )

      complete(session.detach())
    }

    val handle = context.context
    context.close()
    assertFailsWith<IllegalStateException> { context.context }
    // The closed handle stays reserved, so a new registration never takes it.
    WebGraphics.registerWebGlContext(pageContext).use { assertNotEquals(handle, it.context) }
    // Chromium caps the WebGL contexts of a page, so the test frees its own.
    loseWebGlContext(pageContext)
  }

  private companion object {
    const val WIDTH = 32
    const val HEIGHT = 16
    val extent = LogicalExtent(WIDTH.toUInt(), HEIGHT.toUInt(), 1.0)
  }
}

/**
 * Copies the center pixel of [texture], of [width] x [height], through [device], and resolves to
 * its red, green, blue, and alpha bytes, joined by commas.
 */
private fun readCenterPixel(
  device: JsAny,
  texture: JsAny,
  width: Int,
  height: Int,
): Promise<JsString> =
  js(
    """(() => {
      const buffer = device.createBuffer({
        size: 256,
        usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ,
      });
      const encoder = device.createCommandEncoder();
      encoder.copyTextureToBuffer(
        { texture, origin: { x: width >> 1, y: height >> 1 } },
        { buffer, bytesPerRow: 256 },
        [1, 1],
      );
      device.queue.submit([encoder.finish()]);
      const bgra = texture.format.startsWith('bgra');
      return buffer.mapAsync(GPUMapMode.READ).then(() => {
        const [b0, b1, b2, b3] = new Uint8Array(buffer.getMappedRange(0, 4));
        buffer.destroy();
        return (bgra ? [b2, b1, b0, b3] : [b0, b1, b2, b3]).join(',');
      });
    })()""",
  )

/**
 * Creates a `GPUTexture` of [width] x [height] on [device] that a borrowed ring can render into and
 * the page can copy from.
 */
private fun createRingTexture(device: JsAny, width: Int, height: Int): JsAny =
  js(
    """device.createTexture({
      size: [width, height],
      format: 'rgba8unorm',
      usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING |
        GPUTextureUsage.COPY_SRC,
    })""",
  )

private fun createTextureView(texture: JsAny): JsAny = js("texture.createView()")

private fun isSameObject(first: JsAny, second: JsAny): Boolean = js("first === second")

private fun destroyWebGpuDevice(device: JsAny): Unit = js("device.destroy()")

private fun loseWebGlContext(context: JsAny): Unit =
  js("context.getExtension('WEBGL_lose_context')?.loseContext()")
