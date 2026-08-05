package org.maplibre.nativeffi.internal.lifecycle

/**
 * Yields the current thread while a close waits for in-flight work to finish. Close paths spin on
 * this because common code has no multiplatform lock; the spin is bounded by one native call.
 */
internal expect fun yieldWhileClosing()
