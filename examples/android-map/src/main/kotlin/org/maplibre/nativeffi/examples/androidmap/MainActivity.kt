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
    // A smoke launch renders an inline style, so it needs no network, logs its first rendered
    // frame, and finishes.
    val smoke = intent.getBooleanExtra(SMOKE_EXTRA, false)
    mapView = if (smoke) AndroidMapView(this, SMOKE_STYLE, ::finishSmoke) else AndroidMapView(this)
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

  private var smokeReported = false

  private fun finishSmoke() {
    if (smokeReported) return
    smokeReported = true
    Log.i(TAG, SMOKE_RENDERED)
    finish()
  }

  private companion object {
    private const val TAG = "MapLibreAndroidMap"
    private const val SMOKE_EXTRA = "smoke"
    private const val SMOKE_RENDERED = "smoke: rendered a frame"
    private const val SMOKE_STYLE =
      """{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#2a6f97"}}]}"""
  }
}
