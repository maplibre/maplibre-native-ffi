package org.maplibre.nativeffi.internal.callback

/**
 * Tracks whether the caller is inside one of this binding's callbacks.
 *
 * Every other platform makes this thread-local because native can invoke a callback on any thread.
 * A Kotlin/Wasm module runs on one thread, so a plain counter is the whole implementation: a
 * callback reaches Kotlin only by entering this agent, and nothing else can be running when it
 * does.
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
