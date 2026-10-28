package org.maplibre.nativeffi.internal.delivery

/**
 * The delivery queue that a target names beside a registration's callbacks, when its native threads
 * cannot call Kotlin.
 *
 * A threaded target has none, and native calls each callback on its own thread. Kotlin/Wasm names
 * the browser module's one queue, whose dispatch runs each call on the page.
 */
internal expect object HostDelivery {
  /** The queue's handle, or 0 on a target whose native threads call Kotlin. */
  val queue: Long
}
