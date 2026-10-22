package org.maplibre.nativeffi

import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the MapLibre Native FFI binding. */
public expect object Maplibre {
  /** The C ABI version that this binding requires; loading fails on any other. */
  public val EXPECTED_C_ABI_VERSION: Long

  /**
   * Loads the native library, or verifies it where the platform links it, and checks its C ABI
   * version.
   */
  public fun loadNativeLibrary()

  /**
   * Returns the process-lifetime address of the v1 plugin registration function. Pass it to the
   * plugin's own registration entry point before loading dependent styles.
   */
  public fun pluginRegisterFunctionV1(): NativePointer
}
