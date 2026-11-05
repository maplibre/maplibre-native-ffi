package org.maplibre.nativeffi.internal.call

/**
 * Runs a C call that returns only once a native thread that it starts is running.
 *
 * A threaded target runs the call on the calling thread, which waits. A browser page cannot wait,
 * because a thread that it starts runs only after the page yields, so Kotlin/Wasm runs the call on
 * a thread of its own and reports its status from a later task.
 */
internal expect object ThreadStart {
  /** Runs [call] and passes its status to [done] once it returns. */
  fun run(call: () -> Int, done: (Int) -> Unit)
}
