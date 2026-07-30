package org.maplibre.nativeffi.internal.lifecycle

import platform.posix.sched_yield

internal actual fun yieldWhileClosing() {
  sched_yield()
}
