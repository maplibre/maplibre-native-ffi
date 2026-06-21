package org.maplibre.nativeffi.render

/** Explicit handle for a Metal session-owned texture frame. */
public expect class MetalOwnedTextureFrameHandle : AutoCloseable {
  public fun frame(): MetalOwnedTextureFrame

  public val isClosed: Boolean

  override fun close()
}
