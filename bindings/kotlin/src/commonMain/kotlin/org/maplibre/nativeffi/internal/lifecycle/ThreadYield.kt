package org.maplibre.nativeffi.internal.lifecycle

/**
 * Yields the current thread to others while a spin lock waits. Common code spins because it has no
 * multiplatform lock, and yielding lets the holder run even on one core.
 */
internal expect fun yieldThread()
