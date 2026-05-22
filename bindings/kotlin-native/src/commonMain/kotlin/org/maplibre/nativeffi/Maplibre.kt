package org.maplibre.nativeffi

/** Process-global entry points for the Kotlin/Native binding. */
public expect object Maplibre {
  /** Returns the native C ABI contract version. */
  public fun cVersion(): UInt
}
