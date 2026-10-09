package org.maplibre.nativeffi

import android.content.Context
import org.maplibre.nativeffi.internal.javacpp.AndroidNativeBridge
import org.maplibre.nativeffi.internal.status.Status

/** Android-only platform integration entry points. */
public object MaplibreAndroid {
  /**
   * Initializes Android platform services.
   *
   * Required once during Android host setup, before creating a runtime. Pass an Activity or
   * Application; APK assets use that [Context]'s AssetManager.
   */
  public fun initialize(context: Context) {
    NativeAccess.ensureLoaded()
    Status.check(AndroidNativeBridge.initialize(context))
  }
}
