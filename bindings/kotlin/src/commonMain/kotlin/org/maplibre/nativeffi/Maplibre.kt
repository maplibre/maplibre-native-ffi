package org.maplibre.nativeffi

import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the MapLibre Native FFI binding. */
public expect object Maplibre {
  /** C ABI contract version expected by this binding. */
  public val EXPECTED_C_ABI_VERSION: Long

  /** Loads or verifies access to the native library for the current platform. */
  public fun loadNativeLibrary()

  /**
   * Returns the process-lifetime address of the v1 plugin registration function. Pass it to the
   * plugin's own registration entry point before loading dependent styles.
   */
  public fun pluginRegisterFunctionV1(): NativePointer
}
