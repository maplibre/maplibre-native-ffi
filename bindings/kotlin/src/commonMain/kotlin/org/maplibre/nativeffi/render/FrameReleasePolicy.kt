package org.maplibre.nativeffi.render

internal object FrameReleasePolicy {
  fun close(
    isClosed: () -> Boolean,
    releaseNative: () -> Unit,
    ownerClosed: () -> Boolean,
    closeLocal: () -> Unit,
  ) {
    if (isClosed()) return
    try {
      releaseNative()
    } catch (releaseFailure: Throwable) {
      if (ownerClosed()) {
        closeLocal()
        return
      }
      // Keep local frame state live when native release fails so callers can retry.
      throw releaseFailure
    }
    closeLocal()
  }
}
