package org.maplibre.nativeffi.internal.lifecycle

// Kotlin runs on one thread in the browser, so a spin lock never waits for another holder.
internal actual fun yieldThread() {}
