package org.maplibre.nativeffi.reactnative

import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.WritableMap
import com.facebook.react.uimanager.ThemedReactContext
import com.facebook.react.uimanager.UIManagerHelper
import com.facebook.react.uimanager.events.Event

/**
 * Lends its surface to JavaScript through the surface registry. Android takes the surface away when
 * the app goes to the background, and gives a new one when it returns.
 */
class MaplibreSurfaceView(private val reactContext: ThemedReactContext) :
  SurfaceView(reactContext), SurfaceHolder.Callback {

  private var surface = 0

  init {
    holder.addCallback(this)
  }

  override fun surfaceCreated(holder: SurfaceHolder) {
    surface = nativeAdd(holder.surface)
    if (surface != 0) dispatch("topSurfaceAvailable", surface)
  }

  override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}

  override fun surfaceDestroyed(holder: SurfaceHolder) {
    val withdrawn = surface
    surface = 0
    if (withdrawn == 0) return
    // The registry keeps the window until JavaScript releases it, so a frame
    // that is still in flight fails instead of using a freed window.
    nativeWithdraw(withdrawn)
    dispatch("topSurfaceLost", withdrawn)
  }

  private fun dispatch(name: String, surface: Int) {
    UIManagerHelper.getEventDispatcherForReactTag(reactContext, id)
      ?.dispatchEvent(
        SurfaceEvent(UIManagerHelper.getSurfaceId(reactContext), id, name, surface),
      )
  }

  private class SurfaceEvent(
    surfaceId: Int,
    viewId: Int,
    private val name: String,
    private val surface: Int,
  ) : Event<SurfaceEvent>(surfaceId, viewId) {
    override fun getEventName(): String = name

    override fun getEventData(): WritableMap =
      Arguments.createMap().apply { putInt("surface", surface) }
  }

  companion object {
    @JvmStatic private external fun nativeAdd(surface: Surface): Int

    @JvmStatic private external fun nativeWithdraw(surface: Int)
  }
}
