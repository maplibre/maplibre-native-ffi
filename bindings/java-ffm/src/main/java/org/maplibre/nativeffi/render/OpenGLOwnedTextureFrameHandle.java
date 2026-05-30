package org.maplibre.nativeffi.render;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Objects;

/**
 * Explicit handle for an OpenGL session-owned texture frame.
 *
 * <p>The frame and texture object name stay valid until {@link #close()}. Callers must synchronize
 * GPU use before closing the handle, close it on the render session owner thread, and close it
 * before resizing, rendering another update, detaching, or closing the render session.
 */
public final class OpenGLOwnedTextureFrameHandle implements AutoCloseable {
  private final RenderSessionHandle session;
  private final Arena arena;
  private final MemorySegment frameSegment;
  private final FrameScope scope;
  private final OpenGLOwnedTextureFrame frame;
  private final FrameHandleLeakReport.Registration leakRegistration;
  private boolean closed;

  OpenGLOwnedTextureFrameHandle(
      RenderSessionHandle session,
      Arena arena,
      MemorySegment frameSegment,
      FrameScope scope,
      OpenGLOwnedTextureFrame frame) {
    this.session = Objects.requireNonNull(session, "session");
    this.arena = Objects.requireNonNull(arena, "arena");
    this.frameSegment = Objects.requireNonNull(frameSegment, "frameSegment");
    this.scope = Objects.requireNonNull(scope, "scope");
    this.frame = Objects.requireNonNull(frame, "frame");
    this.leakRegistration = FrameHandleLeakReport.register(this, "OpenGLOwnedTextureFrameHandle");
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
    try {
      session.releaseOpenGLFrame(frameSegment, null);
    } catch (Throwable releaseFailure) {
      if (session.isClosed()) {
        closeLocal();
        return;
      }
      // Keep local frame state live when native release fails so callers can retry.
      throw releaseFailure;
    }
    closeLocal();
  }

  private void closeLocal() {
    if (closed) {
      return;
    }
    closed = true;
    leakRegistration.report().markClosed();
    leakRegistration.cleanable().clean();
    try {
      scope.close();
    } finally {
      arena.close();
    }
  }

  private void ensureOpen() {
    if (closed) {
      throw new IllegalStateException("OpenGL owned texture frame handle is closed");
    }
  }
}
