package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.system.MemoryUtil.NULL

internal class MetalBorrowedTexture(graphicsContext: GraphicsContext, viewport: Viewport) :
  AutoCloseable {
  private val metalContext = graphicsContext as MetalContext
  private var texture: Long = metalContext.createBorrowedTexture(viewport)

  fun texture(): Long = texture

  override fun close() {
    if (texture != NULL) {
      metalContext.releaseObject(texture)
      texture = NULL
    }
  }
}
