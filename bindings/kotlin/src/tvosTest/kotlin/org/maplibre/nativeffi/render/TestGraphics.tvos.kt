package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.AcquiredFrameHandle
import org.maplibre.nativeffi.GpuDeviceHandle
import org.maplibre.nativeffi.QueueLock
import org.maplibre.nativeffi.withMetalTexture
import org.maplibre.nativeffi.wrapMetal

internal actual fun TestGraphics.wrapBackendDevice(
  context: TestGraphicsContext,
  queueLock: QueueLock,
): GpuDeviceHandle =
  when (backend) {
    TestBackend.METAL -> GpuDeviceHandle.wrapMetal(metalDevice(context))
    else -> error("this target declares no device wrap for $backend")
  }

internal actual fun TestGraphics.backendTextureSize(frame: AcquiredFrameHandle): Pair<UInt, UInt> =
  when (backend) {
    TestBackend.METAL -> frame.withMetalTexture { it.width to it.height }
    else -> error("this target declares no texture view for $backend")
  }
