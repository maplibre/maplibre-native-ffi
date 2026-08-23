package org.maplibre.nativeffi.examples.composemap.map

import org.maplibre.nativeffi.Maplibre
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
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLContextDescriptor
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanHandle
import org.maplibre.nativeffi.render.WglContextDescriptor

internal object MapLibreNativeSurfaceAdapter {
  val backend: ProducerBackend =
    Maplibre.supportedRenderBackends().mapNotNull { it.toProducerBackend() }.singleOrNull()
      ?: error(
        "Expected exactly one MapLibre render backend, found ${Maplibre.supportedRenderBackends()}"
      )

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
        extent.physicalWidth,
        extent.physicalHeight,
        target.texture.toPointer(),
      )
    return BorrowedTarget(
      sessionKey = SessionKey.Metal(target.device, target.pixelFormat),
      targetKey = TargetKey(target.generation, extent),
      attach = { map -> map.attachMetalBorrowedTexture(descriptor) },
      setTarget = { session -> session.setMetalBorrowedTextureTarget(descriptor) },
    )
  }

  private fun vulkanTarget(target: VulkanImageTarget, extent: SurfaceExtent): BorrowedTarget {
    val descriptor =
      VulkanBorrowedTextureDescriptor(
          extent.toRenderTargetExtent(),
          extent.physicalWidth,
          extent.physicalHeight,
          target.context.toDescriptor(),
          target.image.toVulkanHandle(),
          target.imageView.toVulkanHandle(),
          target.format,
          target.initialLayout,
        )
        .apply { finalLayout = target.finalLayout }
    return BorrowedTarget(
      sessionKey =
        SessionKey.Vulkan(
          context = target.context,
          format = target.format,
          initialLayout = target.initialLayout,
          finalLayout = target.finalLayout,
        ),
      targetKey = TargetKey(target.generation, extent),
      attach = { map -> map.attachVulkanBorrowedTexture(descriptor) },
      setTarget = { session -> session.setVulkanBorrowedTextureTarget(descriptor) },
    )
  }

  private fun openGlTarget(target: OpenGlTextureTarget, extent: SurfaceExtent): BorrowedTarget {
    val descriptor =
      OpenGLBorrowedTextureDescriptor(
        extent.toRenderTargetExtent(),
        extent.physicalWidth,
        extent.physicalHeight,
        target.context.toDescriptor(),
        target.textureName,
        target.textureTarget,
      )
    return BorrowedTarget(
      sessionKey = SessionKey.OpenGl(target.context),
      targetKey = TargetKey(target.generation, extent),
      attach = { map -> map.attachOpenGLBorrowedTexture(descriptor) },
      setTarget = { session -> session.setOpenGLBorrowedTextureTarget(descriptor) },
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
    val attach: (MapHandle) -> RenderSessionHandle,
    val setTarget: (RenderSessionHandle) -> Unit,
  )
}

private fun SurfaceExtent.toRenderTargetExtent(): RenderTargetExtent =
  RenderTargetExtent(width, height, scaleFactor)

private fun NativeHandle.toPointer(): NativePointer = NativePointer.ofAddress(address)

private fun NativeHandle.toVulkanHandle(): VulkanHandle = VulkanHandle.ofBits(address)

private fun RenderBackend.toProducerBackend(): ProducerBackend? =
  when (this) {
    RenderBackend.METAL -> ProducerBackend.METAL
    RenderBackend.VULKAN -> ProducerBackend.VULKAN
    RenderBackend.OPENGL -> ProducerBackend.OPENGL
    // The Skia bridges this example produces for have no WebGPU consumer.
    RenderBackend.WEBGPU -> null
  }

private fun VulkanContextHandles.toDescriptor(): VulkanContextDescriptor =
  VulkanContextDescriptor(
    instance.toPointer(),
    physicalDevice.toPointer(),
    device.toPointer(),
    graphicsQueue.toPointer(),
    graphicsQueueFamilyIndex,
    getInstanceProcAddr.toPointer(),
    getDeviceProcAddr.toPointer(),
  )

private fun OpenGlContextHandles.toDescriptor(): OpenGLContextDescriptor =
  when (this) {
    is EglContextHandles ->
      EglContextDescriptor(
        display.toPointer(),
        config.toPointer(),
        shareContext.toPointer(),
        getProcAddress.toPointer(),
      )
    is WglContextHandles ->
      WglContextDescriptor(
        deviceContext.toPointer(),
        shareContext.toPointer(),
        getProcAddress.toPointer(),
      )
  }
