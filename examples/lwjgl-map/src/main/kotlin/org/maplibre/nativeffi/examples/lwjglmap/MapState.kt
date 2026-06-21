package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.vulkan.VK10
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

internal class MapState
private constructor(
  private val runtime: RuntimeHandle,
  val map: MapHandle,
  private val renderTarget: RenderTarget,
) : AutoCloseable {
  private var renderPending = true

  fun resize(viewport: Viewport) {
    if (renderTarget.needsReattachOnResize()) {
      renderTarget.reattach(viewport)
    } else {
      renderTarget.resize(viewport)
    }
    renderPending = true
  }

  fun requestRender() {
    renderPending = true
  }

  fun step(): Boolean {
    runtime.runOnce()
    drainEvents()
    if (!renderPending) {
      return false
    }
    try {
      if (renderTarget.needsMetalAutoreleasePool()) {
        MacObjectiveC.autoreleasePool().use { renderTarget.renderUpdate() }
      } else {
        renderTarget.renderUpdate()
      }
      renderPending = false
    } catch (_: InvalidStateException) {
      renderPending = true
    }
    return true
  }

  private fun drainEvents() {
    while (true) {
      val event = runtime.pollEvent() ?: return
      if (event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE && event.mapSource == map) {
        renderPending = true
      } else if (
        event.type == RuntimeEventType.MAP_RENDER_FRAME_FINISHED &&
          event.mapSource == map &&
          (event.payload as? RuntimeEventPayload.RenderFrame)?.needsRepaint == true
      ) {
        renderPending = true
      }
    }
  }

  override fun close() {
    try {
      renderTarget.close()
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

    fun create(graphics: GraphicsContext, viewport: Viewport, mode: RenderTargetMode): MapState {
      val runtimeOptions = RuntimeOptions().apply { cachePath = ":memory:" }
      val runtime = RuntimeHandle.create(runtimeOptions)
      val mapOptions =
        MapOptions().apply {
          width = viewport.width()
          height = viewport.height()
          scaleFactor = viewport.scaleFactor()
          mapMode = MapMode.CONTINUOUS
        }
      val map = MapHandle.create(runtime, mapOptions)
      var target: RenderTarget? = null
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
        target = attachRenderTarget(graphics, map, viewport, mode)
        return MapState(runtime, map, target)
      } catch (error: RuntimeException) {
        target?.close()
        map.close()
        runtime.close()
        throw error
      }
    }

    private fun attachRenderTarget(
      graphics: GraphicsContext,
      map: MapHandle,
      viewport: Viewport,
      mode: RenderTargetMode,
    ): RenderTarget =
      when (graphics) {
        is MetalContext -> attachMetalRenderTarget(graphics, map, viewport, mode)
        is VulkanContext -> attachVulkanRenderTarget(graphics, map, viewport, mode)
        is OpenGLContext -> OpenGLRenderTarget.attach(graphics, map, viewport, mode)
        else -> error("Unsupported graphics context: ${graphics.backend()}")
      }

    private fun attachVulkanRenderTarget(
      vulkan: VulkanContext,
      map: MapHandle,
      viewport: Viewport,
      mode: RenderTargetMode,
    ): RenderTarget =
      when (mode) {
        RenderTargetMode.NATIVE_SURFACE -> {
          val descriptor =
            VulkanSurfaceDescriptor(
              extent(viewport),
              vulkanContextDescriptor(vulkan),
              NativePointer.ofAddress(vulkan.surfaceAddress()),
            )
          VulkanSurfaceRenderTarget(map.attachVulkanSurface(descriptor))
        }
        RenderTargetMode.OWNED_TEXTURE -> attachOwnedTextureRenderTarget(vulkan, map, viewport)
        RenderTargetMode.BORROWED_TEXTURE ->
          attachBorrowedTextureRenderTarget(vulkan, map, viewport)
      }

    private fun attachMetalRenderTarget(
      metal: MetalContext,
      map: MapHandle,
      viewport: Viewport,
      mode: RenderTargetMode,
    ): RenderTarget =
      when (mode) {
        RenderTargetMode.NATIVE_SURFACE -> {
          val descriptor =
            MetalSurfaceDescriptor(
              extent(viewport),
              metalContextDescriptor(metal),
              NativePointer.ofAddress(metal.layerAddress()),
            )
          MetalSurfaceRenderTarget(map.attachMetalSurface(descriptor))
        }
        RenderTargetMode.OWNED_TEXTURE -> attachMetalOwnedTextureRenderTarget(metal, map, viewport)
        RenderTargetMode.BORROWED_TEXTURE ->
          attachMetalBorrowedTextureRenderTarget(metal, map, viewport)
      }

    private fun attachOwnedTextureRenderTarget(
      vulkan: VulkanContext,
      map: MapHandle,
      viewport: Viewport,
    ): RenderTarget {
      val descriptor =
        VulkanOwnedTextureDescriptor(extent(viewport), vulkanContextDescriptor(vulkan))
      var session: RenderSessionHandle? = null
      var compositor: VulkanTextureCompositor? = null
      try {
        session = map.attachVulkanOwnedTexture(descriptor)
        compositor = VulkanTextureCompositor(vulkan, viewport)
        return VulkanOwnedTextureRenderTarget(session, compositor)
      } catch (error: RuntimeException) {
        closeSuppressed(error, compositor)
        closeSuppressed(error, session)
        throw error
      }
    }

    private fun attachBorrowedTextureRenderTarget(
      vulkan: VulkanContext,
      map: MapHandle,
      viewport: Viewport,
    ): RenderTarget {
      var image: VulkanBorrowedImage? = null
      var session: RenderSessionHandle? = null
      var compositor: VulkanTextureCompositor? = null
      try {
        image = VulkanBorrowedImage.create(vulkan, viewport)
        val descriptor =
          VulkanBorrowedTextureDescriptor(
              extent(viewport),
              vulkanContextDescriptor(vulkan),
              NativePointer.ofAddress(image.imageAddress()),
              NativePointer.ofAddress(image.viewAddress()),
              VK10.VK_FORMAT_R8G8B8A8_UNORM,
              VK10.VK_IMAGE_LAYOUT_UNDEFINED,
            )
            .apply { finalLayout = VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
        session = map.attachVulkanBorrowedTexture(descriptor)
        compositor = VulkanTextureCompositor(vulkan, viewport)
        return VulkanBorrowedTextureRenderTarget(vulkan, map, session, compositor, image)
      } catch (error: RuntimeException) {
        closeSuppressed(error, compositor)
        closeSuppressed(error, session)
        closeSuppressed(error, image)
        throw error
      }
    }

    private fun vulkanContextDescriptor(vulkan: VulkanContext): VulkanContextDescriptor =
      VulkanContextDescriptor(
        NativePointer.ofAddress(vulkan.instanceAddress()),
        NativePointer.ofAddress(vulkan.physicalDeviceAddress()),
        NativePointer.ofAddress(vulkan.deviceAddress()),
        NativePointer.ofAddress(vulkan.graphicsQueueAddress()),
        vulkan.graphicsQueueFamilyIndex(),
        NativePointer.ofAddress(vulkan.getInstanceProcAddrAddress()),
        NativePointer.ofAddress(vulkan.getDeviceProcAddrAddress()),
      )

    private fun attachMetalOwnedTextureRenderTarget(
      metal: MetalContext,
      map: MapHandle,
      viewport: Viewport,
    ): RenderTarget {
      val descriptor = MetalOwnedTextureDescriptor(extent(viewport), metalContextDescriptor(metal))
      var session: RenderSessionHandle? = null
      var compositor: MetalTextureCompositor? = null
      try {
        session = map.attachMetalOwnedTexture(descriptor)
        compositor = MetalTextureCompositor(metal)
        return MetalOwnedTextureRenderTarget(session, compositor)
      } catch (error: RuntimeException) {
        closeSuppressed(error, compositor)
        closeSuppressed(error, session)
        throw error
      }
    }

    private fun attachMetalBorrowedTextureRenderTarget(
      metal: MetalContext,
      map: MapHandle,
      viewport: Viewport,
    ): RenderTarget {
      var texture: MetalBorrowedTexture? = null
      var session: RenderSessionHandle? = null
      var compositor: MetalTextureCompositor? = null
      try {
        texture = MetalBorrowedTexture(metal, viewport)
        val descriptor =
          MetalBorrowedTextureDescriptor(
            extent(viewport),
            NativePointer.ofAddress(texture.texture()),
          )
        session = map.attachMetalBorrowedTexture(descriptor)
        compositor = MetalTextureCompositor(metal)
        return MetalBorrowedTextureRenderTarget(metal, map, session, compositor, texture)
      } catch (error: RuntimeException) {
        closeSuppressed(error, compositor)
        closeSuppressed(error, session)
        closeSuppressed(error, texture)
        throw error
      }
    }

    private fun metalContextDescriptor(metal: MetalContext): MetalContextDescriptor =
      MetalContextDescriptor(NativePointer.ofAddress(metal.deviceAddress()))

    fun extent(viewport: Viewport): RenderTargetExtent =
      RenderTargetExtent(viewport.width(), viewport.height(), viewport.scaleFactor())

    fun closeSuppressed(error: RuntimeException, closeable: AutoCloseable?) {
      if (closeable == null) {
        return
      }
      try {
        closeable.close()
      } catch (cleanupError: Exception) {
        error.addSuppressed(cleanupError)
      }
    }
  }

  private class VulkanSurfaceRenderTarget(private val session: RenderSessionHandle) : RenderTarget {
    override fun resize(viewport: Viewport) {
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate() {
      session.renderUpdate()
    }

    override fun close() {
      session.close()
    }
  }

  private class MetalSurfaceRenderTarget(private val session: RenderSessionHandle) : RenderTarget {
    override fun needsMetalAutoreleasePool(): Boolean = true

    override fun resize(viewport: Viewport) {
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate() {
      session.renderUpdate()
    }

    override fun close() {
      session.close()
    }
  }

  private class VulkanOwnedTextureRenderTarget(
    private val session: RenderSessionHandle,
    private val compositor: VulkanTextureCompositor,
  ) : RenderTarget {
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate() {
      session.renderUpdate()
      session.acquireVulkanOwnedTextureFrame().use { frameHandle ->
        val frame = frameHandle.frame()
        check(frame.width() > 0 && frame.height() > 0) {
          "MapLibre returned an empty Vulkan owned texture frame"
        }
        check(frame.layout() == VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
          "MapLibre owned texture frame is not shader-readable: layout=${frame.layout()}"
        }
        compositor.drawImageView(frame.imageView().address)
      }
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        session.close()
      }
    }
  }

  private class MetalOwnedTextureRenderTarget(
    private val session: RenderSessionHandle,
    private val compositor: MetalTextureCompositor,
  ) : RenderTarget {
    override fun needsMetalAutoreleasePool(): Boolean = true

    override fun resize(viewport: Viewport) {
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate() {
      session.renderUpdate()
      session.acquireMetalOwnedTextureFrame().use { frameHandle ->
        val frame = frameHandle.frame()
        check(frame.width() != 0 && frame.height() != 0 && !frame.texture().isNull) {
          "owned Metal frame has an empty extent or null texture"
        }
        compositor.drawTexture(frame.texture().address)
      }
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        session.close()
      }
    }
  }

  private class VulkanBorrowedTextureRenderTarget(
    private val vulkan: VulkanContext,
    private val map: MapHandle,
    private var session: RenderSessionHandle?,
    private var compositor: VulkanTextureCompositor?,
    private var image: VulkanBorrowedImage?,
  ) : RenderTarget {
    override fun needsReattachOnResize(): Boolean = true

    override fun reattach(viewport: Viewport) {
      close()
      val replacement = attachBorrowedTextureRenderTarget(vulkan, map, viewport)
      if (replacement is VulkanBorrowedTextureRenderTarget) {
        session = replacement.session
        compositor = replacement.compositor
        image = replacement.image
        replacement.session = null
        replacement.compositor = null
        replacement.image = null
      } else {
        error("unexpected borrowed texture replacement")
      }
    }

    override fun resize(viewport: Viewport) {
      error("borrowed texture resize requires render target reattachment")
    }

    override fun renderUpdate() {
      val currentSession = checkNotNull(session) { "Vulkan borrowed texture session is detached" }
      val currentCompositor =
        checkNotNull(compositor) { "Vulkan borrowed texture compositor is detached" }
      val currentImage = checkNotNull(image) { "Vulkan borrowed image is detached" }
      currentSession.renderUpdate()
      currentCompositor.drawImageView(currentImage.view())
    }

    override fun close() {
      val closingCompositor = compositor
      val closingSession = session
      val closingImage = image
      compositor = null
      session = null
      image = null
      try {
        closingCompositor?.close()
      } finally {
        try {
          closingSession?.close()
        } finally {
          closingImage?.close()
        }
      }
    }
  }

  private class MetalBorrowedTextureRenderTarget(
    private val metal: MetalContext,
    private val map: MapHandle,
    private var session: RenderSessionHandle?,
    private var compositor: MetalTextureCompositor?,
    private var texture: MetalBorrowedTexture?,
  ) : RenderTarget {
    override fun needsMetalAutoreleasePool(): Boolean = true

    override fun needsReattachOnResize(): Boolean = true

    override fun reattach(viewport: Viewport) {
      close()
      val replacement = attachMetalBorrowedTextureRenderTarget(metal, map, viewport)
      if (replacement is MetalBorrowedTextureRenderTarget) {
        session = replacement.session
        compositor = replacement.compositor
        texture = replacement.texture
        replacement.session = null
        replacement.compositor = null
        replacement.texture = null
      } else {
        error("unexpected borrowed texture replacement")
      }
    }

    override fun resize(viewport: Viewport) {
      error("borrowed texture resize requires render target reattachment")
    }

    override fun renderUpdate() {
      val currentSession = checkNotNull(session) { "Metal borrowed texture session is detached" }
      val currentCompositor =
        checkNotNull(compositor) { "Metal borrowed texture compositor is detached" }
      val currentTexture = checkNotNull(texture) { "Metal borrowed texture is detached" }
      currentSession.renderUpdate()
      currentCompositor.drawTexture(currentTexture.texture())
    }

    override fun close() {
      val closingCompositor = compositor
      val closingSession = session
      val closingTexture = texture
      compositor = null
      session = null
      texture = null
      try {
        closingCompositor?.close()
      } finally {
        try {
          closingSession?.close()
        } finally {
          closingTexture?.close()
        }
      }
    }
  }
}
