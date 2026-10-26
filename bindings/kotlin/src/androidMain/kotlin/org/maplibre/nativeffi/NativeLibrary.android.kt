package org.maplibre.nativeffi

import android.content.Context
import org.maplibre.nativeffi.internal.c.Jni
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

public actual fun loadNativeLibrary() {
  ensureNativeLibrary()
}

/**
 * Initializes Android platform services.
 *
 * Required once during Android host setup, before creating a runtime. Pass an Activity or
 * Application; APK assets use that [Context]'s AssetManager.
 */
public fun initAndroid(context: Context) {
  ensureNativeLibrary()
  NativeDiagnostics.check { diagnostic -> Jni.androidInit(context, diagnostic) }
}
