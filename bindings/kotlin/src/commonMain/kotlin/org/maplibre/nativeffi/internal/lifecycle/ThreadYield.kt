package org.maplibre.nativeffi.internal.lifecycle

/** Yields the current thread while a spin lock waits, so the holder runs even on one core. */
internal expect fun yieldThread()
