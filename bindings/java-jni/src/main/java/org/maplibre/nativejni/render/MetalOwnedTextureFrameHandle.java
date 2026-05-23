package org.maplibre.nativejni.render;

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
  private final long[] longs;
  private final int[] ints;
  private final double[] doubles;
  private final FrameScope scope;
  private final MetalOwnedTextureFrame frame;
  private boolean closed;

  MetalOwnedTextureFrameHandle(
      RenderSessionHandle session,
      long[] longs,
      int[] ints,
      double[] doubles,
      FrameScope scope,
      MetalOwnedTextureFrame frame) {
    this.session = Objects.requireNonNull(session, "session");
    this.longs = Objects.requireNonNull(longs, "longs").clone();
    this.ints = Objects.requireNonNull(ints, "ints").clone();
    this.doubles = Objects.requireNonNull(doubles, "doubles").clone();
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
    session.releaseMetalFrame(longs, ints, doubles, null);
    closed = true;
    scope.close();
  }

  private void ensureOpen() {
    if (closed) {
      throw new IllegalStateException("Metal owned texture frame handle is closed");
    }
  }
}
