package org.maplibre.nativeffi

import android.content.Context
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.Jni
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the MapLibre Native FFI binding. */
public actual object Maplibre {
  public actual const val EXPECTED_C_ABI_VERSION: Long =
    org.maplibre.nativeffi.internal.loader.EXPECTED_C_ABI_VERSION

  public actual fun loadNativeLibrary() {
    ensureNativeLibrary()
  }

  public actual fun getPluginRegisterFunctionV1(): NativePointer {
    ensureNativeLibrary()
    return NativePointer.ofAddress(C.mln_plugin_get_register_function_v1())
  }

  /**
   * Initializes Android platform services.
   *
   * Required once during Android host setup, before creating a runtime. Pass an Activity or
   * Application; APK assets use that [Context]'s AssetManager.
   */
  public fun initialize(context: Context) {
    ensureNativeLibrary()
    NativeDiagnostics.check { diagnostic -> Jni.androidInit(context, diagnostic) }
  }
}
