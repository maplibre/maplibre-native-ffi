package org.maplibre.nativeffi.render

/** Tracks callback-scoped access to borrowed native render frame handles. */
internal class FrameScope {
  private var active = true

  fun ensureActive() {
    check(active) { "render frame is no longer active" }
  }

  fun close() {
    active = false
  }
}
