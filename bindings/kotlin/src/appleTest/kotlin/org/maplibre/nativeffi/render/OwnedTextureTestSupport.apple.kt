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
    val session =
      map.attachMetalOwnedTexture(
        MetalOwnedTextureDescriptor(
          extent = RenderTargetExtent(width, height, 1.0),
          context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
        )
      )
    return AppleOwnedTextureSession(device.address(), session)
  }
}

private class AppleOwnedTextureSession(
  private val deviceAddress: Long,
  override val session: RenderSessionHandle,
) : OwnedTextureTestSession {
  override fun attachAnotherOwnedTexture(width: Int, height: Int): RenderSessionHandle =
    session
      .map()
      .attachMetalOwnedTexture(
        MetalOwnedTextureDescriptor(
          extent = RenderTargetExtent(width, height, 1.0),
          context = MetalContextDescriptor(NativePointer.ofAddress(deviceAddress)),
        )
      )

  override fun acquireFrame(): OwnedTextureTestFrame {
    val handle = session.acquireMetalOwnedTextureFrame()
    val frame = handle.frame()
    return object : OwnedTextureTestFrame {
      override val width: Int
        get() = frame.width()

      override val height: Int
        get() = frame.height()

      override val isClosed: Boolean
        get() = handle.isClosed

      override fun close() {
        handle.close()
      }
    }
  }

  override fun close() {
    session.close()
  }
}

private fun ObjCObject.address(): Long = objcPtr().toLong()
