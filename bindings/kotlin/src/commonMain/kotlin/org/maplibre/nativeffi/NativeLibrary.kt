package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.MLN_C_VERSION

/** The C ABI version that this binding requires; loading fails on any other. */
public const val EXPECTED_C_ABI_VERSION: Long = MLN_C_VERSION

/**
 * Loads the native library, or verifies it where the platform links it, and checks its C ABI
 * version.
 */
public expect fun loadNativeLibrary()
