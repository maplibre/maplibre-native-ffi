package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.generated.MapHandle

internal actual object OwnedTextureTestSupport {
  actual fun attach(
    map: MapHandle,
    width: Int,
    height: Int,
    textureRingDepth: UInt,
  ): OwnedTextureTestSession = attachDesktopOwnedTexture(map, width, height, textureRingDepth)
}
