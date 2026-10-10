package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import org.lwjgl.vulkan.VK10
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.GpuSyncKind
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.VulkanBorrowedTexture
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
        AttachedSession.attach(driver, RenderTarget.TEXTURE_RING_DEPTH.toUInt()) { options ->
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
    val ring =
      BorrowedTextureTarget.allocateRing(viewport) { VulkanBorrowedImage.create(context, it) }
    var compositor: VulkanTextureCompositor? = null
    try {
      compositor = VulkanTextureCompositor(context, viewport)
      val attached =
        AttachedSession.attach(driver, RenderTarget.TEXTURE_RING_DEPTH.toUInt()) { options ->
          map.attachVulkanBorrowedTexture(borrowedDescriptor(session, viewport, ring), options)
        }
      return BorrowedTexture(attached, map, context, session, compositor, ring)
    } catch (error: RuntimeException) {
      runCatching { compositor?.close() }.onFailure(error::addSuppressed)
      ring.forEach(AutoCloseable::close)
      throw error
    }
  }

  private fun borrowedDescriptor(
    session: VulkanContextDescriptor,
    viewport: Viewport,
    ring: List<VulkanBorrowedImage>,
  ): VulkanBorrowedTextureDescriptor =
    VulkanBorrowedTextureDescriptor(
      RenderTarget.extent(viewport),
      viewport.framebufferWidth().toUInt(),
      viewport.framebufferHeight().toUInt(),
      session,
      ring.map { VulkanBorrowedTexture(it.imageAddress().toULong(), it.viewAddress().toULong()) },
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

  /** Composes a frame of either texture mode, whose producer work is complete. */
  private fun draw(compositor: VulkanTextureCompositor, frame: AcquiredFrameHandle): Boolean =
    frame.withProducerSync { sync ->
      check(sync.kind == GpuSyncKind.CPU_COMPLETE) {
        "Vulkan compositor requires CPU-complete producer work"
      }
      frame.withVulkanTexture { view ->
        check(view.width > 0u && view.height > 0u) {
          "MapLibre returned an empty Vulkan texture frame"
        }
        check(view.layout == VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.toUInt()) {
          "MapLibre texture frame is not shader-readable: layout=${view.layout}"
        }
        compositor.drawImageView(view.imageView.toLong())
      }
    }

  private class OwnedTexture(
    attached: AttachedSession,
    private val compositor: VulkanTextureCompositor,
  ) : OwnedTextureTarget(attached) {
    override fun draw(frame: AcquiredFrameHandle): Boolean = draw(compositor, frame)

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
    ring: List<VulkanBorrowedImage>,
  ) : BorrowedTextureTarget<VulkanBorrowedImage>(attached, map, ring) {
    override fun allocate(viewport: Viewport): VulkanBorrowedImage =
      VulkanBorrowedImage.create(context, viewport)

    override fun setTarget(
      viewport: Viewport,
      replacement: List<VulkanBorrowedImage>,
    ): Deferred<Unit> =
      session.setVulkanBorrowedTextureTarget(
        borrowedDescriptor(sessionContext, viewport, replacement)
      )

    override fun draw(frame: AcquiredFrameHandle): Boolean = draw(compositor, frame)

    override fun resizeHost(viewport: Viewport) {
      compositor.resize(viewport)
    }

    override fun closeHost() {
      compositor.close()
      super.closeHost()
    }
  }
}
