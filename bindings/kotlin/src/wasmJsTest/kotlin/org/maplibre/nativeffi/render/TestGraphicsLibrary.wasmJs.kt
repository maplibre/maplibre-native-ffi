@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.render

import kotlin.js.Promise
import kotlinx.coroutines.await
import org.maplibre.nativeffi.WebObject

/**
 * The browser's graphics fixtures. The page creates its WebGL 2 context or WebGPU device, and the
 * textures of a borrowed ring on them, and lends each to the module as a [WebObject]. The browser
 * keeps its own fixtures, because WebGL contexts and WebGPU devices come from JavaScript; see
 * tests/graphics/include/mln_test_graphics.h.
 */
internal actual object TestGraphicsLibrary {
  private var pageWebGlContext: JsAny? = null
  private var pageWebGpuDevice: JsAny? = null

  /**
   * Creates a WebGL 2 context on the page over an OffscreenCanvas of its own, or requests a WebGPU
   * device for the page and waits for the browser's answer, and returns its handle.
   */
  actual suspend fun create(backend: UInt): Long =
    when (backend) {
      TestBackend.WEBGL.graphicsBackend -> {
        val context =
          checkNotNull(pageWebGlContext(PAGE_CANVAS_SIZE)) {
            "creating the page's WebGL context failed"
          }
        pageWebGlContext = context
        WebObject.of(context).handle.toLong()
      }
      TestBackend.WEBGPU.graphicsBackend -> {
        val device =
          checkNotNull(requestPageWebGpuDevice().await<JsAny?>()) {
            "the browser provided no WebGPU device; see the console for why"
          }
        pageWebGpuDevice = device
        WebObject.of(device).handle.toLong()
      }
      else ->
        error("the browser fixtures create WebGL and WebGPU contexts, not test backend $backend")
    }

  actual fun context(graphics: Long): TestGraphicsContext =
    when (TestGraphics.backend) {
      TestBackend.WEBGPU -> TestGraphicsContext(webGpuDevice = graphics.toUInt())
      else -> TestGraphicsContext(webGlContext = graphics.toUInt())
    }

  /** WebGL has no current context: the module draws with the context object it holds. */
  actual fun makeCurrent(graphics: Long) {}

  /** A texture that the page creates on its context or device, lent to the module. */
  actual fun createTexture(graphics: Long, width: Int, height: Int): TestTexture =
    when (TestGraphics.backend) {
      TestBackend.WEBGPU -> {
        val device = checkNotNull(pageWebGpuDevice) { "the page has no WebGPU device" }
        val pageTexture = createRingTexture(device, width, height)
        val texture = WebObject.of(pageTexture)
        TestTexture(
          width = width.toUInt(),
          height = height.toUInt(),
          webGpuFormat = "rgba8unorm",
          webGpuTexture = texture.handle,
        ) {
          texture.close()
          destroyWebGpuTexture(pageTexture)
        }
      }
      else -> {
        val context = checkNotNull(pageWebGlContext) { "the page has no WebGL context" }
        val pageTexture = createWebGlTexture(context, width, height)
        val texture = WebObject.of(pageTexture)
        TestTexture(
          width = width.toUInt(),
          height = height.toUInt(),
          openGlTexture = texture.handle,
          openGlTarget = GL_TEXTURE_2D,
        ) {
          texture.close()
          deleteWebGlTexture(context, pageTexture)
        }
      }
    }

  private const val PAGE_CANVAS_SIZE = 64
  private const val GL_TEXTURE_2D = 0x0DE1u
}

/**
 * Creates a WebGL 2 context over an OffscreenCanvas of [size] x [size], with the depth and stencil
 * buffers that MapLibre draws with, or returns null when the browser refuses one. Sessions render
 * into textures of their own, and nothing presents this canvas, so it needs neither antialiasing
 * nor a preserved buffer.
 */
private fun pageWebGlContext(size: Int): JsAny? =
  js(
    """new OffscreenCanvas(size, size).getContext('webgl2', {
      depth: true, stencil: true, antialias: false, preserveDrawingBuffer: false,
    })""",
  )

/**
 * Creates an RGBA8 texture of [width] x [height] on [context] that a borrowed ring can render into.
 */
private fun createWebGlTexture(context: JsAny, width: Int, height: Int): JsAny =
  js(
    """{
      const gl = context;
      const texture = gl.createTexture();
      gl.bindTexture(gl.TEXTURE_2D, texture);
      gl.texStorage2D(gl.TEXTURE_2D, 1, gl.RGBA8, width, height);
      gl.bindTexture(gl.TEXTURE_2D, null);
      return texture;
    }""",
  )

private fun deleteWebGlTexture(context: JsAny, texture: JsAny): Unit =
  js("context.deleteTexture(texture)")

/**
 * Creates an `rgba8unorm` `GPUTexture` of [width] x [height] on [device] that a borrowed ring can
 * render into and the page can copy from.
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

private fun destroyWebGpuTexture(texture: JsAny): Unit = js("texture.destroy()")

/** Requests a WebGPU device for the page, which resolves to null when the browser has none. */
private fun requestPageWebGpuDevice(): Promise<JsAny?> =
  js("navigator.gpu.requestAdapter().then((adapter) => adapter && adapter.requestDevice())")
