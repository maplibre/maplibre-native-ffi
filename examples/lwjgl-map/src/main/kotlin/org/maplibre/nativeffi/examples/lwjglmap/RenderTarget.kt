package org.maplibre.nativeffi.examples.lwjglmap

internal interface RenderTarget : AutoCloseable {
  fun needsReattachOnResize(): Boolean = false

  fun needsMetalAutoreleasePool(): Boolean = false

  fun reattach(viewport: Viewport) {
    throw IllegalStateException("render target does not support reattachment")
  }

  fun resize(viewport: Viewport)

  /** Renders the latest map update. Returns false when no update is available yet. */
  fun renderUpdate(): Boolean

  override fun close()
}
