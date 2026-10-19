package org.maplibre.nativeffi.internal.async

/** Transfers a native result to its wrapper, releasing it if wrapper construction fails. */
internal fun <T, R> adoptOwned(value: T, dispose: (T) -> Unit, adopt: (T) -> R): R =
  try {
    adopt(value)
  } catch (failure: Throwable) {
    try {
      dispose(value)
    } catch (cleanup: Throwable) {
      failure.addSuppressed(cleanup)
    }
    throw failure
  }
