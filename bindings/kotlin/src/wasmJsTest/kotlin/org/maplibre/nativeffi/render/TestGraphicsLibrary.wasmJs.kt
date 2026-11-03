@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.render

import kotlin.js.Promise
import kotlinx.coroutines.await
import org.maplibre.nativeffi.ImportedWebGpuObject
import org.maplibre.nativeffi.NativePointer
import org.maplibre.nativeffi.RegisteredWebGlContext
import org.maplibre.nativeffi.WebGraphics
import org.maplibre.nativeffi.internal.c.unsignedWord
import org.maplibre.nativeffi.internal.memory.NativeMemory

/**
 * The browser's graphics fixtures. The page creates its WebGL 2 context or WebGPU device, which the
 * suite imports through [WebGraphics], and the textures of a borrowed ring on them.
 * `bindings/kotlin/wasm/test_support.c` adds the WebGL textures and the canvases that a session's
 * core worker takes over. The browser keeps its own fixtures, because WebGL contexts and WebGPU
 * devices come from JavaScript; see tests/graphics/include/mln_test_graphics.h.
 */
internal actual object TestGraphicsLibrary {
  // Kept for the process, as the context that every caller-driven session shares.
  private var webGlContext: RegisteredWebGlContext? = null
  private var pageWebGpuDevice: JsAny? = null
  private var webGpuDevice: ImportedWebGpuObject? = null

  /**
   * Creates a WebGL 2 context on the page over an OffscreenCanvas of its own, or requests a WebGPU
   * device for the page and waits for the browser's answer, and imports it into the module. For
   * WebGPU, the graphics value that [context] later reads is the device's handle.
   */
  actual suspend fun create(backend: UInt): Long =
    when (backend) {
      TestBackend.WEBGL.graphicsBackend -> {
        val context =
          checkNotNull(pageWebGlContext(PAGE_CANVAS_SIZE)) {
            "creating the page's WebGL context failed"
          }
        WebGraphics.registerWebGlContext(context).also { webGlContext = it }.context.toLong()
      }
      TestBackend.WEBGPU.graphicsBackend -> {
        val device =
          checkNotNull(requestPageWebGpuDevice().await<JsAny?>()) {
            "the browser provided no WebGPU device; see the console for why"
          }
        pageWebGpuDevice = device
        WebGraphics.importWebGpuDevice(device).also { webGpuDevice = it }.pointer.address
      }
      else ->
        error("the browser fixtures create WebGL and WebGPU contexts, not test backend $backend")
    }

  actual fun context(graphics: Long): TestGraphicsContext =
    when (TestGraphics.backend) {
      // Sessions use no WebGPU instance, and the module imports a device without one.
      TestBackend.WEBGPU -> TestGraphicsContext(webGpuDevice = graphics)
      else -> TestGraphicsContext(webGlContext = graphics.toInt())
    }

  actual fun makeCurrent(graphics: Long) {
    val result = mln_kw_test_webgl_make_current(graphics.toInt())
    check(result == 0) { "making the page's WebGL context current failed: $result" }
  }

  /**
   * A WebGL texture that the page creates on the context [graphics] names, or a WebGPU texture and
   * view that it creates on the page's device and imports through [WebGraphics].
   */
  actual fun createTexture(graphics: Long, width: Int, height: Int): TestTexture =
    when (TestGraphics.backend) {
      TestBackend.WEBGPU -> {
        val device = checkNotNull(pageWebGpuDevice) { "the page has no WebGPU device" }
        val pageTexture = createRingTexture(device, width, height)
        val texture = WebGraphics.importWebGpuTexture(pageTexture)
        val view = WebGraphics.importWebGpuTextureView(createTextureView(pageTexture))
        TestTexture(
          width = width.toUInt(),
          height = height.toUInt(),
          format = WebGraphics.webGpuTextureFormat("rgba8unorm"),
          webGpuTexture = texture.pointer.address,
          webGpuTextureView = view.pointer.address,
        ) {
          view.close()
          texture.close()
          destroyWebGpuTexture(pageTexture)
        }
      }
      else -> webGlTexture(graphics.toInt(), width, height)
    }

  // Sessions render into textures of their own, so the page's canvas only carries the context.
  private const val PAGE_CANVAS_SIZE = 64
}

/**
 * A canvas element that a session takes over, named by [selector]. [close] removes it once that
 * session has closed.
 */
internal class TestCanvas(val selector: String, private val remove: () -> Unit) : AutoCloseable {
  override fun close() = remove()
}

/**
 * Creates a canvas element of [width] x [height] in the document, which a WebGL session's core
 * worker takes over, or a WebGPU session draws to from the page.
 */
internal fun testCanvas(width: Int, height: Int): TestCanvas {
  val canvas = mln_kw_test_canvas_create(width, height)
  check(canvas != 0) { "cannot allocate a test canvas" }
  // The fixture's address is its selector's, a NUL-terminated string.
  val address = unsignedWord(canvas)
  val selector =
    NativeMemory.getBytes(address, NativeMemory.stringLength(address).toInt()).decodeToString()
  return TestCanvas(selector) { mln_kw_test_canvas_destroy(canvas) }
}

private fun mln_kw_test_webgl_make_current(context: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_webgl_make_current(context)")

private fun mln_kw_test_canvas_create(width: Int, height: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_canvas_create(width, height)")

private fun mln_kw_test_canvas_destroy(canvas: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_canvas_destroy(canvas)")

/**
 * A WebGL texture of [width] x [height] that the page creates on [context], an
 * `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for a borrowed ring.
 */
internal fun webGlTexture(context: Int, width: Int, height: Int): TestTexture {
  val texture = mln_kw_test_webgl_texture_create(context, width, height)
  check(texture != 0) { "creating a WebGL texture on the page's context failed" }
  return TestTexture(
    width = width.toUInt(),
    height = height.toUInt(),
    openGlTexture = texture.toUInt(),
    openGlTarget = GL_TEXTURE_2D,
  ) {
    mln_kw_test_webgl_texture_destroy(context, texture)
  }
}

/**
 * Reads the pixel at [x], [y] of a WebGL texture from [webGlTexture] on [context], as red, green,
 * blue, and alpha bytes.
 */
internal fun webGlTexturePixel(context: Int, texture: UInt, x: Int, y: Int): List<Int> {
  val rgba = mln_kw_test_webgl_texture_pixel(context, texture.toInt(), x, y)
  return listOf(rgba and 0xff, (rgba ushr 8) and 0xff, (rgba ushr 16) and 0xff, rgba ushr 24)
}

private const val GL_TEXTURE_2D = 0x0DE1u

private fun mln_kw_test_webgl_texture_create(context: Int, width: Int, height: Int): Int =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_webgl_texture_create(context, width, height)",
  )

private fun mln_kw_test_webgl_texture_pixel(context: Int, texture: Int, x: Int, y: Int): Int =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_webgl_texture_pixel(context, texture, x, y)",
  )

private fun mln_kw_test_webgl_texture_destroy(context: Int, texture: Int): Unit =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_webgl_texture_destroy(context, texture)",
  )

/**
 * Creates a WebGL 2 context over an OffscreenCanvas of [size] x [size], with the depth and stencil
 * buffers that MapLibre draws with, or returns null when the browser refuses one. Sessions render
 * into textures of their own, and nothing presents this canvas, so it needs neither antialiasing
 * nor a preserved buffer.
 */
internal fun pageWebGlContext(size: Int): JsAny? =
  js(
    """new OffscreenCanvas(size, size).getContext('webgl2', {
      depth: true, stencil: true, antialias: false, preserveDrawingBuffer: false,
    })""",
  )

/**
 * Creates an `rgba8unorm` `GPUTexture` of [width] x [height] on [device] that a borrowed ring can
 * render into and the page can copy from.
 */
internal fun createRingTexture(device: JsAny, width: Int, height: Int): JsAny =
  js(
    """device.createTexture({
      size: [width, height],
      format: 'rgba8unorm',
      usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING |
        GPUTextureUsage.COPY_SRC,
    })""",
  )

internal fun createTextureView(texture: JsAny): JsAny = js("texture.createView()")

private fun destroyWebGpuTexture(texture: JsAny): Unit = js("texture.destroy()")

/** Requests a WebGPU device for the page, which resolves to null when the browser has none. */
internal fun requestPageWebGpuDevice(): Promise<JsAny?> =
  js("navigator.gpu.requestAdapter().then((adapter) => adapter && adapter.requestDevice())")

/**
 * A WebGPU surface over a canvas element, imported as [surface], and the [format] that the page
 * prefers. [close] releases it once its session has closed.
 */
internal class WebGpuCanvasSurface(private val surface: ImportedWebGpuObject, val format: UInt) :
  AutoCloseable {
  val pointer: NativePointer
    get() = surface.pointer

  override fun close() = surface.close()
}

/** Creates a WebGPU surface over [canvas], which must be a canvas element of the document. */
internal fun webGpuCanvasSurface(canvas: TestCanvas): WebGpuCanvasSurface {
  val context =
    checkNotNull(webGpuCanvasContext(canvas.selector)) {
      "${canvas.selector} offers no WebGPU context"
    }
  val format = WebGraphics.webGpuTextureFormat(preferredCanvasFormat())
  return WebGpuCanvasSurface(WebGraphics.importWebGpuSurface(context), format)
}

private fun webGpuCanvasContext(selector: String): JsAny? =
  js("document.querySelector(selector).getContext('webgpu')")

private fun preferredCanvasFormat(): String = js("navigator.gpu.getPreferredCanvasFormat()")
