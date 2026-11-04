package org.maplibre.nativeffi.example.compose

import android.app.Application
import org.maplibre.nativeffi.initAndroid

class ExampleApplication : Application() {
  override fun onCreate() {
    super.onCreate()
    // Once per process, before the first runtime.
    initAndroid(this)
  }
}
