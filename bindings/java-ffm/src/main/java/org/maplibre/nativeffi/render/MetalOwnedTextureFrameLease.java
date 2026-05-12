package org.maplibre.nativeffi.render;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Objects;

/**
 * Explicit lease for a Metal session-owned texture frame.
 *
 * <p>This is an advanced API for render integrations that must submit GPU work and release the
 * MapLibre-owned texture only after that work no longer samples it. The frame and its native
 * pointers stay valid until {@link #close()}. Callers must synchronize GPU use before closing the
 * lease, close it on the render session owner thread, and close it before resizing, rendering
 * another update, detaching, or closing the render session.
 */
public final class MetalOwnedTextureFrameLease implements AutoCloseable {
  private final RenderSessionHandle session;
  private final Arena arena;
  private final MemorySegment frameSegment;
  private final FrameScope scope;
  private final MetalOwnedTextureFrame frame;
  private boolean closed;

  MetalOwnedTextureFrameLease(
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
    RuntimeException releaseFailure = null;
    Error releaseError = null;
    try {
      session.releaseMetalFrame(frameSegment, null);
    } catch (RuntimeException error) {
      releaseFailure = error;
      throw error;
    } catch (Error error) {
      releaseError = error;
      throw error;
    } finally {
      closed = true;
      try {
        scope.close();
      } catch (RuntimeException cleanupError) {
        if (releaseFailure != null) {
          releaseFailure.addSuppressed(cleanupError);
        } else if (releaseError != null) {
          releaseError.addSuppressed(cleanupError);
        } else {
          throw cleanupError;
        }
      } finally {
        try {
          arena.close();
        } catch (RuntimeException cleanupError) {
          if (releaseFailure != null) {
            releaseFailure.addSuppressed(cleanupError);
          } else if (releaseError != null) {
            releaseError.addSuppressed(cleanupError);
          } else {
            throw cleanupError;
          }
        }
      }
    }
  }

  private void ensureOpen() {
    if (closed) {
      throw new IllegalStateException("Metal owned texture frame lease is closed");
    }
  }
}
