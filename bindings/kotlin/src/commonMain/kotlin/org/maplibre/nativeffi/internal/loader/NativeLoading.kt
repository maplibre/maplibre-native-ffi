package org.maplibre.nativeffi.internal.loader

import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.internal.c.MLN_C_VERSION

/** Loads the native library and checks its C ABI version once, before the first native call. */
internal expect fun ensureNativeLibrary()

/** Loads the native library without checking its C ABI version, for a call that reads no struct. */
internal expect fun loadNativeLibraryUnchecked()

/** The C ABI contract version this binding expects. */
internal const val EXPECTED_C_ABI_VERSION: Long = MLN_C_VERSION

/** Throws when the loaded library reports another C ABI version. */
internal fun checkAbiVersion(version: Long) {
  if (version != EXPECTED_C_ABI_VERSION) {
    throw AbiVersionMismatchException(version, EXPECTED_C_ABI_VERSION)
  }
}
