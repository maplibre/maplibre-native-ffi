package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

/**
 * Lends the session a ring of [RING_DEPTH] Metal textures. The renderer holds the frame the
 * consumer draws until a newer one arrives, so the session renders into the other texture
 * meanwhile.
 */
internal class MacMetalBridge : NativeSurfaceBridge {
  private val rendererDispatcher = NativeSurfaceRendererDispatcher("compose-map-mac-metal-renderer")
  private var textures: List<NativeHandle> = emptyList()
  private var metalDevice = NativeHandle(0)
  private var pixelFormat = 0L
  private var currentExtent = SurfaceExtent.Empty

  // Read on the Compose thread while the renderer thread writes them.
  @Volatile private var generation = 0L
  @Volatile private var renderedGeneration = 0L

  // The ring a frame last landed in, kept alive until one lands in its replacement. The bridge
  // allocates a new ring for every resize and the map needs a frame or two to fill it, so this is
  // what the consumer draws in between.
  private var retiredTextures: List<NativeHandle> = emptyList()
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
    if (extent == currentExtent && textures.isNotEmpty()) {
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
    if (textures.isEmpty() || extent != currentExtent) {
      resize(extent)
    }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(extent, generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  private fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget {
    if (textures.isEmpty()) {
      throw NativeSurfaceBridgeException("Skiko Metal texture allocation returned null")
    }
    return MetalTextureTarget(
      texture = textures.first(),
      device = metalDevice,
      pixelFormat = pixelFormat,
      extent = extent,
      generation = generation,
      ring = textures,
    )
  }

  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    renderedGeneration = frame.target.generation
  }

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is MetalTextureTarget) {
      return false
    }
    // Only a texture this bridge still holds is safe to draw.
    val held =
      when (target.generation) {
        generation -> textures
        retiredGeneration -> retiredTextures
        else -> emptyList()
      }
    if (target.texture.address == 0L || target.texture !in held) {
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
    val oldTextures = textures
    // Resolved by the caller: asking Skiko from the renderer thread waits on the event dispatch
    // thread, which is already waiting on this one.
    val requiredMetalDevice =
      checkNotNull(skikoDevice) { "The Skiko Metal device is resolved before this hop" }
    // Only the device that allocated a texture can take it back, so a Skiko device change
    // allocates rather than reusing.
    val sameDevice = metalDevice.address == requiredMetalDevice.ptr
    val replacement =
      List(RING_DEPTH) { slot ->
        val reusable = if (sameDevice) oldTextures.getOrNull(slot)?.address ?: 0L else 0L
        NativeHandle(
          MacMetalBridgeNative.createMetalTexture(
            metalDevice = requiredMetalDevice.ptr,
            oldTexture = reusable,
            width = extent.physicalWidth,
            height = extent.physicalHeight,
          )
        )
      }
    if (replacement != oldTextures) {
      retire(oldTextures, deviceChanged = !sameDevice)
    }
    textures = replacement
    metalDevice = NativeHandle(requiredMetalDevice.ptr)
    pixelFormat = MacMetalBridgeNative.texturePixelFormat(replacement.first().address)
  }

  // Holds the outgoing ring for the consumer to draw while the replacement is still empty. A
  // texture from a device Skiko has replaced cannot be drawn on the new one.
  private fun retire(outgoing: List<NativeHandle>, deviceChanged: Boolean) {
    if (outgoing.isEmpty()) {
      return
    }
    if (deviceChanged) {
      // Both belong to the device Skiko replaced.
      outgoing.forEach(::releaseMetalTexture)
      retiredTextures.forEach(::releaseMetalTexture)
      retiredTextures = emptyList()
      retiredGeneration = 0
      return
    }
    if (renderedGeneration != generation) {
      // This one never held a frame, so whatever is already retired stays.
      outgoing.forEach(::releaseMetalTexture)
      return
    }
    retiredTextures.forEach(::releaseMetalTexture)
    retiredTextures = outgoing
    retiredGeneration = generation
  }

  // Released a frame after the replacement rendered, so the consumer's last recorded frame from
  // the retired texture has been flushed.
  private fun releaseRetiredOnceReplaced() {
    if (retiredTextures.isEmpty() || renderedGeneration != generation) {
      return
    }
    retiredTextures.forEach(::releaseMetalTexture)
    retiredTextures = emptyList()
    retiredGeneration = 0
  }

  private fun disposeTexture() {
    textures.forEach(::releaseMetalTexture)
    textures = emptyList()
    retiredTextures.forEach(::releaseMetalTexture)
    retiredTextures = emptyList()
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

  private companion object {
    const val RING_DEPTH = 2
  }
}
