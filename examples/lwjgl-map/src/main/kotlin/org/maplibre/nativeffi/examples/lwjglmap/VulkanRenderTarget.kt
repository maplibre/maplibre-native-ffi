package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.vulkan.VK10
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.generated.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderSessionHandle

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
        context.surfaceAddress().toULong(),
      )
    return Surface(
      map.vulkanSurfaceAttach(descriptor, RenderTarget.callerDriverOptions).let { attachment ->
        RenderTarget.finishAttachment(attachment.session, attachment.ready)
      }
    )
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
      session =
        map.vulkanOwnedTextureAttach(descriptor, RenderTarget.ownedTextureOptions).let { attachment
          ->
          RenderTarget.finishAttachment(attachment.session, attachment.ready)
        }
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
      session =
        map
          .vulkanBorrowedTextureAttach(
            borrowedDescriptor(context, viewport, image),
            RenderTarget.callerDriverOptions,
          )
          .let { attachment -> RenderTarget.finishAttachment(attachment.session, attachment.ready) }
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
      viewport.framebufferWidth().toUInt(),
      viewport.framebufferHeight().toUInt(),
      descriptor(context),
      image.imageAddress().toULong(),
      image.viewAddress().toULong(),
      VK10.VK_FORMAT_R8G8B8A8_UNORM.toUInt(),
      VK10.VK_IMAGE_LAYOUT_UNDEFINED.toUInt(),
      VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.toUInt(),
    )

  private fun descriptor(context: VulkanContext): VulkanContextDescriptor =
    VulkanContextDescriptor(
      NativePointer.ofAddress(context.instanceAddress()),
      NativePointer.ofAddress(context.physicalDeviceAddress()),
      NativePointer.ofAddress(context.deviceAddress()),
      NativePointer.ofAddress(context.graphicsQueueAddress()),
      context.graphicsQueueFamilyIndex().toUInt(),
      NativePointer.ofAddress(context.getInstanceProcAddrAddress()),
      NativePointer.ofAddress(context.getDeviceProcAddrAddress()),
    )

  private class Surface(private val session: RenderSessionHandle) : RenderTarget {
    override fun resize(viewport: Viewport) {
      RenderTarget.completeDriverOperation(session, session.resize(RenderTarget.extent(viewport)))
    }

    override fun renderUpdate(): Boolean {
      val result = RenderTarget.renderFrame(session) ?: return false
      return result.disposition == RenderResult.RENDERED && !result.needsRepaint
    }

    override fun close() {
      RenderTarget.closeSession(session)
    }
  }

  private class OwnedTexture(
    private val session: RenderSessionHandle,
    private val compositor: VulkanTextureCompositor,
  ) : RenderTarget {
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      RenderTarget.completeDriverOperation(session, session.resize(RenderTarget.extent(viewport)))
    }

    override fun renderUpdate(): Boolean {
      val result = RenderTarget.renderFrame(session) ?: return false
      if (result.disposition != RenderResult.RENDERED) return false
      // An empty ring keeps the previously composited frame on screen.
      val frameHandle =
        try {
          session.acquireFrame()
        } catch (error: org.maplibre.nativeffi.error.MaplibreException) {
          if (error.status == org.maplibre.nativeffi.error.MaplibreStatus.NOT_READY) return false
          throw error
        }
      val presented =
        try {
          frameHandle.withGetProducerSync { sync ->
            check(sync.kind == org.maplibre.nativeffi.generated.GpuSyncKind.CPU_COMPLETE) {
              "Vulkan compositor requires CPU-complete producer work"
            }
            frameHandle.withGetVulkanTexture { frame ->
              check(frame.width > 0u && frame.height > 0u) {
                "MapLibre returned an empty Vulkan owned texture frame"
              }
              check(frame.layout == VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.toUInt()) {
                "MapLibre owned texture frame is not shader-readable: layout=${frame.layout}"
              }
              compositor.drawImageView(frame.imageView.toLong())
            }
          }
        } finally {
          frameHandle.release()
        }
      return presented && !result.needsRepaint
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        RenderTarget.closeSession(session)
      }
    }
  }

  private class BorrowedTexture(
    private val context: VulkanContext,
    private val session: RenderSessionHandle,
    private val compositor: VulkanTextureCompositor,
    private var image: VulkanBorrowedImage,
  ) : RenderTarget {
    private var sessionReleased = false

    /** Local to the render loop thread: allocate an image at the new size and hand it over. */
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      val replacement = VulkanBorrowedImage.create(context, viewport)
      try {
        RenderTarget.completeDriverOperation(
          session,
          session.vulkanBorrowedTextureSetTarget(borrowedDescriptor(context, viewport, replacement)),
        )
      } catch (error: RuntimeException) {
        // A failed handover leaves it unknown which image the session holds, so detach before
        // either is released.
        RenderTarget.detachSuppressed(error, session)
        sessionReleased = true
        RenderTarget.closeSuppressed(error, replacement)
        throw error
      }
      // Released only once the session has taken the replacement.
      image.close()
      image = replacement
      // A handover replaces only the graphics resource, so the map still needs the new
      // extent.
      RenderTarget.resizeMap(session.map(), viewport)
    }

    override fun renderUpdate(): Boolean {
      val result = RenderTarget.renderFrame(session) ?: return false
      if (result.disposition != RenderResult.RENDERED) return false
      return compositor.drawImageView(image.view()) && !result.needsRepaint
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        try {
          RenderTarget.closeSession(session, sessionReleased)
        } finally {
          image.close()
        }
      }
    }
  }
}
