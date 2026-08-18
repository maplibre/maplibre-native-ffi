package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.map.MapHandle

/**
 * The Linux Kotlin/Native suite has no EGL pbuffer helper. The JVM suite on this host attaches the
 * owned-texture session used by the common render tests.
 */
internal actual object OwnedTextureTestSupport {
  actual fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureTestSession? = null
}
