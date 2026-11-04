// Gestures. A drag pans, two fingers pinch, rotate, and pan, and a double tap zooms in.
package org.maplibre.nativeffi.example.compose

import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.input.pointer.pointerInput
import kotlin.coroutines.cancellation.CancellationException
import kotlin.math.PI
import kotlin.math.atan2
import org.maplibre.nativeffi.AnimationOptions
import org.maplibre.nativeffi.CameraDelta
import org.maplibre.nativeffi.GesturePhase
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.ScreenPoint

fun Modifier.mapGestures(map: () -> MapHandle?): Modifier =
  pointerInput(Unit) {
    // The map takes points in logical pixels, and Compose reports them in pixels.
    fun point(offset: Offset) =
      ScreenPoint(offset.x / density.toDouble(), offset.y / density.toDouble())
    fun apply(delta: CameraDelta) = map()?.discarding?.applyCameraDelta(delta)

    var lastTap: Pair<Offset, Long>? = null

    awaitEachGesture {
      val down = awaitFirstDown(requireUnconsumed = false)
      var moved = false
      var lastPosition = down.position
      apply(CameraDelta(gesturePhase = GesturePhase.BEGIN))

      // Compose cancels this when the view leaves, as on a rotation, but the map lives on.
      try {
        do {
          val event = awaitPointerEvent()
          // The touches that were down at the last event too, with where they were then.
          val touches = event.changes.filter { it.pressed && it.previousPressed }
          event.changes.forEach { it.consume() }
          val first = touches.getOrNull(0) ?: continue
          lastPosition = first.position
          if ((first.position - down.position).getDistance() > viewConfiguration.touchSlop) {
            moved = true
          }
          val second = touches.getOrNull(1)
          if (second == null) {
            apply(
              CameraDelta(
                offset = point(first.position - first.previousPosition),
                gesturePhase = GesturePhase.UPDATE,
              ),
            )
            continue
          }
          // The span between the touches scales and turns about their midpoint, which moves with
          // them.
          moved = true
          val midBefore = (first.previousPosition + second.previousPosition) / 2f
          val midAfter = (first.position + second.position) / 2f
          val spanBefore = second.previousPosition - first.previousPosition
          val spanAfter = second.position - first.position
          val scale = (spanAfter.getDistance() / spanBefore.getDistance()).toDouble()
          val turn =
            (atan2(spanAfter.y, spanAfter.x) - atan2(spanBefore.y, spanBefore.x)) * (180 / PI)
          apply(
            CameraDelta(
              offset = point(midAfter - midBefore),
              scale = if (scale.isFinite() && scale > 0) scale else 1.0,
              bearing = -wrapDegrees(turn),
              anchor = point(midAfter),
              gesturePhase = GesturePhase.UPDATE,
            ),
          )
        } while (event.changes.any { it.pressed })
      } catch (cancelled: CancellationException) {
        apply(CameraDelta(gesturePhase = GesturePhase.CANCEL))
        throw cancelled
      }

      apply(CameraDelta(gesturePhase = GesturePhase.END))
      if (moved) return@awaitEachGesture
      val now = System.currentTimeMillis()
      val previous = lastTap
      if (
        previous != null &&
          now - previous.second < viewConfiguration.doubleTapTimeoutMillis &&
          (previous.first - lastPosition).getDistance() < viewConfiguration.touchSlop * 3
      ) {
        lastTap = null
        apply(
          CameraDelta(
            scale = 2.0,
            anchor = point(lastPosition),
            animation = AnimationOptions(durationMs = 300.0),
          ),
        )
      } else {
        lastTap = lastPosition to now
      }
    }
  }

/** An angle in degrees, wrapped to [-180, 180). */
private fun wrapDegrees(degrees: Double): Double = ((degrees + 180) % 360 + 360) % 360 - 180
