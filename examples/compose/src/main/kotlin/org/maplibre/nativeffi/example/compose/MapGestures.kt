// Gestures. The map's gesture recognizer turns raw touches, pens, and a mouse into camera moves:
// a drag pans, two fingers pinch, twist, and tilt, a double tap zooms in, a two-finger tap zooms
// out, and a release glides on. This file only translates Compose's pointer events and ticks the
// recognizer when it asks.
package org.maplibre.nativeffi.example.compose

import android.os.SystemClock
import androidx.compose.runtime.withFrameMillis
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.input.pointer.PointerEventType
import androidx.compose.ui.input.pointer.PointerId
import androidx.compose.ui.input.pointer.PointerInputChange
import androidx.compose.ui.input.pointer.PointerType
import androidx.compose.ui.input.pointer.changedToDownIgnoreConsumed
import androidx.compose.ui.input.pointer.changedToUpIgnoreConsumed
import androidx.compose.ui.input.pointer.isSecondaryPressed
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.input.pointer.positionChanged
import kotlinx.coroutines.Job
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.maplibre.nativeffi.GestureDelta
import org.maplibre.nativeffi.GestureEvent
import org.maplibre.nativeffi.GestureInput
import org.maplibre.nativeffi.GestureMode
import org.maplibre.nativeffi.GestureNextTick
import org.maplibre.nativeffi.GesturePointer
import org.maplibre.nativeffi.GesturePointerDown
import org.maplibre.nativeffi.GestureRecognizerHandle
import org.maplibre.nativeffi.GestureResult
import org.maplibre.nativeffi.PointerKind
import org.maplibre.nativeffi.ScreenPoint

/** Feeds the pointer input to [recognizer], and calls [onNotch] where the map has a notch. */
fun Modifier.mapGestures(
  recognizer: () -> GestureRecognizerHandle?,
  onNotch: () -> Unit,
): Modifier =
  pointerInput(Unit) {
    // The recognizer takes points in logical pixels, and Compose reports them in pixels.
    fun point(offset: Offset) =
      ScreenPoint(offset.x / density.toDouble(), offset.y / density.toDouble())

    coroutineScope {
      var ticks: Job? = null

      fun tick(timeMillis: Long): GestureResult? = recognizer()?.tick(timeMillis.toDouble())

      // Answers the result's events, and ticks the recognizer when the result asks: every frame
      // while the map glides or eases, or once at a time, as when a press may become a long press.
      // Input times and frame times both count from boot, as SystemClock.uptimeMillis() does.
      fun follow(result: GestureResult) {
        if (result.events.any { it is GestureEvent.Notch }) onNotch()
        ticks?.cancel()
        ticks =
          when (result.nextTick) {
            GestureNextTick.FRAME -> launch { tick(withFrameMillis { it })?.let(::follow) }
            GestureNextTick.AT ->
              launch {
                delay((result.nextTickMs - SystemClock.uptimeMillis()).toLong())
                tick(SystemClock.uptimeMillis())?.let(::follow)
              }
            else -> null
          }
      }

      fun handle(input: GestureInput, change: PointerInputChange, timeMillis: Long) {
        val result = recognizer()?.handle(input, timeMillis.toDouble()) ?: return
        if (result.handled) change.consume()
        follow(result)
      }

      val pressed = mutableSetOf<PointerId>()
      try {
        awaitPointerEventScope {
          while (true) {
            val event = awaitPointerEvent()
            for (change in event.changes) {
              val pointer = GesturePointer(change.id.value.toUInt(), point(change.position))
              when {
                // A wheel detent zooms a third of a level, and eases.
                event.type == PointerEventType.Scroll -> {
                  val delta =
                    GestureDelta(
                      position = pointer.position,
                      discrete = true,
                      zoom = -change.scrollDelta.y / 3.0,
                    )
                  handle(GestureInput.Delta(delta), change, change.uptimeMillis)
                }
                change.changedToDownIgnoreConsumed() -> {
                  pressed += change.id
                  val modes =
                    if (event.buttons.isSecondaryPressed) GestureMode.ROTATE else GestureMode.NONE
                  val down =
                    GesturePointerDown(pointer.id, kind(change.type), pointer.position, modes)
                  handle(GestureInput.PointerDown(down), change, change.uptimeMillis)
                }
                change.changedToUpIgnoreConsumed() -> {
                  pressed -= change.id
                  handle(GestureInput.PointerUp(pointer), change, change.uptimeMillis)
                }
                change.pressed && change.positionChanged() -> {
                  // The samples that the event batched, so the recognizer measures the release
                  // speed from each.
                  for (sample in change.historical) {
                    val moved = pointer.copy(position = point(sample.position))
                    handle(GestureInput.PointerMove(moved), change, sample.uptimeMillis)
                  }
                  handle(GestureInput.PointerMove(pointer), change, change.uptimeMillis)
                }
              }
            }
          }
        }
      } finally {
        // Compose cancels this when the view leaves, as on a rotation, but the map and its
        // recognizer live on, so the pointers that are still down end without a tap or a glide.
        val now = SystemClock.uptimeMillis().toDouble()
        for (id in pressed) {
          recognizer()?.handle(GestureInput.PointerCancel(GesturePointer(id.value.toUInt())), now)
        }
      }
    }
  }

private fun kind(type: PointerType): PointerKind =
  when (type) {
    PointerType.Mouse -> PointerKind.MOUSE
    PointerType.Stylus,
    PointerType.Eraser -> PointerKind.PEN
    else -> PointerKind.TOUCH
  }
