package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.map.MapHandle

/**
 * Kotlin/Native Android targets only compile. [RenderQueryTest] attaches an EGL owned texture from
 * the Android device suite.
 */
internal actual object OwnedTextureQuerySupport {
  actual fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureQuerySession? = null
}
