package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapFixture
import org.maplibre.nativeffi.NativePointer
import org.maplibre.nativeffi.OpenGlContextDescriptor
import org.maplibre.nativeffi.OpenGlContextDescriptorData
import org.maplibre.nativeffi.OpenGlContextOwnership
import org.maplibre.nativeffi.OpenGlSurfaceDescriptor
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderResult
import org.maplibre.nativeffi.RenderSessionCapabilityFlag
import org.maplibre.nativeffi.RenderSessionState
import org.maplibre.nativeffi.WebGlContextDescriptor
import org.maplibre.nativeffi.WebGlContextKind
import org.maplibre.nativeffi.WebGpuContextDescriptor
import org.maplibre.nativeffi.WebGpuSurfaceDescriptor
import org.maplibre.nativeffi.attachWebGpuSurface
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.withMap

/**
 * A surface over a canvas element of the page, on the build's backend: a WebGL surface whose core
 * worker takes the canvas over, or a WebGPU surface over the canvas that the page drives.
 *
 * The test checks the session's account of the frame, not the canvas. A transferred WebGL canvas
 * belongs to its core worker. Headless Chromium loses every WebGPU device of the page once a WebGPU
 * canvas renders, even one that the page drives through the browser's own WebGPU API, so the WebGPU
 * run gives this test a page of its own.
 */
class CanvasSurfaceTest {
  @Test
  fun aCanvasSurfaceRendersAFrame() = runSuspendTest {
    withMap(options = smallMapOptions(width = SIZE, height = SIZE)) {
      testCanvas(TestCanvasKind.ELEMENT, SIZE, SIZE).use { canvas ->
        withCanvasSurface(canvas) {
          assertTrue(RenderSessionCapabilityFlag.PRESENTATION in session.getCapabilities().flags)
          setStyle(RED_BACKGROUND_STYLE)
          // The browser composites a WebGL or WebGPU canvas itself, so presenting takes no call.
          assertEquals(RenderResult.RENDERED, renderFrame().disposition)

          complete(session.detach())
          assertEquals(RenderSessionState.DETACHED, session.getSnapshot().state)
        }
      }
    }
  }

  private suspend fun <T> MapFixture.withCanvasSurface(
    canvas: TestCanvas,
    block: suspend RenderSessionFixture.() -> T,
  ): T {
    val extent = LogicalExtent(SIZE.toUInt(), SIZE.toUInt(), 1.0)
    return when (TestGraphics.backend) {
      TestBackend.WEBGL -> {
        val descriptor =
          OpenGlSurfaceDescriptor(
            extent,
            OpenGlContextDescriptor(
              OpenGlContextOwnership.DEDICATED,
              OpenGlContextDescriptorData.WebGl(
                WebGlContextDescriptor(
                  WebGlContextKind.TRANSFERRED_CANVAS,
                  canvasSelector = canvas.selector,
                ),
              ),
            ),
            NativePointer.NULL_POINTER,
          )
        withRenderSession(
          RenderDriverKind.CORE_WORKER,
          attach = { options -> map.attachOpenGlSurface(descriptor, options) },
          block = block,
        )
      }
      TestBackend.WEBGPU -> {
        val context = TestGraphics.context()
        webGpuCanvasSurface(canvas).use { surface ->
          val descriptor =
            WebGpuSurfaceDescriptor(
              extent,
              WebGpuContextDescriptor(
                NativePointer.NULL_POINTER,
                NativePointer.ofAddress(context.webGpuDevice),
                NativePointer.NULL_POINTER,
              ),
              surface.pointer,
              surface.format,
            )
          withRenderSession(
            RenderDriverKind.CALLER_GRAPHICS_THREAD,
            attach = { options -> map.attachWebGpuSurface(descriptor, options) },
            block = block,
          )
        }
      }
      else -> error("a browser build renders with WebGL or WebGPU, not ${TestGraphics.backend}")
    }
  }

  private companion object {
    const val SIZE = 64
  }
}
