package org.maplibre.nativeffi.internal.callback

/**
 * Marks a thread while it runs an upcall from native. Native may need that thread back to complete
 * a result, so a blocking wait there could deadlock.
 */
internal expect object DeliveryThread {
  /** Marks this thread, and returns whether an outer upcall had marked it already. */
  fun enter(): Boolean

  /** Restores the mark that the matching [enter] returned. */
  fun exit(outer: Boolean)
}

/** Runs [block] with this thread marked as running an upcall. */
internal inline fun <T> delivering(block: () -> T): T {
  val outer = DeliveryThread.enter()
  try {
    return block()
  } finally {
    DeliveryThread.exit(outer)
  }
}
