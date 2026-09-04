package org.maplibre.nativeffi.internal.lifecycle

/**
 * A blocking mutual-exclusion lock for process-lifetime registries.
 *
 * Instances live for the process, so no platform releases the underlying lock resource.
 */
internal expect class PlatformLock() {
  fun <R> withLock(block: () -> R): R
}
