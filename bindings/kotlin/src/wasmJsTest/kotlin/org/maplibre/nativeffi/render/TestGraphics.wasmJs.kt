package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import org.maplibre.nativeffi.AcquiredFrameHandle
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.attachWebGpuBorrowedTexture
import org.maplibre.nativeffi.withWebGpuTexture

// The page is the only thread that runs Kotlin, so it drives graphics too.
internal actual val testGraphicsThread: CoroutineDispatcher = Dispatchers.Default

internal actual fun TestGraphics.attachBackendBorrowedTexture(
  map: MapHandle,
  textures: List<TestTexture>,
  context: TestGraphicsContext,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  when (backend) {
    TestBackend.WEBGPU ->
      map.attachWebGpuBorrowedTexture(webGpuBorrowedTexture(textures, context), options)
    else -> error("this target declares no attach for $backend")
  }

internal actual fun TestGraphics.backendTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt> =
  when (backend) {
    TestBackend.WEBGPU -> frame.withWebGpuTexture { it.width to it.height }
    else -> error("this target declares no texture view for $backend")
  }
