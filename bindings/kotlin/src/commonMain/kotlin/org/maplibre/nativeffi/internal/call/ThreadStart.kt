package org.maplibre.nativeffi.internal.call

/**
 * Runs a C call that returns only once a native thread that it starts is running.
 *
 * A threaded target runs the call on the calling thread, which waits. A browser page cannot wait,
 * because a thread that it starts runs only after the page yields, so Kotlin/Wasm runs the call on
 * a thread of its own and suspends until the call returns.
 */
internal expect object ThreadStart {
  /** Runs [call] and returns its status. The wait does not cancel. */
  suspend fun run(call: () -> Int): Int
}
