package org.maplibre.nativeffi.internal.loader

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.c.C

@OptIn(ExperimentalAtomicApi::class) private val abiChecked = AtomicInt(0)

/** The host binary links the library, so only its C ABI version needs checking. */
@OptIn(ExperimentalAtomicApi::class)
internal actual fun ensureNativeLibrary() {
  if (abiChecked.load() != 0) return
  checkAbiVersion(C.mln_c_version().toUInt().toLong())
  abiChecked.store(1)
}
