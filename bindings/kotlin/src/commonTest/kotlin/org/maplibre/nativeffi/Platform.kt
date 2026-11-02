package org.maplibre.nativeffi

/** A weak reference, which lets a test reach an object without keeping it reachable. */
internal expect class TestWeakReference(value: Any) {
  /** The value, or null once the collector has cleared it. */
  val value: Any?
}

/**
 * Asks the collector for a full collection. The JVM may ignore the request, so a test asserts only
 * what must hold whether or not anything was collected.
 */
internal expect suspend fun requestCollection()

/**
 * Offers each throwable that reaches the platform's handler, where the binding reports callback
 * failures and leaked handles, to [sink], which returns whether it took it. The previous handler
 * receives the rest. Returns the function that restores the previous handler, or null on Android,
 * where the binding's reports go to the log.
 */
internal expect fun interceptUncaught(sink: (Throwable) -> Boolean): (() -> Unit)?

/**
 * Waits until native's releases and deliveries so far have reached Kotlin. A threaded target runs
 * each one inside native's call, so this returns at once. On wasm, native queues each one, and the
 * page dispatches them in order on later tasks.
 */
internal expect suspend fun awaitDeliveries()
