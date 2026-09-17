package org.maplibre.nativeffi.examples.androidmap

import android.util.Log
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapSize
import org.maplibre.nativeffi.render.FrameDemand
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderDriver
import org.maplibre.nativeffi.render.RenderFrameResult
import org.maplibre.nativeffi.render.RenderSessionAttachOptions
import org.maplibre.nativeffi.render.RenderSessionAttachment
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor

/** A caller-driver native surface serviced on the UI graphics thread. */
internal class SurfaceRenderTarget private constructor(private val session: RenderSessionHandle) :
  AutoCloseable {
  /**
   * Submits one Choreographer-paced demand and reports the frame the driver produced for it, or
   * null when it produced none.
   */
  fun renderUpdate(): RenderFrameResult? {
    session.requestFrame(FrameDemand(present = true))
    session.serviceDriverWork()
    return session.drainFrameResults().lastOrNull()
  }

  /**
   * Applies target changes through the native typed driver mailbox.
   *
   * A session resize carries the map's extent itself. An EGL surface handover replaces only the
   * graphics resource, so that path submits the map resize alongside it.
   */
  fun resize(map: MapHandle, graphics: GraphicsContext, viewport: Viewport) {
    when (graphics) {
      is EglGraphicsContext -> {
        complete(
          session.setOpenGLSurfaceTarget(
            OpenGLSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
          )
        )
        map.resize(MapSize(viewport.logicalWidth, viewport.logicalHeight, viewport.scaleFactor))
      }
      is VulkanGraphicsContext -> complete(session.resize(viewport.extent))
      else -> error("Unsupported graphics context: ${graphics::class.java.name}")
    }
  }

  /**
   * Releases the session. Detach services driver work, which needs the graphics context current, so
   * a platform callback that arrives after the surface is gone falls back to abandoning it.
   */
  override fun close() {
    try {
      complete(session.detach())
    } catch (error: RuntimeException) {
      Log.w(TAG, "detaching the render session failed; abandoning it instead", error)
      runCatching { session.abandon() }
    }
    session.close()
  }

  private fun complete(completed: Deferred<Unit>) {
    while (!completed.isCompleted) session.serviceDriverWork()
    runBlocking { completed.await() }
  }

  companion object {
    private const val TAG = "MapLibreAndroidMap"

    private val callerDriver =
      RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD)

    fun attach(map: MapHandle, graphics: GraphicsContext, viewport: Viewport): SurfaceRenderTarget {
      val attachment =
        when (graphics) {
          is EglGraphicsContext -> {
            val descriptor =
              OpenGLSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
            map.attachOpenGLSurface(descriptor, callerDriver)
          }
          is VulkanGraphicsContext -> {
            val descriptor =
              VulkanSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfaceHandle)
            map.attachVulkanSurface(descriptor, callerDriver)
          }
          else -> error("Unsupported graphics context: ${graphics::class.java.name}")
        }
      return fromAttachment(attachment)
    }

    private fun fromAttachment(attachment: RenderSessionAttachment): SurfaceRenderTarget {
      val target = SurfaceRenderTarget(attachment.session)
      try {
        target.complete(attachment.completed)
        return target
      } catch (error: Throwable) {
        runCatching { attachment.session.abandon() }
        runCatching { attachment.session.close() }
        throw error
      }
    }
  }
}
