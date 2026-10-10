package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import org.lwjgl.vulkan.VK10
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.GpuSyncKind
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.generated.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.render.NativePointer

internal object VulkanRenderTarget {
  fun attach(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
    mode: RenderTargetMode,
    driver: SessionDriver,
  ): RenderTarget {
    val session = descriptor(context, context.graphicsQueueAddress())
    return when (mode) {
      RenderTargetMode.NATIVE_SURFACE ->
        NativeSurfaceTarget(
          AttachedSession.attach(driver) { options ->
            map.attachVulkanSurface(
              VulkanSurfaceDescriptor(
                RenderTarget.extent(viewport),
                session,
                context.surfaceAddress().toULong(),
              ),
              options,
            )
          }
        )
      RenderTargetMode.OWNED_TEXTURE -> attachOwnedTexture(context, map, viewport, driver, session)
      RenderTargetMode.BORROWED_TEXTURE ->
        attachBorrowedTexture(context, map, viewport, driver, session)
    }
  }

  private fun attachOwnedTexture(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
    driver: SessionDriver,
    session: VulkanContextDescriptor,
  ): RenderTarget {
    val compositor = VulkanTextureCompositor(context, viewport)
    try {
      val attached =
        AttachedSession.attach(driver, RenderTarget.OWNED_TEXTURE_RING_DEPTH) { options ->
          map.attachVulkanOwnedTexture(
            VulkanOwnedTextureDescriptor(RenderTarget.extent(viewport), session),
            options,
          )
        }
      return OwnedTexture(attached, compositor)
    } catch (error: RuntimeException) {
      compositor.close()
      throw error
    }
  }

  private fun attachBorrowedTexture(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
    driver: SessionDriver,
    session: VulkanContextDescriptor,
  ): RenderTarget {
    val image = VulkanBorrowedImage.create(context, viewport)
    var compositor: VulkanTextureCompositor? = null
    try {
      compositor = VulkanTextureCompositor(context, viewport)
      val attached =
        AttachedSession.attach(driver) { options ->
          map.attachVulkanBorrowedTexture(borrowedDescriptor(session, viewport, image), options)
        }
      return BorrowedTexture(attached, map, context, session, compositor, image)
    } catch (error: RuntimeException) {
      runCatching { compositor?.close() }.onFailure(error::addSuppressed)
      image.close()
      throw error
    }
  }

  private fun borrowedDescriptor(
    session: VulkanContextDescriptor,
    viewport: Viewport,
    image: VulkanBorrowedImage,
  ): VulkanBorrowedTextureDescriptor =
    VulkanBorrowedTextureDescriptor(
      RenderTarget.extent(viewport),
      viewport.framebufferWidth().toUInt(),
      viewport.framebufferHeight().toUInt(),
      session,
      image.imageAddress().toULong(),
      image.viewAddress().toULong(),
      VK10.VK_FORMAT_R8G8B8A8_UNORM.toUInt(),
      VK10.VK_IMAGE_LAYOUT_UNDEFINED.toUInt(),
      VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.toUInt(),
    )

  /** The session's view of the host's device, submitting to [queue]. */
  private fun descriptor(context: VulkanContext, queue: Long): VulkanContextDescriptor =
    VulkanContextDescriptor(
      NativePointer.ofAddress(context.instanceAddress()),
      NativePointer.ofAddress(context.physicalDeviceAddress()),
      NativePointer.ofAddress(context.deviceAddress()),
      NativePointer.ofAddress(queue),
      context.graphicsQueueFamilyIndex().toUInt(),
      NativePointer.ofAddress(context.getInstanceProcAddrAddress()),
      NativePointer.ofAddress(context.getDeviceProcAddrAddress()),
    )

  private class OwnedTexture(
    attached: AttachedSession,
    private val compositor: VulkanTextureCompositor,
  ) : OwnedTextureTarget(attached) {
    override fun draw(frame: AcquiredFrameHandle): Boolean = frame.withProducerSync { sync ->
      check(sync.kind == GpuSyncKind.CPU_COMPLETE) {
        "Vulkan compositor requires CPU-complete producer work"
      }
      frame.withVulkanTexture { view ->
        check(view.width > 0u && view.height > 0u) {
          "MapLibre returned an empty Vulkan owned texture frame"
        }
        check(view.layout == VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.toUInt()) {
          "MapLibre owned texture frame is not shader-readable: layout=${view.layout}"
        }
        compositor.drawImageView(view.imageView.toLong())
      }
    }

    override fun resizeHost(viewport: Viewport) {
      compositor.resize(viewport)
    }

    override fun closeHost() {
      compositor.close()
    }
  }

  private class BorrowedTexture(
    attached: AttachedSession,
    map: MapHandle,
    private val context: VulkanContext,
    private val sessionContext: VulkanContextDescriptor,
    private val compositor: VulkanTextureCompositor,
    image: VulkanBorrowedImage,
  ) : BorrowedTextureTarget<VulkanBorrowedImage>(attached, map, image) {
    override fun allocate(viewport: Viewport): VulkanBorrowedImage =
      VulkanBorrowedImage.create(context, viewport)

    override fun setTarget(viewport: Viewport, replacement: VulkanBorrowedImage): Deferred<Unit> =
      session.setVulkanBorrowedTextureTarget(
        borrowedDescriptor(sessionContext, viewport, replacement)
      )

    override fun draw(texture: VulkanBorrowedImage): Boolean =
      compositor.drawImageView(texture.view())

    override fun resizeHost(viewport: Viewport) {
      compositor.resize(viewport)
    }

    override fun closeHost() {
      compositor.close()
      texture.close()
    }
  }
}
