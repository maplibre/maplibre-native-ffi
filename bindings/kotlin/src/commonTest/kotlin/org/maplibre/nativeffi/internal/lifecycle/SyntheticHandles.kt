package org.maplibre.nativeffi.internal.lifecycle

/**
 * Handle values for tests that exercise binding-owned bookkeeping without a live native object.
 * Each value carries the kind byte the C API assigns to the type it stands in for, and the C API
 * rejects it as a handle this process never created.
 */
internal object SyntheticHandles {
  fun resourceRequest(ordinal: Long = 1): Long = kind(0x0C) or ordinal

  private fun kind(value: Int): Long = value.toLong() shl 56
}
