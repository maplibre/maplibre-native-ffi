package org.maplibre.nativeffi.render

/**
 * Scaffold for the browser OpenGL session-owned texture frame handle.
 *
 * Every member throws. The actual exists so the `wasmJs` source set compiles while the browser
 * binding is filled in one file at a time; nothing here is finished work.
 */
public actual class OpenGLOwnedTextureFrameHandle private constructor() : AutoCloseable {
  public actual fun frame(): OpenGLOwnedTextureFrame =
    throw NotImplementedError("wasmJs OpenGLOwnedTextureFrameHandle.frame is not implemented yet")

  public actual val isClosed: Boolean
    get() =
      throw NotImplementedError(
        "wasmJs OpenGLOwnedTextureFrameHandle.isClosed is not implemented yet"
      )

  public actual override fun close() {
    throw NotImplementedError("wasmJs OpenGLOwnedTextureFrameHandle.close is not implemented yet")
  }
}
