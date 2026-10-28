package org.maplibre.nativeffi

/** A weak reference that the garbage collector clears once nothing else reaches its value. */
internal expect class TestWeakReference(value: Any) {
  /** The value, or null once the collector has cleared it. */
  val value: Any?

  val isCleared: Boolean
}

/**
 * Collects garbage until [reference] clears, and returns whether it did before the wait timeout.
 *
 * Each round blocks on the collector itself: a reference queue on the JVM and Android, or a
 * synchronous collection on Kotlin/Native. The browser clears a weak reference only after the task
 * that last read it, so on wasm each round yields to the page.
 */
internal expect suspend fun awaitCollected(reference: TestWeakReference): Boolean

/**
 * Asks the collector for a full collection, which queues the cleaners of unreachable handles. The
 * cleaners run on their own worker, so a caller still waits for what they do.
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
