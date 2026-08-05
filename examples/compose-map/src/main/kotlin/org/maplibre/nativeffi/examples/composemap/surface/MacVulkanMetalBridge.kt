package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

internal class MacVulkanMetalBridge : NativeSurfaceBridge {
  private val rendererDispatcher =
    NativeSurfaceRendererDispatcher("compose-map-mac-vulkan-renderer")
  private var vulkan: MacVulkanContext? = null
  private var metalTexture = NativeHandle(0)
  private var metalDevice = NativeHandle(0)
  private var pixelFormat = 0L
  private var importedTexture: MacVulkanImportedTexture? = null
  private var currentExtent = SurfaceExtent.Empty

  // Read on the Compose thread while the renderer thread writes them.
  @Volatile private var generation = 0L
  @Volatile private var renderedGeneration = 0L

  // The Metal texture a frame last landed in, kept alive until one lands in its replacement. Skiko
  // allocates a new texture for every resize and the map needs a frame or two to fill it, so this
  // is what the consumer draws in between.
  private var retiredTexture = NativeHandle(0)
  private var retiredPixelFormat = 0L
  @Volatile private var retiredGeneration = 0L

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
    if (extent == currentExtent && importedTexture != null) {
      return
    }
    recreateTexture(extent, skikoDevice)
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
    renderedGeneration = frame.target.generation
    rendererDispatcher.run { vulkan?.waitIdle() }
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    rendererDispatcher.run {
      releaseRetiredOnceReplaced()
      action()
    }

  override fun <T> withRendererAccess(action: () -> T): T = rendererDispatcher.run(action)

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is VulkanImageTarget) {
      return false
    }
    // Only a texture this bridge still holds is safe to draw.
    val texture: NativeHandle
    val format: Long
    when (target.generation) {
      generation -> {
        texture = metalTexture
        format = pixelFormat
      }
      retiredGeneration -> {
        texture = retiredTexture
        format = retiredPixelFormat
      }
      else -> return false
    }
    if (texture.address == 0L) {
      return false
    }
    return SkikoHost.drawMetalTexture(
      scope,
      MetalTextureTarget(
        texture = texture,
        device = metalDevice,
        pixelFormat = format,
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

  private fun recreateTexture(extent: SurfaceExtent, skikoDevice: SkikoMetalDevice?) {
    if (extent.isEmpty) {
      disposeTexture()
      return
    }

    val oldTexture = metalTexture
    val oldPixelFormat = pixelFormat
    // Resolved by the caller: asking Skiko from the renderer thread waits on the event dispatch
    // thread, which is already waiting on this one.
    val requiredMetalDevice =
      checkNotNull(skikoDevice) { "The Skiko Metal device is resolved before this hop" }
    val requiredMetalAdapter = MacMetalBridgeNative.metalAdapter(requiredMetalDevice.ptr)
    // Only the device that allocated a texture can take it back, so a Skiko device change
    // allocates rather than reusing.
    val reusableTexture =
      if (metalDevice.address == requiredMetalDevice.ptr) oldTexture.address else 0L
    val newTextureAddress =
      MacMetalBridgeNative.createMetalTexture(
        metalDevice = requiredMetalDevice.ptr,
        oldTexture = reusableTexture,
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
      retire(
        oldTexture,
        oldPixelFormat,
        deviceChanged = metalDevice.address != requiredMetalDevice.ptr,
      )
    }
    metalTexture = newTexture
    metalDevice = NativeHandle(requiredMetalDevice.ptr)
    pixelFormat = MacMetalBridgeNative.texturePixelFormat(newTexture.address)
    try {
      val context = vulkan ?: MacVulkanContext.create(requiredMetalAdapter).also { vulkan = it }
      importedTexture = context.createImportedTexture(newTexture, extent)
    } catch (error: RuntimeException) {
      disposeTexture()
      throw error
    }
  }

  // Holds the outgoing texture for the consumer to draw while the replacement is still empty. A
  // texture from a device Skiko has replaced cannot be drawn on the new one.
  private fun retire(outgoing: NativeHandle, outgoingPixelFormat: Long, deviceChanged: Boolean) {
    if (outgoing.address == 0L) {
      return
    }
    if (deviceChanged) {
      // Both belong to the device Skiko replaced.
      releaseMetalTexture(outgoing)
      releaseMetalTexture(retiredTexture)
      retiredTexture = NativeHandle(0)
      retiredPixelFormat = 0
      retiredGeneration = 0
      return
    }
    if (renderedGeneration != generation) {
      // This one never held a frame, so whatever is already retired stays.
      releaseMetalTexture(outgoing)
      return
    }
    releaseMetalTexture(retiredTexture)
    retiredTexture = outgoing
    retiredPixelFormat = outgoingPixelFormat
    retiredGeneration = generation
  }

  // Released a frame after the replacement rendered, so the consumer's last recorded frame from
  // the retired texture has been flushed.
  private fun releaseRetiredOnceReplaced() {
    if (retiredTexture.address == 0L || renderedGeneration != generation) {
      return
    }
    releaseMetalTexture(retiredTexture)
    retiredTexture = NativeHandle(0)
    retiredPixelFormat = 0
    retiredGeneration = 0
  }

  private fun disposeTexture() {
    importedTexture?.close()
    importedTexture = null
    releaseMetalTexture(metalTexture)
    metalTexture = NativeHandle(0)
    releaseMetalTexture(retiredTexture)
    retiredTexture = NativeHandle(0)
    retiredPixelFormat = 0
    retiredGeneration = 0
    metalDevice = NativeHandle(0)
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
