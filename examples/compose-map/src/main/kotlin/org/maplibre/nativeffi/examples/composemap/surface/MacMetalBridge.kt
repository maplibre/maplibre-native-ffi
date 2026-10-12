package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

/** Lends the session a ring of Skiko Metal textures, which the consumer draws directly. */
internal class MacMetalBridge : NativeSurfaceBridge {
  private val rendererDispatcher = NativeSurfaceRendererDispatcher("compose-map-mac-metal-renderer")
  private val ring = MacMetalTextureRing()
  private var currentExtent = SurfaceExtent.Empty

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
    if (extent == currentExtent && !ring.isEmpty) {
      return
    }
    if (extent.isEmpty) {
      ring.dispose()
    } else {
      // Resolved by the caller: asking Skiko from the renderer thread waits on the event dispatch
      // thread, which is already waiting on this one.
      ring.allocate(
        extent,
        checkNotNull(skikoDevice) { "The Skiko Metal device is resolved before this hop" },
      )
    }
    currentExtent = extent
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame {
    if (ring.isEmpty || extent != currentExtent) {
      resize(extent)
    }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(extent),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  private fun target(extent: SurfaceExtent): NativeSurfaceTarget {
    val textures = ring.textures
    if (textures.isEmpty()) {
      throw NativeSurfaceBridgeException("Skiko Metal texture allocation returned null")
    }
    return MetalTextureTarget(
      texture = textures.first(),
      device = ring.device,
      pixelFormat = ring.pixelFormat,
      extent = extent,
      generation = ring.generation,
      ring = textures,
    )
  }

  override fun completeProducerAccess(frame: NativeSurfaceFrame) {
    ring.markRendered(frame.target.generation)
  }

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is MetalTextureTarget) {
      return false
    }
    // Only a texture this bridge still holds is safe to draw.
    val drawable = ring.drawable(target, TextureOrigin.TOP_LEFT) ?: return false
    return SkikoHost.drawMetalTexture(scope, drawable)
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    rendererDispatcher.run {
      ring.releaseRetiredOnceReplaced()
      MacMetalBridgeNative.runInAutoreleasePool(action)
    }

  override fun <T> withRendererAccess(action: () -> T): T = rendererDispatcher.run(action)

  override fun close() {
    try {
      rendererDispatcher.run { ring.dispose() }
    } finally {
      rendererDispatcher.close()
    }
  }
}
