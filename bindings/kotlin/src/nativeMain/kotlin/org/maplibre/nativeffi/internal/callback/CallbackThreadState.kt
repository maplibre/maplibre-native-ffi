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
