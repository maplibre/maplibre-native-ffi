package org.maplibre.nativeffi.example.host

import android.content.Context
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.turbomodule.core.interfaces.BindingsInstallerHolder
import com.facebook.react.turbomodule.core.interfaces.TurboModuleWithJSIBindings

/**
 * Installs the binding and the surface registry when JavaScript first loads this module, on the
 * JavaScript thread.
 *
 * React Native invalidates the module after it stops the JavaScript thread and before it destroys
 * the runtime, so the binding's cleanup runs there.
 */
@ReactModule(name = MaplibreHostModule.NAME)
class MaplibreHostModule(context: ReactApplicationContext) :
  NativeMaplibreHostSpec(context), TurboModuleWithJSIBindings {

  override fun getBindingsInstaller(): BindingsInstallerHolder =
    nativeBindingsInstaller(reactApplicationContext.applicationContext)

  override fun invalidate() {
    nativeCleanup()
    super.invalidate()
  }

  companion object {
    const val NAME = "MaplibreHost"

    init {
      System.loadLibrary("maplibreexamplehost")
    }

    @JvmStatic
    private external fun nativeBindingsInstaller(context: Context): BindingsInstallerHolder

    @JvmStatic private external fun nativeCleanup()
  }
}
