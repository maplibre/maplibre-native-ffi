package org.maplibre.nativejni.render;

import java.util.Objects;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/**
 * Explicit handle for an OpenGL session-owned texture frame.
 *
 * <p>This is an advanced API for render integrations that must submit GPU work and release the
 * MapLibre-owned texture only after that work no longer samples it. The frame stays valid until
 * {@link #close()}. Callers must synchronize GPU use before closing the handle, close it on the
 * render session owner thread, and close it before resizing, rendering another update, detaching,
 * or closing the render session.
 */
public final class OpenGLOwnedTextureFrameHandle implements AutoCloseable {
  private final RenderSessionHandle session;
  private final MaplibreNativeC.mln_opengl_owned_texture_frame nativeFrame;
  private final FrameScope scope;
  private final OpenGLOwnedTextureFrame frame;
  private boolean closed;

  OpenGLOwnedTextureFrameHandle(
      RenderSessionHandle session,
      MaplibreNativeC.mln_opengl_owned_texture_frame nativeFrame,
      FrameScope scope,
      OpenGLOwnedTextureFrame frame) {
    this.session = Objects.requireNonNull(session, "session");
    this.nativeFrame = Objects.requireNonNull(nativeFrame, "nativeFrame");
    this.scope = Objects.requireNonNull(scope, "scope");
    this.frame = Objects.requireNonNull(frame, "frame");
  }

  public OpenGLOwnedTextureFrame frame() {
    ensureOpen();
    return frame;
  }

  public boolean isClosed() {
    return closed;
  }

  @Override
  public void close() {
    if (closed) {
      return;
    }
    session.releaseOpenGLFrame(nativeFrame, null);
    closed = true;
    try {
      scope.close();
    } finally {
      nativeFrame.close();
    }
  }

  private void ensureOpen() {
    if (closed) {
      throw new IllegalStateException("OpenGL owned texture frame handle is closed");
    }
  }
}
