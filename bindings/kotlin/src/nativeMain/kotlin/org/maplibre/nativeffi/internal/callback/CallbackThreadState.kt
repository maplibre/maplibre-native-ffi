package org.maplibre.nativeffi.internal.callback

import kotlin.native.concurrent.ThreadLocal

internal actual class CallbackThreadState actual constructor() {
  actual fun enter() {
    CallbackDepth.depth += 1
  }

  actual fun exit() {
    CallbackDepth.depth -= 1
  }

  actual fun isInCallback(): Boolean = CallbackDepth.depth > 0
}

@ThreadLocal
private object CallbackDepth {
  var depth: Int = 0
}

internal actual fun yieldCallbackClose() {
  // Kotlin/Native has no common thread-yield primitive. Callback close is rare,
  // and this loop only runs while another callback is actively exiting.
}
