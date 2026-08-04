package org.maplibre.nativeffi.internal.lifecycle

import org.maplibre.nativeffi.internal.status.Status

/**
 * On the browser there is no other Kotlin thread to yield to.
 *
 * WebAssembly garbage-collected references cannot be shared between agents, so a Kotlin/Wasm module
 * runs on exactly one thread and no second Kotlin caller can hold the active-use count this drain
 * waits on. Yielding would therefore accomplish nothing.
 *
 * That leaves one way for the count to be non-zero here: a frame further up this same stack holds
 * it. Under this binding's suspension model that is reachable -- a use count held across a parked
 * call is still held while a callback re-enters Kotlin and closes the same handle -- and spinning
 * on it can never converge, because the frame that would release it cannot run until this one
 * returns. The invariant is that no use count is held across a suspending call.
 *
 * So this reports the violated invariant instead of hanging. A diagnosable error beats a page that
 * stops responding with no indication of why.
 */
private const val SPIN_BUDGET = 1024

private var spins = 0

internal actual fun yieldWhileClosing() {
  spins++
  if (spins < SPIN_BUDGET) return
  spins = 0
  throw Status.invalidState(
    "A close is waiting for in-flight work that cannot finish on this thread. A handle's use " +
      "count was held across a suspending call, which the browser binding does not permit."
  )
}
