package org.maplibre.nativeffi.internal.loader

import org.maplibre.nativeffi.error.AbiVersionMismatchException

/**
 * Loads the native library and checks its C ABI version once, before the first native call.
 *
 * Every generated operation calls this; after the first call it costs one volatile read.
 */
internal expect fun ensureNativeLibrary()

/** The C ABI contract version this binding expects. */
internal const val EXPECTED_C_ABI_VERSION: Long = 0L

/** Throws when the loaded library reports another C ABI version. */
internal fun checkAbiVersion(version: Long) {
  if (version != EXPECTED_C_ABI_VERSION) {
    throw AbiVersionMismatchException(version, EXPECTED_C_ABI_VERSION)
  }
}
