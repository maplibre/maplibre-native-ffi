package org.maplibre.nativeffi.internal.callback

/**
 * Tracks whether the caller is inside one of this binding's callbacks.
 *
 * Every other platform makes this thread-local because native can invoke a callback on any thread.
 * A Kotlin/Wasm module runs on one thread, so a plain counter is what is left -- but only for a
 * callback that runs to completion once it has been entered, and that is a condition rather than a
 * property of the target. One thread here carries more than one stack. A body that parks unwinds to
 * the event loop with its count still raised, and whatever the page runs next arrives on a stack of
 * its own; this would report that stack as being inside a callback it never entered.
 *
 * So the invariant is that a gate whose bodies can park does not ask this. Every synchronous
 * callback family satisfies the condition -- `CallbackScope` refuses a suspension inside one, and
 * the log drain runs to the end of its own task -- and the one family that can park, the custom
 * geometry tile callbacks, closes with [CallbackGate.closeWithoutDraining], which asks nothing
 * about which stack is closing.
 *
 * The depth is a count rather than a flag because a callback may legally reach native code that
 * dispatches another callback before the first returns.
 */
internal actual class CallbackThreadState actual constructor() {
  private var depth = 0

  actual fun enter() {
    depth++
  }

  actual fun exit() {
    if (depth > 0) depth--
  }

  actual fun isInCallback(): Boolean = depth > 0
}
