package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.RenderBackendFlag
import org.maplibre.nativeffi.map.MapHandle

internal actual object OwnedTextureTestSupport {
  actual fun attach(
    map: MapHandle,
    width: Int,
    height: Int,
    textureRingDepth: UInt,
  ): OwnedTextureTestSession =
    if (RenderBackendFlag.METAL in GeneratedApi.supportedRenderBackendMask())
      attachAppleMetal(map, width, height, textureRingDepth)
    else attachDesktopOwnedTexture(map, width, height, textureRingDepth)
}
