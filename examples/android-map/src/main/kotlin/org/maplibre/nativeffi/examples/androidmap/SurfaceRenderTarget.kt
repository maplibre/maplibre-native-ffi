package org.maplibre.nativeffi.examples.androidmap

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.FrameDemand
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderDriver
import org.maplibre.nativeffi.render.RenderResult
import org.maplibre.nativeffi.render.RenderSessionAttachOptions
import org.maplibre.nativeffi.render.RenderSessionAttachment
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.OperationHandle

/** A caller-driver native surface serviced on the UI graphics thread. */
internal class SurfaceRenderTarget private constructor(private val session: RenderSessionHandle) :
  AutoCloseable {
  fun renderUpdate(): Boolean {
    session.requestFrame(FrameDemand(present = true))
    session.serviceDriverWork()
    return session.drainFrameResults().lastOrNull()?.disposition == RenderResult.RENDERED
  }

  /** Applies target changes through the native typed driver mailbox. */
  fun resize(graphics: GraphicsContext, viewport: Viewport) {
    val operation =
      when (graphics) {
        is EglGraphicsContext ->
          session.startSetOpenGLSurfaceTarget(
            OpenGLSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
          )
        is VulkanGraphicsContext -> session.startResize(viewport.extent)
        else -> error("Unsupported graphics context: ${graphics::class.java.name}")
      }
    complete(operation)
  }

  override fun close() {
    complete(session.startDetach())
    session.close()
  }

  private fun complete(operation: OperationHandle<*>) {
    operation.use {
      while (!it.poll()) session.serviceDriverWork()
      check(it.terminalStatus() == MaplibreStatus.OK) { it.diagnostic() }
    }
  }

  companion object {
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
              VulkanSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
            map.attachVulkanSurface(descriptor, callerDriver)
          }
          else -> error("Unsupported graphics context: ${graphics::class.java.name}")
        }
      return fromAttachment(attachment)
    }

    private fun fromAttachment(attachment: RenderSessionAttachment): SurfaceRenderTarget {
      val target = SurfaceRenderTarget(attachment.session)
      try {
        target.complete(attachment.operation)
        return target
      } catch (error: Throwable) {
        runCatching { attachment.session.abandon() }
        runCatching { attachment.session.close() }
        throw error
      }
    }
  }
}
