package org.maplibre.nativeffi

import androidx.test.runner.AndroidJUnitRunner

class MaplibreTestRunner : AndroidJUnitRunner() {
  override fun onStart() {
    // Instrumentation context serves androidDeviceTest/assets. The target
    // application context does not.
    initAndroid(context)
    super.onStart()
  }
}
