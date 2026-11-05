package org.maplibre.nativeffi.internal.callback

internal actual object DeliveryThread {
  private val marked = ThreadLocal<Boolean>()

  /** Whether this thread is running an upcall. */
  val isCurrent: Boolean
    get() = marked.get() == true

  actual fun enter(): Boolean = isCurrent.also { if (!it) marked.set(true) }

  actual fun exit(outer: Boolean) {
    if (!outer) marked.set(false)
  }
}
