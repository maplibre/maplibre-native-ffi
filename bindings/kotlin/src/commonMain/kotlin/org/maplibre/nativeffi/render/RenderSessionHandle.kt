package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.map.MapHandle

/** Owns a render session and retains its map through native retirement. */
public expect class RenderSessionHandle :
  org.maplibre.nativeffi.generated.GeneratedRenderSessionOperations, AutoCloseable {
  public val isClosed: Boolean

  public fun map(): MapHandle

  override fun close()
}

/** Owns an acquired frame; generated view callbacks retain graphics resources during host use. */
public expect class AcquiredFrameHandle :
  org.maplibre.nativeffi.generated.GeneratedAcquiredFrameOperations {
  public val isReleased: Boolean
}
