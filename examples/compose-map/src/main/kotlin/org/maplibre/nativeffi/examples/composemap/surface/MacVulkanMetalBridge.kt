package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

/**
 * Lends the session a ring of Vulkan images imported from Skiko Metal textures through MoltenVK,
 * and draws the Metal textures.
 */
internal class MacVulkanMetalBridge : NativeSurfaceBridge {
  private val rendererDispatcher =
    NativeSurfaceRendererDispatcher("compose-map-mac-vulkan-renderer")
  private var vulkan: MacVulkanContext? = null
  private val ring = MacMetalTextureRing()

  // One import per texture of the ring, in slot order.
  private var importedTextures: List<MacVulkanImportedTexture> = emptyList()
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
    val skikoDevice = if (extent.isEmpty) null else SkikoHost.requireMetalDevice()
    rendererDispatcher.run { resizeOnRendererThread(extent, skikoDevice) }
  }

  private fun resizeOnRendererThread(extent: SurfaceExtent, skikoDevice: SkikoMetalDevice?) {
    if (extent == currentExtent && importedTextures.isNotEmpty()) {
      return
    }
    recreateTextures(extent, skikoDevice)
    currentExtent = extent
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame {
    if (importedTextures.isEmpty() || extent != currentExtent) {
      resize(extent)
    }
    check(importedTextures.isNotEmpty()) { "Vulkan texture is not initialized" }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = MacVulkanImportedTexture.ringTarget(importedTextures, ring.generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  // The session reports a rendered frame once its GPU work completes, so the consumer can read it.
  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    ring.markRendered(frame.target.generation)
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    rendererDispatcher.run {
      ring.releaseRetiredOnceReplaced()
      action()
    }

  override fun <T> withRendererAccess(action: () -> T): T = rendererDispatcher.run(action)

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is VulkanImageTarget) {
      return false
    }
    // Only a texture this bridge still holds is safe to draw.
    val drawable = ring.drawable(target, TextureOrigin.TOP_LEFT) ?: return false
    return SkikoHost.drawMetalTexture(scope, drawable)
  }

  override fun close() {
    try {
      rendererDispatcher.run {
        disposeTextures()
        val closingVulkan = vulkan
        vulkan = null
        closingVulkan?.close()
      }
    } finally {
      rendererDispatcher.close()
    }
  }

  private fun recreateTextures(extent: SurfaceExtent, skikoDevice: SkikoMetalDevice?) {
    if (extent.isEmpty) {
      disposeTextures()
      return
    }
    // Resolved by the caller: asking Skiko from the renderer thread waits on the event dispatch
    // thread, which is already waiting on this one.
    val requiredMetalDevice =
      checkNotNull(skikoDevice) { "The Skiko Metal device is resolved before this hop" }
    val requiredMetalAdapter = MacMetalBridgeNative.metalAdapter(requiredMetalDevice.ptr)
    // The session still names these images until its target replacement completes, after this
    // returns. Destroying them first is safe only because each draw waits for its own demand's
    // result, which leaves the core worker idle between draws.
    closeImports()
    ring.allocate(extent, requiredMetalDevice)
    try {
      val context = vulkan ?: MacVulkanContext.create(requiredMetalAdapter).also { vulkan = it }
      val imported = mutableListOf<MacVulkanImportedTexture>()
      try {
        ring.textures.forEach { imported += context.createImportedTexture(it, extent) }
      } catch (error: RuntimeException) {
        imported.forEach(MacVulkanImportedTexture::close)
        throw error
      }
      importedTextures = imported
    } catch (error: RuntimeException) {
      disposeTextures()
      throw error
    }
  }

  private fun closeImports() {
    importedTextures.forEach(MacVulkanImportedTexture::close)
    importedTextures = emptyList()
  }

  private fun disposeTextures() {
    closeImports()
    ring.dispose()
  }
}
