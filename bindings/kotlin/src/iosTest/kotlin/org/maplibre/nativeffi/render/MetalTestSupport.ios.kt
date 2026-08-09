@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.render

import platform.Metal.MTLBlitCommandEncoderProtocol
import platform.Metal.MTLTextureProtocol

internal actual fun MTLBlitCommandEncoderProtocol.synchronizeTextureForCpu(
  texture: MTLTextureProtocol
) {
  // iOS textures use shared storage, so waiting for the command buffer makes
  // the rendered bytes available to the CPU.
}
