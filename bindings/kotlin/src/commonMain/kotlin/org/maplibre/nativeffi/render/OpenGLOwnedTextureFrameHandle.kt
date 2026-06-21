package org.maplibre.nativeffi.render

/** Explicit handle for an OpenGL session-owned texture frame. */
public expect class OpenGLOwnedTextureFrameHandle : AutoCloseable {
  public fun frame(): OpenGLOwnedTextureFrame

  public val isClosed: Boolean

  override fun close()
}
