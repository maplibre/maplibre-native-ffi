package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.Status

/** Platform-neutral closed-state and leak reporting for session-owned texture frame handles. */
@OptIn(ExperimentalAtomicApi::class)
internal class OwnedTextureFrameHandleCore(private val typeName: String) {
  private val closed = AtomicInt(0)

  fun isClosed(): Boolean = closed.load() != 0

  fun ensureOpen() {
    if (isClosed()) throw Status.released(typeName)
  }

  fun close(releaseNative: () -> Unit, ownerClosed: () -> Boolean, releaseLocal: () -> Unit) {
    FrameReleasePolicy.close(
      isClosed = ::isClosed,
      releaseNative = releaseNative,
      ownerClosed = ownerClosed,
      closeLocal = { closeLocal(releaseLocal) },
    )
  }

  fun reportLeak(writeLine: (String) -> Unit = { message -> println(message) }) {
    if (!isClosed()) {
      writeLine(
        "Leaked $typeName; close frame handles explicitly on the render session owner thread."
      )
    }
  }

  private fun closeLocal(releaseLocal: () -> Unit) {
    if (!closed.compareAndSet(0, 1)) return
    releaseLocal()
  }
}
