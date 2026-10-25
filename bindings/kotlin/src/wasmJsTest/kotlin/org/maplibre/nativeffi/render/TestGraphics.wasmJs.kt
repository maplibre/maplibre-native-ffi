package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.NewRenderSession
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.attachWebGpuOwnedTexture
import org.maplibre.nativeffi.generated.withWebGpuTexture

// The page is the only thread that runs Kotlin, so it drives graphics too.
internal actual val testGraphicsThread: CoroutineDispatcher = Dispatchers.Default

internal actual fun TestGraphics.attachBackendOwnedTexture(
  map: MapHandle,
  extent: LogicalExtent,
  context: TestGraphicsContext,
  options: RenderSessionAttachOptions,
): NewRenderSession =
  when (backend) {
    TestBackend.WEBGPU -> map.attachWebGpuOwnedTexture(webGpuOwnedTexture(extent, context), options)
    else -> error("this target declares no attach for $backend")
  }

internal actual fun TestGraphics.backendTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt> =
  when (backend) {
    TestBackend.WEBGPU -> frame.withWebGpuTexture { it.width to it.height }
    else -> error("this target declares no texture view for $backend")
  }
