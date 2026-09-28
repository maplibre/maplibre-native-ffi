package org.maplibre.nativeffi.examples.composemap.map

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.examples.composemap.surface.EglContextHandles
import org.maplibre.nativeffi.examples.composemap.surface.MetalTextureTarget
import org.maplibre.nativeffi.examples.composemap.surface.NativeHandle
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceTarget
import org.maplibre.nativeffi.examples.composemap.surface.OpenGlContextHandles
import org.maplibre.nativeffi.examples.composemap.surface.OpenGlTextureTarget
import org.maplibre.nativeffi.examples.composemap.surface.ProducerBackend
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.examples.composemap.surface.VulkanContextHandles
import org.maplibre.nativeffi.examples.composemap.surface.VulkanImageTarget
import org.maplibre.nativeffi.examples.composemap.surface.WglContextHandles
import org.maplibre.nativeffi.generated.EglContextDescriptor
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptor
import org.maplibre.nativeffi.generated.RenderBackendFlag
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionAttachment
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.generated.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.generated.WglContextDescriptor
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderSessionHandle

internal object MapLibreNativeSurfaceAdapter {
  val backend: ProducerBackend =
    listOf(RenderBackendFlag.METAL, RenderBackendFlag.VULKAN, RenderBackendFlag.OPENGL)
      .filter { it in GeneratedApi.supportedRenderBackendMask() }
      .mapNotNull { it.toProducerBackend() }
      .singleOrNull() ?: error("Expected exactly one supported MapLibre render backend")

  fun borrowedTarget(target: NativeSurfaceTarget, extent: SurfaceExtent): BorrowedTarget =
    when (target) {
      is MetalTextureTarget -> metalTarget(target, extent)
      is VulkanImageTarget -> vulkanTarget(target, extent)
      is OpenGlTextureTarget -> openGlTarget(target, extent)
    }

  private fun metalTarget(target: MetalTextureTarget, extent: SurfaceExtent): BorrowedTarget {
    val descriptor =
      MetalBorrowedTextureDescriptor(
        extent.toRenderTargetExtent(),
        extent.physicalWidth.toUInt(),
        extent.physicalHeight.toUInt(),
        target.texture.toPointer(),
      )
    return BorrowedTarget(
      sessionKey = SessionKey.Metal(target.device, target.pixelFormat),
      targetKey = TargetKey(target.generation, extent),
      attach = { map -> map.metalBorrowedTextureAttach(descriptor, callerDriverOptions) },
      setTarget = { session -> session.metalBorrowedTextureSetTarget(descriptor) },
    )
  }

  private fun vulkanTarget(target: VulkanImageTarget, extent: SurfaceExtent): BorrowedTarget {
    val descriptor =
      VulkanBorrowedTextureDescriptor(
        extent.toRenderTargetExtent(),
        extent.physicalWidth.toUInt(),
        extent.physicalHeight.toUInt(),
        target.context.toDescriptor(),
        target.image.toVulkanHandle(),
        target.imageView.toVulkanHandle(),
        target.format.toUInt(),
        target.initialLayout.toUInt(),
        target.finalLayout.toUInt(),
      )
    return BorrowedTarget(
      sessionKey =
        SessionKey.Vulkan(
          context = target.context,
          format = target.format,
          initialLayout = target.initialLayout,
          finalLayout = target.finalLayout,
        ),
      targetKey = TargetKey(target.generation, extent),
      attach = { map -> map.vulkanBorrowedTextureAttach(descriptor, callerDriverOptions) },
      setTarget = { session -> session.vulkanBorrowedTextureSetTarget(descriptor) },
    )
  }

  private fun openGlTarget(target: OpenGlTextureTarget, extent: SurfaceExtent): BorrowedTarget {
    val descriptor =
      OpenglBorrowedTextureDescriptor(
        extent.toRenderTargetExtent(),
        extent.physicalWidth.toUInt(),
        extent.physicalHeight.toUInt(),
        target.context.toDescriptor(),
        target.textureName.toUInt(),
        target.textureTarget.toUInt(),
      )
    return BorrowedTarget(
      sessionKey = SessionKey.OpenGl(target.context),
      targetKey = TargetKey(target.generation, extent),
      attach = { map -> map.openglBorrowedTextureAttach(descriptor, callerDriverOptions) },
      setTarget = { session -> session.openglBorrowedTextureSetTarget(descriptor) },
    )
  }

  /**
   * The part of a target a live render session cannot be moved across. A session takes a
   * replacement texture only for the graphics context it attached with, so a target whose key still
   * matches is handed over and one whose key changed closes the session and attaches again.
   */
  sealed interface SessionKey {
    /**
     * A Metal texture carries its device and pixel format, which is what a session compares against
     * its own. Attach admits only single-sample textures, so sample count needs no entry.
     */
    data class Metal(val device: NativeHandle, val pixelFormat: Long) : SessionKey

    /** A Vulkan session built its render pass around the format and both layouts. */
    data class Vulkan(
      val context: VulkanContextHandles,
      val format: Int,
      val initialLayout: Int,
      val finalLayout: Int,
    ) : SessionKey

    /** An OpenGL session names its context provider data and nothing else. */
    data class OpenGl(val context: OpenGlContextHandles) : SessionKey
  }

  /**
   * The texture a session is rendering into right now. A bridge counts the generation up every time
   * it allocates, so a matching key means nothing has to be handed over.
   */
  data class TargetKey(val generation: Long, val extent: SurfaceExtent)

  class BorrowedTarget(
    val sessionKey: SessionKey,
    val targetKey: TargetKey,
    val attach: (MapHandle) -> RenderSessionAttachment,
    val setTarget: (RenderSessionHandle) -> Deferred<Unit>,
  )

  private val callerDriverOptions =
    RenderSessionAttachOptions(driver = RenderDriverKind.CALLER_GRAPHICS_THREAD)
}

private fun SurfaceExtent.toRenderTargetExtent(): RenderTargetExtent =
  RenderTargetExtent(width.toUInt(), height.toUInt(), scaleFactor)

private fun NativeHandle.toPointer(): NativePointer = NativePointer.ofAddress(address)

private fun NativeHandle.toVulkanHandle(): ULong = address.toULong()

private fun RenderBackendFlag.toProducerBackend(): ProducerBackend? =
  when (this) {
    RenderBackendFlag.METAL -> ProducerBackend.METAL
    RenderBackendFlag.VULKAN -> ProducerBackend.VULKAN
    RenderBackendFlag.OPENGL -> ProducerBackend.OPENGL
    // The Skia bridges this example produces for have no WebGPU consumer.
    else -> null
  }

private fun VulkanContextHandles.toDescriptor(): VulkanContextDescriptor =
  VulkanContextDescriptor(
    instance.toPointer(),
    physicalDevice.toPointer(),
    device.toPointer(),
    graphicsQueue.toPointer(),
    graphicsQueueFamilyIndex.toUInt(),
    getInstanceProcAddr.toPointer(),
    getDeviceProcAddr.toPointer(),
  )

private fun OpenGlContextHandles.toDescriptor(): OpenglContextDescriptor =
  OpenglContextDescriptor(
    ownership = org.maplibre.nativeffi.generated.OpenglContextOwnership.SHARED,
    data =
      when (this) {
        is EglContextHandles ->
          org.maplibre.nativeffi.generated.OpenglContextDescriptorData.Egl(
            EglContextDescriptor(
              display.toPointer(),
              config.toPointer(),
              shareContext.toPointer(),
              org.maplibre.nativeffi.generated.OpenglClientApi.GLES,
              getProcAddress.toPointer(),
            )
          )
        is WglContextHandles ->
          org.maplibre.nativeffi.generated.OpenglContextDescriptorData.Wgl(
            WglContextDescriptor(
              deviceContext.toPointer(),
              shareContext.toPointer(),
              getProcAddress.toPointer(),
            )
          )
      },
  )
