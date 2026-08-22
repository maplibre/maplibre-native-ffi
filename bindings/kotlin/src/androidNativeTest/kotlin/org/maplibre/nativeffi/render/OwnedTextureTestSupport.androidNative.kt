package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.map.MapHandle

/**
 * Kotlin/Native Android targets only compile. The Android device suite attaches an EGL owned
 * texture for the common render tests.
 */
internal actual object OwnedTextureTestSupport {
  actual fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureTestSession? = null
}
