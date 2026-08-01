package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

internal class MacMetalBridge : NativeSurfaceBridge {
  private val rendererDispatcher = NativeSurfaceRendererDispatcher("compose-map-mac-metal-renderer")
  private var texture = NativeHandle(0)
  private var metalDevice = NativeHandle(0)
  private var pixelFormat = 0L
  private var currentExtent = SurfaceExtent.Empty

  // Read on the Compose thread while the renderer thread writes them.
  @Volatile private var generation = 0L
  @Volatile private var renderedGeneration = 0L

  // The texture a frame last landed in, kept alive until one lands in its
  // replacement. Skiko allocates a new texture for every resize and the map
  // needs a frame or two to fill it, so this is what the consumer draws in
  // between rather than having nothing to show.
  private var retiredTexture = NativeHandle(0)
  @Volatile private var retiredGeneration = 0L

  override val backend: ProducerBackend = ProducerBackend.METAL

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
    if (extent == currentExtent && texture.address != 0L) {
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
    if (texture.address == 0L || extent != currentExtent) {
      resize(extent)
    }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(extent, generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  private fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    MetalTextureTarget(
      texture =
        texture.takeIf { it.address != 0L }
          ?: throw NativeSurfaceBridgeException("Skiko Metal texture allocation returned null"),
      device = metalDevice,
      pixelFormat = pixelFormat,
      extent = extent,
      generation = generation,
    )

  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    renderedGeneration = frame.target.generation
  }

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is MetalTextureTarget) {
      return false
    }
    // Only a texture this bridge still holds is safe to draw, which is the
    // current one and the retired one behind it.
    val held =
      when (target.generation) {
        generation -> texture
        retiredGeneration -> retiredTexture
        else -> NativeHandle(0)
      }
    if (held.address == 0L || held.address != target.texture.address) {
      return false
    }
    return SkikoHost.drawMetalTexture(scope, target)
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    rendererDispatcher.run {
      releaseRetiredOnceReplaced()
      MacMetalBridgeNative.runInAutoreleasePool(action)
    }

  override fun <T> withRendererAccess(action: () -> T): T = rendererDispatcher.run(action)

  override fun close() {
    try {
      disposeTexture()
    } finally {
      rendererDispatcher.close()
    }
  }

  private fun recreateTexture(extent: SurfaceExtent, skikoDevice: SkikoMetalDevice?) {
    if (extent.isEmpty) {
      disposeTexture()
      return
    }
    val oldTexture = texture
    // Resolved by the caller, on a thread that may ask Skiko for it. Reaching
    // for it here would ask from the renderer thread, and answering that waits
    // on the event dispatch thread, which is already waiting on this one.
    val requiredMetalDevice =
      checkNotNull(skikoDevice) { "The Skiko Metal device is resolved before this hop" }
    // Only the device that allocated it can take it back, so a Skiko device change allocates
    // rather than reusing and this bridge never names one device while holding the other's texture.
    val reusableTexture =
      if (metalDevice.address == requiredMetalDevice.ptr) oldTexture.address else 0L
    val textureAddress =
      MacMetalBridgeNative.createMetalTexture(
        metalDevice = requiredMetalDevice.ptr,
        oldTexture = reusableTexture,
        width = extent.physicalWidth,
        height = extent.physicalHeight,
      )
    if (textureAddress != oldTexture.address) {
      retire(oldTexture, deviceChanged = metalDevice.address != requiredMetalDevice.ptr)
    }
    texture = NativeHandle(textureAddress)
    metalDevice = NativeHandle(requiredMetalDevice.ptr)
    pixelFormat = MacMetalBridgeNative.texturePixelFormat(textureAddress)
  }

  // Holds the outgoing texture for the consumer to draw while the replacement
  // is still empty. A texture nothing ever rendered into has nothing to show,
  // and one from a device Skiko has replaced cannot be drawn on the new one.
  private fun retire(outgoing: NativeHandle, deviceChanged: Boolean) {
    if (outgoing.address == 0L) {
      return
    }
    if (deviceChanged) {
      // Both belong to the device Skiko replaced, and neither can be drawn on
      // the new one.
      releaseMetalTexture(outgoing)
      releaseMetalTexture(retiredTexture)
      retiredTexture = NativeHandle(0)
      retiredGeneration = 0
      return
    }
    if (renderedGeneration != generation) {
      // Keeps whatever is already retired: it is still the last texture a frame
      // landed in, and this one never held a frame at all.
      releaseMetalTexture(outgoing)
      return
    }
    releaseMetalTexture(retiredTexture)
    retiredTexture = outgoing
    retiredGeneration = generation
  }

  // Runs a frame after the replacement first rendered, so the consumer's last
  // recorded frame from the retired texture has been flushed by then.
  private fun releaseRetiredOnceReplaced() {
    if (retiredTexture.address == 0L || renderedGeneration != generation) {
      return
    }
    releaseMetalTexture(retiredTexture)
    retiredTexture = NativeHandle(0)
    retiredGeneration = 0
  }

  private fun disposeTexture() {
    releaseMetalTexture(texture)
    texture = NativeHandle(0)
    releaseMetalTexture(retiredTexture)
    retiredTexture = NativeHandle(0)
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
