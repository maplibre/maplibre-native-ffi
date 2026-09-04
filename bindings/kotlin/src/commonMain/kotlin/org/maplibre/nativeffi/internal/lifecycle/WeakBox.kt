package org.maplibre.nativeffi.internal.lifecycle

/**
 * A reference that leaves its value collectable.
 *
 * Registries that outlive the handles they route to hold their entries through this box, so an
 * entry never keeps its handle reachable and never suppresses the unreachable-handle cleanup.
 */
internal expect class WeakBox<T : Any>(value: T) {
  /** Returns the value, or null once it became unreachable. */
  fun get(): T?
}
