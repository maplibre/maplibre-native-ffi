package org.maplibre.nativeffi.examples.composemap.app

import androidx.compose.foundation.background
import androidx.compose.foundation.focusable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.key.KeyEventType
import androidx.compose.ui.input.key.key
import androidx.compose.ui.input.key.onPreviewKeyEvent
import androidx.compose.ui.input.key.type
import androidx.compose.ui.input.pointer.PointerEventType
import androidx.compose.ui.input.pointer.PointerInputChange
import androidx.compose.ui.input.pointer.isCtrlPressed
import androidx.compose.ui.input.pointer.isSecondaryPressed
import androidx.compose.ui.input.pointer.onPointerEvent
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import kotlin.math.pow
import org.maplibre.nativeffi.examples.composemap.map.MapLibreSurfaceRenderer
import org.maplibre.nativeffi.examples.composemap.surface.ComposeNativeSurface
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceState
import org.maplibre.nativeffi.examples.composemap.surface.rememberNativeSurfaceController

@OptIn(ExperimentalComposeUiApi::class)
@Composable
internal fun ComposeMapApp(renderer: MapLibreSurfaceRenderer) {
  val controller = rememberNativeSurfaceController()
  val focusRequester = remember { FocusRequester() }
  val density = LocalDensity.current
  val state by controller.state.collectAsState()

  LaunchedEffect(focusRequester) { focusRequester.requestFocus() }
  LaunchedEffect(controller) {
    while (true) {
      withFrameNanos {}
      controller.requestFrame()
    }
  }

  Box(Modifier.fillMaxSize().background(Color.Black)) {
    ComposeNativeSurface(
      renderer = renderer,
      modifier =
        Modifier.fillMaxSize()
          .focusRequester(focusRequester)
          .focusable()
          .mapKeyboard(renderer)
          .mapGestures(renderer, density.density.toDouble()),
      controller = controller,
    )
    val diagnostic =
      when (val current = state) {
        is NativeSurfaceState.Failed -> current.message
        is NativeSurfaceState.Unsupported ->
          "Native surface unsupported for ${current.requestedBackend} on ${current.host.operatingSystem}/${current.host.consumerBackend}"
        NativeSurfaceState.Inactive,
        is NativeSurfaceState.Ready -> null
      }
    if (diagnostic != null) {
      Text(text = diagnostic, color = Color.White, modifier = Modifier.padding(12.dp))
    }
  }
}

@OptIn(ExperimentalComposeUiApi::class)
private fun Modifier.mapGestures(renderer: MapLibreSurfaceRenderer, scaleFactor: Double): Modifier =
  pointerInput(renderer, scaleFactor) {
      awaitPointerEventScope {
        var previous: PointerInputChange? = null
        val scale = scaleFactor.takeIf { it > 0.0 && it.isFinite() } ?: 1.0
        while (true) {
          val event = awaitPointerEvent()
          val change = event.changes.firstOrNull()
          val current = change?.takeIf { it.pressed }
          if (current == null) {
            previous = null
            continue
          }
          val last = previous
          previous = current
          if (last == null) {
            renderer.cancelTransitions()
            continue
          }

          val delta = current.position - last.position
          val rotate = event.buttons.isSecondaryPressed || event.keyboardModifiers.isCtrlPressed
          if (rotate) {
            renderer.rotateAndPitchBy(delta.x.toDouble() / scale, delta.y.toDouble() / scale)
          } else {
            renderer.moveBy(delta.x.toDouble() / scale, delta.y.toDouble() / scale)
          }
          current.consume()
        }
      }
    }
    .onPointerEvent(PointerEventType.Scroll) { event ->
      val change = event.changes.firstOrNull() ?: return@onPointerEvent
      val scrollY = change.scrollDelta.y
      if (scrollY == 0f) {
        return@onPointerEvent
      }

      change.consume()
      val coordinateScale = scaleFactor.takeIf { it > 0.0 && it.isFinite() } ?: 1.0
      val scale = 2.0.pow(-scrollY.toDouble() * SCROLL_ZOOM_FACTOR)
      renderer.scaleBy(
        scale,
        change.position.x.toDouble() / coordinateScale,
        change.position.y.toDouble() / coordinateScale,
      )
    }

private fun Modifier.mapKeyboard(renderer: MapLibreSurfaceRenderer): Modifier =
  onPreviewKeyEvent { event ->
    if (event.type != KeyEventType.KeyDown) {
      return@onPreviewKeyEvent false
    }
    when (event.key) {
      Key.DirectionLeft,
      Key.A -> renderer.moveByAnimated(KEYBOARD_PAN, 0.0)
      Key.DirectionRight,
      Key.D -> renderer.moveByAnimated(-KEYBOARD_PAN, 0.0)
      Key.DirectionUp,
      Key.W -> renderer.moveByAnimated(0.0, KEYBOARD_PAN)
      Key.DirectionDown,
      Key.S -> renderer.moveByAnimated(0.0, -KEYBOARD_PAN)
      Key.Equals,
      Key.NumPadAdd -> renderer.scaleByAnimated(KEYBOARD_ZOOM)
      Key.Minus,
      Key.NumPadSubtract -> renderer.scaleByAnimated(1.0 / KEYBOARD_ZOOM)
      Key.Q -> renderer.rotateBy(-KEYBOARD_BEARING)
      Key.E -> renderer.rotateBy(KEYBOARD_BEARING)
      Key.RightBracket -> renderer.pitchBy(KEYBOARD_PITCH)
      Key.LeftBracket -> renderer.pitchBy(-KEYBOARD_PITCH)
      Key.Zero,
      Key.NumPad0 -> renderer.resetPitchAndBearing()
      else -> return@onPreviewKeyEvent false
    }
    true
  }

private const val SCROLL_ZOOM_FACTOR = 0.25
private const val KEYBOARD_PAN = 120.0
private const val KEYBOARD_ZOOM = 1.25
private const val KEYBOARD_BEARING = 10.0
private const val KEYBOARD_PITCH = 5.0
