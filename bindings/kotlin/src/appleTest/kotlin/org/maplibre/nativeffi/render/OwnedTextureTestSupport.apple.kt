@file:OptIn(kotlinx.cinterop.BetaInteropApi::class, kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.render

import kotlinx.cinterop.ObjCObject
import kotlinx.cinterop.objcPtr
import kotlinx.cinterop.toLong
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.map.MapHandle
import platform.Metal.MTLCreateSystemDefaultDevice

internal actual object OwnedTextureTestSupport {
  actual fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureTestSession? {
    if (RenderBackend.METAL !in Maplibre.supportedRenderBackends()) return null
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    val attachment =
      map.attachMetalOwnedTexture(
        MetalOwnedTextureDescriptor(
          extent = RenderTargetExtent(width, height, 1.0),
          context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
        ),
        OWNED_TEXTURE_ATTACH_OPTIONS,
      )
    return AppleOwnedTextureSession(device.address(), attachment)
  }
}

private class AppleOwnedTextureSession(
  private val deviceAddress: Long,
  override val attachment: RenderSessionAttachment,
) : OwnedTextureTestSession {
  override fun attachAnotherOwnedTexture(width: Int, height: Int): RenderSessionAttachment =
    session
      .map()
      .attachMetalOwnedTexture(
        MetalOwnedTextureDescriptor(
          extent = RenderTargetExtent(width, height, 1.0),
          context = MetalContextDescriptor(NativePointer.ofAddress(deviceAddress)),
        ),
        OWNED_TEXTURE_ATTACH_OPTIONS,
      )

  override fun frameSize(frame: AcquiredFrameHandle): OwnedTextureFrameSize {
    val texture = frame.metalTexture()
    return OwnedTextureFrameSize(texture.width(), texture.height())
  }

  override fun close() {
    session.abandonAndClose()
  }
}

private fun ObjCObject.address(): Long = objcPtr().toLong()
