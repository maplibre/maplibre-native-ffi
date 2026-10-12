package org.maplibre.nativeffi.internal.callback

import kotlin.native.concurrent.ThreadLocal

@ThreadLocal
internal actual object CallbackContext {
  actual var current: CallbackScope? = null
}
