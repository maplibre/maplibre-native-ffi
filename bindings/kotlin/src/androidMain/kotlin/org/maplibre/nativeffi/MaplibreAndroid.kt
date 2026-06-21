package org.maplibre.nativeffi

import android.content.Context
import org.maplibre.nativeffi.internal.javacpp.AndroidNativeBridge
import org.maplibre.nativeffi.internal.status.Status

/** Android-only platform integration entry points. */
public object MaplibreAndroid {
  /**
   * Initializes Android platform services that require an app [Context].
   *
   * This forwards to `mln_android_init`. Call it before creating a runtime that may issue Android
   * HTTP requests.
   */
  public fun initialize(context: Context) {
    NativeAccess.ensureLoaded()
    val appContext = context.applicationContext ?: context
    Status.check(AndroidNativeBridge.initialize(appContext))
  }
}
