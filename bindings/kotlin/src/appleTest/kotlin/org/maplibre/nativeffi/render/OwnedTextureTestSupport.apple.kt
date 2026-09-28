@file:OptIn(kotlinx.cinterop.BetaInteropApi::class, kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.render

import kotlinx.cinterop.ObjCObject
import kotlinx.cinterop.objcPtr
import kotlinx.cinterop.toLong
import org.maplibre.nativeffi.generated.MetalContextDescriptor
import org.maplibre.nativeffi.generated.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.RenderSessionAttachment
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.map.MapHandle
import platform.Metal.MTLCreateSystemDefaultDevice

internal fun attachAppleMetal(
  map: MapHandle,
  width: Int,
  height: Int,
  textureRingDepth: UInt,
): OwnedTextureTestSession {
  val device = MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
  val attachment =
    map.metalOwnedTextureAttach(
      MetalOwnedTextureDescriptor(
        extent = RenderTargetExtent(width.toUInt(), height.toUInt(), 1.0),
        context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
      ),
      OWNED_TEXTURE_ATTACH_OPTIONS.copy(requestedTextureRingDepth = textureRingDepth),
    )
  return AppleOwnedTextureSession(device, attachment)
}

private class AppleOwnedTextureSession(
  private val device: platform.Metal.MTLDeviceProtocol,
  override val attachment: RenderSessionAttachment,
) : OwnedTextureTestSession {
  override fun attachAnotherOwnedTexture(width: Int, height: Int): RenderSessionAttachment =
    session
      .map()
      .metalOwnedTextureAttach(
        MetalOwnedTextureDescriptor(
          extent = RenderTargetExtent(width.toUInt(), height.toUInt(), 1.0),
          context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
        ),
        OWNED_TEXTURE_ATTACH_OPTIONS,
      )

  override fun frameSize(frame: AcquiredFrameHandle): OwnedTextureFrameSize {
    return frame.withGetMetalTexture { texture ->
      OwnedTextureFrameSize(texture.width.toInt(), texture.height.toInt())
    }
  }

  override fun close() {
    session.abandonAndClose()
  }
}

private fun ObjCObject.address(): Long = objcPtr().toLong()
