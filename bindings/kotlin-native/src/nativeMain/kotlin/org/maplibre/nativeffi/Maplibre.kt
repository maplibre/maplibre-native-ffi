package org.maplibre.nativeffi

import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.internal.c.mln_c_version

/** Process-global entry points for the Kotlin/Native binding. */
@OptIn(ExperimentalForeignApi::class)
public object Maplibre {
  /** Returns the native C ABI contract version. */
  public fun cVersion(): UInt = mln_c_version()
}
