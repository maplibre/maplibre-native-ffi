package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

internal class MacMetalBridge : NativeSurfaceBridge {
  private val rendererDispatcher = NativeSurfaceRendererDispatcher("compose-map-mac-metal-renderer")
  private var texture = NativeHandle(0)
  private var pixelFormat = 0L
  private var generation = 0L
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
    val metalDevice = if (extent.isEmpty) null else SkikoHost.requireMetalDevice()
    rendererDispatcher.run { resizeOnRendererThread(extent, metalDevice) }
  }

  private fun resizeOnRendererThread(extent: SurfaceExtent, metalDevice: SkikoMetalDevice? = null) {
    if (extent == currentExtent && texture.address != 0L) {
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
      pixelFormat = pixelFormat,
      extent = extent,
      generation = generation,
    )

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is MetalTextureTarget || target.texture.address == 0L) {
      return false
    }
    return SkikoHost.drawMetalTexture(scope, target)
  }

  override fun <T> withProducerAccess(frame: NativeSurfaceFrame, action: () -> T): T =
    rendererDispatcher.run {
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

  private fun recreateTexture(extent: SurfaceExtent, metalDevice: SkikoMetalDevice? = null) {
    if (extent.isEmpty) {
      disposeTexture()
      return
    }
    val oldTexture = texture
    val textureAddress =
      MacMetalBridgeNative.createMetalTexture(
        metalDevice = (metalDevice ?: SkikoHost.requireMetalDevice()).ptr,
        oldTexture = oldTexture.address,
        width = extent.physicalWidth,
        height = extent.physicalHeight,
      )
    if (textureAddress != oldTexture.address) {
      releaseMetalTexture(oldTexture)
    }
    texture = NativeHandle(textureAddress)
    pixelFormat = MacMetalBridgeNative.texturePixelFormat(textureAddress)
  }

  private fun disposeTexture() {
    releaseMetalTexture(texture)
    texture = NativeHandle(0)
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
