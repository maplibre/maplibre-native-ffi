@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import org.maplibre.nativeffi.AcquiredFrameHandle
import org.maplibre.nativeffi.GpuDeviceHandle
import org.maplibre.nativeffi.QueueLock
import org.maplibre.nativeffi.WebGpuContextDescriptor
import org.maplibre.nativeffi.WebObject
import org.maplibre.nativeffi.withWebGpuTexture
import org.maplibre.nativeffi.wrapWebGpu

// The page is the only thread that runs Kotlin, so it drives graphics too.
internal actual val testGraphicsThread: CoroutineDispatcher = Dispatchers.Default

internal actual fun TestGraphics.wrapBackendDevice(
  context: TestGraphicsContext,
  queueLock: QueueLock,
): GpuDeviceHandle =
  when (backend) {
    TestBackend.WEBGPU -> GpuDeviceHandle.wrapWebGpu(WebGpuContextDescriptor(context.webGpuDevice))
    else -> error("this target declares no device wrap for $backend")
  }

internal actual fun TestGraphics.backendTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt> =
  when (backend) {
    TestBackend.WEBGPU -> frame.withWebGpuTexture { it.width to it.height }
    else -> error("this target declares no texture view for $backend")
  }

/**
 * An `OffscreenCanvas` that the module presents to, and a 2D canvas of the same size that [capture]
 * copies it into to read its center pixel.
 */
internal actual fun createTestCanvas(width: Int, height: Int): TestCanvas? {
  val canvas = offscreenCanvas(width, height)
  val scratch = readableContext(width, height)
  val lent = WebObject.of(canvas)
  return object : TestCanvas {
    override val handle: UInt
      get() = lent.handle

    override var centerPixel: List<Int>? = null

    override fun capture() {
      val pixel = readCenterPixel(scratch, canvas, width / 2, height / 2)
      centerPixel = List(4) { checkNotNull(pixel[it]).toInt() }
    }

    override fun close() = lent.close()
  }
}

private fun offscreenCanvas(width: Int, height: Int): JsAny =
  js("new OffscreenCanvas(width, height)")

private fun readableContext(width: Int, height: Int): JsAny =
  js("new OffscreenCanvas(width, height).getContext('2d', { willReadFrequently: true })")

private fun readCenterPixel(scratch: JsAny, canvas: JsAny, x: Int, y: Int): JsArray<JsNumber> =
  js(
    """{
      scratch.drawImage(canvas, 0, 0);
      return Array.from(scratch.getImageData(x, y, 1, 1).data);
    }""",
  )
