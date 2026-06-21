package org.maplibre.nativeffi.examples.androidmap

import android.content.Context
import android.view.GestureDetector
import android.view.MotionEvent
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min
import kotlin.math.pow
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle

internal class InputController(
  context: Context,
  private val mapProvider: () -> MapHandle?,
  private val requestRender: () -> Unit,
) {
  private val density = context.resources.displayMetrics.density.takeIf { it > 0f } ?: 1f
  private val tapGestureDetector = GestureDetector(context, TapListener())
  private val pointerTracker = PointerTracker()
  private var doubleTapActive = false

  fun onTouchEvent(event: MotionEvent): Boolean {
    val map = mapProvider() ?: return true
    val wasDoubleTapActive = doubleTapActive
    tapGestureDetector.onTouchEvent(event)
    if (doubleTapActive || wasDoubleTapActive) return true
    pointerTracker.onTouchEvent(map, event)
    return true
  }

  private inner class TapListener : GestureDetector.SimpleOnGestureListener() {
    override fun onDown(event: MotionEvent): Boolean = true

    override fun onDoubleTap(event: MotionEvent): Boolean {
      doubleTapActive = true
      val map = mapProvider() ?: return false
      map.cancelTransitions()
      val zoom = map.camera.zoom ?: 0.0
      val targetZoom = kotlin.math.round(zoom) + 1.0
      map.scaleByAnimated(
        2.0.pow(targetZoom - zoom),
        screenPoint(event.x, event.y),
        AnimationOptions().apply { durationMs = 160.0 },
      )
      requestRender()
      return true
    }

    override fun onDoubleTapEvent(event: MotionEvent): Boolean {
      if (
        event.actionMasked == MotionEvent.ACTION_UP ||
          event.actionMasked == MotionEvent.ACTION_CANCEL
      ) {
        doubleTapActive = false
      }
      return true
    }
  }

  private inner class PointerTracker {
    private var mode = Mode.NONE
    private var primaryPointerId = MotionEvent.INVALID_POINTER_ID
    private var lastPrimaryX = 0f
    private var lastPrimaryY = 0f
    private var twoFingerBaseline: TwoFingerSample? = null

    fun onTouchEvent(map: MapHandle, event: MotionEvent) {
      when (event.actionMasked) {
        MotionEvent.ACTION_DOWN -> {
          map.cancelTransitions()
          beginPan(event)
        }
        MotionEvent.ACTION_POINTER_DOWN ->
          if (event.pointerCount == 2) {
            map.cancelTransitions()
            beginTwoFinger(event)
          }
        MotionEvent.ACTION_MOVE -> {
          when {
            event.pointerCount == 1 && mode == Mode.PAN -> updatePan(map, event)
            event.pointerCount == 2 && mode.isTwoFinger -> updateTwoFinger(map, event)
          }
        }
        MotionEvent.ACTION_POINTER_UP -> {
          if (event.pointerCount == 2) {
            val remainingIndex = if (event.actionIndex == 0) 1 else 0
            primaryPointerId = event.getPointerId(remainingIndex)
            lastPrimaryX = event.getX(remainingIndex)
            lastPrimaryY = event.getY(remainingIndex)
            mode = Mode.PAN
            twoFingerBaseline = null
          } else {
            reset()
          }
        }
        MotionEvent.ACTION_UP,
        MotionEvent.ACTION_CANCEL -> reset()
      }
    }

    private fun beginPan(event: MotionEvent) {
      primaryPointerId = event.getPointerId(0)
      lastPrimaryX = event.x
      lastPrimaryY = event.y
      mode = Mode.PAN
      twoFingerBaseline = null
    }

    private fun updatePan(map: MapHandle, event: MotionEvent) {
      val pointerIndex = event.findPointerIndex(primaryPointerId)
      if (pointerIndex < 0) return
      val x = event.getX(pointerIndex)
      val y = event.getY(pointerIndex)
      val dx = logicalDelta(x - lastPrimaryX)
      val dy = logicalDelta(y - lastPrimaryY)
      lastPrimaryX = x
      lastPrimaryY = y
      if (dx == 0.0 && dy == 0.0) return
      map.moveBy(dx, dy)
      requestRender()
    }

    private fun beginTwoFinger(event: MotionEvent) {
      mode = Mode.UNDECIDED_TWO_FINGER
      primaryPointerId = MotionEvent.INVALID_POINTER_ID
      twoFingerBaseline = TwoFingerSample.read(event)
    }

    private fun updateTwoFinger(map: MapHandle, event: MotionEvent) {
      val baseline = twoFingerBaseline ?: return
      val current = TwoFingerSample.read(event)
      if (current.distance <= 0.0 || baseline.distance <= 0.0) {
        twoFingerBaseline = current
        return
      }

      val scale = current.distance / baseline.distance
      val deltaDegrees = Math.toDegrees(normalizedAngle(current.angle - baseline.angle))
      val deltaY = logicalDelta(current.centroidY - baseline.centroidY)

      if (mode == Mode.UNDECIDED_TWO_FINGER) {
        mode = classifyTwoFingerGesture(scale, deltaDegrees, deltaY)
        if (mode == Mode.UNDECIDED_TWO_FINGER) return
      }

      var changed = false
      if (mode == Mode.SCALE_ROTATE) {
        if (scale.isFinite() && abs(scale - 1.0) >= SCALE_EPSILON) {
          map.scaleBy(scale, screenPoint(current.centroidX, current.centroidY))
          changed = true
        }
        if (abs(deltaDegrees) >= ROTATION_EPSILON_DEGREES) {
          rotateBy(map, deltaDegrees, current.centroidX, current.centroidY)
          changed = true
        }
      } else if (mode == Mode.SHOVE && deltaY != 0.0) {
        val camera = map.camera
        map.jumpTo(
          CameraOptions().apply {
            pitch = min(max((camera.pitch ?: 0.0) - deltaY * 0.1, 0.0), 60.0)
          }
        )
        changed = true
      }

      if (changed) {
        requestRender()
        twoFingerBaseline = current
      }
    }

    private fun rotateBy(map: MapHandle, deltaDegrees: Double, centroidX: Float, centroidY: Float) {
      val camera = map.camera
      map.jumpTo(
        CameraOptions().apply {
          bearing = (camera.bearing ?: 0.0) - deltaDegrees
          anchor = screenPoint(centroidX, centroidY)
        }
      )
    }

    private fun classifyTwoFingerGesture(
      scale: Double,
      deltaDegrees: Double,
      deltaY: Double,
    ): Mode {
      val scaleDelta = abs(scale - 1.0)
      val rotationDelta = abs(deltaDegrees)
      val shoveDelta = abs(deltaY)
      return when {
        shoveDelta >= SHOVE_START_LOGICAL_PX &&
          scaleDelta < SCALE_START_DELTA &&
          rotationDelta < ROTATION_START_DEGREES -> Mode.SHOVE
        scaleDelta >= SCALE_START_DELTA || rotationDelta >= ROTATION_START_DEGREES ->
          Mode.SCALE_ROTATE
        else -> Mode.UNDECIDED_TWO_FINGER
      }
    }

    private fun reset() {
      mode = Mode.NONE
      primaryPointerId = MotionEvent.INVALID_POINTER_ID
      twoFingerBaseline = null
    }
  }

  private enum class Mode {
    NONE,
    PAN,
    UNDECIDED_TWO_FINGER,
    SCALE_ROTATE,
    SHOVE;

    val isTwoFinger: Boolean
      get() = this == UNDECIDED_TWO_FINGER || this == SCALE_ROTATE || this == SHOVE
  }

  private data class TwoFingerSample(
    val firstX: Float,
    val firstY: Float,
    val centroidX: Float,
    val centroidY: Float,
    val distance: Double,
    val angle: Double,
  ) {
    companion object {
      fun read(event: MotionEvent): TwoFingerSample {
        val x0 = event.getX(0)
        val y0 = event.getY(0)
        val x1 = event.getX(1)
        val y1 = event.getY(1)
        val dx = (x1 - x0).toDouble()
        val dy = (y1 - y0).toDouble()
        return TwoFingerSample(
          firstX = x0,
          firstY = y0,
          centroidX = (x0 + x1) * 0.5f,
          centroidY = (y0 + y1) * 0.5f,
          distance = hypot(dx, dy),
          angle = atan2(dy, dx),
        )
      }
    }
  }

  private fun screenPoint(x: Float, y: Float): ScreenPoint =
    ScreenPoint(logicalDelta(x), logicalDelta(y))

  private fun logicalDelta(value: Float): Double = value.toDouble() / density.toDouble()

  private fun normalizedAngle(radians: Double): Double =
    atan2(kotlin.math.sin(radians), kotlin.math.cos(radians))

  private companion object {
    private const val SCALE_START_DELTA = 0.015
    private const val SCALE_EPSILON = 0.001
    private const val ROTATION_START_DEGREES = 2.0
    private const val ROTATION_EPSILON_DEGREES = 0.1
    private const val SHOVE_START_LOGICAL_PX = 4.0
  }
}
