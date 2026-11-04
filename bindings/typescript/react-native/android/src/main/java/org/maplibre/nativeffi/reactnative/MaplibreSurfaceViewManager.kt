package org.maplibre.nativeffi.reactnative

import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.uimanager.SimpleViewManager
import com.facebook.react.uimanager.ThemedReactContext
import com.facebook.react.uimanager.ViewManagerDelegate
import com.facebook.react.viewmanagers.MaplibreSurfaceViewManagerDelegate
import com.facebook.react.viewmanagers.MaplibreSurfaceViewManagerInterface

@ReactModule(name = MaplibreSurfaceViewManager.NAME)
class MaplibreSurfaceViewManager :
  SimpleViewManager<MaplibreSurfaceView>(),
  MaplibreSurfaceViewManagerInterface<MaplibreSurfaceView> {

  private val delegate = MaplibreSurfaceViewManagerDelegate(this)

  override fun getDelegate(): ViewManagerDelegate<MaplibreSurfaceView> = delegate

  override fun getName(): String = NAME

  override fun createViewInstance(context: ThemedReactContext): MaplibreSurfaceView =
    MaplibreSurfaceView(context)

  override fun getExportedCustomDirectEventTypeConstants(): Map<String, Any> =
    mapOf(
      "topSurfaceAvailable" to mapOf("registrationName" to "onSurfaceAvailable"),
      "topSurfaceLost" to mapOf("registrationName" to "onSurfaceLost"),
    )

  companion object {
    const val NAME = "MaplibreSurfaceView"
  }
}
