package org.maplibre.nativeffi.examples.androidmap

import android.app.Activity
import android.os.Bundle
import android.util.Log
import android.view.WindowManager
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.MaplibreAndroid
import org.maplibre.nativeffi.log.LogRecord

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
    Maplibre.clearLogCallback()
    super.onDestroy()
  }

  private fun installMaplibreLogging() {
    Maplibre.setLogCallback { record: LogRecord ->
      Log.i(
        "MapLibre",
        "severity=${record.severity} event=${record.event} code=${record.code}: ${record.message}",
      )
      true
    }
  }

  private companion object {
    private const val TAG = "MapLibreAndroidMap"
  }
}
