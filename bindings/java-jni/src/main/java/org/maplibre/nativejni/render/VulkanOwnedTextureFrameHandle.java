package org.maplibre.nativejni.render;

import java.util.Objects;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/**
 * Explicit handle for a Vulkan session-owned texture frame.
 *
 * <p>This is an advanced API for render integrations that must submit GPU work and release the
 * MapLibre-owned image only after that work no longer samples it. The frame and its native pointers
 * stay valid until {@link #close()}. Callers must synchronize GPU use before closing the handle,
 * close it on the render session owner thread, and close it before resizing, rendering another
 * update, detaching, or closing the render session.
 */
public final class VulkanOwnedTextureFrameHandle implements AutoCloseable {
  private final RenderSessionHandle session;
  private final MaplibreNativeC.mln_vulkan_owned_texture_frame nativeFrame;
  private final FrameScope scope;
  private final VulkanOwnedTextureFrame frame;
  private final FrameHandleLeakReport.Registration leakRegistration;
  private boolean closed;

  VulkanOwnedTextureFrameHandle(
      RenderSessionHandle session,
      MaplibreNativeC.mln_vulkan_owned_texture_frame nativeFrame,
      FrameScope scope,
      VulkanOwnedTextureFrame frame) {
    this.session = Objects.requireNonNull(session, "session");
    this.nativeFrame = Objects.requireNonNull(nativeFrame, "nativeFrame");
    this.scope = Objects.requireNonNull(scope, "scope");
    this.frame = Objects.requireNonNull(frame, "frame");
    this.leakRegistration = FrameHandleLeakReport.register(this, "VulkanOwnedTextureFrameHandle");
  }

  public VulkanOwnedTextureFrame frame() {
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
      session.releaseVulkanFrame(nativeFrame, null);
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
      nativeFrame.close();
    }
  }

  private void ensureOpen() {
    if (closed) {
      throw new IllegalStateException("Vulkan owned texture frame handle is closed");
    }
  }
}
