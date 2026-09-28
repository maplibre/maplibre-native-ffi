package org.maplibre.nativeffi

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.toLong
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.internal.c.mln_plugin_get_register_function_v1
import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the Kotlin/Native binding. */
@OptIn(ExperimentalForeignApi::class)
public actual object Maplibre {
  /** C ABI contract version expected by this Kotlin/Native binding. */
  public actual const val EXPECTED_C_ABI_VERSION: Long = 0L

  /** Native libraries are linked by the host binary for Kotlin/Native. */
  public actual fun loadNativeLibrary() {
    checkCompatibleCAbi()
  }

  internal fun checkCompatibleCAbi(
    actualVersion: Long = org.maplibre.nativeffi.internal.c.mln_c_version().toLong()
  ) {
    if (actualVersion == EXPECTED_C_ABI_VERSION) {
      return
    }

    throw AbiVersionMismatchException(actualVersion, EXPECTED_C_ABI_VERSION)
  }

  public actual fun pluginRegisterFunctionV1(): NativePointer {
    loadNativeLibrary()
    return NativePointer.ofAddress(mln_plugin_get_register_function_v1()!!.rawValue.toLong())
  }
}
