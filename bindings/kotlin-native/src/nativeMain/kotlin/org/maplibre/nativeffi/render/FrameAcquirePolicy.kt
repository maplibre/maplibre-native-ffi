package org.maplibre.nativeffi.render

internal object FrameAcquirePolicy {
  fun cleanupAfterWrapperFailure(
    acquired: Boolean,
    releaseNative: () -> Unit,
    closeLocal: () -> Unit,
    failure: Throwable,
  ): Nothing {
    try {
      if (acquired) {
        runCatching { releaseNative() }
      }
    } finally {
      runCatching { closeLocal() }
    }
    throw failure
  }
}
