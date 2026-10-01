package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.vulkan.VK10
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.GpuSyncKind
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RenderSessionHandle
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
    wakes: LoopWakes,
  ): RenderTarget =
    when (mode) {
      RenderTargetMode.NATIVE_SURFACE -> attachSurface(context, map, viewport, wakes)
      RenderTargetMode.OWNED_TEXTURE -> attachOwnedTexture(context, map, viewport, wakes)
      RenderTargetMode.BORROWED_TEXTURE -> attachBorrowedTexture(context, map, viewport, wakes)
    }

  private fun attachSurface(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
    wakes: LoopWakes,
  ): RenderTarget {
    val descriptor =
      VulkanSurfaceDescriptor(
        RenderTarget.extent(viewport),
        descriptor(context),
        context.surfaceAddress().toULong(),
      )
    return RenderTarget(
      RenderTarget.attached(map.vulkanSurfaceAttach(descriptor, wakes.attachOptions()))
    )
  }

  private fun attachOwnedTexture(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
    wakes: LoopWakes,
  ): RenderTarget {
    val descriptor =
      VulkanOwnedTextureDescriptor(RenderTarget.extent(viewport), descriptor(context))
    val compositor = VulkanTextureCompositor(context, viewport)
    try {
      return OwnedTexture(
        RenderTarget.attached(
          map.vulkanOwnedTextureAttach(
            descriptor,
            wakes.attachOptions(RenderTarget.OWNED_TEXTURE_RING_DEPTH),
          )
        ),
        compositor,
      )
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      throw error
    }
  }

  private fun attachBorrowedTexture(
    context: VulkanContext,
    map: MapHandle,
    viewport: Viewport,
    wakes: LoopWakes,
  ): RenderTarget {
    val image = VulkanBorrowedImage.create(context, viewport)
    var compositor: VulkanTextureCompositor? = null
    try {
      compositor = VulkanTextureCompositor(context, viewport)
      val descriptor = borrowedDescriptor(context, viewport, image)
      return BorrowedTexture(
        context,
        map,
        RenderTarget.attached(map.vulkanBorrowedTextureAttach(descriptor, wakes.attachOptions())),
        compositor,
        image,
      )
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
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

  private class OwnedTexture(
    session: RenderSessionHandle,
    private val compositor: VulkanTextureCompositor,
  ) : RenderTarget(session) {
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      super.resize(viewport)
    }

    override fun present(): Boolean {
      // An empty ring keeps the previously composited frame on screen.
      val frameHandle =
        try {
          session.acquireFrame()
        } catch (error: MaplibreException) {
          if (error.status == MaplibreStatus.NOT_READY) return true
          throw error
        }
      try {
        return frameHandle.withGetProducerSync { sync ->
          check(sync.kind == GpuSyncKind.CPU_COMPLETE) {
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
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        super.close()
      }
    }
  }

  private class BorrowedTexture(
    private val context: VulkanContext,
    private val map: MapHandle,
    session: RenderSessionHandle,
    private val compositor: VulkanTextureCompositor,
    private var image: VulkanBorrowedImage,
  ) : RenderTarget(session) {
    /** Allocates an image at the new size and hands it to the live session. */
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      val replacement = VulkanBorrowedImage.create(context, viewport)
      try {
        handOver {
          session.vulkanBorrowedTextureSetTarget(borrowedDescriptor(context, viewport, replacement))
        }
      } catch (error: RuntimeException) {
        RenderTarget.closeSuppressed(error, replacement)
        throw error
      }
      image.close()
      image = replacement
      RenderTarget.resizeMap(map, viewport)
    }

    override fun present(): Boolean = compositor.drawImageView(image.view())

    override fun close() {
      try {
        compositor.close()
      } finally {
        try {
          super.close()
        } finally {
          image.close()
        }
      }
    }
  }
}
