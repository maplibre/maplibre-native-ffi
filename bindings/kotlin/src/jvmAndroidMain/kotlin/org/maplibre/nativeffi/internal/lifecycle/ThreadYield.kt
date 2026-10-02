package org.maplibre.nativeffi.internal.lifecycle

internal actual fun yieldThread() {
  Thread.yield()
}
