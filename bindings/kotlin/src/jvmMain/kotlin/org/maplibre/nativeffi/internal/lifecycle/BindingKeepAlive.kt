package org.maplibre.nativeffi.internal.lifecycle

internal actual fun bindingKeepAlive(owner: Any) {
  java.lang.ref.Reference.reachabilityFence(owner)
}
