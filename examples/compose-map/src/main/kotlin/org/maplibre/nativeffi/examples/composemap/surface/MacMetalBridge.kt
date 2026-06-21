package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

internal class MacMetalBridge : PlaceholderBridge(ProducerBackend.METAL, ConsumerBackend.METAL) {
  private var texture = NativeHandle(0)
  private var pixelFormat = 0L
  private var generation = 0L
  private var currentExtent = SurfaceExtent.Empty

  override val capabilities: NativeSurfaceCapabilities =
    NativeSurfaceCapabilities(
      producerBackend = backend,
      consumerBackend = consumerBackend,
      supportsExplicitSynchronization = false,
      supportsResizeWithoutRecreate = false,
      isPlaceholder = false,
    )

  override fun resize(extent: SurfaceExtent) {
    if (extent == currentExtent && texture.address != 0L) {
      return
    }
    recreateTexture(extent)
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

  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
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
    MacMetalBridgeNative.runInAutoreleasePool(action)

  override fun close() {
    disposeTexture()
  }

  private fun recreateTexture(extent: SurfaceExtent) {
    if (extent.isEmpty) {
      disposeTexture()
      return
    }
    val metalDevice = SkikoHost.requireMetalDevice()
    val textureAddress =
      MacMetalBridgeNative.createMetalTexture(
        metalDevice = metalDevice.ptr,
        oldTexture = texture.address,
        width = extent.physicalWidth,
        height = extent.physicalHeight,
      )
    texture = NativeHandle(textureAddress)
    pixelFormat = MacMetalBridgeNative.texturePixelFormat(textureAddress)
  }

  private fun disposeTexture() {
    if (texture.address != 0L) {
      SkikoHost.forgetMetalTexture(texture)
      MacMetalBridgeNative.disposeMetalTexture(texture.address)
      texture = NativeHandle(0)
      pixelFormat = 0
    }
  }
}
