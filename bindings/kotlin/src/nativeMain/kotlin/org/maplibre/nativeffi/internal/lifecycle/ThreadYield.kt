package org.maplibre.nativeffi.internal.lifecycle

import platform.posix.sched_yield

internal actual fun yieldThread() {
  sched_yield()
}
