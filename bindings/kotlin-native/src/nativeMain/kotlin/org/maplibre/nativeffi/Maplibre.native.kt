package org.maplibre.nativeffi

import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.internal.c.mln_c_version

/** Kotlin/Native implementation of process-global entry points. */
@OptIn(ExperimentalForeignApi::class)
public actual object Maplibre {
  public actual fun cVersion(): UInt = mln_c_version()
}
