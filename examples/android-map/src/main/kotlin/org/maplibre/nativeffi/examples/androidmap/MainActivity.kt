package org.maplibre.nativeffi.examples.androidmap

import android.app.Activity
import android.os.Bundle
import android.util.Log
import android.view.WindowManager
import org.maplibre.nativeffi.MaplibreAndroid
import org.maplibre.nativeffi.generated.GeneratedApi

class MainActivity : Activity() {
  private lateinit var mapView: AndroidMapView

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
    installMaplibreLogging()
    MaplibreAndroid.initialize(this)
    mapView = AndroidMapView(this)
    setContentView(mapView)
  }

  override fun onResume() {
    super.onResume()
    mapView.enterForeground()
  }

  override fun onPause() {
    mapView.enterBackground()
    super.onPause()
  }

  override fun onDestroy() {
    mapView.close()
    GeneratedApi.logClearCallback()
    super.onDestroy()
  }

  private fun installMaplibreLogging() {
    GeneratedApi.logSetCallback { severity, event, code, message ->
      Log.i("MapLibre", "severity=${severity} event=${event} code=${code}: ${message}")
      1u
    }
  }

  private companion object {
    private const val TAG = "MapLibreAndroidMap"
  }
}
