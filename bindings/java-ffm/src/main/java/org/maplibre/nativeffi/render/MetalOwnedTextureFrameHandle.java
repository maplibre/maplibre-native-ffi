package org.maplibre.nativeffi.render;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Objects;

/**
 * Explicit handle for a Metal session-owned texture frame.
 *
 * <p>This is an advanced API for render integrations that must submit GPU work and release the
 * MapLibre-owned texture only after that work no longer samples it. The frame and its native
 * pointers stay valid until {@link #close()}. Callers must synchronize GPU use before closing the
 * handle, close it on the render session owner thread, and close it before resizing, rendering
 * another update, detaching, or closing the render session.
 */
public final class MetalOwnedTextureFrameHandle implements AutoCloseable {
  private final RenderSessionHandle session;
  private final Arena arena;
  private final MemorySegment frameSegment;
  private final FrameScope scope;
  private final MetalOwnedTextureFrame frame;
  private boolean closed;

  MetalOwnedTextureFrameHandle(
      RenderSessionHandle session,
      Arena arena,
      MemorySegment frameSegment,
      FrameScope scope,
      MetalOwnedTextureFrame frame) {
    this.session = Objects.requireNonNull(session, "session");
    this.arena = Objects.requireNonNull(arena, "arena");
    this.frameSegment = Objects.requireNonNull(frameSegment, "frameSegment");
    this.scope = Objects.requireNonNull(scope, "scope");
    this.frame = Objects.requireNonNull(frame, "frame");
  }

  public MetalOwnedTextureFrame frame() {
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
    session.releaseMetalFrame(frameSegment, null);
    closed = true;
    try {
      scope.close();
    } finally {
      arena.close();
    }
  }

  private void ensureOpen() {
    if (closed) {
      throw new IllegalStateException("Metal owned texture frame handle is closed");
    }
  }
}
