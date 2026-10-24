@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.render

import kotlin.js.Promise
import kotlinx.coroutines.await
import org.maplibre.nativeffi.internal.c.unsignedWord
import org.maplibre.nativeffi.internal.memory.NativeMemory

/**
 * The browser's graphics fixtures. The page creates its WebGL 2 context or WebGPU device, which the
 * suite imports through [WebGraphics], and `bindings/kotlin/wasm/test_support.c` adds the canvases
 * that a session's core worker takes over. The browser keeps its own fixtures, because WebGL
 * contexts and WebGPU devices come from JavaScript; see tests/graphics/include/mln_test_graphics.h.
 */
internal actual object TestGraphicsLibrary {
  // Kept for the process, as the context that every caller-driven session shares.
  private var webglContext: RegisteredWebglContext? = null
  private var webgpuDevice: ImportedWebgpuObject? = null

  /**
   * Creates a WebGL 2 context on the page over an OffscreenCanvas of its own, or requests a WebGPU
   * device for the page and waits for the browser's answer, and imports it into the module. For
   * WebGPU, the graphics value that [context] later reads is the device's handle.
   */
  actual suspend fun create(backend: UInt): Long =
    when (backend) {
      TestBackend.WEBGL.graphicsBackend -> {
        val context =
          checkNotNull(pageWebglContext(PAGE_CANVAS_SIZE)) {
            "creating the page's WebGL context failed"
          }
        WebGraphics.registerWebglContext(context).also { webglContext = it }.context.toLong()
      }
      TestBackend.WEBGPU.graphicsBackend -> {
        val device =
          checkNotNull(requestPageWebgpuDevice().await<JsAny?>()) {
            "the browser provided no WebGPU device; see the console for why"
          }
        WebGraphics.importWebgpuDevice(device).also { webgpuDevice = it }.pointer.address
      }
      else ->
        error("the browser fixtures create WebGL and WebGPU contexts, not test backend $backend")
    }

  actual fun context(graphics: Long): TestGraphicsContext =
    when (TestGraphics.backend) {
      // Sessions use no WebGPU instance, and the module imports a device without one.
      TestBackend.WEBGPU -> TestGraphicsContext(webgpuDevice = graphics)
      else -> TestGraphicsContext(webglContext = graphics.toInt())
    }

  actual fun makeCurrent(graphics: Long) {
    val result = mln_kw_test_webgl_make_current(graphics.toInt())
    check(result == 0) { "making the page's WebGL context current failed: $result" }
  }

  /** An OffscreenCanvas that the transfer takes from the page's canvas registry. */
  actual fun transferredCanvas(width: Int, height: Int): TestCanvas =
    testCanvas(TestCanvasKind.OFFSCREEN, width, height)

  // Sessions render into textures of their own, so the page's canvas only carries the context.
  private const val PAGE_CANVAS_SIZE = 64
}

/** The `mln_kw_test_canvas_kind` values of test_support.c, in order. */
internal enum class TestCanvasKind {
  /** An OffscreenCanvas in the page's canvas registry, as a host's worker would hand over. */
  OFFSCREEN,
  /** A canvas element in the document, as a page would show. */
  ELEMENT,
}

/** Creates a canvas of [kind] and [width] x [height] that a session's core worker takes over. */
internal fun testCanvas(kind: TestCanvasKind, width: Int, height: Int): TestCanvas {
  val canvas = mln_kw_test_canvas_create(kind.ordinal, width, height)
  check(canvas != 0) { "cannot allocate a test canvas" }
  // The fixture's address is its selector's, a NUL-terminated string.
  val address = unsignedWord(canvas)
  val selector =
    NativeMemory.getBytes(address, NativeMemory.stringLength(address).toInt()).decodeToString()
  return TestCanvas(selector) { mln_kw_test_canvas_destroy(canvas) }
}

private fun mln_kw_test_webgl_make_current(context: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_webgl_make_current(context)")

private fun mln_kw_test_canvas_create(kind: Int, width: Int, height: Int): Int =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_canvas_create(kind, width, height)",
  )

private fun mln_kw_test_canvas_destroy(canvas: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_canvas_destroy(canvas)")

/**
 * Creates a WebGL 2 context over an OffscreenCanvas of [size] x [size], with the depth and stencil
 * buffers that MapLibre draws with, or returns null when the browser refuses one. Sessions render
 * into textures of their own, and nothing presents this canvas, so it needs neither antialiasing
 * nor a preserved buffer.
 */
internal fun pageWebglContext(size: Int): JsAny? =
  js(
    """new OffscreenCanvas(size, size).getContext('webgl2', {
      depth: true, stencil: true, antialias: false, preserveDrawingBuffer: false,
    })""",
  )

/** Requests a WebGPU device for the page, which resolves to null when the browser has none. */
internal fun requestPageWebgpuDevice(): Promise<JsAny?> =
  js("navigator.gpu.requestAdapter().then((adapter) => adapter && adapter.requestDevice())")

/**
 * A WebGPU surface over a canvas element, imported as [surface], and the [format] that the page
 * prefers. [close] releases it once its session has closed.
 */
internal class WebgpuCanvasSurface(private val surface: ImportedWebgpuObject, val format: UInt) :
  AutoCloseable {
  val pointer: NativePointer
    get() = surface.pointer

  override fun close() = surface.close()
}

/** Creates a WebGPU surface over [canvas], which must be a canvas element of the document. */
internal fun webgpuCanvasSurface(canvas: TestCanvas): WebgpuCanvasSurface {
  val context =
    checkNotNull(webgpuCanvasContext(canvas.selector)) {
      "${canvas.selector} offers no WebGPU context"
    }
  val format = WebGraphics.webgpuTextureFormat(preferredCanvasFormat())
  return WebgpuCanvasSurface(WebGraphics.importWebgpuSurface(context), format)
}

private fun webgpuCanvasContext(selector: String): JsAny? =
  js("document.querySelector(selector).getContext('webgpu')")

private fun preferredCanvasFormat(): String = js("navigator.gpu.getPreferredCanvasFormat()")
