package org.maplibre.nativeffi.examples.androidmap

import android.app.Activity
import android.os.Bundle
import android.util.Log
import android.view.WindowManager
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.MaplibreAndroid
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.render.RenderBackend

class MainActivity : Activity() {
  private lateinit var mapView: AndroidMapView

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
    installMaplibreLogging()
    MaplibreAndroid.initialize(this)
    validateOpenGLBackend()
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

  private fun validateOpenGLBackend() {
    val backends = Maplibre.supportedRenderBackends()
    Log.i(TAG, "native render backends: ${backendLabel(backends)}")
    check(backends.contains(RenderBackend.OPENGL)) {
      "the loaded MapLibre native library does not support OpenGL"
    }
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

  private fun backendLabel(backends: Set<RenderBackend>): String =
    if (backends.isEmpty()) {
      "none"
    } else {
      backends.joinToString(",") { it.name.lowercase() }
    }

  private companion object {
    private const val TAG = "MapLibreAndroidMap"
  }
}
