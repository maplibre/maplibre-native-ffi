package org.maplibre.nativeffi.example.host

import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.uimanager.SimpleViewManager
import com.facebook.react.uimanager.ThemedReactContext
import com.facebook.react.uimanager.ViewManagerDelegate
import com.facebook.react.viewmanagers.MapSurfaceViewManagerDelegate
import com.facebook.react.viewmanagers.MapSurfaceViewManagerInterface

@ReactModule(name = MapSurfaceViewManager.NAME)
class MapSurfaceViewManager :
  SimpleViewManager<MapSurfaceView>(), MapSurfaceViewManagerInterface<MapSurfaceView> {

  private val delegate = MapSurfaceViewManagerDelegate(this)

  override fun getDelegate(): ViewManagerDelegate<MapSurfaceView> = delegate

  override fun getName(): String = NAME

  override fun createViewInstance(context: ThemedReactContext): MapSurfaceView =
    MapSurfaceView(context)

  override fun getExportedCustomDirectEventTypeConstants(): Map<String, Any> =
    mapOf(
      "topSurfaceAvailable" to mapOf("registrationName" to "onSurfaceAvailable"),
      "topSurfaceLost" to mapOf("registrationName" to "onSurfaceLost"),
    )

  companion object {
    const val NAME = "MapSurfaceView"
  }
}
