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
   * HTTP requests or load `asset://` and `file:///android_asset/` URLs from the APK. Pass an
   * Activity or Application; `asset://` URLs read that [Context]'s AssetManager.
   */
  public fun initialize(context: Context) {
    NativeAccess.ensureLoaded()
    Status.check(AndroidNativeBridge.initialize(context))
  }
}
