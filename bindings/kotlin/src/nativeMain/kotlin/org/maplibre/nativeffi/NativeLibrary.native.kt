package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary

/** Checks the C ABI version of the library that the runtime KLIB links into the binary. */
public actual fun loadNativeLibrary() {
  ensureNativeLibrary()
}
