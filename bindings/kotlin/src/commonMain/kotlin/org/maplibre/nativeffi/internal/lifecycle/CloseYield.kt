package org.maplibre.nativeffi.internal.lifecycle

/**
 * Yields the current thread while a close waits for in-flight work to finish.
 *
 * Close paths that drain an active-use count spin on this rather than blocking on a monitor,
 * because common code has no multiplatform lock. Every such drain waits only for calls that have
 * already entered native code, so the spin is bounded by one native call.
 */
internal expect fun yieldWhileClosing()
