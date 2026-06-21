package org.maplibre.nativeffi.examples.composemap.map

import kotlin.math.max
import kotlin.math.min
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.examples.composemap.surface.MacMetalBridgeNative
import org.maplibre.nativeffi.examples.composemap.surface.MetalTextureTarget
import org.maplibre.nativeffi.examples.composemap.surface.NativeHandle
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceFrame
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderResult
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderer
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceSession
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceTarget
import org.maplibre.nativeffi.examples.composemap.surface.OpenGlTextureTarget
import org.maplibre.nativeffi.examples.composemap.surface.ProducerBackend
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.examples.composemap.surface.VulkanImageTarget
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

internal class MapLibreSurfaceRenderer : NativeSurfaceRenderer, AutoCloseable {
  override val supportedBackends: Set<ProducerBackend> =
    setOf(ProducerBackend.METAL, ProducerBackend.VULKAN, ProducerBackend.OPENGL)

  private var surfaceSession: NativeSurfaceSession? = null
  private var runtime: RuntimeHandle? = null
  private var map: MapHandle? = null
  private var renderSession: AttachedRenderSession? = null
  private var currentExtent = SurfaceExtent.Empty
  private var renderPending = true
  private var closed = false

  override fun onSurfaceAvailable(session: NativeSurfaceSession) {
    check(!closed) { "renderer is closed" }
    surfaceSession = session
    renderPending = true
    session.requestFrame()
  }

  override fun onSurfaceChanged(extent: SurfaceExtent) {
    if (closed || extent.isEmpty) {
      return
    }
    currentExtent = extent
    renderPending = true
    surfaceSession?.requestFrame()
  }

  override fun render(frame: NativeSurfaceFrame): NativeSurfaceRenderResult {
    check(!closed) { "renderer is closed" }
    if (frame.extent.isEmpty) {
      return NativeSurfaceRenderResult.Skipped
    }

    val currentMap = ensureMap(frame.extent)
    val currentRuntime = checkNotNull(runtime) { "runtime was not created" }

    currentRuntime.runOnce()
    drainEvents(currentRuntime, currentMap)

    val attached = ensureAttachedRenderSession(currentMap, frame)
    if (!renderPending) {
      return NativeSurfaceRenderResult.Skipped
    }

    return try {
      if (frame.target is MetalTextureTarget) {
        MacMetalBridgeNative.runInAutoreleasePool(Runnable { attached.session.renderUpdate() })
      } else {
        attached.session.renderUpdate()
      }
      renderPending = false
      NativeSurfaceRenderResult.Rendered
    } catch (_: InvalidStateException) {
      renderPending = true
      surfaceSession?.requestFrame()
      NativeSurfaceRenderResult.Skipped
    }
  }

  override fun onSurfaceLost() {
    surfaceSession = null
    closeRenderSession()
  }

  fun requestRender() {
    if (closed) {
      return
    }
    renderPending = true
    surfaceSession?.requestFrame()
  }

  fun moveBy(deltaX: Double, deltaY: Double) {
    map?.moveBy(deltaX, deltaY)
    requestRender()
  }

  fun scaleBy(scale: Double, anchorX: Double, anchorY: Double) {
    map?.scaleBy(scale, ScreenPoint(anchorX, anchorY))
    requestRender()
  }

  fun moveByAnimated(deltaX: Double, deltaY: Double) {
    map?.moveByAnimated(deltaX, deltaY, KEYBOARD_ANIMATION)
    requestRender()
  }

  fun scaleByAnimated(scale: Double) {
    map?.scaleByAnimated(scale, viewportCenter(), KEYBOARD_ANIMATION)
    requestRender()
  }

  fun rotateAndPitchBy(deltaX: Double, deltaY: Double) {
    val currentMap = map ?: return
    val camera = currentMap.camera
    currentMap.jumpTo(
      CameraOptions().apply {
        bearing = (camera.bearing ?: 0.0) + deltaX * DRAG_ROTATE_FACTOR
        pitch = ((camera.pitch ?: 0.0) - deltaY * DRAG_PITCH_FACTOR).clampPitch()
      }
    )
    requestRender()
  }

  fun rotateBy(deltaDegrees: Double) {
    val currentMap = map ?: return
    currentMap.easeTo(
      CameraOptions().apply { bearing = (currentMap.camera.bearing ?: 0.0) + deltaDegrees },
      KEYBOARD_ANIMATION,
    )
    requestRender()
  }

  fun pitchBy(deltaDegrees: Double) {
    val currentMap = map ?: return
    currentMap.easeTo(
      CameraOptions().apply {
        pitch = ((currentMap.camera.pitch ?: 0.0) + deltaDegrees).clampPitch()
      },
      KEYBOARD_ANIMATION,
    )
    requestRender()
  }

  fun resetPitchAndBearing() {
    map?.easeTo(
      CameraOptions().apply {
        bearing = 0.0
        pitch = 0.0
      },
      RESET_ANIMATION,
    )
    requestRender()
  }

  fun cancelTransitions() {
    map?.cancelTransitions()
  }

  override fun close() {
    if (closed) {
      return
    }
    closed = true
    surfaceSession = null
    closeRenderSession()
    try {
      map?.close()
    } finally {
      map = null
      runtime?.close()
      runtime = null
    }
  }

  private fun ensureMap(extent: SurfaceExtent): MapHandle {
    map?.let {
      return it
    }

    val createdRuntime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
    val createdMap =
      MapHandle.create(
        createdRuntime,
        MapOptions().apply {
          width = extent.width
          height = extent.height
          scaleFactor = extent.scaleFactor
          mapMode = MapMode.CONTINUOUS
        },
      )
    try {
      createdMap.setStyleUrl(STYLE_URL)
      createdMap.jumpTo(
        CameraOptions().apply {
          center = LatLng(37.7749, -122.4194)
          zoom = 13.0
          bearing = 12.0
          pitch = 30.0
        }
      )
      runtime = createdRuntime
      map = createdMap
      return createdMap
    } catch (error: RuntimeException) {
      createdMap.close()
      createdRuntime.close()
      throw error
    }
  }

  private fun ensureAttachedRenderSession(
    map: MapHandle,
    frame: NativeSurfaceFrame,
  ): AttachedRenderSession {
    val descriptor = borrowedDescriptor(frame.target, frame.extent)
    renderSession?.let { existing ->
      if (existing.key == descriptor.key) {
        return existing
      }
    }

    closeRenderSession()
    val attached = AttachedRenderSession(descriptor.key, descriptor.attach(map))
    renderSession = attached
    renderPending = true
    return attached
  }

  private fun borrowedDescriptor(
    target: NativeSurfaceTarget,
    extent: SurfaceExtent,
  ): BorrowedDescriptor =
    when (target) {
      is MetalTextureTarget -> metalDescriptor(target, extent)
      is VulkanImageTarget -> vulkanDescriptor(target, extent)
      is OpenGlTextureTarget -> openGlDescriptor(target, extent)
    }

  private fun metalDescriptor(
    target: MetalTextureTarget,
    extent: SurfaceExtent,
  ): BorrowedDescriptor =
    BorrowedDescriptor(
      key = targetKey(target.backend, target.generation, extent),
      attach = { map ->
        map.attachMetalBorrowedTexture(
          MetalBorrowedTextureDescriptor(extent.toRenderTargetExtent(), target.texture.toPointer())
        )
      },
    )

  private fun vulkanDescriptor(
    target: VulkanImageTarget,
    extent: SurfaceExtent,
  ): BorrowedDescriptor =
    BorrowedDescriptor(
      key = targetKey(target.backend, target.generation, extent),
      attach = {
        // TODO(surface): VulkanImageTarget must expose the producer Vulkan context handles
        // required
        // for VulkanContextDescriptor before MapLibre can attach this borrowed image. Keep
        // external-memory handles, queue ownership transfers, and synchronization in surface.
        throw UnsupportedOperationException(
          "Vulkan Compose surface targets do not yet expose a MapLibre Vulkan context descriptor"
        )
      },
    )

  private fun openGlDescriptor(
    target: OpenGlTextureTarget,
    extent: SurfaceExtent,
  ): BorrowedDescriptor =
    BorrowedDescriptor(
      key = targetKey(target.backend, target.generation, extent),
      attach = {
        // TODO(surface): OpenGlTextureTarget must expose a producer EGL/WGL context descriptor
        // compatible with MapLibre's OpenGL borrowed texture API. Keep Skiko context discovery,
        // external-memory import details, and synchronization in surface.
        throw UnsupportedOperationException(
          "OpenGL Compose surface targets do not yet expose a MapLibre OpenGL context descriptor"
        )
      },
    )

  private fun drainEvents(runtime: RuntimeHandle, map: MapHandle) {
    while (true) {
      val event = runtime.pollEvent() ?: return
      if (event.mapSource != map) {
        continue
      }
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

  private fun closeRenderSession() {
    val closing = renderSession
    renderSession = null
    closing?.session?.close()
  }

  private fun viewportCenter(): ScreenPoint =
    ScreenPoint(currentExtent.width / 2.0, currentExtent.height / 2.0)

  private fun targetKey(
    backend: ProducerBackend,
    generation: Long,
    extent: SurfaceExtent,
  ): TargetKey =
    TargetKey(
      backend = backend,
      generation = generation,
      width = extent.width,
      height = extent.height,
      scaleFactor = extent.scaleFactor,
      physicalWidth = extent.physicalWidth,
      physicalHeight = extent.physicalHeight,
    )

  private data class AttachedRenderSession(val key: TargetKey, val session: RenderSessionHandle)

  private data class TargetKey(
    val backend: ProducerBackend,
    val generation: Long,
    val width: Int,
    val height: Int,
    val scaleFactor: Double,
    val physicalWidth: Int,
    val physicalHeight: Int,
  )

  private class BorrowedDescriptor(
    val key: TargetKey,
    val attach: (MapHandle) -> RenderSessionHandle,
  )

  private companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"
    private const val DRAG_ROTATE_FACTOR = 0.5
    private const val DRAG_PITCH_FACTOR = 0.5
    private val KEYBOARD_ANIMATION = AnimationOptions().apply { durationMs = 160.0 }
    private val RESET_ANIMATION = AnimationOptions().apply { durationMs = 160.0 }
  }
}

private fun SurfaceExtent.toRenderTargetExtent(): RenderTargetExtent =
  RenderTargetExtent(width, height, scaleFactor)

private fun Double.clampPitch(): Double = max(0.0, min(60.0, this))

private fun NativeHandle.toPointer(): NativePointer = NativePointer.ofAddress(address)
