package org.maplibre.nativejni.render;

import java.util.Objects;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.lifecycle.HandleState;

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
  private final MaplibreNativeC.mln_metal_owned_texture_frame nativeFrame;
  private final FrameScope scope;
  private final MetalOwnedTextureFrame frame;
  private final HandleState.ChildRetention sessionRetention;
  private final FrameHandleLeakReport.Registration leakRegistration;
  private boolean closed;

  MetalOwnedTextureFrameHandle(
      RenderSessionHandle session,
      MaplibreNativeC.mln_metal_owned_texture_frame nativeFrame,
      FrameScope scope,
      MetalOwnedTextureFrame frame) {
    this.session = Objects.requireNonNull(session, "session");
    var retention = session.retainChild("MetalOwnedTextureFrameHandle");
    try {
      this.nativeFrame = Objects.requireNonNull(nativeFrame, "nativeFrame");
      this.scope = Objects.requireNonNull(scope, "scope");
      this.frame = Objects.requireNonNull(frame, "frame");
      this.sessionRetention = retention;
      this.leakRegistration =
          FrameHandleLeakReport.register(this, "MetalOwnedTextureFrameHandle", retention);
    } catch (RuntimeException | Error error) {
      retention.close();
      throw error;
    }
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
    try {
      session.releaseMetalFrame(nativeFrame, null);
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
    sessionRetention.close();
    leakRegistration.report().markClosed();
    leakRegistration.cleanable().clean();
    try {
      scope.close();
    } finally {
      nativeFrame.close();
    }
  }

  private void ensureOpen() {
    if (closed) {
      throw new IllegalStateException("Metal owned texture frame handle is closed");
    }
  }
}
