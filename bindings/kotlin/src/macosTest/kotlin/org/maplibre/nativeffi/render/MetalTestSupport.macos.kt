@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.render

import platform.Metal.MTLBlitCommandEncoderProtocol
import platform.Metal.MTLTextureProtocol

internal actual fun MTLBlitCommandEncoderProtocol.synchronizeTextureForCpu(
  texture: MTLTextureProtocol
) {
  synchronizeResource(texture)
}
