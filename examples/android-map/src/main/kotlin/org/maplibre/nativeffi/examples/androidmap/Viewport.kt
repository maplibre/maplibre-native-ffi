package org.maplibre.nativeffi.examples.androidmap

import android.util.Log
import kotlin.math.ceil
import kotlin.math.max
import org.maplibre.nativeffi.render.RenderTargetExtent

internal data class Viewport(
  val logicalWidth: Int,
  val logicalHeight: Int,
  val physicalWidth: Int,
  val physicalHeight: Int,
  val scaleFactor: Double,
  val isEmpty: Boolean,
) {
  val extent: RenderTargetExtent
    get() = RenderTargetExtent(logicalWidth, logicalHeight, scaleFactor)

  fun log(label: String) {
    Log.i(
      TAG,
      "$label: logical=${logicalWidth}x$logicalHeight physical=${physicalWidth}x$physicalHeight scale=$scaleFactor empty=$isEmpty",
    )
  }

  companion object {
    private const val TAG = "MapLibreViewport"

    fun fromView(width: Int, height: Int, density: Float): Viewport {
      val physicalWidth = max(width, 0)
      val physicalHeight = max(height, 0)
      val empty = physicalWidth == 0 || physicalHeight == 0
      val scale = density.takeIf { it.isFinite() && it > 0f }?.toDouble() ?: 1.0
      return Viewport(
        logicalWidth = if (empty) 0 else max(1, ceil(physicalWidth / scale).toInt()),
        logicalHeight = if (empty) 0 else max(1, ceil(physicalHeight / scale).toInt()),
        physicalWidth = physicalWidth,
        physicalHeight = physicalHeight,
        scaleFactor = scale,
        isEmpty = empty,
      )
    }
  }
}
