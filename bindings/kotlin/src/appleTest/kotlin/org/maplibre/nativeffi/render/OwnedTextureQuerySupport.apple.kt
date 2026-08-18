@file:OptIn(kotlinx.cinterop.BetaInteropApi::class, kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.render

import kotlinx.cinterop.ObjCObject
import kotlinx.cinterop.objcPtr
import kotlinx.cinterop.toLong
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.map.MapHandle
import platform.Metal.MTLCreateSystemDefaultDevice

internal actual object OwnedTextureQuerySupport {
  actual fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureQuerySession? {
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
    return object : OwnedTextureQuerySession {
      override val session: RenderSessionHandle = session

      override fun close() {
        session.close()
      }
    }
  }
}

private fun ObjCObject.address(): Long = objcPtr().toLong()
