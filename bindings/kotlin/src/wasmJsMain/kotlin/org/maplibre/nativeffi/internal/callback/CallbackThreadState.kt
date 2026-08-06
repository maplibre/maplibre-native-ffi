package org.maplibre.nativeffi.internal.callback

// A plain count where the other targets keep a thread local: Kotlin/Wasm has one thread, and every
// callback body reaches it from the ring drain on that thread, so this count belongs to it.
internal actual class CallbackThreadState actual constructor() {
  private var depth = 0

  actual fun enter() {
    depth += 1
  }

  actual fun exit() {
    depth -= 1
  }

  actual fun isInCallback(): Boolean = depth > 0
}
