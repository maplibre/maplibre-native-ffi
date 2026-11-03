package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.AcquiredFrameHandle
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.NewRenderSession
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.attachMetalBorrowedTexture
import org.maplibre.nativeffi.withMetalTexture

internal actual fun TestGraphics.attachBackendBorrowedTexture(
  map: MapHandle,
  textures: List<TestTexture>,
  context: TestGraphicsContext,
  options: RenderSessionAttachOptions,
): NewRenderSession =
  when (backend) {
    TestBackend.METAL -> map.attachMetalBorrowedTexture(metalBorrowedTexture(textures), options)
    else -> error("this target declares no attach for $backend")
  }

internal actual fun TestGraphics.backendTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt> =
  when (backend) {
    TestBackend.METAL -> frame.withMetalTexture { it.width to it.height }
    else -> error("this target declares no texture view for $backend")
  }
