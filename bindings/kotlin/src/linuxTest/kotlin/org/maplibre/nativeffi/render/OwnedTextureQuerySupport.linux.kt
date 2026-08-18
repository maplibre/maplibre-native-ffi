package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.map.MapHandle

/**
 * The Linux Kotlin/Native suite has no EGL pbuffer helper. The JVM suite on this host attaches the
 * owned-texture session used by [RenderQueryTest].
 */
internal actual object OwnedTextureQuerySupport {
  actual fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureQuerySession? = null
}
