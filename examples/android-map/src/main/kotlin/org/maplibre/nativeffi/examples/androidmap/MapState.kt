package org.maplibre.nativeffi.examples.androidmap

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

internal class MapState
private constructor(
  private val runtime: RuntimeHandle,
  val map: MapHandle,
  private var renderTarget: SurfaceRenderTarget?,
) : AutoCloseable {
  fun attachOrResize(graphics: GraphicsContext, viewport: Viewport) {
    if (viewport.isEmpty) return
    val target = renderTarget
    if (target == null) {
      renderTarget = SurfaceRenderTarget.attach(map, graphics, viewport)
    } else {
      target.resize(viewport)
    }
    map.requestRepaint()
  }

  fun detachRenderTarget() {
    renderTarget?.close()
    renderTarget = null
  }

  fun runOnce() {
    runtime.runOnce()
  }

  fun drainEvents(): Boolean {
    var renderPending = false
    while (true) {
      val event = runtime.pollEvent() ?: return renderPending
      if (event.mapSource != map) continue
      if (event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE) {
        renderPending = true
      } else if (
        event.type == RuntimeEventType.MAP_RENDER_FRAME_FINISHED &&
          (event.payload as? RuntimeEventPayload.RenderFrame)?.needsRepaint == true
      ) {
        renderPending = true
      }
    }
  }

  fun renderUpdate() {
    renderTarget?.renderUpdate()
  }

  override fun close() {
    try {
      detachRenderTarget()
    } finally {
      try {
        map.close()
      } finally {
        runtime.close()
      }
    }
  }

  companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"

    fun create(graphics: GraphicsContext, viewport: Viewport): MapState {
      val runtime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = viewport.logicalWidth
            height = viewport.logicalHeight
            scaleFactor = viewport.scaleFactor
            mapMode = MapMode.CONTINUOUS
          },
        )
      var target: SurfaceRenderTarget? = null
      try {
        map.setStyleUrl(STYLE_URL)
        map.jumpTo(
          CameraOptions().apply {
            center = LatLng(37.7749, -122.4194)
            zoom = 13.0
            bearing = 12.0
            pitch = 30.0
          }
        )
        map.requestRepaint()
        target = SurfaceRenderTarget.attach(map, graphics, viewport)
        return MapState(runtime, map, target)
      } catch (error: RuntimeException) {
        target?.close()
        map.close()
        runtime.close()
        throw error
      }
    }
  }
}

private class SurfaceRenderTarget(private val session: RenderSessionHandle) : AutoCloseable {
  fun resize(viewport: Viewport) {
    session.resize(viewport.logicalWidth, viewport.logicalHeight, viewport.scaleFactor)
  }

  fun renderUpdate() {
    session.renderUpdate()
  }

  override fun close() {
    session.close()
  }

  companion object {
    fun attach(map: MapHandle, graphics: GraphicsContext, viewport: Viewport): SurfaceRenderTarget =
      when (graphics) {
        is EglGraphicsContext -> {
          val descriptor =
            OpenGLSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
          SurfaceRenderTarget(map.attachOpenGLSurface(descriptor))
        }
        is VulkanGraphicsContext -> {
          val descriptor =
            VulkanSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
          SurfaceRenderTarget(map.attachVulkanSurface(descriptor))
        }
        else -> error("Unsupported graphics context: ${graphics::class.java.name}")
      }
  }
}
