package org.maplibre.nativeffi

import androidx.test.runner.AndroidJUnitRunner

class MaplibreTestRunner : AndroidJUnitRunner() {
  override fun onStart() {
    MaplibreAndroid.initialize(targetContext)
    super.onStart()
  }
}
