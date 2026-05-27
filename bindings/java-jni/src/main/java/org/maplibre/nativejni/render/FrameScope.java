package org.maplibre.nativejni.render;

/** Tracks callback-scoped access to borrowed native render frame handles. */
final class FrameScope {
  private boolean active = true;

  synchronized void ensureActive() {
    if (!active) {
      throw new IllegalStateException("render frame is no longer active");
    }
  }

  synchronized void close() {
    active = false;
  }
}
