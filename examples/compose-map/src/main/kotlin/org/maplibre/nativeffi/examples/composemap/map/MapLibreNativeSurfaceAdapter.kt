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
import org.maplibre.nativeffi.render.WglContextDescriptor

internal object MapLibreNativeSurfaceAdapter {
  val backend: ProducerBackend =
    Maplibre.supportedRenderBackends().mapNotNull { it.toProducerBackend() }.singleOrNull()
      ?: error(
        "Expected exactly one MapLibre render backend, found ${Maplibre.supportedRenderBackends()}"
      )

  fun descriptor(target: NativeSurfaceTarget, extent: SurfaceExtent): BorrowedDescriptor =
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
      attach = { map ->
        map.attachVulkanBorrowedTexture(
          VulkanBorrowedTextureDescriptor(
              extent.toRenderTargetExtent(),
              target.context.toDescriptor(),
              target.image.toPointer(),
              target.imageView.toPointer(),
              target.format,
              target.initialLayout,
            )
            .apply { finalLayout = target.finalLayout }
        )
      },
    )

  private fun openGlDescriptor(
    target: OpenGlTextureTarget,
    extent: SurfaceExtent,
  ): BorrowedDescriptor =
    BorrowedDescriptor(
      key = targetKey(target.backend, target.generation, extent),
      attach = { map ->
        map.attachOpenGLBorrowedTexture(
          OpenGLBorrowedTextureDescriptor(
            extent.toRenderTargetExtent(),
            target.context.toDescriptor(),
            target.textureName,
            target.textureTarget,
          )
        )
      },
    )

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

  data class TargetKey(
    val backend: ProducerBackend,
    val generation: Long,
    val width: Int,
    val height: Int,
    val scaleFactor: Double,
    val physicalWidth: Int,
    val physicalHeight: Int,
  )

  class BorrowedDescriptor(val key: TargetKey, val attach: (MapHandle) -> RenderSessionHandle)
}

private fun SurfaceExtent.toRenderTargetExtent(): RenderTargetExtent =
  RenderTargetExtent(width, height, scaleFactor)

private fun NativeHandle.toPointer(): NativePointer = NativePointer.ofAddress(address)

private fun RenderBackend.toProducerBackend(): ProducerBackend? =
  when (this) {
    RenderBackend.METAL -> ProducerBackend.METAL
    RenderBackend.VULKAN -> ProducerBackend.VULKAN
    RenderBackend.OPENGL -> ProducerBackend.OPENGL
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
