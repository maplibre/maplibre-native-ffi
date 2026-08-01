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
      session = map.attachVulkanBorrowedTexture(borrowedDescriptor(context, viewport, image))
      compositor = VulkanTextureCompositor(context, viewport)
      return BorrowedTexture(context, session, compositor, image)
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      RenderTarget.closeSuppressed(error, session)
      RenderTarget.closeSuppressed(error, image)
      throw error
    }
  }

  private fun borrowedDescriptor(
    context: VulkanContext,
    viewport: Viewport,
    image: VulkanBorrowedImage,
  ): VulkanBorrowedTextureDescriptor =
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
    private val session: RenderSessionHandle,
    private val compositor: VulkanTextureCompositor,
    private var image: VulkanBorrowedImage,
  ) : RenderTarget {
    /** Local to the render loop thread: allocate an image at the new size and hand it over. */
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      val replacement = VulkanBorrowedImage.create(context, viewport)
      try {
        session.setVulkanBorrowedTextureTarget(borrowedDescriptor(context, viewport, replacement))
      } catch (error: RuntimeException) {
        RenderTarget.closeSuppressed(error, replacement)
        throw error
      }
      // Only once the session has taken the replacement: destroying the outgoing image before that
      // call returns would pull it out from under the session.
      image.close()
      image = replacement
    }

    override fun renderUpdate(): Boolean {
      if (!session.renderUpdate()) {
        return false
      }
      compositor.drawImageView(image.view())
      return true
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        try {
          session.close()
        } finally {
          image.close()
        }
      }
    }
  }
}
