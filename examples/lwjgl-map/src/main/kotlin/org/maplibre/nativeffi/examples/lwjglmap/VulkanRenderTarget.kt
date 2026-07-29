package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.vulkan.VK10
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor

internal object VulkanRenderTarget {
  fun attach(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
    mode: RenderTargetMode,
  ): RenderTarget =
    when (mode) {
      RenderTargetMode.NATIVE_SURFACE -> attachSurface(context, map, viewport)
      RenderTargetMode.OWNED_TEXTURE -> attachOwnedTexture(context, map, viewport)
      RenderTargetMode.BORROWED_TEXTURE -> attachBorrowedTexture(context, map, viewport)
    }

  private fun attachSurface(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    val descriptor =
      VulkanSurfaceDescriptor(
        RenderTarget.extent(viewport),
        descriptor(context),
        NativePointer.ofAddress(context.surfaceAddress()),
      )
    return Surface(map.attachVulkanSurface(descriptor))
  }

  private fun attachOwnedTexture(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    val descriptor =
      VulkanOwnedTextureDescriptor(RenderTarget.extent(viewport), descriptor(context))
    var session: RenderSessionHandle? = null
    var compositor: VulkanTextureCompositor? = null
    try {
      session = map.attachVulkanOwnedTexture(descriptor)
      compositor = VulkanTextureCompositor(context, viewport)
      return OwnedTexture(session, compositor)
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      RenderTarget.closeSuppressed(error, session)
      throw error
    }
  }

  private fun attachBorrowedTexture(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    var image: VulkanBorrowedImage? = null
    var session: RenderSessionHandle? = null
    var compositor: VulkanTextureCompositor? = null
    try {
      image = VulkanBorrowedImage.create(context, viewport)
      val descriptor =
        VulkanBorrowedTextureDescriptor(
            RenderTarget.extent(viewport),
            viewport.framebufferWidth(),
            viewport.framebufferHeight(),
            descriptor(context),
            NativePointer.ofAddress(image.imageAddress()),
            NativePointer.ofAddress(image.viewAddress()),
            VK10.VK_FORMAT_R8G8B8A8_UNORM,
            VK10.VK_IMAGE_LAYOUT_UNDEFINED,
          )
          .apply { finalLayout = VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
      session = map.attachVulkanBorrowedTexture(descriptor)
      compositor = VulkanTextureCompositor(context, viewport)
      return BorrowedTexture(context, map, session, compositor, image)
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      RenderTarget.closeSuppressed(error, session)
      RenderTarget.closeSuppressed(error, image)
      throw error
    }
  }

  private fun descriptor(context: VulkanContext): VulkanContextDescriptor =
    VulkanContextDescriptor(
      NativePointer.ofAddress(context.instanceAddress()),
      NativePointer.ofAddress(context.physicalDeviceAddress()),
      NativePointer.ofAddress(context.deviceAddress()),
      NativePointer.ofAddress(context.graphicsQueueAddress()),
      context.graphicsQueueFamilyIndex(),
      NativePointer.ofAddress(context.getInstanceProcAddrAddress()),
      NativePointer.ofAddress(context.getDeviceProcAddrAddress()),
    )

  private class Surface(private val session: RenderSessionHandle) : RenderTarget {
    override fun resize(viewport: Viewport) {
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate(): Boolean = session.renderUpdate()

    override fun close() {
      session.close()
    }
  }

  private class OwnedTexture(
    private val session: RenderSessionHandle,
    private val compositor: VulkanTextureCompositor,
  ) : RenderTarget {
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate(): Boolean {
      if (!session.renderUpdate()) {
        return false
      }
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
      return true
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        session.close()
      }
    }
  }

  private class BorrowedTexture(
    private val context: VulkanContext,
    private val map: MapHandle,
    private var session: RenderSessionHandle?,
    private var compositor: VulkanTextureCompositor?,
    private var image: VulkanBorrowedImage?,
  ) : RenderTarget {
    override fun needsReattachOnResize(): Boolean = true

    /** Local to the render loop thread: close the session, rebuild the image, attach again. */
    override fun reattach(viewport: Viewport) {
      close()
      val replacement = attachBorrowedTexture(context, map, viewport)
      check(replacement is BorrowedTexture) { "unexpected borrowed texture replacement" }
      session = replacement.session
      compositor = replacement.compositor
      image = replacement.image
      replacement.session = null
      replacement.compositor = null
      replacement.image = null
    }

    override fun resize(viewport: Viewport) {
      error("borrowed texture resize requires render target reattachment")
    }

    override fun renderUpdate(): Boolean {
      val currentSession = checkNotNull(session) { "Vulkan borrowed texture session is detached" }
      val currentCompositor =
        checkNotNull(compositor) { "Vulkan borrowed texture compositor is detached" }
      val currentImage = checkNotNull(image) { "Vulkan borrowed image is detached" }
      if (!currentSession.renderUpdate()) {
        return false
      }
      currentCompositor.drawImageView(currentImage.view())
      return true
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
}
