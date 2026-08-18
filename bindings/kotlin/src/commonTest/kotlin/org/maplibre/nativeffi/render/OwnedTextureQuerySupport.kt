package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.map.MapHandle

/** Backend-owned texture session plus the graphics context that keeps it valid. */
internal interface OwnedTextureQuerySession : AutoCloseable {
  val session: RenderSessionHandle
}

/**
 * Attaches an owned-texture session for query tests.
 *
 * Returns null when this source set has no fixture for the loaded native backend, so the common
 * query tests stay compiled on every target.
 */
internal expect object OwnedTextureQuerySupport {
  fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureQuerySession?
}
