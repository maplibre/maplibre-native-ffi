package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner

/** Explicit handle for an OpenGL session-owned texture frame. */
public actual class OpenGLOwnedTextureFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val nativeFrame: MaplibreNativeC.mln_opengl_owned_texture_frame,
  private val scope: FrameScope,
  private val frameValue: OpenGLOwnedTextureFrame,
) : AutoCloseable {
  private val core = OwnedTextureFrameHandleCore("OpenGLOwnedTextureFrameHandle")

  init {
    HandleLeakCleaner.registerFrame(this, core)
  }

  public actual fun frame(): OpenGLOwnedTextureFrame {
    core.ensureOpen()
    return frameValue
  }

  public actual val isClosed: Boolean
    get() = core.isClosed()

  public actual override fun close() {
    core.close(
      releaseNative = { session.releaseOpenGLFrame(nativeFrame) },
      ownerClosed = { session.isClosed },
      releaseLocal = ::releaseLocal,
    )
  }

  private fun releaseLocal() {
    try {
      scope.close()
    } finally {
      try {
        nativeFrame.close()
      } finally {
        session.finishFrameBorrow()
      }
    }
  }
}
