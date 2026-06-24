package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

internal class MacVulkanMetalBridge : NativeSurfaceBridge {
  private val rendererDispatcher =
    NativeSurfaceRendererDispatcher("compose-map-mac-vulkan-renderer")
  private var vulkan: MacVulkanContext? = null
  private var metalTexture = NativeHandle(0)
  private var pixelFormat = 0L
  private var importedTexture: MacVulkanImportedTexture? = null
  private var generation = 0L
  private var currentExtent = SurfaceExtent.Empty

  override val backend: ProducerBackend = ProducerBackend.VULKAN

  override val consumerBackend: ConsumerBackend = ConsumerBackend.METAL

  override val capabilities: NativeSurfaceCapabilities =
    NativeSurfaceCapabilities(
      producerBackend = backend,
      consumerBackend = consumerBackend,
      supportsExplicitSynchronization = false,
      supportsResizeWithoutRecreate = false,
    )

  override fun resize(extent: SurfaceExtent) {
    val metalDevice = if (extent.isEmpty) null else SkikoHost.requireMetalDevice()
    rendererDispatcher.run { resizeOnRendererThread(extent, metalDevice) }
  }

  private fun resizeOnRendererThread(extent: SurfaceExtent, metalDevice: SkikoMetalDevice? = null) {
    if (extent == currentExtent && importedTexture != null) {
      return
    }
    recreateTexture(extent, metalDevice)
    currentExtent = extent
    generation += 1
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame {
    if (importedTexture == null || extent != currentExtent) {
      resize(extent)
    }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    rendererDispatcher.run { vulkan?.waitIdle() }
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    rendererDispatcher.run(action)

  override fun <T> withRendererAccess(action: () -> T): T = rendererDispatcher.run(action)

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is VulkanImageTarget || metalTexture.address == 0L) {
      return false
    }
    return SkikoHost.drawMetalTexture(
      scope,
      MetalTextureTarget(
        texture = metalTexture,
        pixelFormat = pixelFormat,
        extent = target.extent,
        generation = target.generation,
      ),
    )
  }

  override fun close() {
    try {
      rendererDispatcher.run {
        disposeTexture()
        val closingVulkan = vulkan
        vulkan = null
        closingVulkan?.close()
      }
    } finally {
      rendererDispatcher.close()
    }
  }

  private fun target(generation: Long): NativeSurfaceTarget =
    checkNotNull(importedTexture) { "Vulkan texture is not initialized" }.target(generation)

  private fun recreateTexture(extent: SurfaceExtent, metalDevice: SkikoMetalDevice? = null) {
    if (extent.isEmpty) {
      disposeTexture()
      return
    }

    val oldTexture = metalTexture
    val requiredMetalDevice = metalDevice ?: SkikoHost.requireMetalDevice()
    val requiredMetalAdapter = MacMetalBridgeNative.metalAdapter(requiredMetalDevice.ptr)
    val newTextureAddress =
      MacMetalBridgeNative.createMetalTexture(
        metalDevice = requiredMetalDevice.ptr,
        oldTexture = oldTexture.address,
        width = extent.physicalWidth,
        height = extent.physicalHeight,
      )
    val newTexture = NativeHandle(newTextureAddress)
    if (newTexture == oldTexture && importedTexture != null) {
      return
    }

    importedTexture?.close()
    importedTexture = null
    if (newTexture != oldTexture) {
      releaseMetalTexture(oldTexture)
    }
    metalTexture = newTexture
    pixelFormat = MacMetalBridgeNative.texturePixelFormat(newTexture.address)
    try {
      val context = vulkan ?: MacVulkanContext.create(requiredMetalAdapter).also { vulkan = it }
      importedTexture = context.createImportedTexture(newTexture, extent)
    } catch (error: RuntimeException) {
      disposeTexture()
      throw error
    }
  }

  private fun disposeTexture() {
    importedTexture?.close()
    importedTexture = null
    releaseMetalTexture(metalTexture)
    metalTexture = NativeHandle(0)
    pixelFormat = 0
  }

  private fun releaseMetalTexture(texture: NativeHandle) {
    if (texture.address == 0L) {
      return
    }
    SkikoHost.forgetMetalTexture(texture)
    MacMetalBridgeNative.disposeMetalTexture(texture.address)
  }
}
