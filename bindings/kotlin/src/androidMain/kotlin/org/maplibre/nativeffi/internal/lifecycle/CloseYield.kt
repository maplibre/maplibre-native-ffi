package org.maplibre.nativeffi.internal.lifecycle

internal actual fun yieldWhileClosing() {
  Thread.yield()
}
